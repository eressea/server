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

#ifndef H_ATTRIBUTE_REDUCEPRODUCTION
#define H_ATTRIBUTE_REDUCEPRODUCTION
#ifdef __cplusplus
extern "C" {
#endif

extern struct attrib * make_reduceproduction(int percent, int time);
extern struct attrib_type at_reduceproduction;
extern void init_reduceproduction(void);

#ifdef __cplusplus
}
#endif
#endif

