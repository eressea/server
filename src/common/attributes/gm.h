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

#ifndef H_ATTRIBUTE_GM
#define H_ATTRIBUTE_GM
#ifdef __cplusplus
extern "C" {
#endif

/* this is an attribute used by the kernel (isallied) */

struct plane;
extern struct attrib_type at_gm;

extern struct attrib * make_gm(const struct plane *pl);

#ifdef __cplusplus
}
#endif
#endif

