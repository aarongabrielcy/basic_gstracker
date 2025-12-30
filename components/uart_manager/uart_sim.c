#include "uart_sim.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "esp_check.h"
#include <string.h>
#include "esp_log.h"
#include "sim7000.h"

#define TAG "MAIN UART"

TaskHandle_t main_uart_task = NULL;

void uart_task_init(void);
void main_uart_init(void);
int uartManager_readEvent();
static void uart_main_asd(void *pvParameters);

void main_uart_init(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_SIM, &uart_config);
    uart_set_pin(UART_SIM, TXD_MAIN_UART, RXD_MAIN_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_SIM, BUF_SIZE * 2,BUF_SIZE * 2, 0, NULL, 0);

    uart_task_init();
}

static void uart_main_asd(void *pvParameters){
    char response[1024];
    while(1){
        int len = uartManager_readEvent(response, sizeof(response), 300);
        if (len > 0) {
            if (strstr(response, "SMS Ready") != NULL){
                ESP_LOGI(TAG, "LISTO PA PROBAR: %s", response);
                //sim7000_init();
            }
            else{
                ESP_LOGI(TAG, "%s", response);
            }
        }
    }
}

int uartManager_readEvent(char *buffer, int max_length, int timeout_ms) {
    int len = uart_read_bytes(UART_SIM, (uint8_t *)buffer, max_length - 1, pdMS_TO_TICKS(timeout_ms));
    if (len > 0) {
        buffer[len] = '\0';
    }
    return len;
}


void uart_task_init(){
    xTaskCreate(uart_main_asd, "uart_main", 4096, NULL, 10, &main_uart_task);
}

