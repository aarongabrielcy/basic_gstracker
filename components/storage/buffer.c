#include "buffer.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "config.h"

static const char *TAG = "BUFFER";
spi_device_handle_t spi;
uint32_t write_ptr = START_ADDR; // inicio del mensaje

bool sst26_read_jedec_id();
void send_cmd(uint8_t cmd);
void sst26_wait_for_write_complete();
void sst26_unlock_protection();

void sst26_sector_erase(uint32_t addr);
void sst26_page_program(uint32_t addr, uint8_t *data, size_t len);
void sst26_read_data(uint32_t addr, uint8_t *dest, size_t len);

void sst26_guardar_mensaje(const char *message);
void sst26_leer_enviar_y_borrar();

void buffer_init(){
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_MISO, .mosi_io_num = PIN_MOSI, .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 4096
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, .mode = 0, .spics_io_num = PIN_CS,
        .queue_size = 7,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    if (!sst26_read_jedec_id()) {
        ESP_LOGE(TAG, "No se detectó la memoria SST26!");
        return;
    }
    sst26_unlock_protection();
    vTaskDelay(pdMS_TO_TICKS(500));

}

bool sst26_read_jedec_id() {
    uint8_t tx[4] = {0x9F, 0, 0, 0}; 
    uint8_t rx[4] = {0};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 32; t.rxlength = 32;
    t.tx_buffer = tx; t.rx_buffer = rx;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    
    if (rx[1] == 0xBF && rx[2] == 0x26) return true;
    return false;
}

// FUNCIONES DE COMUNICACION 
void send_cmd(uint8_t cmd) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

void sst26_wait_for_write_complete() {
    uint8_t status_reg;
    uint8_t cmd_status = 0x05;
    spi_transaction_t t;
    do {
        memset(&t, 0, sizeof(t));
        t.length = 8; t.tx_buffer = &cmd_status;
        t.rxlength = 8; t.rx_buffer = &status_reg;
        ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    } while (status_reg & 0x01); 
}

void sst26_unlock_protection() {
    send_cmd(0x06); // Write Enable
    vTaskDelay(pdMS_TO_TICKS(10));
    send_cmd(0x98); // ULBPR (Global Block Protection Unlock)
    vTaskDelay(pdMS_TO_TICKS(100)); // Espera a que el chip procese
    ESP_LOGI(TAG, "Protección desbloqueada");
}

// FUNCIONES DE MEMORIA

void sst26_sector_erase(uint32_t addr) {
    send_cmd(0x06); // WREN
    uint8_t cmd[4] = {0x20, (uint8_t)(addr >> 16), (uint8_t)(addr >> 8), (uint8_t)addr};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 32; t.tx_buffer = cmd;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    sst26_wait_for_write_complete();
}

void sst26_page_program(uint32_t addr, uint8_t *data, size_t len) {
    size_t cur_pos = 0;
    
    while (cur_pos < len) {
        // Calcular cuánto queda para el final de la página actual (256 bytes)
        size_t page_offset = addr % 256;
        size_t can_write = 256 - page_offset;
        size_t to_write = (len - cur_pos < can_write) ? len - cur_pos : can_write;

        send_cmd(0x06); // WREN para cada fragmento

        uint8_t *buffer = heap_caps_malloc(4 + to_write, MALLOC_CAP_DMA);
        if (!buffer) return;

        buffer[0] = 0x02; // Page Program Command
        buffer[1] = (addr >> 16) & 0xFF;
        buffer[2] = (addr >> 8) & 0xFF;
        buffer[3] = addr & 0xFF;
        memcpy(&buffer[4], &data[cur_pos], to_write);

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = (4 + to_write) * 8;
        t.tx_buffer = buffer;
        
        ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
        sst26_wait_for_write_complete();
        
        free(buffer);

        // Actualizar punteros para el siguiente fragmento
        addr += to_write;
        cur_pos += to_write;
    }
}

