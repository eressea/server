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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "unit.h"

#include "building.h"
#include "faction.h"
#include "goodies.h"
#include "group.h"
#include "border.h"
#include "item.h"
#include "movement.h"
#include "race.h"
#include "region.h"
#include "ship.h"

#ifdef AT_MOVED
# include <attributes/moved.h>
#endif

/* util includes */
#include <resolve.h>
#include <base36.h>
#include <event.h>
#ifdef OLD_TRIGGER
# include <old/trigger.h>
#endif

/* libc includes */
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define FIND_FOREIGN_TEMP

/* ------------------------------------------------------------- */

const unit u_peasants = { NULL, NULL, NULL, NULL, NULL, 2, "die Bauern" };
const unit u_unknown = { NULL, NULL, NULL, NULL, NULL, 1, "eine unbekannte Einheit" };

#define DMAXHASH 8191
typedef struct dead {
	struct dead * nexthash;
	faction * f;
	int no;
} dead;

static dead* deadhash[DMAXHASH];

static void
dhash(int no, faction * f)
{
	dead * hash = (dead*)calloc(1, sizeof(dead));
	dead * old = deadhash[no % DMAXHASH];
	hash->no = no;
	hash->f = f;
	deadhash[no % DMAXHASH] = hash;
	hash->nexthash = old;
}

faction *
dfindhash(int i)
{
	dead * old;
	for (old = deadhash[i % DMAXHASH]; old; old = old->nexthash)
		if (old->no == i)
			return old->f;
	return 0;
}

static unit * udestroy = NULL;

#undef DESTROY
/* Einheiten werden nicht wirklich zerstört. */
void
destroy_unit(unit * u)
{
	region *r = u->region;
	boolean zombie = false;

	if (!ufindhash(u->no)) return;

	if (u->race != RC_ILLUSION && u->race != RC_SPELL) {
		item ** p_item = &u->items;
		unit * u3;

		u->faction->no_units--;

		if (r) for (u3 = r->units; u3; u3 = u3->next) {
			if (u3 != u && u3->faction == u->faction && !race[u3->race].nonplayer) {
				i_merge(&u3->items, &u->items);
				u->items = NULL;
				break;
			}
		}
		u3 = NULL;
		while (*p_item) {
			item * item = *p_item;
			if (item->number && item->type->flags & ITF_NOTLOST) {
				if (u3==NULL) {
					u3 = r->units;
					while (u3 && u3!=u) u3 = u3->next;
					if (!u3) {
						zombie = true;
						break;
					}
				}
				if (u3) {
					i_add(&u3->items, i_remove(p_item, item));
				}
			}
			if (*p_item == item) p_item=&item->next;
		}
	}
	if (zombie) {
		u_setfaction(u, findfaction(MONSTER_FACTION));
		scale_number(u, 1);
		u->race = u->irace = RC_ZOMBIE;
	} else {
		if (r && rterrain(r) != T_OCEAN)
			rsetmoney(r, rmoney(r) + get_money(u));
		dhash(u->no, u->faction);
		u_setfaction(u, NULL);
		if (r) leave(r, u);
		uunhash(u);
		if (r) choplist(&r->units, u);
		u->next = udestroy;
		udestroy = u;
#ifdef OLD_TRIGGER
		do_trigger(u, TYP_UNIT, TR_DESTRUCT);
#endif
		if (u->number) set_number(u, 0);
#ifdef NEW_TRIGGER
		handle_event(&u->attribs, "destroy", u);
#endif
	}
}

unit *
findnewunit (const region * r, const faction *f, int n)
{
	unit *u2;

	if (n == 0)
		return 0;

	for (u2 = r->units; u2; u2 = u2->next)
		if (u2->faction == f && ualias(u2) == n)
			return u2;
#ifdef FIND_FOREIGN_TEMP
	for (u2 = r->units; u2; u2 = u2->next)
		if (ualias(u2) == n)
			return u2;
#endif
	return 0;
}

