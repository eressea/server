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

#ifndef H_ATTRIBUTE_RACENAME_H
#define H_ATTRIBUTE_RACENAME_H
#ifdef __cplusplus
extern "C" {
#endif

struct attrib_type;
struct attrib;

extern void set_racename(struct attrib ** palist, const char * name);
extern const char * get_racename(struct attrib * alist);
extern void init_racename(void);

extern struct attrib_type at_racename;

#ifdef __cplusplus
}
#endif
#endif

