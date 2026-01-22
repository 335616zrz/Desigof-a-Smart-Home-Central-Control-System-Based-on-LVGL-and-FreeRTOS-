#include "join_path.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int is_abs_url(const char *s) {
    return s && ((strncmp(s, "http://", 7) == 0) || (strncmp(s, "https://", 8) == 0));
}

static size_t base_root_len(const char *base) {
    if (!base) return 0;
    const char *scheme = strstr(base, "://");
    if (!scheme) {
        const char *p = strchr(base, '/');
        return p ? (size_t)(p - base) : strlen(base);
    }
    const char *p = strchr(scheme + 3, '/');
    return p ? (size_t)(p - base) : strlen(base);
}

int join_path(char *buf, size_t buf_len, const char *base, const char *path) {
    if (!buf || buf_len == 0 || !path) return -1;

    if (is_abs_url(path)) {
        int n = snprintf(buf, buf_len, "%s", path);
        return (n >= 0 && (size_t)n < buf_len) ? n : -1;
    }

    if (!base) {
        int n = snprintf(buf, buf_len, "%s", path);
        return (n >= 0 && (size_t)n < buf_len) ? n : -1;
    }

    int n = -1;
    if (path[0] == '/') {
        size_t root_len = base_root_len(base);
        n = snprintf(buf, buf_len, "%.*s%s", (int)root_len, base, path);
    } else {
        size_t bl = strlen(base);
        size_t dir_len = bl;
        if (bl > 0 && base[bl - 1] != '/') {
            const char *last_slash = strrchr(base, '/');
            dir_len = last_slash ? (size_t)(last_slash - base + 1) : bl;
        }
        n = snprintf(buf, buf_len, "%.*s%s", (int)dir_len, base, path);
    }
    return (n >= 0 && (size_t)n < buf_len) ? n : -1;
}

char *join_url(const char *base, const char *path) {
    if (!path) return NULL;

    /* 先估个最大长度：base + '/' + path + 1 */
    size_t max_len = 1 + (base ? strlen(base) : 0) + strlen(path) + 1;
    char *buf = (char *)malloc(max_len);
    if (!buf) return NULL;

    if (join_path(buf, max_len, base, path) < 0) {
        free(buf);
        return NULL;
    }
    return buf; /* 调用方负责 free */
}

