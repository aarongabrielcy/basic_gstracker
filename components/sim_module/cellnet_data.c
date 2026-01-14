#include "cellnet_data.h"
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "utilities.h"

#define TAG "CELL_NETWORK"
// Inicializar los valores por defecto
cellnetData_t cpsi = {
    .sys_mode = "na",
    .oper_mode = "na",
    .mcc = 0,
    .mnc = 0,
    .lac_tac = "FFFF",
    .cell_id = "00000000",
    .rxlvl_rsrp = 999,
    .frequency_band = "NO BAND"
};

void parseGSM(char *tokens);
void parseLTE(char *tokens);
void parseWCDMA(char *tokens);
void parseCDMA(char *tokens);
void parseEVDO(char *tokens);


bool parsePSI(char *response) {
    char *cleanResponse = cleanData(response, "CPSI");

    if (cleanResponse == NULL) {
        ESP_LOGE(TAG, "Error: No se pudo limpiar la respuesta CPSI.");
        return false;
    }
    // Verificar con qué cabecera comienza la cadena
    if (strncmp(cleanResponse, "GSM", 3) == 0) {
        parseGSM(cleanResponse);
    } else if (strncmp(cleanResponse, "LTE", 3) == 0) {
        parseLTE(cleanResponse);
    } else if (strncmp(cleanResponse, "WCDMA", 5) == 0) {
        parseWCDMA(cleanResponse);
    } else if (strncmp(cleanResponse, "CDMA", 4) == 0) {
        parseCDMA(cleanResponse);
    } else if (strncmp(cleanResponse, "EVDO", 4) == 0) {
        parseEVDO(cleanResponse);
    }else if (strncmp(cleanResponse, "NO SERVICE", 10) == 0) {
        ESP_LOGW(TAG, "Red celular: %s", cleanResponse);
        return false;
    }else {
        ESP_LOGW(TAG, "Formato de CPSI inválido: %s", cleanResponse);
        return false;
    }
    return true;
}
void parseGSM(char *tokens) {
    char *values[10] = {NULL};  
    int count = 0;
    
    // Separar la cadena en tokens
    char *token = strtok(tokens, ",");
    while (token != NULL && count < 10) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 9) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en GSM.");
        return;
    }
    strncpy(cpsi.sys_mode, values[0], sizeof(cpsi.sys_mode) - 1);
    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    strncpy(cpsi.lac_tac, removeHexPrefix(values[3]), sizeof(cpsi.lac_tac) - 1);
    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.rxlvl_rsrp = atoi(values[6]) /10;

    /*ESP_LOGI(TAG, "GSM Parseado: MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);*/
}

void parseLTE(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    // Separar tokens
    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en LTE.");
        return;
    }

    strncpy(cpsi.sys_mode, values[0], sizeof(cpsi.sys_mode) - 1);
    cpsi.sys_mode[sizeof(cpsi.sys_mode) - 1] = '\0';

    strncpy(cpsi.oper_mode, values[1], sizeof(cpsi.oper_mode) - 1);
    cpsi.oper_mode[sizeof(cpsi.oper_mode) - 1] = '\0';

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4); // Según tu formato MCC-MNC, sigues leyendo bien

    const char *lac_tac_clean = removeHexPrefix(values[3]);
    strncpy(cpsi.lac_tac, lac_tac_clean, sizeof(cpsi.lac_tac) - 1);
    cpsi.lac_tac[sizeof(cpsi.lac_tac) - 1] = '\0';

    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.cell_id[sizeof(cpsi.cell_id) - 1] = '\0';

    cpsi.rxlvl_rsrp = atoi(values[11]) /10;
    ESP_LOGI(TAG, "LTE Parseado: MCC:%d, MNC:%d, TAC:%s, CellID:%s, RSRP:%d",
             cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);
}

void parseWCDMA(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en WCDMA.");
        return;
    }

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    strncpy(cpsi.lac_tac, removeHexPrefix(values[3]), sizeof(cpsi.lac_tac) - 1);
    strncpy(cpsi.cell_id, values[4], sizeof(cpsi.cell_id) - 1);
    cpsi.rxlvl_rsrp = atoi(values[12] ) /10; 

    /*ESP_LOGI(TAG, "WCDMA Parseado: MCC:%d, MNC:%d, LAC:%s, CellID:%s, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.lac_tac, cpsi.cell_id, cpsi.rxlvl_rsrp);*/
}

void parseCDMA(char *tokens) {
    char *values[15] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 15) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 14) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en CDMA.");
        return;
    }

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    cpsi.rxlvl_rsrp = atoi(values[6]) /10;

    /*ESP_LOGI(TAG, "CDMA Parseado: MCC:%d, MNC:%d, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.rxlvl_rsrp);*/
}

void parseEVDO(char *tokens) {
    char *values[10] = {NULL};  
    int count = 0;

    char *token = strtok(tokens, ",");
    while (token != NULL && count < 10) {
        values[count++] = token;
        token = strtok(NULL, ",");
    }

    if (count < 10) {
        ESP_LOGE(TAG, "Error: Datos insuficientes en EVDO.");
        return;
    }

    cpsi.mcc = atoi(values[2]);
    cpsi.mnc = atoi(values[2] + 4);
    cpsi.rxlvl_rsrp = atoi(values[5]) /10;

    /*ESP_LOGI(TAG, "EVDO Parseado: MCC:%d, MNC:%d, RXLVL:%d",
             cpsi.mcc, cpsi.mnc, cpsi.rxlvl_rsrp);*/
}