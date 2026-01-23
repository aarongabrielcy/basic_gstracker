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
#include "nvs_manager.h"

#define TAG "TRACKING PROCESSOR"

char date_time[34];
char latitud[20] = "+0.000000";
char longitud[20] = "+0.000000";

bool network = false;
bool curve_tracking = false;
int event = DEFAULT;
static int keep_alive_interval = 600000; // Valor en milisegundos (10 minutos)

//AT+CIPSTART="TCP","201.122.135.23",6100
//AT+CIPSTART="TCP","34.196.135.179",5200
// //6008
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
    - Valida el estado del GPIO de IGN  para activar/descactivar keep alive
    - validar si tiene SIM antes de guardar en buffer
    PRIORIDAD 06/01/2026

*/
//agregar setTimeUTC a utilities.h
void setTimeUTC(const char *time){
    strncpy(date_time, time, sizeof(date_time) - 1);
    date_time[sizeof(date_time) - 1] = '\0';
}

void set_net_connectivity(uint8_t signal){
    tkr.network_status = signal;
}

void sync_tracker_data(){
    if(gnss.fix == 0){
        getTimeLocal();
        ESP_LOGI(TAG, "No hay FIX GNSS, usando ultima fecha y hora valida: %s", date_time);
        //UNIR UNA SOLA VALIDACION LAT Y LON
        /*if (nvs_data.last_latitud[0] != '\0' && nvs_data.last_longitud[0] != '\0') {
            printf("ya hay coordenas: %s, %s\n", nvs_data.last_latitud, nvs_data.last_longitud);

            strcpy(latitud, nvs_data.last_latitud);
            strcpy(longitud, nvs_data.last_longitud);
        
        } else if(nvs_read_str("last_valid_lat", latitud, sizeof(latitud)) != NULL 
            && nvs_read_str("last_valid_lon", longitud, sizeof(longitud)) != NULL) {
            ESP_LOGI(TAG, "last_coordinates_NVS=%s,%s", latitud, longitud);   
        }
        else {
            ESP_LOGI(TAG, "No hay coordenadas validas GNSS: %s, %s", latitud, longitud);
        } */ 
    }else {
        strcpy(latitud, formatCoordinates(gnss.lat, gnss.ns));
        strcpy(nvs_data.last_latitud, latitud);
        strcpy(longitud, formatCoordinates(gnss.lon, gnss.ew));
        strcpy(nvs_data.last_longitud, longitud);
        //guarda en nvs
        /*if(gnss.speed > 10.0){
            nvs_save_str("last_valid_lat", nvs_data.last_latitud);
            nvs_save_str("last_valid_lon", nvs_data.last_longitud);
            ESP_LOGI(TAG, "Saving last coordinates to NVS: %s, %s", nvs_data.last_latitud, nvs_data.last_longitud);
        }*/
        snprintf(date_time, sizeof(date_time), "%s;%s", formatDate(gnss.date), formatTime(gnss.utctime)); 
        ESP_LOGI(TAG, "Using current date_time: %s", date_time);  
    }
    /**
     * CREAR UNA TAREA CADA 10 SEGUNDOS QUE EJECUTE ESTO
     1. getCellData
     2. Cuando se ejecuta trackeo en curva no alcanza a leer el CPSI
     */
    getCellData();// ejecutar cada 28 segundos cambiar de lugar
    vTaskDelay(100);
    //if (network){
        uartManager_sendCommand("AT+CIPSEND");
   /* }
    else{
        // buffer
    }*/
}
void reconnect_network(){
    ESP_LOGI(TAG, "Reconnecting to network...");
    /*uartManager_sendCommand("AT+CIICR");
    vTaskDelay(2000);
    uartManager_sendCommand("AT+CIFSR");
    vTaskDelay(2000);*/
    uartManager_sendCommand("AT+CIPSTART=\"TCP\",\"201.122.135.23\",6100");
    vTaskDelay(1000);
    //uartManager_sendCommand("AT+CIPSEND");
    request_cipstatus();
}
static void send_ctrlZ(const char *message){
    char ctrl_z_str[2] = { 0x1A, '\0' };
    uartManager_sendCommand(message);
    vTaskDelay(100);
    uartManager_sendCommand(ctrl_z_str);
}

