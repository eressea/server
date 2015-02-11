/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2005   |  Enno Rehling <enno@eressea.de>
 +-------------------+  

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#ifndef H_GC_XMLREPORT
#define H_GC_XMLREPORT
#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>

  extern void xmlreport_cleanup(void);
  extern void register_xr(void);

  extern int crwritemap(const char *filename);

#ifdef __cplusplus
}
#endif
#endif
