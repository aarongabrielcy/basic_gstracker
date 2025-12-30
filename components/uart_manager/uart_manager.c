#include "uart_manager.h"
#include "uart_gnss.h"
#include "uart_serial.h"
#include "uart_sim.h"

void uarts_init(){
    main_uart_init();
    monitor_uart_init();
    uart_init_2();
}