/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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


extern struct resource_type * rt_seed;
extern void init_seed(void);

extern struct resource_type * rt_mallornseed;
extern void init_mallornseed(void);

#ifdef __cplusplus
}
#endif
#endif
