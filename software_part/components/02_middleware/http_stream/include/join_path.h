#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 连接 base 与 path，返回在堆上分配的新字符串（需要调用方 free）。
 * 规则：
 *  - path 是绝对 URL（http/https）→ 直接拷贝
 *  - path 以 '/' 开头 → 用 base 的 scheme+host（不含路径）拼接
 *  - 其他相对路径 → 拼到 base 的“目录”后
 * 失败返回 NULL。
 */
char *join_url(const char *base, const char *path);

/* 如果你在别处需要写入已有缓冲区，可以用这个 */
int join_path(char *buf, size_t buf_len, const char *base, const char *path);

#ifdef __cplusplus
}
#endif

