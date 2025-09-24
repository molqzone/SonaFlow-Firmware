idf_component_register(SRCS "application.cpp" "main.cpp"
                    INCLUDE_DIRS "."
                    REQUIRES ble_manager audio_sampler common_defs )