#pragma once

#include <stdbool.h>  
#include <stdint.h> 

typedef struct {
    int mode;
    int gps_svs;         // 0
    int glss_svs;
    int beid_svs;
    double lat;        // "+00.000000"
    char ns;
    double lon;        // "+/-00.000000"
    char ew;        
    char date[10];       // "00000000"
    char utctime[10];       // "00:00:00"
    float alt;
    float speed;         // 0.00
    float course;        // 0.00
    float pdop;
    float hdop;
    float vdop;
    int fix;             // 0
} gnssData_t;

extern gnssData_t gnss;

void parse_nmea_sentence(const char *line, gnssData_t *out_data);
bool checkSignificantCourseChange(float currentCourse);