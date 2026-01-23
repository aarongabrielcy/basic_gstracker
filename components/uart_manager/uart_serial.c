#include "uart_serial.h"
#include "uart_sim.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "esp_check.h"
#include "string.h"
#include "io_manager.h"
#include "nvs_manager.h"
#include <ctype.h>
#include "nvsData.h"

#include "eventHandler.h"
#include "esp_event.h"
//#include "adc.h"
//#include "sim7000.h"
//#include "input.h"
//#include "output.h"
//#include "accel.h"

// CABECERA DE PRUEBAS (SE VA A ELIMINAR)
#include "device_manager.h"
#include "buffer.h"

#define TAG "UART_MANAGER_SERIAL"
#define BUF_SIZE 1024

typedef struct {
    int number;
    char symbol;
    char value[64];  // Arreglo para almacenar el valor

} ParsedCommand;

char id[20];
char ccid[25];
char pss_wf[10];

char location[100];
char lat[20] = "+0.000000";
char lon[20] = "+0.000000";

TaskHandle_t monitor_uart_task = NULL;

void serial_task_init(void);
void uart_serial_init(void);
void uartManager_sendCommand(const char *command);
static void serialConsole_task(void *pvParameters);

static int validateCommand(const char *input,  ParsedCommand *parsed);
static char *proccessAction(ParsedCommand *parsed);
static char *proccessQuery(ParsedCommand *parsed);
static char *proccessQueryWithValue(ParsedCommand *parsed);
static char *processSVPT(const char *data);
static char *proccessCLOP(const char *data);
static char * resetDevice(const char *value);
static char * validatePassword(const char *password);
static char * firmwareUpdate(const char * value);
char *processCmd(const char *command);

void uart_serial_init(){
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);

    serial_task_init();
}

static void serialConsole_task(void *pvParameters){
    char data[512];
    while(1){
        int len = uart_read_bytes(UART_NUM_0, (uint8_t *)data, sizeof(data) - 1, pdMS_TO_TICKS(300));
        if (len > 0) {
            data[len] = '\0';
            if (strstr(data, "AT") != NULL){
                uartManager_sendCommand((char*)data);
            }
            // ESTE ES DE PRUEBAS SOLO PARA CHECAR EL NVS SE VA A RETIRAR TERMINANDO PRUEBAS
            else if(strstr(data, "IMEI_DELETE=1") != NULL){
                nvs_delete_key("dev_simei");
            }
            // AQUI TERMINA LA FUNCION DE PRUEBAS ------------------
            else if(strstr(data, "RESESP=1") != NULL){
                esp_restart();
            }
            else if(strstr(data, "ADC_E") != NULL){
                //ESP_LOGI(TAG, "VOLTAJE BATERIA EXTERNO: %.2f", getBatteryExtern());
            }
            else if (strstr(data, "ADC_I") != NULL){
                //ESP_LOGI(TAG, "VOLTAJE BATERIA INTERNO: %.2f", getBatteryIntern());
            }
            else if (strstr(data, "SIM7000=1") != NULL){
                //sim7000_init();
            }
            else if (strstr(data, "SIM7000=0") != NULL){
                uartManager_sendCommand("AT+CGNSPWR=0");
            }
            else if (strstr(data, "IN?") != NULL){
                ESP_LOGI(TAG, "VALOR DE ENTRADA ACTUAL: %d", getInValue());
            }
            else if (strstr(data, "IGN?") != NULL){
                //ESP_LOGI(TAG, "VALOR DE IGNICION ACTUAL: %d", getIGNValue());
            }
            else if (strstr(data, "OU?") != NULL){
                //ESP_LOGI(TAG, "VALOR DE OUTPUT ACTUAL: %d", getOUValue());
            }
            else if (strstr(data, "OU=1") != NULL){
                ESP_LOGI(TAG, "CAMBIANDO OUTPUT");
                //OU_on();
            }
            else if (strstr(data, "OU=0") != NULL){
                ESP_LOGI(TAG, "CAMBIANDO OUTPUT");
                //OU_off();
            }
            else if (strstr(data, "ACCEL?") != NULL){
                //int16_t x,y,z;
                //getAccelValue(&x, &y, &z);
                //ESP_LOGI(TAG, "VALOR DE ACELERACION X: %d, Y: %d, Z: %d", x, y, z);
            }
            else if (strstr(data, "StatusReq") != NULL){
                esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, TRACKING_RPT, NULL, 0, portMAX_DELAY);
            }
            else if (strstr(data, "KeepAlive") != NULL){
                esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, KEEP_ALIVE, NULL, 0, portMAX_DELAY);
            }
            else if (strstr(data, "IGN=1") != NULL){
                esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, IGNITION_ON, NULL, 0, portMAX_DELAY);
            }
            else if (strstr(data, "IGN=0") != NULL){
                esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, IGNITION_OFF, NULL, 0, portMAX_DELAY);
            }
            else if (strstr(data, "IGN=3") != NULL){
                esp_event_post_to(get_event_loop(), SYSTEM_EVENTS, IGNITION_OFF, NULL, 0, portMAX_DELAY);
            }
            else if (strstr(data, "BUFFER=1") != NULL){
                ESP_LOGI(TAG, "--- RECUPERANDO MENSAJES GUARDADOS ---");
                sst26_leer_enviar_y_borrar();

            }
            else{
                ESP_LOGI(TAG,"%s",processCmd((char*)data));
            }
        }
    }
}

