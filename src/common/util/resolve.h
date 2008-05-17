/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef RESOLVE_H
#define RESOLVE_H

#include "variant.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef void (*resolve_fun)(variant data, void * address);
  extern void ur_add(variant data, void * ptrptr, resolve_fun fun);
  extern void resolve(void);

#ifdef __cplusplus
}
#endif
#endif