/* ------------------------------------------------------------- */


/*********************/
/*   at_alias   */
/*********************/
attrib_type at_alias = {
	"alias",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

int
ualias(const unit * u) {
	attrib * a = a_find(u->attribs, &at_alias);
	if (!a) return 0;
	return a->data.i;
}

/*********************/
/*   at_private   */
/*********************/
attrib_type at_private = {
	"private",
	DEFAULT_INIT,
	a_finalizestring,
	DEFAULT_AGE,
	a_writestring,
	a_readstring
};

const char *
uprivate(const unit * u) {
	attrib * a = a_find(u->attribs, &at_private);
	if (!a) return NULL;
	return (const char*)a->data.v;
}

void
usetprivate(unit * u, const char * str) {
	attrib * a = a_find(u->attribs, &at_private);

	if(str == NULL) {
		if(a) a_remove(&u->attribs, a);
		return;
	}
	if (!a) a = a_add(&u->attribs, a_new(&at_private));
	if (a->data.v) free(a->data.v);
	a->data.v = strdup(str);
}

/*********************/
/*   at_potion       */
/*********************/
/* Einheit BESITZT einen Trank */
attrib_type at_potion = {
	"potion",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
#if RELEASE_VERSION<ATTRIBFIX_VERSION
	DEFAULT_WRITE,
#else
	NO_WRITE,
#endif
	DEFAULT_READ
};

/*********************/
/*   at_potionuser   */
/*********************/
/* Einheit BENUTZT einen Trank */
attrib_type at_potionuser = {
	"potionuser",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

void
usetpotionuse(unit * u, const potion_type * ptype)
{
	attrib * a = a_find(u->attribs, &at_potionuser);
	if (!a) a = a_add(&u->attribs, a_new(&at_potionuser));
	a->data.v = (void*)ptype;
}

const potion_type *
ugetpotionuse(const unit * u) {
	attrib * a = a_find(u->attribs, &at_potionuser);
	if (!a) return NULL;
	return (const potion_type *)a->data.v;
}

/*********************/
/*   at_target   */
/*********************/
attrib_type at_target = {
	"target",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

unit *
utarget(const unit * u) {
	attrib * a;
	if (!fval(u, FL_TARGET)) return NULL;
	a = a_find(u->attribs, &at_target);
	assert (a || !"flag set, but no target found");
	return (unit*)a->data.v;
}

void
usettarget(unit * u, const unit * t)
{
	attrib * a = a_find(u->attribs, &at_target);
	if (!a && t) a = a_add(&u->attribs, a_new(&at_target));
	if (a) {
		if (!t) {
			a_remove(&u->attribs, a);
			freset(u, FL_TARGET);
		}
		else {
			a->data.v = (void*)t;
			fset(u, FL_TARGET);
		}
	}
}

/*********************/
/*   at_siege   */
/*********************/

void
a_writesiege(const attrib * a, FILE * f)
{
	struct building * b = (struct building*)a->data.v;
	write_building_reference(b, f);
}

int
a_readsiege(attrib * a, FILE * f)
{
	return read_building_reference((struct building**)&a->data.v, f);
}

attrib_type at_siege = {
	"siege",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	a_writesiege,
	a_readsiege
};

struct building *
usiege(const unit * u) {
	attrib * a;
	if (!fval(u, FL_SIEGE)) return NULL;
	a = a_find(u->attribs, &at_siege);
	assert (a || !"flag set, but no siege found");
	return (struct building *)a->data.v;
}

void
usetsiege(unit * u, const struct building * t)
{
	attrib * a = a_find(u->attribs, &at_siege);
	if (!a && t) a = a_add(&u->attribs, a_new(&at_siege));
	if (a) {
		if (!t) {
			a_remove(&u->attribs, a);
			freset(u, FL_SIEGE);
		}
		else {
			a->data.v = (void*)t;
			fset(u, FL_SIEGE);
		}
	}
}

/*********************/
/*   at_contact   */
/*********************/
attrib_type at_contact = {
	"contact",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

void
usetcontact(unit * u, const unit * u2)
{
	attrib * a = a_find(u->attribs, &at_contact);
	while (a && a->data.v!=u2) a = a->nexttype;
	if (a) return;
	a_add(&u->attribs, a_new(&at_contact))->data.v = (void*)u2;
}

boolean
ucontact(const unit * u, const unit * u2)
/* Prüft, ob u den Kontaktiere-Befehl zu u2 gesetzt hat. */
{
	attrib *ru;

	/* Alliierte kontaktieren immer */
	if (allied(u, u2->faction, HELP_GIVE) == HELP_GIVE)
		return true;

	/* Explizites KONTAKTIERE */
	for (ru = a_find(u->attribs, &at_contact); ru; ru = ru->nexttype)
		if (((unit*)ru->data.v) == u2)
			return true;

	return false;
}

void *
resolve_unit(void * id)
{
   return ufindhash((int)id);
}


/***
 ** init & cleanup module
 **/

void
free_units(void)
{
	while (udestroy) {
		unit * u = udestroy;
		udestroy = udestroy->next;
		stripunit(u);
		free(u);
	}
}

void
write_unit_reference(const unit * u, FILE * F)
{
	fprintf(F, "%s ", u?itoa36(u->no):"0");
}

void
read_unit_reference(unit ** up, FILE * F)
{
	char zId[10];
	int i;
	fscanf(F, "%s", zId);
	i = atoi36(zId);
	if (i==0) *up = NULL;
	{
		*up = findunit(i);
		if (*up==NULL) ur_add((void*)i, (void**)up, resolve_unit);
	}
}

attrib_type at_stealth = {
	"stealth", NULL, NULL, NULL, DEFAULT_WRITE, DEFAULT_READ
};

void
u_seteffstealth(unit * u, int value)
{
	attrib * a = a_find(u->attribs, &at_stealth);
	if (!a && value<0) return;
	if (!a) a = a_add(&u->attribs, a_new(&at_stealth));
	a->data.i = value;
}

int
u_geteffstealth(const struct unit * u)
{
	attrib * a = a_find(u->attribs, &at_stealth);
	return (a?a->data.i:-1);
}

int
change_skill (unit * u, skill_t id, int byvalue)
{
	skillvalue *i = u->skills;
	int wounds = 0;

	if (id == SK_AUSDAUER) {
		wounds = unit_max_hp(u)*u->number - u->hp;
	}
	for (; i != u->skills + u->skill_size; ++i)
		if (i->id == id) {
			i->value = max(0, i->value + byvalue);
			if (id == SK_AUSDAUER) {
				u->hp = unit_max_hp(u)*u->number - wounds;
			}
			return i->value;
		}

	set_skill(u, id, byvalue);

	if (id == SK_AUSDAUER) {
		u->hp = unit_max_hp(u)*u->number - wounds;
	}
	return byvalue;
}

int
change_skill_transfermen (unit * u, skill_t id, int byvalue)
{
	skillvalue *i = u->skills;

	for (; i != u->skills + u->skill_size; ++i)
		if (i->id == id) {
			i->value = max(0, i->value + byvalue);
			return i->value;
		}

	set_skill(u, id, byvalue);

	return byvalue;
}

void
set_skill (unit * u, skill_t id, int value)
{
	skillvalue *i = u->skills;

	assert(value>=0);
	for (; i != u->skills + u->skill_size; ++i)
		if (i->id == id) {
			if (value)
				i->value = value;
			else {
				*i = *(u->skills + u->skill_size - 1);
				--u->skill_size;
			}
			return;
		}
	if (!value)
		return;
	++u->skill_size;
	u->skills = realloc(u->skills, u->skill_size * sizeof(skillvalue));
	(u->skills + u->skill_size - 1)->value = value;
	(u->skills + u->skill_size - 1)->id = id;
}

static attrib_type at_leftship = {
	"leftship",
};

static attrib *
make_leftship(struct ship * leftship)
{
	attrib * a = a_new(&at_leftship);
	a->data.v = leftship;
	return a;
}

void
set_leftship(unit *u, ship *sh)
{
	a_add(&u->attribs, make_leftship(sh));
}

ship *
leftship(const unit *u)
{
	attrib * a = a_find(u->attribs, &at_leftship);

	/* Achtung: Es ist nicht garantiert, daß der Rückgabewert zu jedem
	 * Zeitpunkt noch auf ein existierendes Schiff zeigt! */

	if (a) return (ship *)(a->data.v);

	return NULL;
}

void
leave_ship(unit * u)
{
	struct ship * sh = u->ship;
	if (sh==NULL) return;
	u->ship = NULL;
	set_leftship(u, sh);

	if (fval(u, FL_OWNER)) {
		unit *u2, *owner = NULL;
		freset(u, FL_OWNER);

		for (u2 = u->region->units; u2; u2 = u2->next) {
			if (u2->ship == sh) {
				if (u2->faction == u->faction) {
					owner = u2;
					break;
				} 
				else if (owner==NULL) owner = u2;
			}
		}
		if (owner!=NULL) fset(owner, FL_OWNER);
	}
}

void
leave_building(unit * u)
{
	struct building * b = u->building;
	if (!b) return;
	u->building = NULL;

	if (fval(u, FL_OWNER)) {
		unit *u2, *owner = NULL;
		freset(u, FL_OWNER);

		for (u2 = u->region->units; u2; u2 = u2->next) {
			if (u2->building == b) {
				if (u2->faction == u->faction) {
					owner = u2;
					break;
				} 
				else if (owner==NULL) owner = u2;
			}
		}
		if (owner!=NULL) fset(owner, FL_OWNER);
	}
}

void
leave(struct region * r, unit * u)
{
	if (u->building) leave_building(u);
	else if (u->ship) leave_ship(u);
	unused(r);
}

const struct race_type * 
urace(const struct unit * u)
{
	return &race[u->race];
}

boolean
can_survive(const unit *u, const region *r)
{
  if (((terrain[rterrain(r)].flags & WALK_INTO)
			&& (race[u->race].flags & RCF_WALK)) ||
			((terrain[rterrain(r)].flags & SWIM_INTO)
			&& (race[u->race].flags & RCF_SWIM)) ||
			((terrain[rterrain(r)].flags & FLY_INTO)
			&& (race[u->race].flags & RCF_FLY))) {

		if (get_item(u, I_HORSE) && !(terrain[rterrain(r)].flags & WALK_INTO))
			return false;

		if(is_undead(u) && is_cursed(r->attribs, C_HOLYGROUND, 0))
			return false;

		return true;
	}
	return false;
}

void
move_unit(unit * u, region * r, unit ** ulist)
{
	assert(u && r);

	if (u->region == r) return;
	if (!ulist) ulist = (&r->units);
	if (u->region) {
#ifdef AT_MOVED
		set_moved(&u->attribs);
#endif
		setguard(u, GUARD_NONE);
		fset(u, FL_MOVED);
		if (u->ship || u->building) leave(u->region, u);
		translist(&u->region->units, ulist, u);
	} else
		addlist(ulist, u);

	u->faction->first = 0;
	u->faction->last = 0;
	u->region = r;
}

/* ist mist, aber wegen nicht skalierender attirbute notwendig: */
#include "alchemy.h"

void
transfermen(unit * u, unit * u2, int n)
{
	skill_t sk;
	const attrib * a;
	int skills[MAXSKILLS];
	int hp = 0;
	region * r = u->region;

	if (!n) return;

	/* "hat attackiert"-status wird übergeben */

	if (u2) {
		if (fval(u, FL_LONGACTION)) fset(u2, FL_LONGACTION);
		hp = u->hp;
		if (u->skills)
			for (sk = 0; sk < MAXSKILLS; ++sk)
				skills[sk] = get_skill(u, sk);
		a = a_find(u->attribs, &at_effect);
		while (a) {
			effect_data * olde = (effect_data*)a->data.v;
			change_effect(u2, olde->type, olde->value);
			a = a->nexttype;
		}
		if (is_mage(u)) {
		  /* Magier sind immer 1-Personeneinheiten und können nicht
		   * übergeben werden. Sollte bereits in givemen abgefangen
		   * werden */
		}
		if (u->attribs) {
			transfer_curse(u, u2, n);
		}
	}
	scale_number(u, u->number - n);
	if (u2) {
		set_number(u2, u2->number + n);
		hp -= u->hp;
		u2->hp += hp;
		if (u->skills) {
			for (sk = 0; sk < MAXSKILLS; ++sk) {
				change_skill_transfermen(u2, sk, skills[sk] - get_skill(u, sk));
			}
		}
		/* TODO: Das ist schnarchlahm! und gehört ncht hierhin */
		a = a_find(u2->attribs, &at_effect);
		while (a) {
			effect_data * olde = (effect_data*)a->data.v;
			int e = get_effect(u, olde->type);
			if (e!=0) change_effect(u2, olde->type, -e);
			a = a->nexttype;
		}
	}
	else if (r->land)
		rsetpeasants(r, rpeasants(r) + n);
}

struct building * inside_building(const struct unit * u)
{
	if (u->building==NULL) return NULL;

	if (!fval(u->building, BLD_WORKING)) {
		/* Unterhalt nicht bezahlt */
		return NULL;
	} else if (u->building->size < u->building->type->maxsize) {
		/* Gebäude noch nicht fertig */
		return NULL;
	} else {
		int p = 0, cap = buildingcapacity(u->building);
		const unit * u2;
		for (u2 = u->region->units; u2; u2 = u2->next) {
			if (u2->building == u->building) {
				p += u2->number;
				if (u2 == u) {
					if (p <= cap) return u->building;
					return NULL;
				}
				if (p > cap) return NULL;
			}
		}	
	}
	return NULL;
}

void
u_setfaction(unit * u, faction * f)
{
	int cnt = u->number;
	if (u->faction==f) return;
#ifndef NDEBUG
	assert(u->debug_number == u->number);
#endif
	if (u->faction) {
		set_number(u, 0);
		join_group(u, NULL);
	}
	if (u->prevF) u->prevF->nextF = u->nextF;
	else if (u->faction) {
		assert(u->faction->units==u);
		u->faction->units = u->nextF;
	}
	if (u->nextF) u->nextF->prevF = u->prevF;

	if (f!=NULL) {
		u->nextF = f->units;
		f->units = u;
	}
	else u->nextF = NULL;
	if (u->nextF) u->nextF->prevF = u;
	u->prevF = NULL;

	u->faction = f;
	if (cnt && f) set_number(u, cnt);
}

/* vorsicht Sprüche können u->number == 0 (RS_FARVISION) haben! */
void
set_number (unit * u, int count)
{
	assert (count >= 0);
#ifndef NDEBUG
	assert (u->debug_number == u->number);
	assert (u->faction != 0 || u->number > 0);
#endif
	if (u->faction && u->race != u->faction->race && !nonplayer(u)
	    && u->race != RC_SPELL && u->race != RC_SPECIAL
			&& !(is_cursed(u->attribs, C_SLAVE, 0))){
		u->faction->num_migrants += count - u->number;
	}

	u->faction->num_people += count - u->number;
	u->number = count;
#ifndef NDEBUG
	u->debug_number = count;
#endif
}
