#include "uart_manager.h"
#include "uart_gnss.h"
#include "uart_serial.h"
#include "uart_sim.h"

void uarts_init(){
    uart_sim_init();
    uart_serial_init();
    uart_gnss_init();
}