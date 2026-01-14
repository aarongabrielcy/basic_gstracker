idf_component_register(SRCS "tracker_model.c" "tracking.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver config common uart_manager sim_module utils storage)