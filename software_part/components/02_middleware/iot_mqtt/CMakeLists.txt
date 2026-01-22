idf_component_register(
    SRCS "iot_mqtt.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt net_wifi_sntp
)
