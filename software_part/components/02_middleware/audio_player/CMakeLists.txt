idf_component_register(
    SRCS
        "audio_player.c"
        "i2s_sink_element.c"
        "file_src_element.c"
        "tts_player.c"
        "music_spectrum.c"
        "server_root_cert.c"
    INCLUDE_DIRS
        "include"
    REQUIRES
        audio_pipeline
        http_stream
        esp_http_client
        esp_ringbuf
        esp-adf-libs         # 解码器位于 ADF libs
    PRIV_REQUIRES
        audio_i2s
        board
        log
)

# 先启用最常用解码器；其它格式按需再开（或转码）
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    HAVE_MP3_DEC=1
    HAVE_WAV_DEC=1
    HAVE_FLAC_DEC=1
)

# 仅对本组件生效的优化（不污染全局）
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Os
    -ffunction-sections -fdata-sections
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)
target_link_options(${COMPONENT_LIB} PRIVATE -Wl,--gc-sections)
