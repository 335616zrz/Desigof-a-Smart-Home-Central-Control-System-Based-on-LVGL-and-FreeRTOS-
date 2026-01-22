#include <string.h>
#include <stdbool.h>
#include "audio_mem.h"  // audio_malloc / audio_free

static inline bool is_unreserved(unsigned char c) {
    /* RFC 3986 unreserved: ALPHA / DIGIT / "-" / "." / "_" / "~"
       我们另外保留 "/"，因为文件路径里会用到它 */
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') ||
           c == '-' || c == '_' || c == '.' || c == '~' || c == '/';
}

char *url_encode_utf8(const char *s)
{
    if (!s) return NULL;
    size_t in_len = strlen(s);
    /* 最坏情况每个字节变成 %XX（三字节），再 +1 结尾符 */
    size_t out_cap = in_len * 3 + 1;

    char *out = (char *)audio_malloc(out_cap);
    if (!out) return NULL;

    static const char HEX[] = "0123456789ABCDEF";
    size_t j = 0;

    for (size_t i = 0; i < in_len; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (is_unreserved(c)) {
            out[j++] = (char)c;
        } else {
            out[j++] = '%';
            out[j++] = HEX[(c >> 4) & 0x0F];
            out[j++] = HEX[c & 0x0F];
        }
    }
    out[j] = '\0';
    return out;
}

