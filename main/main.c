#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "leds_pwr_manager.h"
#include "uart_manager.h"
#include "nvsManager.h"
#include "deviceManager.h"


#include "esp_log.h"
#include "esp_check.h"

void app_main(void)
{
    nvs_init();
    pwr_init();
    uarts_init();
    //device_init();
}