void send_track_data() {

    char message[256];
    switch (event) {       
        case TRACKING_RPT:
            snprintf(message, sizeof(message), "STT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;11;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;0;1;5676;4.1;11.94", nvs_data.device_id, date_time, cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, latitud, longitud, gnss.speed,gnss.course, gnss.gps_svs,gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign);
            event = tkr.tkr_course || tkr.tkr_meters ? TRACKING_RPT : DEFAULT;
        break;
        case IGNITION_ON:
            snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;11;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, latitud, longitud, gnss.speed, gnss.course,gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign, 33);
        event = DEFAULT;
        break;
        case IGNITION_OFF:
            snprintf(message, sizeof(message), "ALT;%s;3FFFFF;95;1.0.21;1;%s;%s;%d;%d;%s;11;%s;%s;%.2f;%.2f;%d;%d;%d%d00000%d;00000000;%d;;",nvs_data.device_id, date_time,cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, latitud, longitud, gnss.speed, gnss.course,gnss.gps_svs, gnss.fix, tkr.tkr_course, tkr.tkr_meters, tkr.ign, 34);
            event = DEFAULT;
        break;
        case KEEP_ALIVE:
            snprintf(message, sizeof(message), "ALV;%s",nvs_data.device_id);    
            event = DEFAULT;
        break;    
        default:
            snprintf(message, sizeof(message), "ALV;%s",nvs_data.device_id);
            ESP_LOGW(TAG, "<GNSS DATA>\n<date_time>%s,<lat>%s,<lon>%s,<speed>%.2f,<fix>%d", 
                date_time, latitud, longitud, gnss.speed, gnss.fix);
            ESP_LOGW(TAG, "<CELLNET DATA>\n<sys_mode>%s<oper>%s<cell_id>%s<mcc>%d<mnc>%d<lac>%s<rx_lvl>%d<freq_band>%s", 
                cpsi.sys_mode, cpsi.oper_mode, cpsi.cell_id, cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.rxlvl_rsrp, cpsi.frequency_band);
            ESP_LOGW(TAG, "<NVS TKR DATA>\n<id>%s,<ccid>%s,<wifi_AP_mac>%s,<Ble Mac>%s<tkr_course>%d,<tkr_meters>%d, <last_lat>%s,<last_lon>%s",
                nvs_data.device_id, nvs_data.sim_iccid, nvs_data.wifi_ap, nvs_data.blue_addr, tkr.tkr_course, tkr.tkr_meters, nvs_data.last_latitud, nvs_data.last_longitud);
            ESP_LOGW(TAG, "<TRACKER DATA>\n<ign>%d<sim_inserted>%d<network_status>%d<tcp_connected>%d<wifi_connected>%d<bluetooth_connected>%d", tkr.ign, tkr.sim_inserted, tkr.network_status, tkr.tcp_connected, tkr.wifi_connected, tkr.bluetooth_connected);
        break;
    }
    ESP_LOGI(TAG, "Event:%d, Message:%s", event, message);
    send_ctrlZ(message);
}

void system_event_handler(void *handler_arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch (event_id) {
        case IGNITION_ON:
            ESP_LOGI(TAG, "Ignition=> ENCENDIDA"); 
            tkr.ign = 1;
            event = IGNITION_ON;
            start_tracking_report_timer();
            stop_keep_alive_timer();
            sync_tracker_data();
            break;
        case IGNITION_OFF:
            ESP_LOGI(TAG, "Ignition=> APAGADA"); 
            tkr.ign = 0;
            event = IGNITION_OFF;
            stop_tracking_report_timer();
            start_keep_alive_timer();
            sync_tracker_data();
            break;
        case INPUT1_ON:
            ESP_LOGI(TAG, "INPUT_1=> ENCENDIDA");
            sync_tracker_data();
            break;
        case INPUT1_OFF:
            ESP_LOGI(TAG, "INPUT_1=> APAGADA");
            sync_tracker_data();
            break;
        case KEEP_ALIVE:
            request_cipstop();
            event = KEEP_ALIVE;
            ESP_LOGI(TAG, "Evento KEEP_ALIVE: han pasado %d minutos", keep_alive_interval / 60000);
            sync_tracker_data();
            break;
        case TRACKING_RPT:
            event = TRACKING_RPT;
            ESP_LOGI(TAG, "Generando TRACKING_RPT");
            // Esto ya funciona
            // buscar el CONNECT OK PARA ACEPTAR TRACKEOS
            // AL llegar a este este evento debe mandar un primer trackeo antes de empezar a mandarlo con el timmer
            sync_tracker_data();
        break;
        /*case DEFAULT:
        event = DEFAULT;
             ESP_LOGI(TAG, "Evento DEFAULT: Deteniendo timers");
            stop_tracking_report_timer();
            stop_keep_alive_timer();            
        break;*/
    }
}

void start_event_handler(void) {
    esp_event_loop_handle_t loop = get_event_loop();
    esp_event_handler_register_with(loop, SYSTEM_EVENTS, ESP_EVENT_ANY_ID, system_event_handler, NULL);
}
