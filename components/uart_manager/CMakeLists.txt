idf_component_register(SRCS "uart_gnss.c" "uart_serial.c" "uart_sim.c" "uart_manager.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver config sim_module common power_supply storage tracking utils)