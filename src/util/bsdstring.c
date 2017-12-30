#ifdef _MSC_VER
#include <platform.h>
#endif
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>

#include "bsdstring.h"
#include "log.h"
#include "strings.h"

int wrptr(char **ptr, size_t * size, int result)
{
    size_t bytes = (size_t)result;
    if (result < 0) {
        /* buffer was too small */
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

char * strlcpy_w(char *dst, const char *src, size_t *siz, const char *err, const char *file, int line)
{
    size_t bytes = str_strlcpy(dst, src, *siz);
    char * buf = dst;
    assert(bytes <= INT_MAX);
    if (wrptr(&buf, siz, (int)bytes) != 0) {
        if (err) {
            log_warning("%s: static buffer too small in %s:%d\n", err, file, line);
        } else {
            log_warning("static buffer too small in %s:%d\n", file, line);
        }
    }
    return buf;
}
