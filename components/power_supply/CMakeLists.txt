idf_component_register(
    SRCS "io_manager.c" "leds_pwr_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES driver config common)