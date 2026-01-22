# components/02_middleware/asr_client/CMakeLists.txt

idf_component_register(
    SRCS
        "asr_client.c"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        mem_utils
        freertos
        log
        esp_event
        esp_websocket_client
        mbedtls
        json
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra
    -Wno-unused-parameter
)

