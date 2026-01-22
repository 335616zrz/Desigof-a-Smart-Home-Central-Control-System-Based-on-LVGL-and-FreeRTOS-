# components/01_application/app_music/CMakeLists.txt

idf_component_register(
    SRCS
        "app_music.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        audio_player      # ap_init / ap_play_url
        http_stream       # url_encode
        my_ui             # music_volume
        lvgl_port         # lvgl_port_lock/unlock for lv_async_call safety
        freertos
        esp_wifi
        esp_event
        esp_netif
)

# 仅对本组件生效的工程级优化
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
