idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES application common_defs esp_wifi esp_event esp_system freertos)