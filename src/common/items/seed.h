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

#ifndef H_ITM_SEED
#define H_ITM_SEED
#ifdef __cplusplus
extern "C" {
#endif


#if GROWING_TREES
extern struct item_type it_seed;
extern struct resource_type rt_seed;
extern void register_seed(void);

extern struct item_type it_mallornseed;
extern struct resource_type rt_mallornseed;
extern void register_mallornseed(void);
#else
#error seed.h should not be included when building with GROWING_TREES==0
#endif

#ifdef __cplusplus
}
#endif
#endif
