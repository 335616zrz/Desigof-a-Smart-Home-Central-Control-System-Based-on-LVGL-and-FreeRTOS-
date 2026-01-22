idf_component_register(
    SRCS "weather_client.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_client json
)
