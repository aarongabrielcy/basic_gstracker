idf_component_register(SRCS  "buffer.c" "nvs_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver config nvs_flash)