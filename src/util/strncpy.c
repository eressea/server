
/*
 * Faster replacement for ISO-C strncpy, does not pad with zeros
 */

#include <stddef.h>

char *
strncpy(char *to, const char *from, size_t size)
{
  char *t = to, *f = (char *)from;
  int copied = 0;

  while(copied < size) {
    *t = *f;
    if(*f == '\0') break;
    t++; f++; copied++;
  }

  return to;
}

