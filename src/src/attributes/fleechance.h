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

#ifndef H_ATTRIBUTE_FLEECHANCE
#define H_ATTRIBUTE_FLEECHANCE
#ifdef __cplusplus
extern "C" {
#endif

extern struct attrib_type at_fleechance;

extern struct attrib * make_fleechance(float fleechance);
extern void init_fleechance(void);

#ifdef __cplusplus
}
#endif
#endif
