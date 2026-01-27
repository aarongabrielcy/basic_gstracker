#pragma once

#include <stdbool.h>
#include <stdint.h>

void sim7000_init(void);
void sim7000_connection_init(void);
void getTimeUTC(void);
void getTimeLocal(void);
uint8_t request_imei(void);
uint8_t request_ccid(void);
void getCellData(void);
uint8_t request_cipstatus(void);
uint8_t request_cipstop(void);