void uartManager_sendCommand(const char *command) {
    char buf[256];
    int n = snprintf(buf, sizeof(buf), "%s\r\n", command);
    uart_write_bytes(UART_SIM, buf, n);
}

void serial_task_init(){
    xTaskCreate(serialConsole_task, "serial_console_task", 8192, NULL, 5, &monitor_uart_task);
}

char *processCmd(const char *command) {
    static char buffer[256];  // Buffer estático compartido
    ParsedCommand cmd;
    switch (validateCommand(command, &cmd)) {
        case EMPTY:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "EMPTY CMD");
            return buffer;
        case QUERY_WITHOUT_VALUE:
            snprintf(buffer, sizeof(buffer), "%d%s&%s", cmd.number, cmd.value, proccessQuery(&cmd));
            return buffer;
        case ACTION:
            snprintf(buffer, sizeof(buffer), "%d%c%s&%s", cmd.number, cmd.symbol, cmd.value, proccessAction(&cmd));
            return buffer;
        case INVALID_CMD:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID CMD");
            return buffer;
        case INVALID_SYMBOL:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID SYMBOL");
            return buffer;
        case INVALID_ACTION:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID ACTION");
            return buffer;
        case INVALID_NUMBER:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID NUMBER");
            return buffer;
        case QUERY_WITH_VALUE:
            snprintf(buffer, sizeof(buffer), "%d%c%s&%s", cmd.number, cmd.symbol, cmd.value, proccessQueryWithValue(&cmd));
            return buffer;
        case INVALID_QUERY_VALUE:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID QUERY VALUE");
            return buffer;
        case INVALID_END_SYMBOL:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "INVALID END SYMBOL");
            return buffer;
        default:
            snprintf(buffer, sizeof(buffer), "%s%s", command, "NO RESULT");
            return buffer;
    }
}

