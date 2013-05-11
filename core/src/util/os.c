#include "os.h"

#if defined(WIN32)
#include <direct.h>
#else   /* WIN32 */
#include <sys/types.h>
#include <sys/stat.h>
#endif  /* WIN32 */
int os_mkdir(const char *path, int mode) 
{
  
#ifdef WIN32
  mode = mode;
  return _mkdir(path);  
#else   /* POSIX is our last hope */
  return mkdir(path, (mode_t) mode);  
#endif  /* WIN32 */
}
