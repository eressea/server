/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef GIVEITEM_H
#define GIVEITEM_H
#ifdef __cplusplus
extern "C" {
#endif

/* all types we use are defined here to reduce dependencies */
struct trigger_type;
struct trigger;
struct unit;
struct item_type;

extern struct trigger_type tt_giveitem;

extern struct trigger * trigger_giveitem(struct unit * mage, const struct item_type * itype, int number);

#ifdef __cplusplus
}
#endif
#endif