static int validateCommand(const char *input, ParsedCommand *parsed) {
    if (input == NULL || *input == '\0' || parsed == NULL) {
        return EMPTY;
    }
    int i = 0;
    while (isdigit((unsigned char)input[i])) {
        i++;
    }
    if (i == 0) {
        return INVALID_CMD;
    }
    // Guardar número
    char numberStr[6] = {0};
    if (i >= sizeof(numberStr)) {
        return INVALID_NUMBER;
    }
    strncpy(numberStr, input, i);
    parsed->number = atoi(numberStr);

    // Verificar símbolo
    char symbol = input[i];
    if (symbol != '#' && symbol != '?') {
        return INVALID_SYMBOL;
    }
    parsed->symbol = symbol;
    const char *value = input + i + 1;

    // Verificar terminación en '$'
    const char *end = strchr(value, '$');
    if (end == NULL) {
        return INVALID_END_SYMBOL;
    }

    int len = end - value;
    if (len >= sizeof(parsed->value)) {
        len = sizeof(parsed->value) - 1;
    }
    strncpy(parsed->value, value, len);
    parsed->value[len] = '\0';

    // Evaluar tipo de comando según el símbolo
    if (symbol == '#') {
        if (parsed->value[0] == '\0') {
            return INVALID_ACTION;
        }
        return ACTION;
    } else if (symbol == '?') {
        if (parsed->value[0] == '\0') {
            return QUERY_WITHOUT_VALUE;
        }
        if (!isdigit((unsigned char)parsed->value[0]) || parsed->value[1] != '\0') {
            return INVALID_QUERY_VALUE;
        }
        return QUERY_WITH_VALUE;
    }
    ESP_LOGI(TAG, "Saliendo de la validación...");
    return INVALID_CMD;
}

char *proccessAction(ParsedCommand *parsed) {
    switch (parsed->number) {
        case KLRT:
            uint32_t minutes = strtoul(parsed->value, NULL, 10);
            if (minutes < 5) {
                ESP_LOGI(TAG, "Valor KLRT inválido (min < 5): %s", parsed->value);
                return "NA";
            }
            if (minutes >= 60)  {
                ESP_LOGI(TAG, "Valor KLRT inválido (no puede ser mayor a 1 hora): %s", parsed->value);
                return "NA";
            }
            uint32_t interval_ms = minutes * 60UL * 1000UL;
            esp_err_t err = nvs_save_int(NVS_KEY_KEEPALIVE, interval_ms);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error NVS KLRT: %s", esp_err_to_name(err));
                return "ERR SAVE";
            }
            /* ===============================
             * ACTUALIZACIÓN EN RUNTIME
             * =============================== */
            nvs_data.keep_alive_time = interval_ms;
            update_keep_alive_timer(interval_ms);
            ESP_LOGI(TAG, "keep alive actualizado a %lu minutos", minutes);
            return "OK";           
        case TKRT:
            uint32_t seconds = strtoul(parsed->value, NULL, 10);
            if (seconds < 10) {
                ESP_LOGI(TAG, "Valor TKRT inválido (seg < 10): %s", parsed->value);
                return "NA";
            }
            if (seconds > 60)  {
                ESP_LOGI(TAG, "Valor TKRT inválido (no puede ser mayor a 1 minuto): %s", parsed->value);
                return "NA";
            }
            uint32_t interval_tkr_ms = seconds * 1000UL;
            esp_err_t err_tkr = nvs_save_int(NVS_KEY_TRACKING_REPORT, interval_tkr_ms);
            if (err_tkr != ESP_OK) {
                ESP_LOGE(TAG, "Error NVS TKRT: %s", esp_err_to_name(err_tkr));
                return "ERR SAVE";
            }
            /* ===============================
             * ACTUALIZACIÓN EN RUNTIME
             * =============================== */
            nvs_data.tracking_time = interval_tkr_ms;
            update_tracking_report_timer(interval_tkr_ms);
            ESP_LOGI(TAG, "tracking report actualizado a %lu segundos", seconds);
            return "OK";
        case SVPT:
            return processSVPT(parsed->value);
        case CLOP:
            return proccessCLOP(parsed->value);
        case MTRS:
            return resetDevice(parsed->value);
        case OPCT:
        if(atoi(parsed->value) == 1 ) {
            if(OU_on()) {
                return "ON";
            } else { return "ERR ON"; }
        } else if(atoi(parsed->value) == 0) {
            if(OU_off()) {
                return "OFF";
            } else { return "ERR OFF"; }
        }
        return "NA";
        case DLBF:
            /*if(atoi(parsed->value) > 0) {
                if(spiffs_delete_block(atoi(parsed->value) )) {
                    return "DELETE OK";
                } else {
                    return "NOT DELETED";
                }
            } else {return "NOT FOUND";} */
        case CWPW:
            char * response = validatePassword(parsed->value);
            if (strncmp(response, "save", 4) == 0) {
                nvs_save_str("password_wifi", parsed->value);
                
            } else if (strncmp(response, "Error", 5) == 0) {
                return response;
            }
            return response;

        /*case AEWF:
            if(atoi(parsed->value) == 1 ) {
                if (nvs_read_str("password_wifi", pss_wf, sizeof(pss_wf)) != NULL) {
                    /////// Activa el wifi ///////
                    if(wifi_manager_enable(atoi(parsed->value)) ){
                        return "WIFI ON";
                    } else { return "ERR WIFI"; } 
                } else { return "PASS NO CONFIG"; }
                
            } else if(atoi(parsed->value) == 0 ) {
                if(wifi_manager_enable(atoi(parsed->value)) ){
                    return "WIFI OFF";
                } else { return "ERR WIFI"; }
            }
            return "ERR";*/
        case AEGP:
        /*if (strcmp(parsed->value, "1") == 0) {
            const char *value = "1";
            char command[20];
            snprintf(command, sizeof(command), "AT+CGPS=%s", value);
            printf("Comando AT: %s\n", command);
            if(sim7600_sendReadCommand(command)){
                return "GPS ON";
            } else {
                return "GPS ON ERR";
            }
        } else if (strcmp(parsed->value, "0") == 0) {
            const char *value = "0";
            char command[20];
            snprintf(command, sizeof(command), "AT+CGPS=%s", value);
            printf("Comando AT: %s\n", command);
            if(sim7600_sendReadCommand(command)){
                return "GPS OFF";
            } else {
                return "GPS OFF ERR";
            }
        }
            return "ERR";
        if(atoi(parsed->value) >= 10 ) {
                char command[50];
                snprintf(command, sizeof(command), "AT+CGNSSINFO=%s", parsed->value);
                printf("Comando AT: %s\n", command);
            } else { return "ERR: The reporting time cannot be less than 10 seconds."; }
            return "OK";*/
        case FWUP:
            return firmwareUpdate(parsed->value);
        default:
            return "CMD ACTION NOT FOUND";
    }
}

