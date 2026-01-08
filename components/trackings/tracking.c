#include "tracking.h"
#include "esp_event.h"
#include "eventHandler.h"
#include "esp_log.h"
#include "esp_check.h"
#include "uart_serial.h"
#include <string.h>
#include "sim7000.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define TAG "EVENT HANDLER"

char date_time[34];
bool ready_to_track = false;

void system_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data); 

void start_event_handler(void) {
    esp_event_loop_handle_t loop = get_event_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, ESP_EVENT_ANY_ID, system_event_handler, NULL);
}

//AT+CIPSTART="TCP","201.122.135.23",6100
//AT+CIPSTART="TCP","34.196.135.179",5200
// //6008

void system_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case IGNITION_ON:
            ESP_LOGI(TAG, "Ignition=> ENCENDIDA"); 
            start_tracking_report_timer();
            stop_keep_alive_timer();
            break;
        case IGNITION_OFF:
            ESP_LOGI(TAG, "Ignition=> APAGADA, generando Keep alives"); 
            stop_tracking_report_timer();
            start_keep_alive_timer();
            break;
        case INPUT1_ON:
            ESP_LOGI(TAG, "INPUT_1=> ENCENDIDA"); 
            break;
        case INPUT1_OFF:
            ESP_LOGI(TAG, "INPUT_1=> APAGADA");
            break;
        case KEEP_ALIVE:
            ESP_LOGI(TAG, "Generando KEEP ALIVE");
            break;
        case TRACKING_RPT:
            ESP_LOGI(TAG, "Generando TRACKING_RPT");
            // Esto ya funciona
            // buscar el CONNECT OK PARA ACEPTAR TRACKEOS

            getTimeLocal();
            vTaskDelay(100);
            uartManager_sendCommand("AT+CIPSEND");
        break;
        case DEFAULT:
            stop_tracking_report_timer();
            stop_keep_alive_timer();            
        break;
    }
}

/*
    PRIORIDAD 06/01/2026
    
    - CHEQUEAR LO DEL CLOCK PARA TRACKEOS (Hecho)
    - NVS (Coordenadas 
            )
    - CHEQUEAR LO DEL FIX
    - TRACKEO POR CURVA
    - OTA

    - INPUTS/ OUTPUTS
    - BUFFER
    
    PRIORIDAD 06/01/2026

*/

void setTimeUTC(const char *time){
    strncpy(date_time, time, sizeof(date_time) - 1);
    date_time[sizeof(date_time) - 1] = '\0';
}

void readyToTrack(){
    ready_to_track = true;
}

void sendHardTracking(){
    char imei[10] = "2049830928";
    char message[256];
    snprintf(message, sizeof(message), "STT;2049830928;3FFFFF;68;1.0.36;1;%s;000039C5;334;20;1222;29;+21.023450;-89.584602;0.00;0.00;0;0;00000000;00000000;0;1;5676;4.1;11.94;;;;", date_time);
    char ctrl_z_str[2] = { 0x1A, '\0' };
    uartManager_sendCommand(message);
    uartManager_sendCommand(ctrl_z_str);
}