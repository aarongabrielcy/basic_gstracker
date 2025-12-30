#include "uart_serial.h"
#include "uart_sim.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "esp_check.h"
#include "string.h"
//#include "adc.h"
//#include "sim7000.h"
//#include "input.h"
//#include "output.h"
//#include "accel.h"

#define TAG "UART_MANAGER_SERIAL"
#define BUF_SIZE 1024

TaskHandle_t monitor_uart_task = NULL;

void serial_task_init(void);
void uart_serial_init(void);
void uartManager_sendCommand(const char *command);
static void serialConsole_task(void *pvParameters);

void uart_serial_init(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);

    serial_task_init();
}

static void serialConsole_task(void *pvParameters){
    char data[512];
    while(1){
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)data, sizeof(data) - 1, pdMS_TO_TICKS(300));
        if (len > 0) {
            data[len] = '\0';
            if (strstr(data, "AT") != NULL){
                uartManager_sendCommand((char*)data);
            }
            if (strstr(data, "ADC_E") != NULL){
                //ESP_LOGI(TAG, "VOLTAJE BATERIA EXTERNO: %.2f", getBatteryExtern());
            }
            if (strstr(data, "ADC_I") != NULL){
                //ESP_LOGI(TAG, "VOLTAJE BATERIA INTERNO: %.2f", getBatteryIntern());
            }
            if (strstr(data, "SIM7000=1") != NULL){
                //sim7000_init();
            }
            if (strstr(data, "SIM7000=0") != NULL){
                uartManager_sendCommand("AT+CGNSPWR=0");
            }
            if (strstr(data, "IN?") != NULL){
                //ESP_LOGI(TAG, "VALOR DE ENTRADA ACTUAL: %d", getInValue());
            }
            if (strstr(data, "IGN?") != NULL){
                //ESP_LOGI(TAG, "VALOR DE IGNICION ACTUAL: %d", getIGNValue());
            }
            if (strstr(data, "OU?") != NULL){
                //ESP_LOGI(TAG, "VALOR DE OUTPUT ACTUAL: %d", getOUValue());
            }
            if (strstr(data, "OU=1") != NULL){
                ESP_LOGI(TAG, "CAMBIANDO OUTPUT");
                //OU_on();
            }
            if (strstr(data, "OU=0") != NULL){
                ESP_LOGI(TAG, "CAMBIANDO OUTPUT");
                //OU_off();
            }
            if (strstr(data, "ACCEL?") != NULL){
                //int16_t x,y,z;
                //getAccelValue(&x, &y, &z);
                //ESP_LOGI(TAG, "VALOR DE ACELERACION X: %d, Y: %d, Z: %d", x, y, z);
            }
            
        }
    }
}

void uartManager_sendCommand(const char *command) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "%s\r\n", command);
    uart_write_bytes(UART_SIM, buf, n);
}

void serial_task_init(){
    xTaskCreate(serialConsole_task, "serial_console_task", 8192, NULL, 5, &monitor_uart_task);
}

