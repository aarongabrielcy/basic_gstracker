#include "sim7000.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "uart_serial.h"


#define TAG "SIM7000"
TaskHandle_t sim700_task = NULL;
TaskHandle_t sim700_connection_task = NULL;
void sim7000_stop(void);
void sim7000_conn_stop(void);
void checkTime(void);

void sim7000_command(const char *command){
    uartManager_sendCommand(command);
}

void sim7000_basic_config() {
    checkTime();
    //sim7000_command("AT+CLTS?");
    //vTaskDelay(200);
    sim7000_command("AT+GSN");
    vTaskDelay(200);
    sim7000_command("AT+CCID");
    vTaskDelay(500);
    /*sim7000_command("AT+CGNSCFG=2");
    vTaskDelay(200);
    sim7000_command("AT+CGNSPWR=1");
    vTaskDelay(200);*/
    sim7000_command("AT+CSTT=\"internet.itelcel.com\"");
    vTaskDelay(500);
    sim7000_command("AT+CIICR");
    vTaskDelay(200);
    sim7000_command("AT+CIFSR");
    vTaskDelay(200);
    sim7000_command("AT+CIPSTART=\"TCP\",\"201.122.135.23\",6100");
    vTaskDelay(200);
    sim7000_stop();
}
/*
    pasos para conectarse en TCP
    - AT+CSTT="internet.itelcel.com"
    - AT+CIICR
    - AT+CIFSR
    - AT+CIPSTART="TCP","201.122.135.23",6100

    - POSTERIORRMENTE SE DEBE MANDAR AT+CIPSEND PARA CADA TRAQUEO
*/
void checkTime(){
    sim7000_command("AT+CLTS?");
}

void sim7000_connection_with_server(){
    sim7000_command("AT+CSTT='internet.itelcel.com'");
    vTaskDelay(200);
    sim7000_command("AT+CIICR");
    vTaskDelay(200);
    sim7000_command("AT+CIPSTART='TCP','201.122.135.23',6100");
    vTaskDelay(200);
    sim7000_conn_stop();
}

void sim7000_init(){
    xTaskCreate(sim7000_basic_config, "sim7000_task", 2048, NULL, 10, &sim700_task);
}

void sim7000_connection_init(){
    xTaskCreate(sim7000_connection_with_server, "sim7000_connection_with_server", 2048, NULL, 10, &sim700_connection_task);
}

void sim7000_stop(){
    vTaskDelete(sim700_task);
}

void sim7000_conn_stop(){
    vTaskDelete(sim700_connection_task);
}

void getTimeUTC(){
    sim7000_command("AT+CLTS=1");
}

void getTimeLocal(){
    sim7000_command("AT+CCLK?");
}