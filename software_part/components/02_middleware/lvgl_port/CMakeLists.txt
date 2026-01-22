idf_component_register(
    SRCS "lvgl_port.c"
    INCLUDE_DIRS "include"
    REQUIRES
        lvgl
        freertos
        esp_timer
        hal_display_touch      # 依赖 HAL（其内部完成底层 init）
        drivers_st7796         # 本文件直接调用 st7796/ft6336 API，显式声明以获得头文件/链接
        drivers_ft6336
)

# 仅对本组件生效的编译/链接优化
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

