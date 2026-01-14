#include "leds_pwr_manager.h"
#include "uart_manager.h"
#include "nvs_manager.h"
#include "device_manager.h"
#include "eventHandler.h"
#include "buffer.h"

void app_main(void) {
    nvs_init();
    pwr_init();
    buffer_init();
    uarts_init();
    init_event_loop();
    device_init();
}