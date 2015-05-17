#ifndef UTIL_BSDSTRING_H
#define UTIL_BSDSTRING_H

#include <stddef.h>
extern int wrptr(char **ptr, size_t * size, size_t bytes);

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#ifndef HAVE_SLPRINTF
extern size_t slprintf(char * dst, size_t size, const char * format, ...);
#endif

#define WARN_STATIC_BUFFER() log_warning("static buffer too small in %s:%d\n", __FILE__, __LINE__)

#endif
