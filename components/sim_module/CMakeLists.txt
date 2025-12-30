idf_component_register(SRCS "sim7000.c" "gnss_nmea.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver config uart_manager)
