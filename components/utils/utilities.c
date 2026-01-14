#include "utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "esp_check.h"

static const char *TAG = "UTILS";

char *cleanData(char *response, const char *command) {
    char *prefix = strstr(response, "+");
    if (!prefix) return response;

    prefix = strchr(prefix, ':');
    if (!prefix) return response;

    return prefix + 2;  // Saltar ": " para obtener la data limpia
}
char* cleanATResponse(const char *input) {
    static char cleaned[100];  // Buffer estático para la respuesta limpia
    const char *start = strchr(input, ':');  // Encontrar el primer ':'   
    if (start) {
        start += 2;  // Saltar ": " (dos caracteres)
        const char *end = strstr(start, ",OK");  // Buscar ",OK" para eliminarlo
        if (!end) {
            end = input + strlen(input);  // Si no hay ",OK", tomar hasta el final
        }
        strncpy(cleaned, start, end - start);  // Copiar la parte limpia
        cleaned[end - start] = '\0';  // Agregar terminador de cadena   
        // **Eliminar comillas si están al inicio y al final**
        size_t len = strlen(cleaned);
        if (len > 1 && cleaned[0] == '"' && cleaned[len - 1] == '"') {
            memmove(cleaned, cleaned + 1, len - 2); // Mover el contenido a la izquierda
            cleaned[len - 2] = '\0';  // Ajustar terminador de cadena
        }
    } else {
        strcpy(cleaned, "");  // Devolver cadena vacía si no hay formato válido
    }
    return cleaned;
}
/** modifica esta funcion para que no ponga comas ";" */
/**ARREGLA ESTE ERROR DE "Fallo al reservar memoria en cleanResponse" Después de cierto tiempo  */

char* cleanResponse(const char *response) {
    if (response == NULL) {
        ESP_LOGE(TAG, "cleanResponse recibió un puntero NULL");
        return NULL;
    }
    char *cleanResponse = (char *)malloc(strlen(response) + 1); // Reservar memoria
    if (cleanResponse == NULL) {
        ESP_LOGE(TAG, "Fallo al reservar memoria en cleanResponse");
        return NULL; // Manejo de error en caso de fallo en la asignación de memoria
    }
    int j = 0;
    for (int i = 0; response[i] != '\0'; i++) {
        if (response[i] == '\r' || response[i] == '\n') {
            // Si el siguiente carácter también es un salto de línea, ignorarlo
            if (response[i + 1] == '\r' || response[i + 1] == '\n') {
                continue;
            }
            // Si no es el final de la línea y no hay una coma previa, agregar una coma ","
            if (j > 0 && response[i + 1] != '\0' && response[i + 1] != '\r' && response[i + 1] != '\n') {
                cleanResponse[j++] = ',';
            }
        } else {
            cleanResponse[j++] = response[i];
        }
    }
    cleanResponse[j] = '\0';  // Asegurar terminación de cadena
    return cleanResponse;  // Retornar la nueva cadena procesada
}
char* formatCoordinates(double coord, char direction) { 
    static char buffer[16]; // Suficiente para almacenar el número con signo

    // Separar grados y minutos
    /*int degrees = (int)(coord / 100);
    double minutes = coord - (degrees * 100);
    double decimalDegrees = degrees + (minutes / 60.0);

    // Aplicar signo según la dirección
    if (direction == 'S' || direction == 'W') {
        decimalDegrees = -decimalDegrees;
    }*/

    // Formatear el número como cadena con signo explícito
    snprintf(buffer, sizeof(buffer), "%+.6f", coord);
  
    return buffer; // Retornar el puntero al buffer estático
}

char *formatDate(const char *date) {
    // Formato de entrada: DDMMYY (e.g., "090325")
    // Formato de salida: YYYYMMDD (e.g., "20250309")
    char *output = (char *)malloc(9 * sizeof(char)); // Reservar memoria para YYYYMMDD + '\0'
    if (output != NULL) {
        snprintf(output, 9, "20%c%c%c%c%c%c", date[4], date[5], date[2], date[3], date[0], date[1]);
    }
    return output;
}

