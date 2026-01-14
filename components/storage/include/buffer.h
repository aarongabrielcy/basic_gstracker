#pragma once

#define START_ADDR 0x001000 // Dirección base (Sector 1) (BLOQUE)
#define MAX_ADDR   0x002000

void buffer_init(void);
void sst26_guardar_mensaje(const char *message);
void sst26_leer_enviar_y_borrar();