# main/CMakeLists.txt —— production-leaning (scope-local only)

# 公开依赖：仅暴露本工程自有上层接口
set(public_requires
    app_ui
    app_music
    app_env_sht40
    app_inmp441
    app_ws2812
    voice_commands
    ota_update
)

# 私有依赖：main 自身编译/链接所需的系统组件
set(private_requires
    esp_event
    esp_netif
    esp_wifi
    app_update
    esp_partition
    lvgl_port
    iot_mqtt
    net_wifi_sntp
    audio_i2s      # I2S 预初始化（DMA 内存保留）
)

# 按需启用电源管理依赖（避免无谓链接）
if(CONFIG_PM_ENABLE)
    list(APPEND private_requires esp_pm)
endif()

idf_component_register(
    SRCS
        "app_main.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        ${public_requires}
    PRIV_REQUIRES
        ${private_requires}
)

# —— 仅作用于 main 组件的编译/链接优化（不污染全局）——
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -Werror=return-type
    -Os
    -ffunction-sections -fdata-sections
)

target_link_options(${COMPONENT_LIB} PRIVATE
    -Wl,--gc-sections
)

# 如需严格 C 标准（IDF 默认 gnu99），可按需开启：
# target_compile_features(${COMPONENT_LIB} PRIVATE c_std_99)