char *formatTime(const char *utcTime) {
    // Formato de entrada: HHMMSS.s (e.g., "141640.0")
    // Formato de salida: HH:MM:SS (e.g., "14:16:40")
    char *output = (char *)malloc(9 * sizeof(char)); // Reservar memoria para HH:MM:SS + '\0'
    if (output != NULL) {
        snprintf(output, 9, "%c%c:%c%c:%c%c", utcTime[0], utcTime[1], utcTime[2], utcTime[3], utcTime[4], utcTime[5]);
    }
    return output;
}

char* getFormatUTC(const char* input) {
    static char output[34];  // Suficiente para el formato "YYYYMMDD;HH:MM:SS"
    int year, month, day, hour, minute, second, tz_offset;
    char *src = input;
    char *dst = input;
    while (*src) {
        if (*src != '\"') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
    // Registro de entrada
    //ESP_LOGI(TAG, "data formatUTC => %s", input);
    // Extraer los valores, asegurando que tz_offset pueda manejar valores negativos
    if (sscanf(input, "%2d/%2d/%2d,%2d:%2d:%2d%3d", 
               &year, &month, &day, &hour, &minute, &second, &tz_offset) != 7) {
        strcpy(output, "00000000;00:00:00");  // Valor por defecto en caso de error
        return output;
    }
    // Ajustar año al formato completo (asume 2000+)
    year += 2000;
    // **Validar valores antes de continuar**
    if (year < 2000 || year > 2099) year = 2000;
    if (month < 1 || month > 12) month = 1;
    if (day < 1 || day > 31) day = 1;
    if (hour < 0 || hour > 23) hour = 0;
    if (minute < 0 || minute > 59) minute = 0;
    if (second < 0 || second > 59) second = 0;   
    // **Validar tz_offset para evitar desbordamiento**
    if (tz_offset < -96 || tz_offset > 96) tz_offset = 0;
    // **Convertir a UTC**
    int offsetMinutes = tz_offset * 15;
    hour -= offsetMinutes / 60;
    minute -= offsetMinutes % 60;
    // **Normalizar hora y fecha**
    while (minute >= 60) {
        hour += 1;
        minute -= 60;
    }
    while (minute < 0) {
        hour -= 1;
        minute += 60;
    }
    while (hour >= 24) {
        hour -= 24;
        day += 1;
    }
    while (hour < 0) {
        hour += 24;
        day -= 1;
    }
    // **Formatear la salida de manera segura**
    snprintf(output, sizeof(output), "%04d%02d%02d;%02d:%02d:%02d", 
             year, month, day, hour, minute, second);
    return output;
}
const char *formatDevID(const char *input) {
    static char output[11]; // Buffer estático para almacenar el resultado (10 caracteres + '\0')
    size_t len = strlen(input);
    if (len >= 10) {
        strncpy(output, input + (len - 10), 10); // Copiar los últimos 10 caracteres
    } else {
        strncpy(output, input, len); // Copiar el string completo si es menor a 10 caracteres
    }
    output[10] = '\0'; // Asegurar terminador nulo
    return output;
}
/*char* removeHexPrefix(const char *hexValue) {
    if (hexValue == NULL) {
        return NULL;
    }
    if ((hexValue[0] == '0' && (hexValue[1] == 'x' || hexValue[1] == 'X'))) {
        return strdup(hexValue + 2); 
    }
    return strdup(hexValue);
}*/
const char* removeHexPrefix(const char *hexValue) {
    if (hexValue == NULL) {
        return "";
    }

    // Verificar si empieza con "0x" o "0X"
    if (hexValue[0] == '0' && (hexValue[1] == 'x' || hexValue[1] == 'X')) {
        return hexValue + 2;  // Solo avanzar el puntero
    }

    return hexValue;  // Devolver el mismo puntero
}

/**quita la validacion de las comas cuando le quites a clean respose */
char* clean(const char* text, const char* word) {

    if (text == NULL || word == NULL) {
        return NULL;
    }

    char *text_copy = strdup(text);
    if (text_copy == NULL) {
        ESP_LOGE("UTILS", "strdup falló (sin memoria)");
        return NULL;
    }
    char command_clean[100];
    strncpy(command_clean, word, sizeof(command_clean) - 1);
    command_clean[sizeof(command_clean) - 1] = '\0';
    
    // Eliminar la palabra "word" si se encuentra en cualquier parte del texto
    char* ok_pos = strstr(text_copy, ",OK");
    if (ok_pos != NULL) {
        // Mover la parte posterior de ",OK" hacia el inicio para eliminar ",OK"
        memmove(ok_pos, ok_pos + 4, strlen(ok_pos + 4) + 1); // 4 porque ",OK" tiene 4 caracteres
    }
    char* pos = strstr(text_copy, word);
    if (pos != NULL) {
        memmove(pos, pos + strlen(word), strlen(pos + strlen(word)) + 1); // Mover el resto de la cadena
        char* comma_pos = strchr(text_copy, ',');
        if (comma_pos != NULL) {
            memmove(comma_pos, comma_pos + 1, strlen(comma_pos));
        }
    }
    // Eliminar "AT" al inicio si está presente
    if (strncmp(command_clean, "AT", 2) == 0) {
        memmove(command_clean, command_clean + 2, strlen(command_clean) - 1);
    }
    // Eliminar '?' al final si está presente
    if (command_clean[strlen(command_clean) - 1] == '?') {
        command_clean[strlen(command_clean) - 1] = '\0';
    }
    strcat(command_clean, ": ");
    //printf("command format=>%s", command_clean);

    char* pos_cmd = strstr(text_copy, command_clean);
    if (pos_cmd != NULL) {
        memmove(pos_cmd, pos_cmd + strlen(command_clean), strlen(pos_cmd + strlen(command_clean)) + 1); // Mover el resto de la cadena
    }
    
    return text_copy; // Retornar la cadena modificada
}
char* removeDoubleQuotesInPlace(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src != '\"') {  // Copia solo si el carácter no es una comilla
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';  // Termina la cadena modificada
    return str;
}
/*char *clean(const char *response, const char *command) {
    if (response == NULL || command == NULL) {
        return NULL;
    }

    // Crear una copia de response
    char *text_copy = strdup(response);
    if (!text_copy) {
        ESP_LOGE(TAG, "Error: No se pudo asignar memoria para text_copy");
        return NULL;
    }

    // 1. Eliminar "OK"
    char *last_comma = strrchr(text_copy, ',');
    if (last_comma && strstr(last_comma, "OK")) {
        *last_comma = '\0';
    }

    // 2. Eliminar todas las comas ","
    char *read_ptr = text_copy, *write_ptr = text_copy;
    while (*read_ptr) {
        if (*read_ptr != ',') {
            *write_ptr++ = *read_ptr;
        }
        read_ptr++;
    }
    *write_ptr = '\0';

    // 3. Limpiar "command"
    char *command_clean = strdup(command);
    if (!command_clean) {
        free(text_copy);
        ESP_LOGE(TAG, "Error: No se pudo asignar memoria para command_clean");
        return NULL;
    }

    if (strncmp(command_clean, "AT", 2) == 0) {
        memmove(command_clean, command_clean + 2, strlen(command_clean) - 1);
    }
    if (command_clean[strlen(command_clean) - 1] == '?') {
        command_clean[strlen(command_clean) - 1] = '\0';
    }

    // Crear comando formateado correctamente
    char formatted_command[32];
    snprintf(formatted_command, sizeof(formatted_command), "%s: ", command_clean);
    free(command_clean);

    // 4. Buscar y eliminar palabra limpia
    char *result = strstr(text_copy, formatted_command);
    if (result != NULL) {
        result += strlen(formatted_command);
        char *final_result = strdup(result);
        free(text_copy);
        return final_result;  // Devuelve nueva copia para usar en otras funciones
    }

    free(text_copy);
    return NULL;
}*/