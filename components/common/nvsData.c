#include "nvsData.h"
#include "eventHandler.h"

NvsData nvs_data = {
    .device_id = "0000000000",
    .imei_module = "000000000000000",
    .sim_iccid = "0000000000000000000",
    .wifi_station = "AA:BB:CC:DD:EE:FF",
    .wifi_ap = "AA:BB:CC:DD:EE:FF",
    .blue_addr = "AA:BB:CC:DD:EE:FF",
    .rst_count = 0,
    .pss_wifi = "MyPass",
    .last_latitud = "",
    .last_longitud = "",
    .tracking_time = 30000, // 30 seconds
    .tracking_curve_time = 3000,
    .keep_alive_time = KEEPALIVE_REPORT_TIME, // 30 minutes
};