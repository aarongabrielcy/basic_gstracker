#include "buffer.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "config.h"

static const char *TAG = "BUFFER";

static spi_device_handle_t spi = NULL;
static SemaphoreHandle_t g_flash_mutex = NULL;

void fifo_erase_all(void);
void fifo_dump_used_sectors_blocks(uint32_t block_size);
bool fifo_read_abs(uint32_t addr, uint8_t *out, size_t len);
bool fifo_read_sector_block(uint32_t sector_idx, uint32_t offset, uint8_t *out, size_t len);
void fifo_print_used_sectors(void);
void fifo_print_sector_messages(uint32_t sector_idx);


static void flash_lock(void)   { if (g_flash_mutex) xSemaphoreTake(g_flash_mutex, portMAX_DELAY); }
static void flash_unlock(void) { if (g_flash_mutex) xSemaphoreGive(g_flash_mutex); }

static esp_err_t spi_txrx(const uint8_t *tx, uint8_t *rx, size_t bytes)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = bytes * 8;
    t.tx_buffer = tx;
    t.rx_buffer = rx;
    return spi_device_transmit(spi, &t);
}

static void send_cmd(uint8_t cmd)
{
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));
}

static uint8_t sst26_read_status(void)
{
    uint8_t tx[2] = {0x05, 0x00};   // RDSR + dummy
    uint8_t rx[2] = {0x00, 0x00};
    ESP_ERROR_CHECK(spi_txrx(tx, rx, sizeof(tx)));
    return rx[1];
}

static void sst26_wait_for_write_complete(void)
{
    uint8_t sr;
    do {
        sr = sst26_read_status();
    } while (sr & 0x01); // WIP
}

static void sst26_write_enable(void)
{
    send_cmd(0x06); // WREN
    // Espera WEL=1
    uint8_t sr;
    do {
        sr = sst26_read_status();
    } while (!(sr & 0x02));
}

static bool sst26_read_jedec_id(void)
{
    uint8_t tx[4] = {0x9F, 0x00, 0x00, 0x00};
    uint8_t rx[4] = {0};
    ESP_ERROR_CHECK(spi_txrx(tx, rx, sizeof(tx)));

    // SST26VF064B: Manufacturer 0xBF, Memory Type 0x26
    return (rx[1] == 0xBF && rx[2] == 0x26);
}

static void sst26_unlock_protection(void)
{
    // ULBPR (Global Block Protection Unlock)
    sst26_write_enable();
    send_cmd(0x98);
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "Proteccion desbloqueada (ULBPR)");
}

static void sst26_sector_erase(uint32_t addr)
{
    // 4KB Sector Erase = 0x20
    sst26_write_enable();

    uint8_t cmd[4] = {
        0x20,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 32;
    t.tx_buffer = cmd;
    ESP_ERROR_CHECK(spi_device_transmit(spi, &t));

    sst26_wait_for_write_complete();
}

#define MAX_SPI_CHUNK  4096   // debe ser <= buscfg.max_transfer_sz (tú lo tienes en 4096)

static void sst26_page_program(uint32_t addr, const uint8_t *data, size_t len)
{
    size_t cur = 0;

    while (cur < len) {
        // Limite por pagina (256)
        size_t page_off = addr % 256;
        size_t can_page = 256 - page_off;
        size_t to_write = (len - cur < can_page) ? (len - cur) : can_page;

        // Limite por max_transfer (incluye 4 bytes de cmd+addr)
        if (to_write > (MAX_SPI_CHUNK - 4)) {
            to_write = MAX_SPI_CHUNK - 4;
        }

        sst26_write_enable();

        uint8_t *buf = (uint8_t*)heap_caps_malloc(4 + to_write, MALLOC_CAP_DMA);
        if (!buf) return;

        buf[0] = 0x02;
        buf[1] = (addr >> 16) & 0xFF;
        buf[2] = (addr >> 8) & 0xFF;
        buf[3] = addr & 0xFF;
        memcpy(&buf[4], &data[cur], to_write);

        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        t.length = (4 + to_write) * 8;
        t.tx_buffer = buf;

        esp_err_t err = spi_device_transmit(spi, &t);
        free(buf);
        ESP_ERROR_CHECK(err);

        sst26_wait_for_write_complete();

        addr += to_write;
        cur  += to_write;
    }
}


static void sst26_read_data(uint32_t addr, uint8_t *dest, size_t len)
{
    // READ (0x03) + 3 bytes addr + data
    uint8_t *tx = (uint8_t*)heap_caps_calloc(1, 4 + len, MALLOC_CAP_DMA);
    uint8_t *rx = (uint8_t*)heap_caps_calloc(1, 4 + len, MALLOC_CAP_DMA);
    if (!tx || !rx) {
        if (tx) free(tx);
        if (rx) free(rx);
        return;
    }

    tx[0] = 0x03;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = (addr) & 0xFF;

    ESP_ERROR_CHECK(spi_txrx(tx, rx, 4 + len));
    memcpy(dest, &rx[4], len);

    free(tx);
    free(rx);
}

// =========================
// FIFO META
// =========================
#define FIFO_META_MAGIC 0x4649464F  // 'FIFO'

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t gen;         // monotonic (para elegir el record mas nuevo)
    uint32_t head_sector; // 0..FIFO_SECTORS_DATA-1
    uint32_t head_off;    // 0..4095
    uint32_t tail_sector; // 0..FIFO_SECTORS_DATA-1
    uint32_t tail_off;    // 0..4095
    uint32_t crc;
} fifo_meta_t;

