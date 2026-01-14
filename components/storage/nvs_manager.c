#include "nvs_manager.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "esp_check.h"

//#define STORAGE_NAMESPACE "storage"
static const char *TAG = "NVS_MANAGER";

// Inicializa NVS (Solo llamar una vez en `app_main()`)
esp_err_t nvs_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}
// Guardar un string con una clave específica
esp_err_t nvs_save_str(const char* key, const char* value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK) {
        nvs_commit(handle);
        ESP_LOGI(TAG, "%s guardado: %s", key, value);
    }

    nvs_close(handle);
    return err;
}

// Leer un string con una clave específica
char* nvs_read_str(char* key, char* buffer, size_t buffer_size) {
    nvs_handle_t handle;
    size_t required_size;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return NULL;

    err = nvs_get_str(handle, key, NULL, &required_size);
    if (err != ESP_OK || required_size > buffer_size) {
        nvs_close(handle);
        return NULL;
    }

    err = nvs_get_str(handle, key, buffer, &required_size);
    nvs_close(handle);

    return (err == ESP_OK) ? buffer : NULL;
}


// Guardar un entero con una clave específica
esp_err_t nvs_save_int(const char* key, int value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK) {
        nvs_commit(handle);
        ESP_LOGI(TAG, "%s guardado: %d", key, value);
    }
    nvs_close(handle);
    return err;
}

//Leer un entero con una clave específica
int nvs_read_int(const char* key) {
    nvs_handle_t handle;
    int32_t value = 0;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return 0;

    err = nvs_get_i32(handle, key, &value);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "%s leído: %ld", key, (long)value);
    }
    nvs_close(handle);
    return value;
}
// Borrar un valor (string o entero) con una clave específica
esp_err_t nvs_delete_key(const char* key) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
        nvs_commit(handle);
        ESP_LOGI(TAG, "%s eliminado", key);
    }
    nvs_close(handle);
    return err;
}
/**~~~~~~~~~~~~~~~~~~~~ FIFO SAVE IN NVS ~~~~~~~~~~~~~~~~~~~~**/

// Obtiene un índice almacenado en NVS (lectura/escritura)
static esp_err_t get_index(nvs_handle_t handle, const char *key, int32_t *index) {
    esp_err_t err = nvs_get_i32(handle, key, index);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *index = 0;
        err = ESP_OK;
    }
    return err;
}
// Actualiza un índice en NVS
static esp_err_t set_index(nvs_handle_t handle, const char *key, int32_t index) {
    return nvs_set_i32(handle, key, index);
}
// Guarda datos en FIFO dentro de NVS
esp_err_t save_data_in_nvs(const char *data) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error abriendo NVS: %s", esp_err_to_name(err));
        return err;
    }

    int32_t write_index;
    get_index(handle, WRITE_INDEX_KEY, &write_index);
    char key[16];
    snprintf(key, sizeof(key), "%s%ld", KEY_PREFIX, write_index);

    err = nvs_set_str(handle, key, data);
    if (err == ESP_OK) {
        write_index++;
        set_index(handle, WRITE_INDEX_KEY, write_index);
        nvs_commit(handle);
        ESP_LOGI(TAG, "Guardado en %s: %s", key, data);
    } else {
        ESP_LOGE(TAG, "Error guardando en %s: %s", key, esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}
// Lee el primer dato almacenado (FIFO) sin eliminarlo
char* read_first_list_storage() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error abriendo NVS: %s", esp_err_to_name(err));
        return NULL;
    }
    int32_t read_index, write_index;
    get_index(handle, READ_INDEX_KEY, &read_index);
    get_index(handle, WRITE_INDEX_KEY, &write_index);

    if (read_index == write_index) {
        ESP_LOGI(TAG, "FIFO vacío, no hay datos.");
        nvs_close(handle);
        return NULL;
    }
    char key[16];
    snprintf(key, sizeof(key), "%s%ld", KEY_PREFIX, read_index);

    size_t required_size = 0;
    err = nvs_get_str(handle, key, NULL, &required_size);
    if (err != ESP_OK) {
        nvs_close(handle);
        return NULL;
    }
    char *data = malloc(required_size);
    if (!data) {
        ESP_LOGE(TAG, "Memoria insuficiente");
        nvs_close(handle);
        return NULL;
    }
    err = nvs_get_str(handle, key, data, &required_size);
    if (err != ESP_OK) {
        free(data);
        data = NULL;
    }
    nvs_close(handle);
    return data;
}
// Borra el primer dato después de procesarlo
esp_err_t clean_data_in_nvs() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error abriendo NVS: %s", esp_err_to_name(err));
        return err;
    }

    int32_t read_index;
    get_index(handle, READ_INDEX_KEY, &read_index);

    char key[16];
    snprintf(key, sizeof(key), "%s%ld", KEY_PREFIX, read_index);

    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
        read_index++;
        set_index(handle, READ_INDEX_KEY, read_index);
        nvs_commit(handle);
        ESP_LOGI(TAG, "Borrado %s, nuevo índice de lectura: %ld", key, read_index);
    } else {
        ESP_LOGE(TAG, "Error borrando %s: %s", key, esp_err_to_name(err));
    }

    nvs_close(handle);
    return err;
}