char *proccessQuery(ParsedCommand *parsed) {
    static char buffer[32];
    switch (parsed->number) {
        case KLRT:
            uint32_t keep_alive_ms = nvs_read_int(NVS_KEY_KEEPALIVE);
            nvs_data.keep_alive_time = keep_alive_ms;
            uint32_t keep_alive_min = keep_alive_ms / 1000 / 60;
             snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)keep_alive_min);
             return buffer;
        case TKRT:
            uint32_t tracking_report_ms = nvs_read_int(NVS_KEY_TRACKING_REPORT);
            nvs_data.tracking_time = tracking_report_ms;
            uint32_t tracking_report_sec = tracking_report_ms / 1000;
             snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)tracking_report_sec);
             return buffer;
        case DVID:
            if (nvs_read_str("device_id", id, sizeof(id)) != NULL) {
                return id;
            }else { return "ERR"; }
        case RTCT:
            int value = nvs_read_int("dev_reboots");
            snprintf(buffer, sizeof(buffer), "%d", value);  // convierte el int a string
            return buffer;
        case SIID:
            if (nvs_read_str("sim_id", ccid, sizeof(ccid)) != NULL) {
                return ccid;
            }else { return "ERR"; }
        case PWFR:
            if (nvs_read_str("password_wifi", pss_wf, sizeof(pss_wf)) != NULL) {
                return pss_wf;
            }else { return "ERR"; }
        case TKRP: 
            esp_event_loop_handle_t loop = get_event_loop();            
            if (loop) {
                esp_err_t err = esp_event_post_to(loop, SYSTEM_EVENTS, TRACKING_RPT, NULL, 0, portMAX_DELAY);
                if (err == ESP_OK) {
                    return "OK";  // El evento se propagó correctamente
                } else {
                    return "ERR";  // Falló al propagar
                }
            } else {
                return "ERR";  // El event loop no está disponible
            }
        case LOCA:
            if(nvs_read_str("last_valid_lon", lon, sizeof(lon)) != NULL) {
                ESP_LOGI(TAG, "last_lat_NVS=%s", lon);   
            } else { return "ERR LON"; }
            if(nvs_read_str("last_valid_lat", lat, sizeof(lat)) != NULL) {
                ESP_LOGI(TAG, "last_lat_NVS=%s", lat);   
            } else { return "ERR LAT"; }
            snprintf(location, sizeof(location), "https://www.google.com/maps/search/?api=1&query=%s,%s&zoom=20", lat, lon);
        return location;
        default:
            return "NOT FOUND";
    }
}

