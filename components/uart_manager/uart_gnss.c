#include "uart_gnss.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "gnss_nmea.h"
#include "string.h"
#include "eventHandler.h"
#include "tracker_model.h"
#define EPSILON 0.0001

static const char *TAG = "UART_MANAGER_GNSS";
bool noChangeReported = true;

#define BUF_SIZE_OTA 1024
int uartGnss_readEvent(char *buffer, int max_length, int timeout_ms);
void uartManager_start_2(void);

void uart_gnss_init() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TX_GNSS_UART, RX_GNSS_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_2, BUF_SIZE * 2, 0, 0, NULL, 0);
    uartManager_start_2();
}

// FUNCION AUXILIAR PARA IMPRIMIR EL STRUCT DE LAS SENTENCIAS NMEA

void print_gnss_data(const gnssData_t *data) {
    ESP_LOGI("GPS_LOG", "----------------------------------------");
    /*ESP_LOGI("GPS_LOG", "ESTADO FIX : %d", data->fix);
    ESP_LOGI("GPS_LOG", "MODO       : %dD", data->mode);
    ESP_LOGI("GPS_LOG", "SATELITES  : GPS:%d | GLSS:%d | BEID:%d", data->gps_svs, data->glss_svs, data->beid_svs);
    
    // Usamos %.6f para no perder precisión en las coordenadas
    ESP_LOGI("GPS_LOG", "UBICACION  : LAT: %.6f %c | LON: %.6f %c", data->lat, data->ns, data->lon, data->ew);
    
    ESP_LOGI("GPS_LOG", "ALTITUD    : %.2f m", data->alt);
    ESP_LOGI("GPS_LOG", "VELOCIDAD  : %.2f km/h", data->speed);
    ESP_LOGI("GPS_LOG", "CURSO      : %.2f °", data->course);
    
    // Imprimimos cadenas de texto (Fecha y Hora)
    ESP_LOGI("GPS_LOG", "FECHA      : %s", data->date);
    ESP_LOGI("GPS_LOG", "HORA (UTC) : %s", data->utctime);
    
    // Precisión (DOP)
    ESP_LOGI("GPS_LOG", "DOP        : PDOP:%.2f | HDOP:%.2f | VDOP:%.2f", data->pdop, data->hdop, data->vdop);*/
    ESP_LOGI(TAG, "lat: %.6f, lon: %.6f, alt: %.2f, speed: %.2f, course: %.2f, fix: %d, sat %d:, date: %s, time: %s", data->lat, data->lon, data->alt, data->speed, data->course, data->fix, data->gps_svs, data->date, data->utctime);
    ESP_LOGI("GPS_LOG", "----------------------------------------");
}

static void gnss_task_init(void *arg) {
    char response_gnss[1024];
    while (1) {
        int len = uartGnss_readEvent(response_gnss, sizeof(response_gnss), 300);
        if (len > 0) {
            char *line;
            char *saveptr; 
            line = strtok_r(response_gnss, "\r\n", &saveptr);
            while (line != NULL) {
                if (line[0] == '$') {
                    parse_nmea_sentence(line, &gnss);
                }
                line = strtok_r(NULL, "\r\n", &saveptr);
            }
            if (gnss.fix > 0) {
                if(checkSignificantCourseChange(gnss.course) && tkr.ign){
                    ESP_LOGI(TAG, " Envío en curva ===============>");
                    curve_tracking_timer();
                    noChangeReported = false;
                    
                }
                else if(!noChangeReported){
                    ESP_LOGI(TAG, " cambiando a reporte normal ~~~~~~~~~~~~~~~~>");
                    normal_tracking_timer();
                    noChangeReported = true;
                }
            } else {
                //ESP_LOGI(TAG, "Esperando señal GPS...");
            }
            if(GNSS_DATA) {
                print_gnss_data(&gnss);
            }
        }
    }
}

int uartGnss_readEvent(char *buffer, int max_length, int timeout_ms) {
    int len = uart_read_bytes(UART_NUM_2, (uint8_t *)buffer, max_length - 1, pdMS_TO_TICKS(timeout_ms));
    if (len > 0) {
        buffer[len] = '\0';
    }
    return len;
}

void uartManager_start_2() {
    xTaskCreate(gnss_task_init, "uart_task", 8192, NULL, 5, NULL);
}