#pragma once
#include <stdint.h>

typedef struct {
    char rep_map[16];    // "3FFFFF"
    int model;           // 99
    char sw_ver[8];      // "1.0.1"
    int msg_type;        // 1
    int tkr_course;
    int tkr_meters;
    /*int in1_state;    // "00000100"
    int in_ig_st;
    int out1_state;   // "00001000" */
    int mode;            // 0
    int stt_rpt_type;    // 0
    int msg_num;         // 0
    float bck_volt;      // 0.00
    float power_volt;    // 0.00
    int ign;
    int sim_inserted;
    int network_status;
    int tcp_connected;
    int wifi_connected;
    int bluetooth_connected;
} trackerData_t;

extern trackerData_t tkr;