idf_component_register(SRCS "application.cpp" "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ble_manager audio_sampler common_defs esp_wifi esp_event esp_system freertos)