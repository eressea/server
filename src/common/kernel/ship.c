/* vi: set ts=2:
 *
 *	$Id: ship.c,v 1.2 2001/01/26 16:19:40 enno Exp $
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

#include <config.h>
#include "eressea.h"
#include "ship.h"

/* kernel includes */
#include "unit.h"
#include "item.h"
#include "region.h"

/* util includes */
#include <base36.h>
#include <event.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>


ship_typelist *shiptypes = NULL;

const ship_type *
st_find(const char* name)
{
	const struct ship_typelist * stl = shiptypes;
	while (stl && strcasecmp(stl->type->name[0], name)) stl = stl->next;
	if (!stl) {
		stl = shiptypes;
		while (stl && strncasecmp(stl->type->name[0], name, strlen(name))) stl = stl->next;
	}
	return stl?stl->type:NULL;
}

void
st_register(const ship_type * type) {
	struct ship_typelist * stl = malloc(sizeof(ship_type));
	stl->type = type;
	stl->next = shiptypes;
	shiptypes = stl;
}

static const terrain_t coast_large[] = {
	T_OCEAN, T_PLAIN, NOTERRAIN
};

static const terrain_t coast_small[] = {
	T_OCEAN, T_PLAIN, T_SWAMP, T_DESERT, T_HIGHLAND, T_MOUNTAIN, T_GLACIER,
	T_GRASSLAND, T_VOLCANO, T_VOLCANO_SMOKING, T_ICEBERG_SLEEP, T_ICEBERG,
	NOTERRAIN
};

#define SMAXHASH 8191
ship *shiphash[SMAXHASH];
void
shash(ship * s)
{
	ship *old = shiphash[s->no % SMAXHASH];

	shiphash[s->no % SMAXHASH] = s;
	s->nexthash = old;
}

void
sunhash(ship * s)
{
	ship **show;

	for (show = &shiphash[s->no % SMAXHASH]; *show; show = &(*show)->nexthash) {
		if ((*show)->no == s->no)
			break;
	}
	if (*show) {
		assert(*show == s);
		*show = (*show)->nexthash;
		s->nexthash = 0;
	}
}

static ship *
sfindhash(int i)
{
	ship *old;

	for (old = shiphash[i % SMAXHASH]; old; old = old->nexthash)
		if (old->no == i)
			return old;
	return 0;
}

struct ship *
findship(int i)
{
	return sfindhash(i);
}

void
damage_ship(ship * sh, double percent)
{
	double damage = DAMAGE_SCALE * percent * sh->size + sh->damage;
	sh->damage = (int)damage;
}

unit *
captain(ship *sh, region *r)
{
	unit *u;

	for(u = r->units; u; u = u->next)
		if(u->ship == sh && fval(u, FL_OWNER)) return u;

	return NULL;
}

/* Alte Schiffstypen: */

static requirement boat_req[] = {
  {I_WOOD, 1},
  {0, 0}
};

static const construction boat_bld = {
  SK_SHIPBUILDING, 1,
  5, 1, boat_req,
  NULL
};

const ship_type st_boat = {
	{ "Boot", "ein Boot" }, 2,
	SFL_OPENSEA, 0, 1.00, 1.00,
	5, 50*100,
	1, 1, 2, coast_small,
	&boat_bld
};

static requirement longboat_req[] = {
  {I_WOOD, 1},
  {0, 0}
};

static const construction longboat_bld = {
  SK_SHIPBUILDING, 1,
  50, 1, longboat_req,
  NULL
};

const ship_type st_longboat = {
	{ "Langboot", "ein Langboot" }, 3,
	SFL_OPENSEA, 0, 1.00, 1.00,
	50, 500*100,
	1, 1, 10, coast_large,
	&longboat_bld
};

static requirement dragonship_req[] = {
  {I_WOOD, 1},
  {0, 0}
};
static const construction dragonship_bld = {
  SK_SHIPBUILDING, 2,
  100, 1, dragonship_req,
  NULL
};
const ship_type st_dragonship = {
	{ "Drachenschiff", "ein Drachenschiff" }, 5,
	SFL_OPENSEA, 0, 1.00, 1.00,
	100, 1000*100,
	2, 1, 50, coast_large,
	&dragonship_bld
};

static requirement caravelle_req[] = {
  {I_WOOD, 1},
  {0, 0}
};
static const construction caravelle_bld = {
  SK_SHIPBUILDING, 3,
  250, 1, caravelle_req,
  NULL
};
const ship_type st_caravelle = {
	{ "Karavelle", "eine Karavelle" }, 5,
	SFL_OPENSEA, 0, 1.00, 1.00,
	300, 3000*100,
	3, 1, 30, coast_large,
	&caravelle_bld
};

static requirement trireme_req[] = {
  {I_WOOD, 1},
  {0, 0}
};
static const construction trireme_bld = {
  SK_SHIPBUILDING, 4,
  200, 1, trireme_req,
  NULL
};
const ship_type st_trireme = {
	{ "Trireme", "eine Trireme" }, 7,
	SFL_OPENSEA, 0, 1.00, 1.00,
	200, 2000*100,
	4, 1, 120, coast_large,
	&trireme_bld
};

ship *
new_ship(const ship_type * stype)
{
	static char buffer[7 + IDSIZE + 1];
	ship *sh = (ship *) calloc(1, sizeof(ship));

	sh->no = newcontainerid();
	sh->coast = NODIRECTION;
	sh->type = stype;

	sprintf(buffer, "Schiff %s", shipid(sh));
	set_string(&sh->name, buffer);
	set_string(&sh->display, "");
	fset(sh, FL_UNNAMED);
	shash(sh);
	return sh;
}

void
destroy_ship(ship * s, region * r)
{
	unit * u = r->units;

	if(!findship(s->no)) return;
#ifdef OLD_TRIGGER
	do_trigger(s, TYP_SHIP, TR_DESTRUCT);
#endif
	while (u) {
		if (u->ship == s) {
			leave_ship(u);
		}
		u = u->next;
	}
	sunhash(s);
#ifdef OLD_TRIGGER
	change_all_pointers(s, TYP_SHIP, NULL);
#endif
	choplist(&r->ships, s);
	handle_event(&s->attribs, "destroy", s);
}

char *
shipname(const ship * sh)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;
	char *buf = idbuf[(++nextbuf) % 8];
	sprintf(buf, "%s (%s)", strcheck(sh->name, NAMESIZE), itoa36(sh->no));
	return buf;
}

unit *
shipowner(const region * r, const ship * sh)
{
	unit *u;
	unit *first = NULL;

	/* Prüfen ob Eigentümer am leben. */

	for (u = r->units; u; u = u->next) {
		if (u->ship == sh) {
			if (!first && u->number > 0)
				first = u;
			if (fval(u, FL_OWNER) && u->number > 0)
				return u;
			if (u->number == 0)
				freset(u, FL_OWNER);
		}
	}

	/* Eigentümer tot oder kein Eigentümer vorhanden. Erste lebende Einheit
	 * nehmen. */

	if (first)
		fset(first, FL_OWNER);
	return first;
}
