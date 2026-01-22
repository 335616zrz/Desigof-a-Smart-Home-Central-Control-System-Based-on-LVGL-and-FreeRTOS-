idf_component_register(
    SRCS "sht40_driver.c"
    INCLUDE_DIRS "include"
    REQUIRES board driver freertos i2c_touch_bus
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Os
    -ffunction-sections -fdata-sections
)
target_link_options(${COMPONENT_LIB} PRIVATE -Wl,--gc-sections)
