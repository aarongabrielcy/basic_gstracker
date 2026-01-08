#include "leds_pwr_manager.h"
#include "uart_manager.h"
#include "nvsManager.h"
#include "deviceManager.h"
#include "eventHandler.h"

void app_main(void)
{
    nvs_init();
    pwr_init();
    uarts_init();
    init_event_loop();
    //device_init();
}