char *proccessQueryWithValue(ParsedCommand *parsed) {
    switch (parsed->number) {
        case OPST: 
            return getOUValue()? "ON" : "OFF";
        default:
            return "NOT FOUND";
        break;
    }
}

static char *proccessCLOP(const char *data) {
    char command[99];
snprintf(command, sizeof(command), "AT+CGDCONT=1,\"IP\",\"%s\"", data);
    printf("Comando AT: %s\n", command);
    if(1){
        return "OK1";
    }else {
        return "ERROR";
    }
}
static char *processSVPT(const char *data) {
    /*char server[32];  // Almacena la IP
    char port[6];     // Almacena el puerto (máximo 5 dígitos + terminador)

    // Buscar la posición del ':' en la cadena
    char *ptr = strchr(data, ':');
    if (ptr == NULL) {
        return "Error format";
    }
    // Copiar la parte de la IP antes de ':'
    size_t len = ptr - data;
    strncpy(server, data, len);
    server[len] = '\0';  // Agregar terminador de cadena

    // Copiar la parte del puerto después de ':'
    strncpy(port, ptr + 1, sizeof(port) - 1);
    port[sizeof(port) - 1] = '\0';  // Asegurar terminador

    // Construir el comando AT
    char command[64];
    snprintf(command, sizeof(command), "AT+CIPOPEN=0,\"TCP\",\"%s\",%s", server, port);
    // Imprimir el comando resultante
    printf("Comando AT: %s\n", command);
    if(sim7600_sendReadCommand(command) ){
        return "OK";
    }else {
        return "ERR";
    }*/
   return "OK";    
}

static char * resetDevice(const char *value) {
    if(atoi(value) == 1 ) { ///////// cambia el 1 por una contraseña (pass_reset)
        return "RST";
    }
    else { return "ERR"; }
}

static char* validatePassword(const char *password) {
    if (strlen(password) < 8) {
        return "Error: Password must be at least 8 characters long";
    }

    bool has_number = false;
    bool has_special = false;
    const char *special_chars = "!#%&/(){}[]?¡*+-.";

    for (size_t i = 0; i < strlen(password); ++i) {
        if (isdigit((unsigned char)password[i])) {
            has_number = true;
        }
        if (strchr(special_chars, password[i])) {
            has_special = true;
        }
    }
    if (!has_number) {
        return "Error: Password must contain at least one number";
    }
    if (!has_special) {
        return "Error: Password must contain at least one special character";
    }
    return "save successfully";
    //linkzero234.
}

static char* firmwareUpdate(const char *value) {
    //uart_flush(UART_NUM_1);
    //sim7600_sendATCommand("AT+CGNSSINFO=0");
    //vTaskDelay(200/portTICK_PERIOD_MS);
    //sim7600_sendATCommand("AT+CPSI=0");
    //stop_tracking_report_timer();
    //stop_keep_alive_timer();
    esp_err_t err = ESP_OK;
    //err = ota_manager_perform_update();
    if (err != ESP_OK){
        ESP_LOGI(TAG, "Error en pta_manager_perform_update: %s",esp_err_to_name(err));
        esp_restart();
    }
    //err = ota_prepare_http(value);
    if (err != ESP_OK){
        //ESP_LOGI(TAG, "Error en ota_prepare_http: %s",esp_err_to_name(err));
        esp_restart();
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
    return "ERR";
}