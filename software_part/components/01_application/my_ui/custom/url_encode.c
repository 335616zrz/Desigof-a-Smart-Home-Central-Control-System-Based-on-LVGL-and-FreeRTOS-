#include <stdlib.h>
#include <string.h>
#include "url_encode.h"

/* 按 RFC3986，避免 locale 影响：仅 ASCII 字符集判断 */
static inline int is_alnum_ascii(unsigned char c) {
    return (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9');
}
static inline int is_unreserved(unsigned char c) {
    return is_alnum_ascii(c) || c=='-'||c=='_'||c=='.'||c=='~';
}

char *url_encode_utf8(const char *s)
{
    if (!s) return NULL;
    size_t n = 0;
    for (const unsigned char *p=(const unsigned char*)s; *p; ++p) {
        n += is_unreserved(*p) ? 1u : 3u;
    }

    char *out = (char*)malloc(n + 1);
    if (!out) return NULL;

    char *q = out;
    for (const unsigned char *p=(const unsigned char*)s; *p; ++p) {
        if (is_unreserved(*p)) *q++ = (char)*p;
        else { static const char hex[]="0123456789ABCDEF";
            *q++ = '%'; *q++ = hex[*p >> 4]; *q++ = hex[*p & 0xF];
        }
    }
    *q = '\0';
    return out;
}

