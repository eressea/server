/* vi: set ts=2:
 *
 *	$Id: killunit.h,v 1.2 2001/01/26 16:19:41 enno Exp $
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

#ifndef KILLUNIT_H
#define KILLUNIT_H

struct trigger_type;
struct trigger;

struct unit;

extern struct trigger_type tt_killunit;
extern struct trigger * trigger_killunit(struct unit * u);

#endif
