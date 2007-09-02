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

#ifndef H_ATTRIBUTE_TARGETREGION
#define H_ATTRIBUTE_TARGETREGION
#ifdef __cplusplus
extern "C" {
#endif

extern struct attrib_type at_targetregion;

struct region;
extern struct attrib * make_targetregion(struct region *);
extern void init_targetregion(void);

#ifdef __cplusplus
}
#endif
#endif

