#include <stdbool.h>  
#include <stdint.h> 

typedef struct {
    char sys_mode[20];
    char oper_mode[20];
    int mcc;
    int mnc;
    char lac_tac[10];
    char cell_id[16];
    int rxlvl_rsrp;
    char frequency_band[50];
} cellnetData_t;

extern cellnetData_t cpsi;

bool parsePSI(char *response);