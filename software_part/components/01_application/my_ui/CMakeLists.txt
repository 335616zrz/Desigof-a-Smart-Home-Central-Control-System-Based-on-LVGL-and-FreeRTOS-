# components/01_application/my_ui/CMakeLists.txt

file(GLOB_RECURSE MY_UI_SRCS
    "generated/*.c"
    "custom/*.c"
    "generated/guider_fonts/*.c"
    "generated/guider_customer_fonts/*.c"
)

# 显式列出新增的 custom 源，避免某些环境下 GLOB 未触发重新配置导致漏编译
list(APPEND MY_UI_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/custom/music_screen_common.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/music_styles.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/chat_ui.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/pinyin_dict_simplified.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/ota_upgrade_dialog.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/startup_ready.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/start_screen.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/bg_noise_tile.c
    ${CMAKE_CURRENT_LIST_DIR}/custom/cute_devil_img.c
)

idf_component_register(
    SRCS        ${MY_UI_SRCS}
    INCLUDE_DIRS
        "generated"
        "custom"
        "include"
        "generated/guider_fonts"
        "generated/guider_customer_fonts"
        "generated/images"
    EMBED_TXTFILES
        ${CMAKE_SOURCE_DIR}/services/weather-service/data/admin_divisions_2023.csv
    REQUIRES
        lvgl__lvgl
        json
        audio_player
        app_env_sht40
    PRIV_REQUIRES
        lvgl_port
        esp_wifi
        esp_netif
        lwip
        esp_event
        nvs_flash
        esp_http_client
        esp_websocket_client
        audio_i2s
        esp-tls
        mbedtls
        net_wifi_sntp
        ota_update
        sdcard_spi
        app_inmp441
)
