idf_component_register(
    SRCS "sdcard_spi.c"
    INCLUDE_DIRS "include"
    REQUIRES board driver freertos fatfs sdmmc
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    DRV_LOG_ENABLE=1
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Os
    -ffunction-sections -fdata-sections
)
target_link_options(${COMPONENT_LIB} PRIVATE -Wl,--gc-sections)
