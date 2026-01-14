#pragma once
#include "driver/gpio.h"

#define POWER_KEY_PIN 46
#define POWER_SIM_PIN 40
#define GNSS_LED_PIN  1
#define POWER_LED_PIN 45
//#define SIM_DTR_PIN   11
#define IGNITION_PIN  8
#define TIMER_INTERVAL (30 * 60 * 1000) // 30 minutos en milisegundos
#define OUTPUT1_PIN 20
#define INPUT2_PIN 21 
#define RXD_MAIN_UART 18 //10
#define TXD_MAIN_UART 17 //11
#define UART_SIM UART_NUM_1
#define RX_GNSS_UART 7
#define TX_GNSS_UART 6
#define UART_GNSS UART_NUM_2
#define BUF_SIZE 1024
#define I2C_MASTER_SCL_IO 15
#define I2C_MASTER_SDA_IO 16
#define PIN_CS    19
#define PIN_CLK   12
#define PIN_MOSI  10
#define PIN_MISO  9 
#define RAW_NMEA false
#define RAW_GNSS false