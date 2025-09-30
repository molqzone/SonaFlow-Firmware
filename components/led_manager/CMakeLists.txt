idf_component_register(SRCS "led_manager.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver led_strip common_defs freertos)