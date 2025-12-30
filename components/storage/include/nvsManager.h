#pragma once

#include <stdio.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"

#define STORAGE_NAMESPACE "storage"
#define MAX_ENTRIES 1000      // Máximo de cadenas FIFO
#define MAX_STRING_SIZE 140   // Tamaño máximo de cada cadena
#define MAX_BUFFER_SIZE 10240 // Tamaño del buffer para concatenar cadenas

#define KEY_PREFIX "entry_"  // Prefijo de las claves
#define READ_INDEX_KEY "read_index"
#define WRITE_INDEX_KEY "write_index"

// Inicializa NVS (una sola vez)
esp_err_t nvs_init(void);

// Guardar y leer strings (ejemplo: dev_id, WiFi SSID, etc.)
esp_err_t nvs_save_str(const char* key, const char* value);
char* nvs_read_str(char* key, char* buffer, size_t buffer_size);

// Guardar y leer enteros (ejemplo: modo de configuración)
esp_err_t nvs_save_int(const char* key, int value);
int nvs_read_int(const char* key);

esp_err_t nvs_delete_key(const char* key);

/** BUFFER FIFO **/
esp_err_t save_data_in_nvs(const char *data);
char* read_first_list_storage();
esp_err_t clean_data_in_nvs();