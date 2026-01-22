# components/01_application/app_ui/CMakeLists.txt

idf_component_register(
    SRCS
        "app_ui.c"
    INCLUDE_DIRS
        "include"          # 公开头文件路径
    PRIV_REQUIRES
        lvgl__lvgl         # 直接包含 lvgl.h，显式依赖 lvgl（managed component 目标名）
        lvgl_port          # 本地 LVGL 端口层
        hal_display_touch  # 显示与触摸 HAL
        audio_player       # 播放器 API（供回调调用）
        my_ui              # GUI Guider 生成的头文件/资源（仅编译期依赖）
        sdcard_spi         # SD 卡 SPI + 音乐索引驱动
)

# ---- 仅对本组件生效的工程级优化，不影响全局 ----
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

# 说明：
# 1) 使用 PRIV_REQUIRES 是因为 app_ui 的对外头文件不需要暴露 lvgl/hal 的包含路径；
#    这些依赖仅在编译 app_ui.c 时需要，避免向上游组件泄漏依赖。
# 2) 若你的 lvgl 组件在环境中不是 "lvgl__lvgl" 命名（极少数场景），
#    请在 lvgl_port 的 CMake 中确保 REQUIRES 正确指向 lvgl，
#    或将上面的 lvgl__lvgl 替换为实际组件名。
