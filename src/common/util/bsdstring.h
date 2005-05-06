#ifndef UTIL_BSDSTRING_H
#define UTIL_BSDSTRING_H

#if !defined(HAVE_STRLCPY)
# ifdef HAVE_INLINE
#  include "bsdstring.c"
# else
   extern size_t strlcpy(char *dst, const char *src, size_t siz);
   extern size_t strlcat(char * dst, const char * src, size_t siz);
# endif
#endif

#endif
