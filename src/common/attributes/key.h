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

#ifndef H_ATTRIBUTE_KEY
#define H_ATTRIBUTE_KEY
#ifdef __cplusplus
extern "C" {
#endif

extern struct attrib_type at_key;

extern struct attrib * make_key(int key);
extern struct attrib * find_key(struct attrib * alist, int key);
extern struct attrib * add_key(struct attrib ** alist, int key);
extern void init_key(void);

#ifdef __cplusplus
}
#endif
#endif

