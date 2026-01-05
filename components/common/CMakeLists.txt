idf_component_register(
    SRCS "nvsData.c" "deviceManager.c" "eventHandler.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_timer storage trackings
)