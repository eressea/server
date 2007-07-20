/* vi: set ts=2:
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
#ifdef __cplusplus
extern "C" {
#endif

const xmlChar * getbuf(FILE *, int encoding);

#ifdef __cplusplus
}
#endif
#endif
