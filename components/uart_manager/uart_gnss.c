#include "uart_gnss.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "esp_check.h"
#include "esp_log.h"
#define EPSILON 0.0001

static const char *TAG = "UART_MANAGER_GNSS";


#define BUF_SIZE_OTA 1024
int uartGnss_readEvent(char *buffer, int max_length, int timeout_ms);
void uartManager_start_2(void);

void uart_gnss_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TX_GNSS_UART, RX_GNSS_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
    uartManager_start_2();
}

static void gnss_task_init(void *arg) {
    char response_gnss[1024];
    while (1) {
        int len = uartGnss_readEvent(response_gnss, sizeof(response_gnss), 300);
        if (len > 0) { 
            ESP_LOGI(TAG, "%s", response_gnss);
        }
    }
}


int uartGnss_readEvent(char *buffer, int max_length, int timeout_ms) {
    int len = uart_read_bytes(UART_NUM_2, (uint8_t *)buffer, max_length - 1, pdMS_TO_TICKS(timeout_ms));
    if (len > 0) {
        buffer[len] = '\0';
    }
    return len;
}

void uartManager_start_2() {
    xTaskCreate(gnss_task_init, "uart_task", 8192, NULL, 5, NULL);
}