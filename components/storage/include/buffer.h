#pragma once
#include "stdint.h"

#define START_ADDR 0x001000 // Dirección base (Sector 1) (BLOQUE)
#define MAX_ADDR   0x002000

#define SST26_SECTOR_SIZE      4096
#define SST26_PAGE_SIZE        256
#define SST26_MAX_MSG_LEN      1000

//#define MAX_SPI_CHUNK  2048

// SST26VF064B = 8MB => end no-inclusivo 0x800000
#define FLASH_START            0x000000
#define FLASH_END              0x800000

// META = 1 sector
#define FIFO_META_ADDR         FLASH_START                  // 0x000000

// DATA = resto
#define FIFO_DATA_START        (FLASH_START + SST26_SECTOR_SIZE) // 0x001000
#define FIFO_REGION_END        FLASH_END                     // 0x800000

#define FIFO_DATA_BYTES        (FIFO_REGION_END - FIFO_DATA_START)
#define FIFO_SECTORS_DATA      (FIFO_DATA_BYTES / SST26_SECTOR_SIZE)

// Cada cuántas escrituras guardas metadata (para no gastar el sector META)
#define META_SAVE_EVERY        10

void buffer_init(void);
void sst26_guardar_mensaje(const char *message);
void sst26_leer_enviar_y_borrar();
void fifo_print_used_sectors(void);
void fifo_erase_all(void);
void fifo_print_sector_messages(uint32_t sector_idx);