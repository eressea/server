/* vi: set ts=2:
 *
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */
#ifndef H_MOD_WORMHOLE
#define H_MOD_WORMHOLE
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WORMHOLE_MODULE
#error "must define WORMHOLE_MODULE to use this module"
#endif

  extern void create_wormholes(void);
  extern void register_wormholes(void);

#ifdef __cplusplus
}
#endif
#endif
