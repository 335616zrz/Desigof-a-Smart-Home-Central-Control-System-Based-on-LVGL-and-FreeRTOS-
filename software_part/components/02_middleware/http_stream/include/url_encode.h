#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* 把 UTF-8 字符串做 URL 百分号编码。
 * 返回值由 audio_free() 释放；失败返回 NULL。
 */
char *url_encode_utf8(const char *s);

#ifdef __cplusplus
}
#endif

