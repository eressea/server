/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef CREATEUNIT_H
#define CREATEUNIT_H

/* all types we use are defined here to reduce dependencies */
struct trigger_type;
struct trigger;
struct region;
struct faction;
struct unit;

extern struct trigger_type tt_createunit;

extern struct trigger * trigger_createunit(struct region * r, struct faction * f, const struct race * rc, int number);

#endif
