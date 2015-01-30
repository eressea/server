/* 
* +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
* |                   |  Enno Rehling <enno@eressea.de>
* | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
* | (c) 1998 - 2005   |
* |                   |  This program may not be used, modified or distributed
* +-------------------+  without prior permission by the authors of Eressea.
*
*/
#ifndef UTIL_FILEREADER_H
#define UTIL_FILEREADER_H

#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif

#define ENCODING_UTF8   0
#define ENCODING_LATIN1 1

    const char *getbuf(FILE *, int encoding);

#ifdef __cplusplus
}
#endif
#endif
