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

#include <config.h>
#include "eressea.h"
#include "ship.h"

/* kernel includes */
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
destroy_ship(ship * s, region * r)
{
	unit * u = r->units;

	if(!findship(s->no)) return;
	while (u) {
		if (u->ship == s) {
			leave_ship(u);
		}
		u = u->next;
	}
	sunhash(s);
	choplist(&r->ships, s);
	handle_event(&s->attribs, "destroy", s);
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

static int 
tagend(struct xml_stack * stack)
{
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "ship")==0) {
		st_register((ship_type*)stack->state);
		stack->state = 0;
	}
	return XML_OK;
}

static int 
tagbegin(struct xml_stack * stack)
{
	ship_type * st = (ship_type *)stack->state;
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "ship")==0) {
		const char * name = xml_value(tag, "name");
		if (name!=NULL) {
			st = stack->state = calloc(sizeof(ship_type), 1);
			st->name[0] = strdup(name);
			st->name[1] = strcat(strcpy(malloc(strlen(name)+3), name),"_a");
			st->cabins = xml_ivalue(tag, "cabins");
			st->cargo = xml_ivalue(tag, "cargo");
			st->combat = xml_ivalue(tag, "combat");
			st->cptskill = xml_ivalue(tag, "cptskill");
			st->damage = xml_fvalue(tag, "damage");
			if (xml_bvalue(tag, "fly")) st->flags |= SFL_FLY;
			if (xml_bvalue(tag, "opensea")) st->flags |= SFL_OPENSEA;
			st->minskill = xml_ivalue(tag, "minskill");
			st->range = xml_ivalue(tag, "range");
			st->storm = xml_fvalue(tag, "storm");
			st->sumskill = xml_ivalue(tag, "sumskill");
		}
	} else if (st!=NULL) {
		if (strcmp(tag->name, "coast")==0) {
			int size=0;
			terrain_t t;
			terrain_t * add;
			const char * tname = xml_value(tag, "terrain");
			if (tname!=NULL) {
				if (st->coast) {
					terrain_t * tnew;
					for (;st->coast[size]!=NOTERRAIN;++size);
					tnew = malloc(sizeof(terrain_t) * (size+2));
					memcpy(tnew, st->coast, size*sizeof(terrain_t));
					free(st->coast);
					st->coast = tnew;
					add = st->coast+size;
				} else {
					st->coast = malloc(sizeof(terrain_t) * 2);
					add = st->coast;
				}
				add[0] = NOTERRAIN;
				for (t=0;t!=MAXTERRAINS;++t) if (strcmp(tname, terrain[t].name)==0) {
					add[0] = t;
					break;
				}
				add[1] = NOTERRAIN;
			}
		} else if (strcmp(tag->name, "construction")==0) {
			construction * con = st->construction = calloc(sizeof(construction), 1);
			con->maxsize = xml_ivalue(tag, "maxsize");
			con->minskill = xml_ivalue(tag, "minskill");
			con->reqsize = xml_ivalue(tag, "reqsize");
			con->skill = sk_find(xml_value(tag, "skill"));
		} else if (strcmp(tag->name, "requirement")==0) {
			xml_readrequirement(tag, st->construction);
		}
	}
	return XML_OK;
}

static xml_callbacks xml_ships = {
	tagbegin, tagend, NULL
};

void
register_ships(void)
{
	xml_register(&xml_ships, "eressea ship", 0);
}

