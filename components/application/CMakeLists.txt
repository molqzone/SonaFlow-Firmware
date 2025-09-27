idf_component_register(
    SRCS 
    "application.cpp"
    "states/state_base.cpp"
    "states/waiting_state.cpp"
    "states/connected_idle_state.cpp"
    "states/streaming_state.cpp"
    "states/fatal_error_state.cpp"
    INCLUDE_DIRS "." "states"
    REQUIRES ble_manager audio_source common_defs driver
)