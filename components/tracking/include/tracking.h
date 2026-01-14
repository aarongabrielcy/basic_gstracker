#pragma once
#include <stdint.h>
void start_event_handler(void);
void send_track_data(void);
void setTimeUTC(const char *time);
void set_net_connectivity(uint8_t signal);