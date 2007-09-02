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

#ifndef H_ATTRIBUTE_MOVEMENT
#define H_ATTRIBUTE_MOVEMENT
#ifdef __cplusplus
extern "C" {
#endif

extern boolean get_movement(struct attrib * const * alist, int type);
extern void set_movement(struct attrib ** alist, int type);
extern void init_movement(void);

extern struct attrib_type at_movement;

#ifdef __cplusplus
}
#endif
#endif
