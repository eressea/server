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

#ifndef H_ATTRIBUTE_VIEWRANGE
#define H_ATTRIBUTE_VIEWRANGE
#ifdef __cplusplus
extern "C" {
#endif

extern struct attrib_type at_viewrange;

extern struct attrib * make_viewrange(const char *function);
extern struct attrib * add_viewrange(struct attrib ** alist, const char *function);
extern void init_viewrange(void);

#ifdef __cplusplus
}
#endif
#endif

