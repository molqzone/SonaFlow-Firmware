idf_component_register(
    SRCS "ble_packet.cpp" "ble_protocol.cpp" "ble_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES common_defs
)