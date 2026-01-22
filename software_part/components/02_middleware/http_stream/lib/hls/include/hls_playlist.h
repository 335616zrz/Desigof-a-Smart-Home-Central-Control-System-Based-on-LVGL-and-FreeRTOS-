/*
 * ESPRESSIF MIT License
 */

#ifndef _HLS_PLAYLIST_H
#define _HLS_PLAYLIST_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* hls_handle_t;

/* ========= 新增：供解析/播放两侧共用的枚举 ========= */

/* 粗粒度文件类型（Master or Media） */
typedef enum {
    HLS_FILE_TYPE_NONE = 0,
    HLS_FILE_TYPE_MASTER_PLAYLIST,
    HLS_FILE_TYPE_MEDIA_PLAYLIST,
} hls_file_type_t;

/* 播放列表类型（是否随时间变化） */
typedef enum {
    HLS_PLAYLIST_TYPE_EVENT = 0,
    HLS_PLAYLIST_TYPE_VOD,
} hls_playlist_type_t;

/* 媒体类型（#EXT-X-MEDIA TYPE=... & 内部使用的 AV 组合类型） */
typedef enum {
    HLS_TYPE_AUDIO = 0,
    HLS_TYPE_VIDEO,
    HLS_TYPE_SUBTITLES,
    HLS_TYPE_CLOSED_CAPTION,
    HLS_TYPE_AV,                 /* 内部使用：音视频合流 */
} hls_type_t;

/* 加密方式（#EXT-X-KEY METHOD=...） */
typedef enum {
    HLS_ENCRYPT_METHOD_NONE = 0,
    HLS_ENCRYPT_METHOD_AES128,
    HLS_ENCRYPT_METHOD_SAMPLE_AES,
} hls_encrypt_method_t;

/* ========= 你原本就有的定义（保留） ========= */

/**
 * @brief HLS stream type（对外选择偏好用）
 */
typedef enum {
   HLS_STREAM_TYPE_AUDIO,    /*!< Audio stream type */
   HLS_STREAM_TYPE_VIDEO,    /*!< Video stream type */
   HLS_STREAM_TYPE_AV,       /*!< Audio and Video stream type */
   HLS_STREAM_TYPE_SUBTITLE, /*!< Subtitle stream type */
} hls_stream_type_t;

/**
 * @brief HLS AES key information
 */
typedef struct {
   char key[16];
   char iv[16];
} hls_stream_key_t;

/**
 * @brief Callback for HLS media uri
 */
typedef int (*hls_uri_callback) (char* uri, void* tag);

/**
 * @brief HLS playlist parser configuration
 */
typedef struct {
    uint32_t         prefer_bitrate;   /*!< Prefer bitrate used to filter media playlist */
    hls_uri_callback cb;               /*!< HLS media stream uri callback */
    void*            ctx;              /*!< Input context */
    char*            uri;              /*!< M3U8 host url */
} hls_playlist_cfg_t;

/* ========= APIs ========= */

/**
 * @brief         Open HLS playlist parser
 */
hls_handle_t hls_playlist_open(hls_playlist_cfg_t* cfg);

/**
 * @brief         Check whether playlist is master playlist
 */
bool hls_playlist_is_master(hls_handle_t h);

/**
 * @brief         Check whether media playlist contain #ENDLIST tag
 */
bool hls_playlist_is_media_end(hls_handle_t h);

/**
 * @brief         Filter url by stream type and given bitrate
 */
char* hls_playlist_get_prefer_url(hls_handle_t h, hls_stream_type_t type);

/**
 * @brief           Parse data of HLS playlist
 */
int hls_playlist_parse_data(hls_handle_t h, uint8_t* data, int size, bool eos);

/**
 * @brief         Check whether playlist is encrypt or not
 */
bool hls_playlist_is_encrypt(hls_handle_t h);

/**
 * @brief         Get key URI string
 */
const char *hls_playlist_get_key_uri(hls_handle_t h);

/**
 * @brief  Get key URI string by sequence number
 */
char *hls_playlist_get_key_uri_by_seq(hls_handle_t h, uint64_t sequence_no);

/**
 * @brief         Get sequence number
 */
uint64_t hls_playlist_get_sequence_no(hls_handle_t h);

/**
 * @brief         Get AES key information
 */
int hls_playlist_get_key(hls_handle_t h, uint64_t sequence_no, hls_stream_key_t* key);

/**
 * @brief           Parse HLS key
 */
int hls_playlist_parse_key(hls_handle_t h, uint8_t* buffer, int size);

/**
 * @brief         Close parse for HLS playlist
 */
int hls_playlist_close(hls_handle_t h);

#ifdef __cplusplus
}
#endif

#endif /* _HLS_PLAYLIST_H */

