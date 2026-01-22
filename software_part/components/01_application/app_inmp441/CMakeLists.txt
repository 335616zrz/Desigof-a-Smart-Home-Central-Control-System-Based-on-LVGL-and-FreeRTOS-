idf_component_register(
    SRCS
        "app_inmp441.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        drivers_inmp441
        freertos
        log
        mem_utils
        asr_client
        audio_player      # provides server_root_cert_pem
        net_wifi_sntp     # 提供 net_wait_ip / net_is_connected
        ota_update
        audio_aec         # AEC 回声消除
        audio_ns          # NS 噪声抑制
        audio_i2s         # I2S TX 参考信号
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
