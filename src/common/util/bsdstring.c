#include <config.h>
#include <string.h>
#include "bsdstring.h"

#ifndef HAVE_INLINE
# undef INLINE_FUNCTION
# define INLINE_FUNCTION
#endif

#if !defined(HAVE_STRLCPY)
INLINE_FUNCTION size_t
strlcpy(char *dst, const char *src, size_t siz)   /* copied from OpenBSD source code */
{
  register char *d = dst;
  register const char *s = src;
  register size_t n = siz;

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
      *d = '\0';              /* NUL-terminate dst */
    while (*s++)
      ;
  }

  return(s - src - 1);    /* count does not include NUL */
}


INLINE_FUNCTION size_t
strlcat(char * dst, const char * src, size_t siz)
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
    return(dlen + strlen(s));
  while (*s != '\0') {
    if (n != 1) {
      *d++ = *s;
      n--;
    }
    s++;
  }
  *d = '\0';

  return(dlen + (s - src));       /* count does not include NUL */
}
#endif
