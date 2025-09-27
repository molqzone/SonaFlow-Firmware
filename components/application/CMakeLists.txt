idf_component_register(
    SRCS "application.cpp"
    INCLUDE_DIRS .
    REQUIRES ble_manager audio_source common_defs driver
)