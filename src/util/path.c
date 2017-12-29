#ifdef _MSC_VER
#include <platform.h>
#endif

#include "path.h"
#include "bsdstring.h"

#include <assert.h>
#include <string.h>

char * path_join(const char *p1, const char *p2, char *dst, size_t len) {
    size_t sz;
    assert(p1 && p2);
    assert(p2 != dst);
    if (dst == p1) {
        sz = strlen(p1);
    }
    else {
        sz = strlcpy(dst, p1, len);
    }
    assert(sz < len);
    dst[sz++] = PATH_DELIM;
    strlcpy(dst + sz, p2, len - sz);
    return dst;
}

