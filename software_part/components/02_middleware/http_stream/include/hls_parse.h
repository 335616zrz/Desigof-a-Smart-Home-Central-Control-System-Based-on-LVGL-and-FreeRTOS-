#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "hls_playlist.h"   /* 复用其中的 hls_file_type_t / hls_playlist_type_t / hls_type_t / hls_encrypt_method_t */

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 常量（供 hls_parse.c 使用） ================= */
#define HLS_STR_EXTM3U                "#EXTM3U"
#define HLS_STR_EXT                   "#EXT"
#define HLS_STR_EXT_X_                "#EXT-X-"
#define HLS_STR_BYTERANGE             "BYTERANGE"
#define HLS_STR_DISCONTINUITY         "DISCONTINUITY"
#define HLS_STR_ENDLIST               "ENDLIST"
#define HLS_STR_INF                   "INF"
#define HLS_STR_I_FRAME_STREAM_INF    "I-FRAME-STREAM-INF"
#define HLS_STR_INDEPENDENT_SEGMENTS  "INDEPENDENT-SEGMENTS"
#define HLS_STR_KEY                   "KEY"
#define HLS_STR_MEDIA                 "MEDIA"
#define HLS_STR_MEDIA_SEQUENCE        "MEDIA-SEQUENCE"
#define HLS_STR_MAP                   "MAP"
#define HLS_STR_PLAYLIST_TYPE         "PLAYLIST-TYPE"
#define HLS_STR_STREAM_INF            "STREAM-INF"
#define HLS_STR_SESSION_KEY           "SESSION-KEY"
#define HLS_STR_TARGETDURATION        "TARGETDURATION"
#define HLS_STR_VERSION               "VERSION"
#define HLS_STR_EXT_X_STREAM_INF      "#EXT-X-STREAM-INF"
#define HLS_STR_EXT_X_MEDIA           "#EXT-X-MEDIA"
#define HLS_STR_EXTINF                "#EXTINF"

/* 供日志/转字符串使用的辅助常量（你之前缺少这些，导致未定义） */
#define HLS_STR_IGNORE                "IGNORE"
#define HLS_STR_DURATION              "DURATION"
#define HLS_STR_TITLE                 "TITLE"
#define HLS_STR_INT                   "INT"

/* 值枚举字符串 */
#define HLS_STR_EVENT                 "EVENT"
#define HLS_STR_VOD                   "VOD"
#define HLS_STR_AUDIO                 "AUDIO"
#define HLS_STR_VIDEO                 "VIDEO"
#define HLS_STR_SUBTITLES             "SUBTITLES"
#define HLS_STR_CLOSED_CAPTIONS       "CLOSED-CAPTIONS"
#define HLS_STR_YES                   "YES"
#define HLS_STR_AES_128               "AES-128"
#define HLS_STR_SAMPLE_AES            "SAMPLE-AES"

/* 属性键字符串（解析时匹配） */
#define HLS_STR_AUTOSELECT            "AUTOSELECT"
/* 兼容 hls_parse.c 里使用的 HLS_STR_AUTO_SELECT 写法 */
#define HLS_STR_AUTO_SELECT           HLS_STR_AUTOSELECT

#define HLS_STR_BANDWIDTH             "BANDWIDTH"
#define HLS_STR_CODECS                "CODECS"
#define HLS_STR_DEFAULT               "DEFAULT"
#define HLS_STR_FORCED                "FORCED"
#define HLS_STR_GROUP_ID              "GROUP-ID"
#define HLS_STR_IV                    "IV"
#define HLS_STR_KEYFORMAT             "KEYFORMAT"
#define HLS_STR_KEYFORMATVERSION      "KEYFORMATVERSIONS"
#define HLS_STR_LANGUAGE              "LANGUAGE"
#define HLS_STR_METHOD                "METHOD"
#define HLS_STR_NAME                  "NAME"
#define HLS_STR_PROGRAM_ID            "PROGRAM-ID"
#define HLS_STR_RESOLUTION            "RESOLUTION"
#define HLS_STR_TYPE                  "TYPE"
#define HLS_STR_URI                   "URI"

/* =================== 属性键/标签枚举 =================== */
#ifndef HLS_MAX_ATTR_NUM
#define HLS_MAX_ATTR_NUM   16
#endif

