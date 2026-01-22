idf_component_register(
    SRCS
        "app_env_sht40.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        drivers_sht40
        i2c_touch_bus
        board
        freertos
        log
        iot_mqtt
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
