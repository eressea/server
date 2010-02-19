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

#ifndef _SCURSE_H
#define _SCURSE_H
#ifdef __cplusplus
extern "C" {
#endif

struct locale;
struct message;
extern struct message * cinfo_ship(const void * obj, typ_t typ, const struct curse *c, int self);
extern void register_shipcurse(void);
	
#ifdef __cplusplus
}
#endif
#endif /* _SCURSE_H */
