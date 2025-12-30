#include "esp_system.h"
#include "esp_mac.h"
#include "deviceManager.h"
#include "nvsManager.h"
#include "esp_log.h"
#include "nvsData.h"
#include "esp_check.h"
//#include "sim7600.h"

#define TAG "REBOOT_TRACKER"

static void increment_reboot_counter(void);
static void assign_nvs_data(void);

void device_init(void) {
    //assign_nvs_data();
    increment_reboot_counter();   
}

static void assign_nvs_data(void) {
    /*if(nvs_read_str("device_id", nvs_data.device_id, sizeof(nvs_data.device_id)) != NULL){
        ESP_LOGI(TAG, "DEV ID=%s", nvs_data.device_id);   
    } else {
        if(sim7600_sendReadCommand("AT+SIMEI?") ) {
            ESP_LOGI(TAG, "Nuevo DEVICE ID guardado!");
        }
    }
    if(nvs_read_str("dev_simei", nvs_data.imei_module, sizeof(nvs_data.imei_module)) != NULL){
        ESP_LOGI(TAG, "SIM MOD=%s", nvs_data.imei_module);   
    } else {
        if(sim7600_sendReadCommand("AT+SIMEI?") ) {
            ESP_LOGI(TAG, "Nuevo SIMEI guardado!");
        }
    }

    if(nvs_read_str("sim_id", nvs_data.sim_iccid, sizeof(nvs_data.sim_iccid)) != NULL){
        ESP_LOGI(TAG, "SIM CCID=%s", nvs_data.sim_iccid);   
    } else {
        if(sim7600_sendReadCommand("AT+CICCID") ) {
            ESP_LOGI(TAG, "Nuevo CICCID guardado!");       
        }
    }

    /// valida NVS antes de inicializar si ya está guardadas en NVS retorna
    if(nvs_read_str("wifi_mac_ap", nvs_data.wifi_ap, sizeof(nvs_data.wifi_ap)) != NULL){
        ESP_LOGI(TAG, "WIFI MAC=%s", nvs_data.wifi_ap);   
    } else {
        uint8_t raw_ap[6];
        esp_read_mac(raw_ap, ESP_MAC_WIFI_SOFTAP);
        snprintf(nvs_data.wifi_ap, sizeof(nvs_data.wifi_ap),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             raw_ap[0], raw_ap[1], raw_ap[2],
             raw_ap[3], raw_ap[4], raw_ap[5]);

        nvs_save_str("wifi_mac_ap", nvs_data.wifi_ap);
    } 

    if(nvs_read_str("blue_mac", nvs_data.blue_addr, sizeof(nvs_data.blue_addr)) != NULL){
        ESP_LOGI(TAG, "BLUE MAC=%s", nvs_data.blue_addr);   
    } else {

        uint8_t raw_bt[6];
        esp_read_mac(raw_bt, ESP_MAC_BT);  
        snprintf(nvs_data.blue_addr, sizeof(nvs_data.blue_addr),
                "%02X:%02X:%02X:%02X:%02X:%02X",
                raw_bt[0], raw_bt[1], raw_bt[2],
                raw_bt[3], raw_bt[4], raw_bt[5]);
        
        nvs_save_str("blue_mac", nvs_data.blue_addr);

    }

    if(nvs_read_str("wifi_mac_sta", nvs_data.wifi_station, sizeof(nvs_data.wifi_station)) != NULL){
        ESP_LOGI(TAG, "WIFI STATION MAC=%s", nvs_data.wifi_station);   
    } else {
        uint8_t raw_sta[6];
        esp_read_mac(raw_sta, ESP_MAC_WIFI_STA);
            snprintf(nvs_data.wifi_station, sizeof(nvs_data.wifi_station),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             raw_sta[0], raw_sta[1], raw_sta[2],
             raw_sta[3], raw_sta[4], raw_sta[5]);
            nvs_save_str("wifi_mac_sta", nvs_data.wifi_station);

    }*/
}

static void increment_reboot_counter(void) {
    const char* reboot_key = "dev_reboots";

    // Leer el valor actual
    int count = nvs_read_int(reboot_key);
    nvs_data.rst_count = count;
    // Incrementar el contador
    count++;

    // Guardar el nuevo valor
    esp_err_t err = nvs_save_int(reboot_key, count);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Reinicio #%d guardado correctamente", count);
    } else {
        ESP_LOGE(TAG, "Error al guardar el contador de reinicios: %s", esp_err_to_name(err));
    }
}







