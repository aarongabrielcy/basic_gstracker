idf_component_register(
    SRCS "device_manager.c" "nvsData.c" "eventHandler.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_timer storage tracking sim_module utils)