#include "sim7000.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "uart_serial.h"


#define TAG "SIM7000"
TaskHandle_t sim700_task = NULL;
void sim7000_stop(void);

void sim7000_command(const char *command){
    uartManager_sendCommand(command);
}

void sim7000_basic_config() {
    sim7000_command("AT+GSN");
    vTaskDelay(200);
    sim7000_command("AT+CCID");
    vTaskDelay(200);
    sim7000_command("AT+CGNSCFG=2");
    vTaskDelay(200);
    sim7000_command("AT+CGNSPWR=1");
    sim7000_stop();
}

void sim7000_init(){
    xTaskCreate(sim7000_basic_config, "sim7000_task", 2048, NULL, 10, &sim700_task);
}

void sim7000_stop(){
    vTaskDelete(sim700_task);
}