#ifndef UTIL_BSDSTRING_H
#define UTIL_BSDSTRING_H

#ifdef HAVE_INLINE
# include "bsdstring.c"
#else
extern size_t strlcpy(char *dst, const char *src, size_t siz);

extern size_t strlcat(char *dst, const char *src, size_t siz);

extern int wrptr(char **ptr, size_t * size, int bytes);
#endif

#define WARN_STATIC_BUFFER() log_warning(("static buffer too small in %s:%d\n", __FILE__, __LINE__))

#if !defined(HAVE_STRLPRINTF)
# define HAVE_STRLPRINTF
# define slprintf snprintf
#endif

#endif
