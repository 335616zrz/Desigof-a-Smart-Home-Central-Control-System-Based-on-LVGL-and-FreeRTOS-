idf_component_register(
    SRCS
        "hal_display_touch.c"
    INCLUDE_DIRS
        "include"
    REQUIRES                # 对外暴露的头/符号依赖
        board               # 提供 board_config.h 的 include 目录
        drivers_st7796
        drivers_ft6336
        driver              # v5.0.8: GPIO在driver组件中（v5.1+才有esp_driver_gpio）
        freertos
        log
)

# 与其它用到 LVGL 的组件保持一致（不强依赖 lvgl 目标，仅定义宏）
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE)

# ---- 仅作用于本组件的编译/链接优化（不污染全局）----
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Os
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)
target_link_options(${COMPONENT_LIB} PRIVATE -Wl,--gc-sections)
