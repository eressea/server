/* vi: set ts=2:
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

#ifdef __cplusplus
extern "C" {
#endif

struct faction;
struct attrib;
extern struct attrib_type at_otherfaction;
extern void init_otherfaction(void);
extern struct faction * get_otherfaction(const struct attrib * a);
extern struct attrib * make_otherfaction(struct faction * f);
extern struct faction * visible_faction(const struct faction *f, const struct unit * u);

#ifdef __cplusplus
extern "C" {
#endif
