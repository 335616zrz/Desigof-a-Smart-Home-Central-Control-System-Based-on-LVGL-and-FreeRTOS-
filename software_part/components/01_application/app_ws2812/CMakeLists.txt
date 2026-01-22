idf_component_register(
    SRCS
        "app_ws2812.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        drivers_ws2812b
        board
        freertos
        log
        iot_mqtt
        json
        esp_timer
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Os
    -ffunction-sections -fdata-sections
)

target_link_options(${COMPONENT_LIB} PRIVATE
    -Wl,--gc-sections
)