static fifo_meta_t g_meta;
static uint32_t g_meta_dirty_writes = 0;

static uint32_t crc32_simple(const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t*)data;
    uint32_t c = 0xA5A5A5A5;
    for (size_t i = 0; i < len; i++) {
        c = (c << 5) ^ (c >> 27) ^ p[i];
    }
    return c;
}

static inline uint32_t sector_addr(uint32_t sector_idx)
{
    return FIFO_DATA_START + (sector_idx * SST26_SECTOR_SIZE);
}

static inline uint32_t next_sector(uint32_t s)
{
    return (s + 1) % FIFO_SECTORS_DATA;
}

static inline bool fifo_empty(void)
{
    return (g_meta.head_sector == g_meta.tail_sector) && (g_meta.head_off == g_meta.tail_off);
}

static bool fifo_meta_valid(const fifo_meta_t *m)
{
    if (m->magic != FIFO_META_MAGIC) return false;
    if (m->version != 1) return false;

    fifo_meta_t tmp = *m;
    tmp.crc = 0;
    uint32_t want = crc32_simple(&tmp, sizeof(tmp));
    return (want == m->crc);
}

static void fifo_save_meta_force(void)
{
    fifo_meta_t m = g_meta;
    m.crc = 0;
    m.crc = crc32_simple(&m, sizeof(m));

    // Busca primer slot vacio (0xFFFFFFFF en primera palabra)
    for (uint32_t off = 0; off + sizeof(fifo_meta_t) <= SST26_SECTOR_SIZE; off += sizeof(fifo_meta_t)) {
        uint32_t word = 0;
        sst26_read_data(FIFO_META_ADDR + off, (uint8_t*)&word, sizeof(word));
        if (word == 0xFFFFFFFF) {
            sst26_page_program(FIFO_META_ADDR + off, (uint8_t*)&m, sizeof(m));
            return;
        }
    }

    // Sector meta lleno -> erase y escribe de nuevo
    sst26_sector_erase(FIFO_META_ADDR);
    sst26_page_program(FIFO_META_ADDR, (uint8_t*)&m, sizeof(m));
}

static void fifo_save_meta_maybe(void)
{
    g_meta_dirty_writes++;
    if (g_meta_dirty_writes >= META_SAVE_EVERY) {
        fifo_save_meta_force();
        g_meta_dirty_writes = 0;
    }
}

static void fifo_load_meta(void)
{
    fifo_meta_t best = {0};
    bool found = false;

    // Escanea todos los records del sector META
    for (uint32_t off = 0; off + sizeof(fifo_meta_t) <= SST26_SECTOR_SIZE; off += sizeof(fifo_meta_t)) {
        fifo_meta_t m;
        sst26_read_data(FIFO_META_ADDR + off, (uint8_t*)&m, sizeof(m));
        if (fifo_meta_valid(&m)) {
            if (!found || m.gen > best.gen) {
                best = m;
                found = true;
            }
        }
    }

    if (found) {
        g_meta = best;
        return;
    }

    // No hay meta valida -> inicializa y borra TODO (1ra vez)
    memset(&g_meta, 0, sizeof(g_meta));
    g_meta.magic = FIFO_META_MAGIC;
    g_meta.version = 1;
    g_meta.gen = 1;
    g_meta.head_sector = 0;
    g_meta.head_off = 0;
    g_meta.tail_sector = 0;
    g_meta.tail_off = 0;

    ESP_LOGW(TAG, "FIFO: No hay metadata valida. Inicializando y borrando toda la flash FIFO...");

    sst26_sector_erase(FIFO_META_ADDR);

    // Borra todos los sectores de datos (puede tardar)
    for (uint32_t s = 0; s < FIFO_SECTORS_DATA; s++) {
        sst26_sector_erase(sector_addr(s));
    }

    fifo_save_meta_force();
    g_meta_dirty_writes = 0;
}

