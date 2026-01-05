#include "io_manager.h"
#include "config.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_event.h"
#include "eventHandler.h"

#define ESP_INTR_FLAG_DEFAULT 0
TaskHandle_t io_task = NULL;
uint8_t ou = 0;

static void IRAM_ATTR setValue(void *args){
    //hacer algo
}

static void IRAM_ATTR setValue_ign(void *args){
    if(gpio_get_level(IGNITION_PIN)){
        esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, IGNITION_OFF, NULL, 0, portMAX_DELAY);
    }else{
        esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, IGNITION_ON, NULL, 0, portMAX_DELAY);
    }
}

void configure_gpio_input(void *pvParameters){
    gpio_config_t in_conf = {};
    in_conf.intr_type = GPIO_INTR_NEGEDGE; 
    in_conf.mode = GPIO_MODE_INPUT;      
    in_conf.pin_bit_mask = (1ULL << INPUT2_PIN); 
    in_conf.pull_down_en = 0;
    in_conf.pull_up_en = 0;
    gpio_config(&in_conf);
    
    gpio_config_t ign_conf = {};
    ign_conf.intr_type = GPIO_INTR_ANYEDGE; 
    ign_conf.mode = GPIO_MODE_INPUT;      
    ign_conf.pin_bit_mask = (1ULL << IGNITION_PIN); 
    ign_conf.pull_down_en = 0;
    ign_conf.pull_up_en = 0;
    gpio_config(&ign_conf);
    
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(IGNITION_PIN, setValue_ign, NULL);
    gpio_isr_handler_add(INPUT2_PIN, setValue, NULL);
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void input_init(){
    xTaskCreate(configure_gpio_input, "io_task", 4096, NULL, 10, &io_task);
}

uint8_t getInValue(){
    return gpio_get_level(INPUT2_PIN);
}

uint8_t getIGNValue(){
    return gpio_get_level(IGNITION_PIN);
}

void configure_gpio_ou(void)
{
    gpio_config_t ou_conf = {};
    ou_conf.intr_type = GPIO_INTR_DISABLE; 
    ou_conf.mode = GPIO_MODE_OUTPUT;      
    ou_conf.pin_bit_mask = (1ULL << OUTPUT1_PIN); 
    ou_conf.pull_down_en = 0;
    ou_conf.pull_up_en = 0;
    gpio_config(&ou_conf);

}

void ou_init(){
    configure_gpio_ou();
}

uint8_t OU_on(void){
    if(ou == 0){
        ou ^= 1;
        gpio_set_level(OUTPUT1_PIN, 0); 
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(OUTPUT1_PIN, 1);
        return 1;
    }else{
        return 0;
    }
}

uint8_t OU_off(void){
    if(ou == 1){
        ou ^= 1;
        gpio_set_level(OUTPUT1_PIN, 0); 
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(OUTPUT1_PIN, 1);
        return 0;
    }else{
        return 1;
    }
}

uint8_t getOUValue(void){
    return ou;
}