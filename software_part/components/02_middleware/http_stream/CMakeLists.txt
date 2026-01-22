idf_component_register(
    SRCS
        http_playlist.c
        http_stream.c
        line_reader.c
        join_path.c
        url_encode.c
        lib/gzip/gzip_miniz.c
        lib/hls/hls_parse.c
        lib/hls/hls_playlist.c
    INCLUDE_DIRS
        include
        lib/gzip/include
        lib/hls/include
    REQUIRES                # 这些组件会把它们的头文件路径暴露给本组件
        audio_pipeline      # 提供 audio_error.h / audio_element.h / audio_mem.h 等
        audio_sal           # 提供 audio_idf_version.h 等
        esp_http_client
        esp_event
        esp_ringbuf
        log
        mbedtls             # HLS AES 解密用到
)

