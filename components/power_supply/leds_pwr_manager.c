#include "leds_pwr_manager.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "config.h"
#include "esp_log.h"
#include "io_manager.h"
#define TAG "PWR"

void pwr_init(void);
void power_on_led(void);
void power_on_module(void);
void power_gnss_led(void);

// public
void pwr_init(){
    power_on_module();
    power_on_led();
    power_gnss_led();
    input_init();
    ou_init();
}

// private

void power_on_module() {
    gpio_reset_pin(POWER_SIM_PIN);

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << POWER_SIM_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(POWER_SIM_PIN, 1);
}

void power_on_led() {

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << POWER_LED_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(POWER_LED_PIN, 1);
}

void power_gnss_led(){
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << GNSS_LED_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    gpio_config(&io_conf);
    gpio_set_level(GNSS_LED_PIN, 1);
}