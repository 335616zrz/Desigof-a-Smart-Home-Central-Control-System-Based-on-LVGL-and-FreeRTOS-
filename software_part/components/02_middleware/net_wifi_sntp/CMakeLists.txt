idf_component_register(
    SRCS "net_wifi_sntp.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event esp_netif nvs_flash lwip esp_timer
)

