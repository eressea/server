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

#include <config.h>
#include "eressea.h"
#include "ship.h"

/* kernel includes */
#include "build.h"
#include "unit.h"
#include "item.h"
#include "region.h"
#include "skill.h"

/* util includes */
#include <base36.h>
#include <event.h>
#include <xml.h>
#include <language.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>


ship_typelist *shiptypes = NULL;

static local_names * snames;

const ship_type *
findshiptype(const char * name, const struct locale * lang)
{
	local_names * sn = snames;
	void * i;

	while (sn) {
		if (sn->lang==lang) break;
		sn=sn->next;
	}
	if (!sn) {
		struct ship_typelist * stl = shiptypes;
		sn = calloc(sizeof(local_names), 1);
		sn->next = snames;
		sn->lang = lang;
		while (stl) {
			const char * n = locale_string(lang, stl->type->name[0]);
			addtoken(&sn->names, n, (void*)stl->type);
			stl=stl->next;
		}
		snames = sn;
	}
	if (findtoken(&sn->names, name, &i)==E_TOK_NOMATCH) return NULL;
	return (const ship_type*)i;
}

const ship_type *
st_find(const char* name)
{
	const struct ship_typelist * stl = shiptypes;
	while (stl && strcasecmp(stl->type->name[0], name)) stl = stl->next;
	if (!stl) { /* compatibility for old datafiles */
		stl = shiptypes;
		while (stl && strcasecmp(locale_string(default_locale, stl->type->name[0]), name)) stl = stl->next;
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

#define SMAXHASH 7919
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

struct ship *
findshipr(const region *r, int n)
{
  ship * sh;

  for (sh = r->ships; sh; sh = sh->next) {
    if (sh->no == n) {
      assert(sh->region == r);
      return sh;
    }
  }
  return 0;
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
		if(u->ship == sh && fval(u, UFL_OWNER)) return u;

	return NULL;
}

/* Alte Schiffstypen: */


ship *
new_ship(const ship_type * stype, region * r)
{
	static char buffer[7 + IDSIZE + 1];
	ship *sh = (ship *) calloc(1, sizeof(ship));

	sh->no = newcontainerid();
	sh->coast = NODIRECTION;
	sh->type = stype;
	sh->region = r;

	sprintf(buffer, "Schiff %s", shipid(sh));
	set_string(&sh->name, buffer);
	set_string(&sh->display, "");
	fset(sh, FL_UNNAMED);
	shash(sh);
	return sh;
}

void
destroy_ship(ship * sh)
{
  region * r = sh->region;
  unit * u = r->units;

  while (u) {
    if (u->ship == sh) {
      leave_ship(u);
    }
    u = u->next;
  }
  sunhash(sh);
  choplist(&r->ships, sh);
  handle_event(&sh->attribs, "destroy", sh);
}

const char *
shipname(const ship * sh)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;
	char *buf = idbuf[(++nextbuf) % 8];
	sprintf(buf, "%s (%s)", strcheck(sh->name, NAMESIZE), itoa36(sh->no));
	return buf;
}

int
shipcapacity (const ship * sh)
{
	int i;

	/* sonst ist construction:: size nicht ship_type::maxsize */
	assert(!sh->type->construction || sh->type->construction->improvement==NULL);

	if (sh->type->construction && sh->size!=sh->type->construction->maxsize)
		return 0;

#ifdef SHIPDAMAGE
	i = ((sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE)
		* sh->type->cargo / sh->size;
	i += ((sh->size * DAMAGE_SCALE - sh->damage) % DAMAGE_SCALE)
		* sh->type->cargo / (sh->size*DAMAGE_SCALE);
#else
	i = sh->type->cargo;
#endif
	return i;
}

void
getshipweight(const ship * sh, int *sweight, int *scabins)
{
	unit * u;
	*sweight = 0;
	*scabins = 0;
	for (u = sh->region->units; u; u = u->next)
	if (u->ship == sh) {
		*sweight += weight(u);
		*scabins += u->number;
	}
}

unit *
shipowner(const ship * sh)
{
	unit *u;
	unit *first = NULL;

        const region * r = sh->region;

        /* Prüfen ob Eigentümer am leben. */
	for (u = r->units; u; u = u->next) {
		if (u->ship == sh) {
			if (!first && u->number > 0)
				first = u;
			if (fval(u, UFL_OWNER) && u->number > 0)
				return u;
			if (u->number == 0)
				freset(u, UFL_OWNER);
		}
	}

	/* Eigentümer tot oder kein Eigentümer vorhanden. Erste lebende Einheit
	 * nehmen. */

	if (first)
		fset(first, UFL_OWNER);
	return first;
}

void
register_ships(void)
{
}

