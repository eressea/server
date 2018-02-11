
#include <stddef.h>

#ifdef WIN32
#define PATH_DELIM '\\'
#else
#define PATH_DELIM '/'
#endif

char * path_join(const char *p1, const char *p2, char *dst, size_t len);
