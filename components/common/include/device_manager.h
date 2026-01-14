#pragma once

void device_init(void);
void check_nvs_data(void);
void check_imei_match(const char *current_imei);
void check_ccid_match(const char *current_ccid);