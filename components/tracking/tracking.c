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
#include "nvsData.h"
#include "cellnet_data.h"
#include "gnss_nmea.h"
#include "tracker_model.h"
#include "utilities.h"
#include <stdint.h>
#include "buffer.h"
#define TAG "EVENT HANDLER"

char date_time[34];
char latitud[20] = "+0.000000";
char longitud[20] = "+0.000000";
bool network = false;
bool curve_tracking = false;

void sync_tracker_data();
void send_inputOnAlert_data();
void send_inputOffAlert_data();

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
            tkr.ign = 1;
            start_tracking_report_timer();
            stop_keep_alive_timer();
            break;
        case IGNITION_OFF:
            ESP_LOGI(TAG, "Ignition=> APAGADA, generando Keep alives"); 
            tkr.ign = 0;
            stop_tracking_report_timer();
            start_keep_alive_timer();
            break;
        case INPUT1_ON:
            ESP_LOGI(TAG, "INPUT_1=> ENCENDIDA");
            sync_tracker_data();
            send_inputOnAlert_data(); 
            break;
        case INPUT1_OFF:
        ESP_LOGI(TAG, "INPUT_1=> APAGADA");
            sync_tracker_data();
            send_inputOffAlert_data();
            break;
        case KEEP_ALIVE:
            ESP_LOGI(TAG, "Generando KEEP ALIVE");
            break;
        case TRACKING_RPT:
            ESP_LOGI(TAG, "Generando TRACKING_RPT");
            // Esto ya funciona
            // buscar el CONNECT OK PARA ACEPTAR TRACKEOS
            sync_tracker_data();
            send_track_data();
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

void set_net_connectivity(uint8_t signal){
    network = (signal) ? true : false;
}

void sync_tracker_data(){
    getTimeLocal();
    vTaskDelay(100);
    getCellData();
    vTaskDelay(100);
    if (network){
        uartManager_sendCommand("AT+CIPSEND");
    }
    else{
        // buffer
    }
    
}

static void send_ctrlZ(const char *message){
    char ctrl_z_str[2] = { 0x1A, '\0' };
    uartManager_sendCommand(message);
    uartManager_sendCommand(ctrl_z_str);
}

void send_track_data(){
    char message[256];
    strcpy(latitud, formatCoordinates(gnss.lat, gnss.ns));
    strcpy(longitud, formatCoordinates(gnss.lon, gnss.ew));
    //snprintf(message, sizeof(message), "STT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;%d;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;0;1;5676;4.1;11.94", nvs_data.device_id, date_time, cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, latitud, longitud, gnss.speed,gnss.course, gnss.gps_svs,gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign);
    snprintf(message, sizeof(message), "STT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;11;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;0;1;5676;4.1;11.94", nvs_data.device_id, date_time, cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, latitud, longitud, gnss.speed,gnss.course, gnss.gps_svs,gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign);
    // Esto solo es para simular el mandado
    ESP_LOGI(TAG, "TRACK:%s", message);
    //sst26_guardar_mensaje(message);
    send_ctrlZ(message);
}

void send_inputOnAlert_data(){
    char message[256];
    strcpy(latitud, formatCoordinates(gnss.lat, gnss.ns));
    strcpy(longitud, formatCoordinates(gnss.lon, gnss.ew));
    snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;11;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, longitud, latitud,gnss.speed, gnss.course,gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign, 33);
    ESP_LOGI(TAG, "ALT:%s", message);
    send_ctrlZ(message);
}

void send_inputOffAlert_data(){
    char message[256];
    strcpy(latitud, formatCoordinates(gnss.lat, gnss.ns));
    strcpy(longitud, formatCoordinates(gnss.lon, gnss.ew));
    snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;11;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, longitud, latitud,gnss.speed, gnss.course,gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign, 34);
    ESP_LOGI(TAG, "ALT:%s", message);
    send_ctrlZ(message);
}