#ifndef HLS_MAX_LINE_CHAR
#define HLS_MAX_LINE_CHAR  2048
#endif

typedef enum {
    HLS_ATTR_IGNORE = 0,
    HLS_ATTR_AUTO_SELECT,
    HLS_ATTR_AUDIO,
    HLS_ATTR_BANDWIDTH,
    HLS_ATTR_CODECS,
    HLS_ATTR_DEFAULT,
    HLS_ATTR_FORCED,
    HLS_ATTR_GROUP_ID,
    HLS_ATTR_IV,
    HLS_ATTR_KEYFORMAT,
    HLS_ATTR_KEYFORMAT_VERSION,
    HLS_ATTR_LANGUAGE,
    HLS_ATTR_METHOD,
    HLS_ATTR_NAME,
    HLS_ATTR_PROGRAM_ID,
    HLS_ATTR_RESOLUTION,
    HLS_ATTR_SUBTITLES,
    HLS_ATTR_TYPE,
    HLS_ATTR_URI,
    /* 你之前没有这两个，hls_attr2str() 会用到 */
    HLS_ATTR_VERSION,
    HLS_ATTR_TITLE,
    /* 以及解析数值/时长用到的占位 */
    HLS_ATTR_DURATION,
    HLS_ATTR_INT,
    HLS_ATTR_MAX
} hls_attr_t;

typedef enum {
    HLS_TAG_IGNORE = 0,
    HLS_TAG_URI,
    HLS_TAG_BYTE_RANGE,
    HLS_TAG_DISCONTINUITY,
    HLS_TAG_ENDLIST,
    HLS_TAG_INF,
    HLS_TAG_I_FRAME_STREAM_INF,
    HLS_TAG_INDEPENDENT_SEGMENTS,
    HLS_TAG_KEY,
    HLS_TAG_MEDIA,
    HLS_TAG_MEDIA_SEQUENCE,
    HLS_TAG_MAP,
    HLS_TAG_PLAYLIST_TYPE,
    HLS_TAG_STREAM_INF,
    HLS_TAG_SESSION_KEY,
    HLS_TAG_TARGET_DURATION,
    HLS_TAG_VERSION,
    /* 你在 .c 里会产生的“续行追加”两种标签，也需要声明 */
    HLS_TAG_STREAM_INF_APPEND,
    HLS_TAG_INF_APPEND,
} hls_tag_t;

#define HLS_TAG_TARGETDURATION HLS_TAG_TARGET_DURATION

/* 属性值联合体 */
typedef union {
    uint64_t v;
    float    f;
    char    *s;
} hls_attr_val_u;

/* 为回调提前声明结构体名，便于在回调类型里引用 */
struct hls_tag_info;

/* 解析回调：返回 0 表示成功/继续解析，非 0 可中断解析 */
typedef int (*hls_tag_callback)(struct hls_tag_info *info, void *ctx);

/* 一行（或一个条目）解析后的键值集 */
typedef struct hls_tag_info {
    hls_tag_t       tag;
    int             attr_num;
    hls_attr_t     *k;   // 指向 parser->k
    hls_attr_val_u *v;   // 指向 parser->v
} hls_tag_info_t;

/* 解析器状态（对外不关心内部 reader 具体类型） */
typedef struct {
    void        *reader;                      /* line_reader_t* */
    char        *attr[HLS_MAX_ATTR_NUM];      /* 临时分割用 */
    hls_attr_t   k[HLS_MAX_ATTR_NUM];
    hls_attr_val_u v[HLS_MAX_ATTR_NUM];
    hls_tag_t    tag;
} hls_parse_t;

/* ================ 回调式 API（与 hls_parse.c 对齐） ================ */
int  hls_parse_init(hls_parse_t *parser);
void hls_parse_deinit(hls_parse_t *parser);
int  hls_parse_add_buffer(hls_parse_t *parser, uint8_t *buffer, int size, bool eos);
int  hls_parse(hls_parse_t *parser, hls_tag_callback cb, void *ctx);

/* 粗判一段缓冲是 Master 还是 Media（供上层先行路由） */
hls_file_type_t hls_get_file_type(uint8_t *b, int len);

#ifdef __cplusplus
}
#endif

