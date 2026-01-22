idf_component_register(
    SRCS "net_rtt_ping.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES lwip esp_netif esp_event esp_timer log newlib
)

