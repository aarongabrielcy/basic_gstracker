#pragma once

typedef struct {
    char device_id[11];
    char imei_module[16];
    char sim_iccid[20];
    char wifi_station[18];
    char wifi_ap[18];
    char blue_addr[18];
    int rst_count;
    char pss_wifi[10];
} NvsData;

extern NvsData nvs_data;