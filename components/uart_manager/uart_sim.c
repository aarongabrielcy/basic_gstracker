#include "uart_sim.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "esp_check.h"
#include <string.h>
#include "esp_log.h"
#include "sim7000.h"
#include "tracking.h"
#include "utilities.h"
#include "device_manager.h"
#include "cellnet_data.h"

#define TAG "UART_MANAGER_SIM"

TaskHandle_t main_uart_task = NULL;

void uart_task_init(void);
void uart_sim_init(void);
int uartManager_readEvent();
static void uart_sim_task(void *pvParameters);

void uart_sim_init(){
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

/*
    pasos para conectarse en TCP
    - AT+CSTT="internet.itelcel.com"
    - AT+CIICR
    - AT+CIFSR
    - AT+CIPSTART="TCP","201.122.135.23",6100

    - POSTERIORRMENTE SE DEBE MANDAR AT+CIPSEND PARA CADA TRAQUEO

    USAR PSUTTZ PARA VALIDAR ZONA UTC
    

    Pendientes .- Revisar validacion de trackeos con: CONNECT OK

*/

static void uart_sim_task(void *pvParameters){
    char response[1024];
    while(1){
        int len = uartManager_readEvent(response, sizeof(response), 300);
        if (len > 0) {
            // Se propone leer todas las respuestas para un mayor control
            if (strstr(response, "RDY") != NULL){
                // EN ESPERA DE APROBACION
                //ESP_LOGI(TAG, "CHECKING NVS DATA:%s", response);
                //check_nvs_data();
            }
            if (strstr(response, "SMS Ready") != NULL){
                ESP_LOGI(TAG, "LISTO PA PROBAR: %s", response);
                // Este es el que uso ahora
                sim7000_init();
                
                // Se planea iniciar y aislar la conexion con este
                //sim7000_connection_init();
            }
            else if (strstr(response, ">") != NULL){
                ESP_LOGE(TAG, "Detected '>'");
                send_track_data();
                //sim7000_init();
                //RETUR BOOL
            }
            else if (strstr(response, "CONNECT OK") != NULL) {
                ESP_LOGI(TAG, "CONNECTED TCP:%s", response);
                sync_tracker_data();
            }
            else if (strstr(response, "SEND OK") != NULL){
                ESP_LOGI(TAG, "RESP SUCCESSFULLY CIPSEND:%s", response);
            }
            else if (strstr(response, "SEND FAIL") != NULL){
                ESP_LOGI(TAG, "SEND FAIL CIPSEND:%s", response);
                request_cipstatus();
            }
            else if (strstr(response, "ERROR") != NULL){
                ESP_LOGI(TAG, "ERROR CIPSEND CONN:%s", response);
                request_cipstatus();
            }
            else if (strstr(response, "CLOSED") != NULL){
                ESP_LOGI(TAG, "CLOSED FAIL CIPSEND:%s", response);
                reconnect_network();
            }
            else if (strstr(response, "SHUT OK") != NULL){
                ESP_LOGI(TAG, "PDP CONTEXT CLOSE:%s", response);
                reconnect_network();
            }
            else if (strstr(response, "PDP DEACT") != NULL){
                ESP_LOGI(TAG, "PDP DEACT CIP STATUS:%s", response);
                reconnect_pdp();
            }
            else if (strstr(response, "+CLTS:") != NULL){
                int data = -1;
                char *start_data_clts = strstr(response, "+CLTS:");
                start_data_clts += strlen("+CLTS:");
                data = atoi(start_data_clts);
                ESP_LOGI(TAG, "RESPONSE CLTS:%s,%d", response, data);
                if (data != 1){
                    getTimeUTC();
                    ESP_LOGI(TAG, "Activating UTC from network");
                }
                else{
                    ESP_LOGI(TAG, "UTC already activated");
                }
            }
            else if (strstr(response, "*PSUTTZ:") != NULL){
                char *start_data_psuttz = strstr(response, "*PSUTTZ: ");
                start_data_psuttz += strlen("*PSUTTZ: ");
                //Aqui iria la UTC
                ESP_LOGI(TAG, "psuttz Response,%s", start_data_psuttz);
            }
            else if (strstr(response, "+CCLK:") != NULL){ 
                // aqui va lo de la coordinacion de la hora local
                char *start_data_clts = strstr(response, "+CCLK:");
                start_data_clts += strlen("+CCLK: ");
                char *result = getFormatUTC(start_data_clts);
                setTimeUTC(result);
                //ESP_LOGI(TAG, "FORMATEO DE LA HORA:%s -- %s", response, result);
            }
            else if (strstr(response, "AT+CGSN") != NULL){
                //char *start_data_imei = strstr(response, "AT+GSN");
                //start_data_imei += strlen("AT+GSN\n");
                char current_imei[16]; 
                char *clean_response = cleanResponse(response);
                if(sscanf(clean_response, "AT+CGSN,%[^,],OK",current_imei)==1){
                    check_imei_match(current_imei);
                }
                else{
                    ESP_LOGI(TAG, "ERROR EN EL RESPONSE:%s", clean_response);
                }
            }
            else if (strstr(response, "AT+CCID") != NULL){
                //char *start_data_imei = strstr(response, "AT+GSN");
                //start_data_imei += strlen("AT+GSN\n");
                char current_ccid[20]; 
                char *clean_response = cleanResponse(response);
                if(sscanf(clean_response, "AT+CCID,%19[^,],OK",current_ccid)==1){
                    check_ccid_match(current_ccid);
                }
                else{
                    ESP_LOGI(TAG, "ERROR EN EL RESPONSE:%s", clean_response);
                }
            }
            else if (strstr(response, "+CPSI:") != NULL){
                //ESP_LOGI(TAG, "ENTRA CPSI");
                if (parsePSI(response)){
                    // habilitar trackeo tiempo real
                    set_net_connectivity(1);
                }else{
                    // habilitar buffer
                    set_net_connectivity(0);
                }
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
    xTaskCreate(uart_sim_task, "uart_main", 4096, NULL, 10, &main_uart_task);
}