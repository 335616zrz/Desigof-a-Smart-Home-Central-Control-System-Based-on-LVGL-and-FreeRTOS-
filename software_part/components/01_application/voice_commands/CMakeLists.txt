idf_component_register(
    SRCS
        "voice_commands.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        voice_afe
        esp_wifi
        esp_http_client
        app_env_sht40
        audio_player
        asr_client
        http_stream
        net_wifi_sntp
        ota_update
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
)
