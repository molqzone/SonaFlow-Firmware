idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ble_manager audio_source common_defs esp_wifi esp_event esp_system freertos)