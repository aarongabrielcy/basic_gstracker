#include "eventHandler.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "tracking.h"
#include "nvsData.h"
#include "nvs_manager.h"
#define TAG "HANDLER"

static track_mode_t tracking_mode = TRACK_MODE_NORMAL;

ESP_EVENT_DEFINE_BASE(SYSTEM_EVENTS);

static esp_event_loop_handle_t event_loop_handle = NULL;
static esp_timer_handle_t keep_alive_timer = NULL;
static uint32_t keep_alive_interval; // 20 minutos por defecto / 10 minutos: 600000

static esp_timer_handle_t tracking_report_timer = NULL;
 //volver dinamico 30000 = 30 seg.  1000 = 1 seg.
static uint32_t tracking_report_interval = TRACKING_REPORT_TIME; //volver dinamico 30000 = 30 seg.  1000 = 1 seg.
uint32_t curve_tracking_interval; // 1 seg.

static void keep_alive_callback(void *arg) {
    uint32_t keep_alive_data = keep_alive_interval;
    esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, KEEP_ALIVE, &keep_alive_data, sizeof(uint32_t), portMAX_DELAY);
}
static void tracking_report_callback(void *arg) {
    uint32_t tracking_report_data = tracking_report_interval;
    esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, TRACKING_RPT, &tracking_report_data, sizeof(uint32_t), portMAX_DELAY);
}

esp_event_loop_handle_t init_event_loop(void) {

    if (!event_loop_handle) {
        ESP_LOGW(TAG, "Event loop no inicializado. Creando...");
        
        esp_event_loop_args_t loop_args = {
            .queue_size = 10,
            .task_name = "event_task",
            .task_priority = 5,
            .task_stack_size = 4096,  // Incrementado el stack por seguridad
            .task_core_id = 0
        };

        esp_err_t err = esp_event_loop_create(&loop_args, &event_loop_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error creando event loop: %s", esp_err_to_name(err));
            return NULL;
        }
        tracking_report_interval = nvs_read_int("tracking_t");
        if (tracking_report_interval < 10UL * MS_PER_SEC) {   
            ESP_LOGW(TAG, "reporte del rastreo no puede ser menor a 10 segundos, usando default");
            tracking_report_interval = TRACKING_REPORT_TIME;
        }
        keep_alive_interval = nvs_read_int("keepalive_t");
        if (keep_alive_interval < 5UL * MS_PER_MIN) {   
            ESP_LOGW(TAG, "keep a live no puede ser menor a 5 minutos, usando default");
            keep_alive_interval = KEEPALIVE_REPORT_TIME;
        }
        curve_tracking_interval = nvs_data.tracking_curve_time;
        if (curve_tracking_interval > 5000) {
            ESP_LOGW(TAG, "trackeo en curva No pude ser mayor, usando default");
            curve_tracking_interval = TRACKING_REPORT_CURVE_TIME;
        }
        start_event_handler();
    }
       
    return event_loop_handle;
}

esp_event_loop_handle_t get_event_loop(){
    if (!event_loop_handle) {
        ESP_LOGW(TAG, "Event loop no inicializado. Creando...");
        
        esp_event_loop_args_t loop_args = {
            .queue_size = 10,
            .task_name = "event_task",
            .task_priority = 5,
            .task_stack_size = 4096,  // Incrementado el stack por seguridad
            .task_core_id = 0
        };

        esp_err_t err = esp_event_loop_create(&loop_args, &event_loop_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Error creando event loop: %s", esp_err_to_name(err));
            return NULL;
        }
        
        //initialized = true;
    }   
    return event_loop_handle;
}

// Timers
void start_tracking_report_timer(void) {
    if (tracking_report_timer != NULL) {
        ESP_LOGW(TAG, "tracking_report ya está en ejecución");
        return;
    }
    esp_timer_create_args_t timer_args = {
        .callback = &tracking_report_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "tracking_report"
    };
    if (esp_timer_create(&timer_args, &tracking_report_timer) == ESP_OK) {
        esp_timer_start_periodic(tracking_report_timer, tracking_report_interval * 1000ULL);
        ESP_LOGI(TAG, "tracking_report iniciado");
    } else {
        ESP_LOGE(TAG, "Error creando el tracking_report");
    }
}
void stop_tracking_report_timer(void) {
    if (tracking_report_timer != NULL) {
        esp_timer_stop(tracking_report_timer);
        tracking_report_timer = NULL;
        ESP_LOGI(TAG, "tracking_report_timer detenido y eliminado");
    }
}

void start_keep_alive_timer(void) {
    if (keep_alive_timer != NULL) {
        ESP_LOGW(TAG, "keep_alive_timer ya está en ejecución");
        return;
    }

    esp_timer_create_args_t timer_args = {
        .callback = &keep_alive_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "keep_alive_timer"
    };
    if (esp_timer_create(&timer_args, &keep_alive_timer) == ESP_OK) {
        esp_timer_start_periodic(keep_alive_timer, keep_alive_interval * 1000ULL);
        ESP_LOGI(TAG, "keep_alive_timer iniciado");
    } else {
        ESP_LOGE(TAG, "Error creando el keep_alive_timer");
    }
}

void stop_keep_alive_timer(void) {
    if (keep_alive_timer != NULL) {
        esp_timer_stop(keep_alive_timer);
        keep_alive_timer = NULL;
        ESP_LOGI(TAG, "keep_alive_timer detenido y eliminado");
    }
}

void curve_tracking_timer(void) {
    if (tracking_mode == TRACK_MODE_CURVE) return; // ya está en 1s
    esp_timer_stop(tracking_report_timer);
    esp_timer_start_periodic(tracking_report_timer, (uint64_t)curve_tracking_interval * 1000ULL);
    tracking_mode = TRACK_MODE_CURVE;
    ESP_LOGI(TAG, "Timer -> CURVE (%lu ms)", (unsigned long)curve_tracking_interval);
}

void normal_tracking_timer(void) {
    if (tracking_mode == TRACK_MODE_NORMAL) return; // ya está en 30s
    esp_timer_stop(tracking_report_timer);
    esp_timer_start_periodic(tracking_report_timer, (uint64_t)tracking_report_interval * 1000ULL);
    tracking_mode = TRACK_MODE_NORMAL;
    ESP_LOGI(TAG, "Timer -> NORMAL (%lu ms)", (unsigned long)tracking_report_interval);
}

void update_keep_alive_timer(uint32_t new_interval_ms) {
    keep_alive_interval = new_interval_ms;
    if (keep_alive_timer != NULL) {
        esp_timer_stop(keep_alive_timer);
        esp_timer_start_periodic(
            keep_alive_timer,
            (uint64_t)new_interval_ms * 1000ULL
        );
        ESP_LOGI(TAG, "keep_alive_timer actualizado a %lu ms",
                 new_interval_ms);
    }
}
void update_tracking_report_timer(uint32_t new_interval_ms) {
    tracking_report_interval = new_interval_ms;
    if (tracking_report_timer != NULL) {
        esp_timer_stop(tracking_report_timer);
        esp_timer_start_periodic(
            tracking_report_timer,
            (uint64_t)new_interval_ms * 1000ULL
        );
        ESP_LOGI(TAG, "tracking_report_timer actualizado a %lu ms",
                 new_interval_ms);
    }
}

void update_curve_tracking_interval(uint8_t new_interval_s) {
    curve_tracking_interval = new_interval_s;
    ESP_LOGI(TAG, "curve_tracking_interval actualizado a %u s",
             new_interval_s);
}