// =========================
// PUBLIC: INIT
// =========================
void buffer_init(void)
{
    if (!g_flash_mutex) {
        g_flash_mutex = xSemaphoreCreateMutex();
    }

    // Config SPI
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_MISO,
        .mosi_io_num = PIN_MOSI,
        .sclk_io_num = PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_CS,
        .queue_size = 7
    };

    // Inicializa bus solo si no existe (si tu app lo inicializa en otro lado, quita estas 2 lineas)
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi));

    flash_lock();

    if (!sst26_read_jedec_id()) {
        ESP_LOGE(TAG, "No se detecto SST26VF064B!");
        flash_unlock();
        return;
    }

    sst26_unlock_protection();

    // FIFO meta
    fifo_load_meta();

    ESP_LOGI(TAG, "FIFO ready: sectors=%lu  (META 0x%06X, DATA 0x%06X..0x%06X)",(unsigned long)FIFO_SECTORS_DATA,(unsigned)FIFO_META_ADDR,(unsigned)FIFO_DATA_START,(unsigned)FIFO_REGION_END);

    ESP_LOGI(TAG, "META: head S%lu off%lu | tail S%lu off%lu | gen%lu",(unsigned long)g_meta.head_sector, (unsigned long)g_meta.head_off,(unsigned long)g_meta.tail_sector, (unsigned long)g_meta.tail_off,(unsigned long)g_meta.gen);

    flash_unlock();
}

// =========================
// FIFO WRITE (guardar mensaje)
// =========================
void sst26_guardar_mensaje(const char *message)
{
    if (!message) return;

    uint16_t len = (uint16_t)strlen(message);
    if (len == 0 || len > SST26_MAX_MSG_LEN) return;

    uint32_t needed = (uint32_t)len + 2;
    if (needed > SST26_SECTOR_SIZE) return; // no deberia pasar con MAX_MSG_LEN=1000

    flash_lock();

    // Si no cabe en el sector actual, salta al siguiente sector
    if (g_meta.tail_off + needed > SST26_SECTOR_SIZE) {
        uint32_t next = next_sector(g_meta.tail_sector);

        // Si el siguiente sector es donde esta el head => buffer lleno, descarta el sector mas viejo
        if (next == g_meta.head_sector) {
            g_meta.head_sector = next_sector(g_meta.head_sector);
            g_meta.head_off = 0;
        }

        // Borra el sector donde escribiremos
        sst26_sector_erase(sector_addr(next));

        g_meta.tail_sector = next;
        g_meta.tail_off = 0;

        g_meta.gen++;
        fifo_save_meta_force();  // cambio estructural: fuerza guardado
        g_meta_dirty_writes = 0;
    }

    uint32_t addr = sector_addr(g_meta.tail_sector) + g_meta.tail_off;

    uint8_t *temp = (uint8_t*)malloc(needed);
    if (!temp) {
        flash_unlock();
        return;
    }

    temp[0] = (uint8_t)(len >> 8);
    temp[1] = (uint8_t)(len & 0xFF);
    memcpy(&temp[2], message, len);

    sst26_page_program(addr, temp, needed);

    // verificacion header
    uint8_t check[2] = {0};
    sst26_read_data(addr, check, 2);
    ESP_LOGI(TAG, "FIFO WRITE: S%lu off%lu addr=0x%06lX | Escrito 0x%02X%02X Leido 0x%02X%02X",(unsigned long)g_meta.tail_sector, (unsigned long)g_meta.tail_off,(unsigned long)addr,temp[0], temp[1], check[0], check[1]);

    g_meta.tail_off += needed;

    g_meta.gen++;
    fifo_save_meta_maybe();

    free(temp);
    flash_unlock();
}

// =========================
// FIFO READ helpers (peek + pop)
// =========================
static bool fifo_peek(char **out_msg, uint16_t *out_len)
{
    if (fifo_empty()) return false;

    // Si no cabe ni header, salta sector
    if (g_meta.head_off + 2 > SST26_SECTOR_SIZE) {
        g_meta.head_sector = next_sector(g_meta.head_sector);
        g_meta.head_off = 0;
        if (fifo_empty()) return false;
    }

    uint32_t addr = sector_addr(g_meta.head_sector) + g_meta.head_off;

    uint8_t header[2];
    sst26_read_data(addr, header, 2);
    uint16_t len = ((uint16_t)header[0] << 8) | header[1];

    // Si esta borrado o invalido, considera que ese sector ya no tiene data valida: salta sector
    if (len == 0xFFFF || len == 0 || len > SST26_MAX_MSG_LEN || (g_meta.head_off + 2 + len) > SST26_SECTOR_SIZE) {
        g_meta.head_sector = next_sector(g_meta.head_sector);
        g_meta.head_off = 0;
        g_meta.gen++;
        fifo_save_meta_force();
        g_meta_dirty_writes = 0;
        return false;
    }

    char *buf = (char*)malloc(len + 1);
    if (!buf) return false;

    sst26_read_data(addr + 2, (uint8_t*)buf, len);
    buf[len] = '\0';

    *out_msg = buf;
    *out_len = len;
    return true;
}

