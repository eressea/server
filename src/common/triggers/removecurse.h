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

#ifndef REMOVECURSE_H
#define REMOVECURSE_H
#ifdef __cplusplus
extern "C" {
#endif

/* all types we use are defined here to reduce dependencies */
struct trigger_type;
struct trigger;

struct unit;
struct curse;

extern struct trigger_type tt_removecurse;

extern struct trigger * trigger_removecurse(struct curse * c, struct unit * target);

#ifdef __cplusplus
}
#endif
#endif
