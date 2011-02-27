#include "os.h"

#if defined(WIN32)
#include <direct.h>
#elif defined(__GCC__)
#include <sys/stat.h>
#endif

int os_mkdir(const char * path, int mode)
{
#ifdef WIN32
  mode = mode;
  return _mkdir(path);
#else /* POSIX is our last hope */
  return mkdir(path, (mode_t)mode);
#endif
}