| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- | ----- |

# Hello World Example

Starts a FreeRTOS task to print "Hello World".

(See the README.md file in the upper level 'examples' directory for more information about examples.)

## How to use example

Follow detailed instructions provided specifically for this example.

Select the instructions depending on Espressif chip installed on your development board:

- [ESP32 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/index.html)
- [ESP32-S2 Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)


## Example folder contents

The project **hello_world** contains one source file in C language [hello_world_main.c](main/hello_world_main.c). The file is located in folder [main](main).

ESP-IDF projects are built using CMake. The project build configuration is contained in `CMakeLists.txt` files that provide set of directives and instructions describing the project's source files and targets (executable, library, or both).

Below is short explanation of remaining files in the project folder.

```
├── CMakeLists.txt
├── pytest_hello_world.py      Python script used for automated testing
├── main
│   ├── CMakeLists.txt
│   └── hello_world_main.c
└── README.md                  This is the file you are currently reading
```

For more information on structure and contents of ESP-IDF projects, please refer to Section [Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html) of the ESP-IDF Programming Guide.

## Troubleshooting

* Program upload failure

    * Hardware connection is not correct: run `idf.py -p PORT monitor`, and reboot your board to see if there are any output logs.
    * The baud rate for downloading is too high: lower your baud rate in the `menuconfig` menu, and try again.

## Technical support and feedback

Please use the following feedback channels:

* For technical queries, go to the [esp32.com](https://esp32.com/) forum
* For a feature request or bug report, create a [GitHub issue](https://github.com/espressif/esp-idf/issues)

We will get back to you as soon as possible.

## activate debug mode via command

develop mode bool
debug mode bool
raw data NMEA  bool
raw data GNSS bool
raw data cellNet bool

# Estructura de cadenas guardadas en FLASH externa

- `block_n.txt` →  RABF=n (n = numero de block)
- `block_1.txt` →  RABF=1

# ACTION
| CMD          | CODE  | DESCRIPTION                 | <DEVID>,<CMD><SYMB><VALUE><ENDSYM>     |
|--------------|-------|-----------------------------|----------------------------------------|
| KLRT         | 11    | Keep a live report time     | {DEVID},11#{min}$ (minino cada 10 min) |
| RTMS         | 12    | Reset SIM module            | {DEVID},12#1$                          |
| RTMC         | 13    | Reset microcontroller       | {DEVID},13#1$                          |
| SVPT         | 14    | Server and port             | {DEVID},14#{ip:port}$                  |
| TMTR         | 15    | Timer report tracking       | {DEVID},15#{seg}$ (minimo cada 10 seg) |
| DITR         | 16    | Distance tracking report    | {DEVID},16#{mts}$ (minimo cada 100 mts)|

# QUERY
| CMD          | CODE  | DESCRIPTION                 | <DEVID>,<CMD><SYMB><ENDSYM>            |
|--------------|-------|-----------------------------|----------------------------------------|
| KLRP         | 11    | Keep a live report time     | {DEVID},11?$                           |
| RTCT         | 26    | Number of device restarts   | {DEVID},26?$                           |

## Nomenclaturas usadas en comandos
- `KLRT` → Keep a live report time
- `KLRP` → Keep a live report
- `RTMC` → Reset microcontroler
- `RTMS` → Reset modulo SIM
- `SVPT` → servidor y puerto TCP
- `TKRT` → tracking report time
- `TKRP` → tracking report
- `DTRM` → Distance tracking report meters
- `DTRP` → Distance tracking report
- `DRNV` → Delete read NVS
- `CLOP` → operador celular
- `DLBF` → Borrar block flash ext. por indice
- `RABF` → Leer block flash ext. por indice (ECEPTO SMS)
- `WTBF` → Leer y borrar todos los blocks flash ext. por indice
- `SAIO` → Estado Y Activación de inpus/outputs
- `DBMD` → Modo debug
- `RTCT` → Numero de reinicios del dispositivo.
- `CLDT` → Cellular data (CPSI -> Cellular Protocol System Information)
- `CLLR` → Cellular report 
- `GNSR` → GNSS report
- `OPCT` → Output control
- `OPST` → Output state
- `MTRS` → Reinicio del dispositivo completo (MASTER RESET)
- `LOCA` → última posición valida (LOCATION)
- `CWPW` → Crear contraseña WIFI (create wifi password)
- `EBWF` → Habilitar WIFI (Enable wifi)
- `EBGS` → Habilitar GPS (Enable GPS)
- `EBGR` → Habilitar GPRS (Enable GPRS)
- `FWUD` → Habilitar GPRS (Enable GPRS)

# Datos guardados en NVS memoria no volatil
- `dev_imei` → imei del modulo SIM → AT command
- `dev_id` → id del dispositivo → AT command
- `sim_id` → operador celular (validar al reiniciar) → AT command
- `Keep_a_live_time` → tiempo de reporte de latido → UART/http/TCP/SMS (minutos)
- `tracking_time` → tiempo de reporte de trackeo → timer (segundos)
- `tracking_curve_time` → tiempo de reporte en curva (segundos)
- `dev_password` → contraseña para ingresar a modificar parametros (ASCII)
- `life_time` → tiempo de vida encendido el dispositivo 
- `dev_reboots` → Reinicios del dispositivo
- `last_evt_gen` → ultimo evento generado
- `trackings_sent` → numero de mensajes enviados desde encendido
- `debug_mode` → estado del modo debug
- `pass_reset` → contraseña para reiniciar el dispositivo
- `auth_phone` → telefono autorizado para mandar SMS (MAX 2)
- `last_valid_lat` → ultima latitude valida
- `last_valid_lon` → ultima longitude valida
- `gnss_report` → tiempo de reporte en segundos (maximo 5 seg. normal 3 seg.)
- `cpsi_report` → tiempo reporte informacion de la red celular (minimo 10 seg, normal 32 seg.)

# Errores GSTracker generados
- `0` → error sending data to the server