static void fifo_pop(uint16_t len)
{
    g_meta.head_off += (uint32_t)len + 2;

    // Si ya no cabe header, salta sector
    if (g_meta.head_off + 2 > SST26_SECTOR_SIZE) {
        g_meta.head_sector = next_sector(g_meta.head_sector);
        g_meta.head_off = 0;
    }

    g_meta.gen++;
    fifo_save_meta_maybe();
}

// =========================
// PUBLIC: leer/procesar FIFO y consumir (pop)
// =========================
void sst26_leer_enviar_y_borrar(void)
{
    flash_lock();

    ESP_LOGI(TAG, "\n--- FIFO: INICIANDO LECTURA/POP ---");
    int contador = 0;

    // OJO: aqui solo "imprime y consume".
    // Si quieres enviar y solo consumir si envio OK:
    //   - mete tu envio donde indica y si falla: break (NO pop)
    while (!fifo_empty()) {
        char *msg = NULL;
        uint16_t len = 0;

        if (!fifo_peek(&msg, &len)) {
            // pudo haber saltado sector invalido
            continue;
        }

        ESP_LOGI(TAG, "FIFO MSG[%d] (S%lu off%lu Len:%u): %s",contador,(unsigned long)g_meta.head_sector,(unsigned long)g_meta.head_off,(unsigned)len,msg);

        // === TU ENVIO IRIA AQUI ===
        // bool ok = enviar(msg);
        // if (!ok) { free(msg); break; }

        free(msg);

        fifo_pop(len);
        contador++;
    }

    ESP_LOGI(TAG, "FIFO: procesados/consumidos %d mensajes", contador);
    ESP_LOGI(TAG, "--- FIFO: FIN ---\n");

    flash_unlock();
}

static bool sst26_sector_has_data(uint32_t sector_addr_base)
{
    // offsets a muestrear (inicio, medio, final)
    const uint32_t offs[] = {0, 16, 256, 2048, 4096-16};
    uint8_t buf[16];

    for (int i = 0; i < (int)(sizeof(offs)/sizeof(offs[0])); i++) {
        sst26_read_data(sector_addr_base + offs[i], buf, sizeof(buf));
        for (int j = 0; j < (int)sizeof(buf); j++) {
            if (buf[j] != 0xFF) return true;
        }
    }
    return false;
}

// Imprime todos los sectores de DATA que tienen info
void fifo_print_used_sectors(void)
{
    flash_lock();

    uint32_t used = 0;
    for (uint32_t s = 0; s < FIFO_SECTORS_DATA; s++) {
        uint32_t base = sector_addr(s);
        if (sst26_sector_has_data(base)) {
            ESP_LOGI(TAG, "SECTOR CON INFO: idx=%lu addr=0x%06lX", (unsigned long)s, (unsigned long)base);
            used++;
        }
    }
    ESP_LOGI(TAG, "Total sectores con info: %lu / %lu", (unsigned long)used, (unsigned long)FIFO_SECTORS_DATA);

    flash_unlock();
}

bool fifo_read_sector_block(uint32_t sector_idx, uint32_t offset, uint8_t *out, size_t len){
    if (!out) return false;
    if (sector_idx >= FIFO_SECTORS_DATA) return false;
    if (offset + len > SST26_SECTOR_SIZE) return false;

    uint32_t addr = sector_addr(sector_idx) + offset;

    flash_lock();
    sst26_read_data(addr, out, len);
    flash_unlock();

    return true;
}

bool fifo_read_abs(uint32_t addr, uint8_t *out, size_t len){
    if (!out) return false;
    if (addr + len > FLASH_END) return false;

    flash_lock();
    sst26_read_data(addr, out, len);
    flash_unlock();

    return true;
}

static void print_hex(const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++) printf("%02X ", p[i]);
    printf("\n");
}

