/* vi: set ts=2:
 *
 *	$Id: ship.h,v 1.2 2001/01/26 16:19:40 enno Exp $
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

#ifndef SHIP_H
#define SHIP_H

#include "build.h"

#define DAMAGE_SCALE 100 /* multiplier for sh->damage */

/* ship_type::flags */
#define SFL_OPENSEA 0x01

typedef struct ship_type {
	const char * name[2];

	int range;  /* range in regions */
	int flags;  /* flags */
	int combat; /* modifier for combat */

	double storm; /* multiplier for chance to drift in storm */
	double damage; /* multiplier for damage taken by the ship */

	int cabins;   /* max. cabins (people) */
	int cargo;    /* max. cargo (weight) */

	int cptskill; /* min. skill of captain */
	int minskill; /* min. skill to sail this (crew) */
	int sumskill; /* min. sum of crew+captain */

	const terrain_t * coast; /* coast that this ship can land on */

	const construction * construction; /* how to build a ship */
} ship_type;

typedef struct ship_typelist {
	struct ship_typelist * next;
	const ship_type * type;
} ship_typelist;

extern ship_typelist *shiptypes;

/* Alte Schiffstypen: */

extern const ship_type st_boat;
extern const ship_type st_longboat;
extern const ship_type st_dragonship;
extern const ship_type st_caravelle;
extern const ship_type st_trireme;

extern const ship_type st_transport;

extern const ship_type * st_find(const char* name);
extern void st_register(const ship_type * type);

#define NOSHIP NULL

extern void damage_ship(ship *sh, double percent);
extern struct unit *captain(ship *sh, struct region *r);
extern struct unit *shipowner(const struct region * r, const struct ship * sh);

extern ship *new_ship(const struct ship_type * stype);
extern char *shipname(const struct ship * sh);
extern ship *findship(int n);
#endif
