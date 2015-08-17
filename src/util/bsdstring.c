#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include "bsdstring.h"
#include "log.h"

int wrptr(char **ptr, size_t * size, int result)
{
    size_t bytes = (size_t)result;
    if (result < 0) {
        // _snprintf buffer was too small
        if (*size > 0) {
            **ptr = 0;
            *size = 0;
        }
        errno = 0;
        return ERANGE;
    }
    if (bytes == 0) {
        return 0;
    }
    if (bytes <= *size) {
        *ptr += bytes;
        *size -= bytes;
        return 0;
    }

    *ptr += *size;
    *size = 0;
    return ERANGE;
}

#ifndef HAVE_STRLCPY
#define HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz)
{                               /* copied from OpenBSD source code */
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;

    assert(src && dst);
    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';                /* NUL-terminate dst */
        while (*s++);
    }

    return (s - src - 1);         /* count does not include NUL */
}
#endif

char * strlcpy_w(char *dst, const char *src, size_t *siz, const char *err, const char *file, int line)
{
    size_t bytes = strlcpy(dst, src, *siz);
    char * buf = dst;
    if (wrptr(&buf, siz, bytes) != 0)
        log_warning("%s: static buffer too small in %s:%d\n", err, file, line);
    return buf;
}



#ifndef HAVE_STRLCAT
#define HAVE_STRLCAT
size_t strlcat(char *dst, const char *src, size_t siz)
{
    register char *d = dst;
    register const char *s = src;
    register size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (*d != '\0' && n-- != 0)
        d++;
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
        return (dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src));    /* count does not include NUL */
}
#endif

#ifndef HAVE_SLPRINTF
#define HAVE_SLPRINTF
size_t slprintf(char * dst, size_t size, const char * format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = vsnprintf(dst, size, format, args);
    if (result < 0 || result >= (int)size) {
        dst[size - 1] = '\0';
        return size;
    }
    va_start(args, format);
    va_end(args);

    return (size_t)result;
}
#endif