void fifo_dump_used_sectors_blocks(uint32_t block_size){
    if (block_size == 0 || block_size > SST26_SECTOR_SIZE) return;

    uint8_t *tmp = malloc(block_size);
    if (!tmp) return;

    flash_lock();

    for (uint32_t s = 0; s < FIFO_SECTORS_DATA; s++) {
        uint32_t base = sector_addr(s);
        if (!sst26_sector_has_data(base)) continue;

        ESP_LOGI(TAG, "DUMP sector=%lu addr=0x%06lX", (unsigned long)s, (unsigned long)base);

        // lee el sector por bloques
        for (uint32_t off = 0; off < SST26_SECTOR_SIZE; off += block_size) {
            uint32_t to_read = block_size;
            if (off + to_read > SST26_SECTOR_SIZE) to_read = SST26_SECTOR_SIZE - off;

            sst26_read_data(base + off, tmp, to_read);

            // Aquí decides qué hacer con el bloque (print, enviar, etc.)
            printf("  off=0x%03lX: ", (unsigned long)off);
            print_hex(tmp, to_read);

            // Si quieres cortar cuando ya todo es 0xFF:
            bool all_ff = true;
            for (uint32_t i = 0; i < to_read; i++) { if (tmp[i] != 0xFF) { all_ff = false; break; } }
            if (all_ff) break;
        }
    }

    flash_unlock();
    free(tmp);
}

void fifo_erase_all(void){
    flash_lock();

    ESP_LOGW(TAG, "FIFO ERASE ALL: borrando META y todos los sectores DATA...");

    // borra META
    sst26_sector_erase(FIFO_META_ADDR);

    // borra todos los sectores de DATA
    for (uint32_t s = 0; s < FIFO_SECTORS_DATA; s++) {
        sst26_sector_erase(sector_addr(s));
    }

    // reset meta en RAM
    memset(&g_meta, 0, sizeof(g_meta));
    g_meta.magic = FIFO_META_MAGIC;
    g_meta.version = 1;
    g_meta.gen = 1;
    g_meta.head_sector = 0;
    g_meta.head_off = 0;
    g_meta.tail_sector = 0;
    g_meta.tail_off = 0;

    g_meta_dirty_writes = 0;
    fifo_save_meta_force();

    ESP_LOGW(TAG, "FIFO ERASE ALL: listo (vacio).");

    flash_unlock();
}

void fifo_print_sector_messages(uint32_t sector_idx)
{
    if (sector_idx >= FIFO_SECTORS_DATA) {
        ESP_LOGE(TAG, "Sector_idx invalido: %lu (max %lu)",
                 (unsigned long)sector_idx, (unsigned long)(FIFO_SECTORS_DATA - 1));
        return;
    }

    uint32_t base = sector_addr(sector_idx);

    flash_lock();

    ESP_LOGI(TAG, "=== MENSAJES EN SECTOR %lu  addr=0x%06lX ===",
             (unsigned long)sector_idx, (unsigned long)base);

    uint32_t off = 0;
    int msg_i = 0;

    while (off + 2 <= SST26_SECTOR_SIZE) {
        uint8_t header[2];
        sst26_read_data(base + off, header, 2);

        uint16_t len = ((uint16_t)header[0] << 8) | header[1];

        // fin: zona borrada
        if (len == 0xFFFF) {
            ESP_LOGI(TAG, "Fin (zona borrada) en off=%lu", (unsigned long)off);
            break;
        }

        // header invalido o basura => detenemos (para no imprimir cosas raras)
        if (len == 0 || len > SST26_MAX_MSG_LEN) {
            ESP_LOGW(TAG, "Header invalido en off=%lu: len=%u. Stop.",
                     (unsigned long)off, (unsigned)len);
            break;
        }

        // si el payload se sale del sector, detenemos
        if (off + 2 + len > SST26_SECTOR_SIZE) {
            ESP_LOGW(TAG, "Mensaje se sale del sector: off=%lu len=%u. Stop.",
                     (unsigned long)off, (unsigned)len);
            break;
        }

        char *msg = (char*)malloc(len + 1);
        if (!msg) {
            ESP_LOGE(TAG, "No hay RAM para leer mensaje len=%u", (unsigned)len);
            break;
        }

        sst26_read_data(base + off + 2, (uint8_t*)msg, len);
        msg[len] = '\0';

        ESP_LOGI(TAG, "MSG[%d] off=%lu len=%u: %s", msg_i, (unsigned long)off, (unsigned)len, msg);

        free(msg);

        off += 2 + len;
        msg_i++;
    }

    ESP_LOGI(TAG, "=== FIN SECTOR %lu (mensajes=%d) ===", (unsigned long)sector_idx, msg_i);

    flash_unlock();
}
