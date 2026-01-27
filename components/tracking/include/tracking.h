#pragma once
#include <stdint.h>
void start_event_handler(void);
void reconnect_network();
void reconnect_pdp();
void send_track_data(void);
void setTimeUTC(const char *time);
void set_net_connectivity(uint8_t signal);
void sync_tracker_data(void);