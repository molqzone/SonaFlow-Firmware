idf_component_register(SRCS "storage_manager.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES driver spiffs ble_manager common_defs freertos)