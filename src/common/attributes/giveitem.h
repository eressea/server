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
#ifndef H_ATTRIBUTE_GIVEITEM
#define H_ATTRIBUTE_GIVEITEM
#ifdef __cplusplus
extern "C" {
#endif

struct building;
struct item;

extern struct attrib_type at_giveitem;

extern struct attrib * make_giveitem(struct building * b, struct item * items);
extern void init_giveitem(void);

#ifdef __cplusplus
}
#endif
#endif
