idf_component_register(
    SRCS
        "ws2812b_driver.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        driver
        esp_driver_rmt
        board
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
