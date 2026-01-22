#pragma once
#ifdef __cplusplus
extern "C" {
#endif
/* 以 malloc 方式返回新串，失败返回 NULL；用 free 释放 */
char *url_encode_utf8(const char *s);
#ifdef __cplusplus
}
#endif

