#pragma once

typedef enum {
    KLRT = 11,
    RTMS = 12,
    RTMC = 13,
    SVPT = 14,
    TKRT = 15, 
    DTRM = 16,
    DLBF = 17,
    MTRS = 18,
    OPCT = 19,
    CWPW = 101,
    EBWF = 102,
    AEGP = 103, //ACTIVE GPS
    FWUP = 104
  } cmd_action_t;
  
  typedef enum {
    TKRP = 21,
    CLOP = 22,
    DVID = 23,
    DVIM = 24,
    CLDT = 25,
    RTCT = 26,
    IGST = 27,
    SIID = 28,
    OPST = 29,
    PWFR = 31,
    LOCA = 32
  } cmd_query_t;

  typedef enum {
    EMPTY = 0,
    QUERY_WITHOUT_VALUE = 1,
    QUERY_WITH_VALUE = 2,
    ACTION = 3,
    INVALID_CMD = 4,
    INVALID_SYMBOL = 5,
    INVALID_ACTION = 6,
    INVALID_NUMBER = 7,
    INVALID_QUERY_VALUE = 8,
    INVALID_END_SYMBOL = 9
} type_command_t;

#define NVS_KEY_KEEPALIVE "keepalive_t"
#define NVS_KEY_TRACKING_REPORT "tracking_t"

void uart_serial_init(void);
void uartManager_sendCommand(const char *command);