void sst26_read_data(uint32_t addr, uint8_t *dest, size_t len) {
    uint8_t *tx_buf = heap_caps_calloc(1, 4 + len, MALLOC_CAP_DMA);
    uint8_t *rx_buf = heap_caps_calloc(1, 4 + len, MALLOC_CAP_DMA);
    tx_buf[0] = 0x03;
    tx_buf[1] = (addr >> 16) & 0xFF;
    tx_buf[2] = (addr >> 8) & 0xFF;
    tx_buf[3] = addr & 0xFF;
    
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = (4 + len) * 8;
    t.rxlength = (4 + len) * 8;
    t.tx_buffer = tx_buf;
    t.rx_buffer = rx_buf;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
    memcpy(dest, &rx_buf[4], len);
    free(tx_buf); free(rx_buf);
}

// Funciones externas
void sst26_guardar_mensaje(const char *message) {
    uint16_t len = (uint16_t)strlen(message);
    
    if (write_ptr == START_ADDR) {
        sst26_sector_erase(START_ADDR);
    }

    // Creamos un solo buffer temporal para enviar todo junto
    uint8_t *temp = (uint8_t*)malloc(len + 2);
    if (!temp) return;

    temp[0] = (uint8_t)(len >> 8);
    temp[1] = (uint8_t)(len & 0xFF);
    memcpy(&temp[2], message, len);

    // Escribimos el bloque completo
    sst26_page_program(write_ptr, temp, len + 2);
    uint8_t check[2];
    sst26_read_data(write_ptr, check, 2);
    ESP_LOGI(TAG, "VERIFICACION INMEDIATA: Escrito 0x%02X%02X, Leido 0x%02X%02X\n", 
        (uint8_t)(len >> 8), (uint8_t)(len & 0xFF), check[0], check[1]);
    printf("Guardado: \"%s\" en 0x%06lX (Total bytes: %d)\n", message, write_ptr, len + 2);

    write_ptr += (len + 2); // El puntero avanza exactamente lo que se escribió
    free(temp);
}

void sst26_leer_enviar_y_borrar() {
    // Usamos printf para mayor velocidad y evitar que el buffer de log se pierda
    ESP_LOGI(TAG, "\n--- INICIANDO LECTURA DE MENSAJES ---\n");
    
    uint32_t read_ptr = START_ADDR;
    int contador = 0;

    // Escaneamos el sector (4096 bytes)
    while (read_ptr < write_ptr) {
        uint8_t header[2];
        sst26_read_data(read_ptr, header, 2);
        
        // Calculamos la longitud guardada
        uint16_t len = (header[0] << 8) | header[1];

        // Si leemos 0xFFFF, llegamos al final de los datos reales
        if (len == 0xFFFF || len == 0 || len > 1000) {
            break;
        }

        // Apartamos RAM para el texto
        char *buffer = (char*)malloc(len + 1);
        if (buffer != NULL) {
            // Leemos el contenido saltando los 2 bytes de cabecera
            sst26_read_data(read_ptr + 2, (uint8_t*)buffer, len);
            buffer[len] = '\0'; // Aseguramos que sea un string válido

            ESP_LOGI(TAG,"Mensaje [%d] en 0x%06lX (Len:%d): %s\n", contador, read_ptr, len, buffer);
            
            free(buffer);
            read_ptr += (2 + len);
            contador++;
        } else {
            ESP_LOGI(TAG,"Error: No hay RAM para procesar el mensaje\n");
            break;
        }
    }

    if (contador > 0) {
        ESP_LOGI(TAG,"Se procesaron %d mensajes. Limpiando memoria...\n", contador);
        sst26_sector_erase(START_ADDR);
    } else {
        ESP_LOGI(TAG,"No se encontraron mensajes válidos.\n");
    }

    // RESET del puntero al inicio
    write_ptr = START_ADDR;
    ESP_LOGI(TAG,"--- OPERACION FINALIZADA ---\n\n");
}