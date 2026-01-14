#include "gnss_nmea.h"
#include <string.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "tracker_model.h"
#include <math.h>
#include "esp_check.h"
#include "esp_log.h"
#include "config.h"

#define ANGLE_THRESHOLD 15.0  // Umbral de cambio de ángulo
float previousCourse = -1.0;
#define COURSE_SPEED_THRESHOLD_KN  0.5f
/**
Ajuste de umbral:

- 0.2 kn sensible (≈0.37 km/h)
- 0.5 kn recomendado
- 1.0 kn "cero" cuando está casi quieto (≈1.85 km/h)
 */
#define TAG "NMEA"
// Inicializar los valores por defecto
gnssData_t gnss = {
    .mode = 0,
    .gps_svs = 0,
    .glss_svs = 0,
    .beid_svs = 0,
    .lat = 0.0,
    .ns = 'N',
    .lon = 0.0,
    .ew = 'W',
    .date = "00000000",
    .utctime = "00:00:00",
    .alt = 0.0,
    .speed = 0.00,
    .course = 0.00,    
    .pdop = 0.0,
    .hdop = 0.0,
    .vdop = 0.0,
    .fix = 0,
};

static double nmea_to_decimal(double nmea_coord, char direction) {
    if (nmea_coord == 0.0) return 0.0;
    
    int degrees = (int)(nmea_coord / 100);
    double minutes = nmea_coord - (degrees * 100);
    double decimal = degrees + (minutes / 60.0);
    
    if (direction == 'S' || direction == 'W') {
        decimal *= -1.0;
    }
    return decimal;
}

void parse_nmea_sentence(const char *line, gnssData_t *out_data) {
    if(RAW_NMEA) {
        ESP_LOGI(TAG, "RAW NMEA line DATA: %s", line);
    }
    if (line == NULL || out_data == NULL) return;

    // 1. $GNRMC - Datos básicos (Lat, Lon, Fecha, Hora, Velocidad)
    if (strstr(line, "$GNRMC")) {
        double raw_lat = 0, raw_lon = 0;
        char status;
        // Formato: $GNRMC,time,status,lat,N,lon,W,speed,course,date,...
        int parsed = sscanf(line, "$GNRMC,%[^,],%c,%lf,%c,%lf,%c,%f,%f,%[^,]",out_data->utctime, &status, &raw_lat, &out_data->ns, &raw_lon, &out_data->ew, &out_data->speed, &out_data->course, out_data->date);
        if (parsed < 3) return;
        if (status != 'A') {
            out_data->course = 0.0f;
            // opcional: también podrías poner speed=0
            // out_data->speed = 0.0f;
            return;
        }
        if (status == 'A' && raw_lat != 0.0 && raw_lon != 0.0) { // 'A' = Valid, 'V' = Void
            out_data->lat = nmea_to_decimal(raw_lat, out_data->ns);
            out_data->lon = nmea_to_decimal(raw_lon, out_data->ew);
        }
        // Zero out course when stationary:
        // Si la velocidad es muy baja, el rumbo no es confiable => 0
        if (out_data->speed < COURSE_SPEED_THRESHOLD_KN) {
            out_data->course = 0.0f;
        }
    }

    // 2. $GNGGA - Fix, Satélites en uso y Altitud
    else if (strstr(line, "$GNGGA")) {
        sscanf(line, "$GNGGA,%*[^,],%*f,%*c,%*f,%*c,%d,%d,%f,%f",&out_data->fix, &out_data->gps_svs, &out_data->hdop, &out_data->alt);
    }

    // 3. $GPGSA / $BDGSA - Precisión (DOP) y Modo
    else if (strstr(line, "$GPGSA") || strstr(line, "$BDGSA")) {
        // El modo está en el segundo campo, los DOP al final
        sscanf(line, "$%*2sGSA,%*c,%d", &out_data->mode);
        
        // Buscamos los últimos 3 valores (PDOP, HDOP, VDOP) antes del checksum
        //const char *last_commas = strrchr(line, ','); 
        // Nota: Un parseo real de GSA requiere contar comas, 
        // aquí simplificamos capturando los campos finales si existen.
        sscanf(line, "$%*2sGSA,%*c,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%*d,%f,%f,%f",&out_data->pdop, &out_data->hdop, &out_data->vdop);
    }

    // 4. $GPGSV / $BDGSV - Satélites en vista
    else if (strstr(line, "$GPGSV")) {
        // $GPGSV,num_msgs,msg_num,sats_in_view,...
        sscanf(line, "$GPGSV,%*d,%*d,%d", &out_data->gps_svs);
    }
    else if (strstr(line, "$BDGSV")) {
        sscanf(line, "$BDGSV,%*d,%*d,%d", &out_data->beid_svs);
    }
}

bool checkSignificantCourseChange(float currentCourse) {
    if (isnan(currentCourse)) {
        ESP_LOGI(TAG, "Advertencia: el valor del curso no es válido.");
        return false;
    }

    float difference = fabs(currentCourse - previousCourse);
    if (difference >= ANGLE_THRESHOLD) {
        ESP_LOGI(TAG, "Cambio significativo detectado en course:%.2f", difference);
        tkr.tkr_course = 1;
        previousCourse = currentCourse;  // Actualizar el valor anterior
        return true;
    }
    tkr.tkr_course = 0;
    //previousCourse = currentCourse;  // Actualizar de todos modos para la próxima comparación
    return false;
}