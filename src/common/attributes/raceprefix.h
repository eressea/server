
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

#ifndef H_ATTRIBUTE_RACEPREFIX
#define H_ATTRIBUTE_RACEPREFIX
#ifdef __cplusplus
extern "C" {
#endif

extern struct attrib_type at_raceprefix;
extern void set_prefix(struct attrib ** ap, const char * str);
extern const char * get_prefix(const struct attrib * a);

#ifdef __cplusplus
}
#endif
#endif

