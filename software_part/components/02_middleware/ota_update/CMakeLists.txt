idf_component_register(
    SRCS
        "ota_update.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        esp_https_ota
        esp_http_client
        app_update
        esp_app_format
        esp_event
        esp_timer
        freertos
        net_wifi_sntp
        log
        audio_player
)

