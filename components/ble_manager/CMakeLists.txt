idf_component_register(
    SRCS "ble_manager.cpp" "ble_packet.cpp"
    INCLUDE_DIRS .
    REQUIRES common_defs bt nvs_flash
)