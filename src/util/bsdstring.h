#ifndef UTIL_BSDSTRING_H
#define UTIL_BSDSTRING_H

#include <stddef.h>

int wrptr(char **ptr, size_t * size, int bytes);
char * strlcpy_w(char *dst, const char *src, size_t *siz, const char *err, const char *file, int line);

#define WARN_STATIC_BUFFER_EX(foo) log_warning("%s: static buffer too small in %s:%d\n", (foo), __FILE__, __LINE__)
#define WARN_STATIC_BUFFER() log_warning("static buffer too small in %s:%d\n", __FILE__, __LINE__)
#define INFO_STATIC_BUFFER() log_info("static buffer too small in %s:%d\n", __FILE__, __LINE__)
#define STRLCPY(dst, src, siz) strlcpy_w((dst), (src), &(siz), 0, __FILE__, __LINE__)
#define STRLCPY_EX(dst, src, siz, err) strlcpy_w((dst), (src), (siz), (err), __FILE__, __LINE__)

#endif
