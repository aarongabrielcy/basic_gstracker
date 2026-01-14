#ifndef EVENT_HANDLER_H
#define EVENT_HANDLER_H

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(SYSTEM_EVENTS);

typedef enum {
    IGNITION_OFF,
    IGNITION_ON,
    INPUT1_ON, 
    INPUT1_OFF,
    INPUT2_ON, 
    INPUT2_OFF, 
    KEEP_ALIVE,
    TRACKING_RPT,
    SMS_DETECTED,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    DEFAULT
} system_event_t;

esp_event_loop_handle_t init_event_loop(void);
esp_event_loop_handle_t get_event_loop(void);

#define TRACKING_REPORT_TIME 10000 //30000
#define KEEPALIVE_REPORT_TIME 10000//1200000

/*void set_keep_alive_interval(uint32_t interval_ms);*/
void start_keep_alive_timer(void);
void stop_keep_alive_timer(void);

void start_tracking_report_timer(void);
void stop_tracking_report_timer(void);
void curve_tracking_timer();
void normal_tracking_timer();

#endif // EVENT_HANDLER_H