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
#include "group.h"
#include "karma.h"
#include "border.h"
#include "item.h"
#include "movement.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"

#include <attributes/moved.h>

/* util includes */
#include <base36.h>
#include <event.h>
#include <goodies.h>
#include <resolve.h>

/* libc includes */
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define FIND_FOREIGN_TEMP


int demonfix = 0;
/* ------------------------------------------------------------- */

const unit *
u_peasants(void)
{
	static unit peasants = { NULL, NULL, NULL, NULL, NULL, 2, NULL };
	if (peasants.name==NULL) {
		peasants.name = strdup("die Bauern");
	}
	return &peasants;
}

const unit *
u_unknown(void)
{
	static unit unknown = { NULL, NULL, NULL, NULL, NULL, 1, NULL };
	if (unknown.name==NULL) {
		unknown.name =strdup("eine unbekannte Einheit");
	}
	return &unknown;
}

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
dfindhash(int no)
{
	dead * old;

	if(no < 0) return 0;

	for (old = deadhash[no % DMAXHASH]; old; old = old->nexthash)
		if (old->no == no)
			return old->f;
	return 0;
}

unit * udestroy = NULL;

#undef DESTROY
/* Einheiten werden nicht wirklich zerstört. */
void
destroy_unit(unit * u)
{
	region *r = u->region;
	boolean zombie = false;
	unit *clone;

	if (!ufindhash(u->no)) return;

	if (!fval(u->race, RCF_ILLUSIONARY)) {
		item ** p_item = &u->items;
		unit * u3;

		/* u->faction->no_units--; */ /* happens in u_setfaction now */

		if (r) for (u3 = r->units; u3; u3 = u3->next) {
			if (u3 != u && u3->faction == u->faction && playerrace(u3->race)) {
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

	/* Wir machen das erst nach dem Löschen der Items. Der Klon darf keine
	 * Items haben, sonst Memory-Leak. */
	
	clone = has_clone(u);
	if (clone && rand()%100 < 90) {
		attrib *a;
		int i;

		/* TODO: Messages generieren. */
		u->region = clone->region;
		u->ship = clone->ship;
		u->building = clone->building;
		u->hp = 1;
		i = u->no;
		uunhash(u);
		uunhash(clone);
		u->no = clone->no;
		clone->no = i;
		uhash(u);
		uhash(clone);
		set_number(u, 1);
		set_spellpoints(u, 0);
		a = a_find(u->attribs, &at_clone);
		a_remove(&u->attribs, a);
		a = a_find(clone->attribs, &at_clonemage);
		a_remove(&clone->attribs, a);
		fset(u, FL_LONGACTION);
		set_number(clone, 0);
		u = clone;
		zombie = false;
	}
		
	if (zombie) {
		u_setfaction(u, findfaction(MONSTER_FACTION));
		scale_number(u, 1);
		u->race = u->irace = new_race[RC_ZOMBIE];
	} else {
		if (u->number) set_number(u, 0);
		handle_event(&u->attribs, "destroy", u);
		if (r && rterrain(r) != T_OCEAN)
			rsetmoney(r, rmoney(r) + get_money(u));
		dhash(u->no, u->faction);
		u_setfaction(u, NULL);
		if (r) leave(r, u);
		uunhash(u);
		if (r) choplist(&r->units, u);
		u->next = udestroy;
		udestroy = u;
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
	if (alliedunit(u, u2->faction, HELP_GIVE) == HELP_GIVE)
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

int
read_unit_reference(unit ** up, FILE * F)
{
	char zId[10];
	int i;
	fscanf(F, "%s", zId);
	if (up==NULL) {
		return AT_READ_FAIL;
	}
	i = atoi36(zId);
	if (i==0) {
		*up = NULL;
		return AT_READ_FAIL;
	}
	*up = findunit(i);
	if (*up==NULL) ur_add((void*)i, (void**)up, resolve_unit);
	return AT_READ_OK;
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
get_level(const unit * u, skill_t id)
{
	skill * sv = u->skills;
	while (sv != u->skills + u->skill_size) {
		if (sv->id == id) {
			return sv->level;
		}
		++sv;
	}
	return 0;
}

void 
set_level(unit * u, skill_t sk, int value)
{
	skill * sv = u->skills;
	if (value==0) {
		remove_skill(u, sk);
		return;
	}
	while (sv != u->skills + u->skill_size) {
		if (sv->id == sk) {
			sk_set(sv, value);
			return;
		}
		++sv;
	}
	sk_set(add_skill(u, sk), value);
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

const struct race * 
urace(const struct unit * u)
{
	return u->race;
}

boolean
can_survive(const unit *u, const region *r)
{
  if (((terrain[rterrain(r)].flags & WALK_INTO)
			&& (u->race->flags & RCF_WALK)) ||
			((terrain[rterrain(r)].flags & SWIM_INTO)
			&& (u->race->flags & RCF_SWIM)) ||
			((terrain[rterrain(r)].flags & FLY_INTO)
			&& (u->race->flags & RCF_FLY))) {

		if (get_item(u, I_HORSE) && !(terrain[rterrain(r)].flags & WALK_INTO))
			return false;

		if (fval(u->race, RCF_UNDEAD) && is_cursed(r->attribs, C_HOLYGROUND, 0))
			return false;

		return true;
	}
	return false;
}

void
move_unit(unit * u, region * r, unit ** ulist)
{
	int maxhp = 0;
	assert(u && r);

	if (u->region == r) return;
	if (u->region!=NULL) maxhp = unit_max_hp(u);
	if (!ulist) ulist = (&r->units);
	if (u->region) {
#ifdef DELAYED_OFFENSE
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
	/* keine automatische hp reduzierung bei bewegung */
	/* if (maxhp>0) u->hp = u->hp * unit_max_hp(u) / maxhp; */
}

/* ist mist, aber wegen nicht skalierender attirbute notwendig: */
#include "alchemy.h"

void
transfermen(unit * u, unit * u2, int n)
{
	const attrib * a;
	int hp = u->hp;
	region * r = u->region;

	if (!n) return;

	/* "hat attackiert"-status wird übergeben */

	if (u2) {
		skill *sv, *sn;
		skill_t sk;
		assert(u2->number+n>0);

		if (demonfix && u2->race==new_race[RC_DAEMON]) fset(u2, UFL_DEBUG);

		for (sk=0; sk!=MAXSKILLS; ++sk) {
			double dlevel = 0.0;
			int weeks, level = 0;

			sv = get_skill(u, sk);
			sn = get_skill(u2, sk);

			if (sv==NULL && sn==NULL) continue;
			if (sv && sv->level) {
				dlevel += (sv->level + 1 - sv->weeks/(sv->level+1.0)) * n;
				level += sv->level * n;
			}
			if (sn && sn->level) {
				dlevel += (sn->level + 1 - sn->weeks/(sn->level+1.0)) * u2->number;
				level += sn->level * u2->number;
			}

			dlevel = dlevel / (n + u2->number);
			level = level / (n + u2->number);
			if (level<=dlevel) {
				/* apply the remaining fraction to the number of weeks to go.
				 * subtract the according number of weeks, getting closer to the
				 * next level */
				level = (int)dlevel;
				weeks = (level+1) - (int)((dlevel - level) * (level+1));
			} else {
				/* make it harder to reach the next level. 
				 * weeks+level is the max difficulty, 1 - the fraction between
				 * level and dlevel applied to the number of weeks between this
				 * and the previous level is the added difficutly */
				level = (int)dlevel+1;
				weeks = 1 + 2 * level - (int)((1 + dlevel - level) * level);
			}
			if (level) {
				if (sn==NULL) sn = add_skill(u2, sk);
				sn->level = (unsigned char)level;
				sn->weeks = (unsigned char)weeks;
				assert(sn->weeks>0 && sn->weeks<=sn->level*2+1);
			} else if (sn) {
				remove_skill(u2, sk);
				sn = NULL;
			}
		}
		a = a_find(u->attribs, &at_effect);
		while (a) {
			effect_data * olde = (effect_data*)a->data.v;
			if (olde->value) change_effect(u2, olde->type, olde->value);
			a = a->nexttype;
		}
		if (fval(u, FL_LONGACTION)) fset(u2, FL_LONGACTION);
		if (u->attribs) {
			transfer_curse(u, u2, n);
		}
	}
	scale_number(u, u->number - n);
	if (u2) {
		set_number(u2, u2->number + n);
		hp -= u->hp;
		u2->hp += hp;
		/* TODO: Das ist schnarchlahm! und gehört ncht hierhin */
		a = a_find(u2->attribs, &at_effect);
		while (a) {
			attrib * an = a->nexttype;
			effect_data * olde = (effect_data*)a->data.v;
			int e = get_effect(u, olde->type);
			if (e!=0) change_effect(u2, olde->type, -e);
			a = an;
		}
	}
	else if (r->land)
		if(u->race != new_race[RC_DAEMON]) {
			if(u->race != new_race[RC_URUK]) {
				rsetpeasants(r, rpeasants(r) + n);
			} else {
				rsetpeasants(r, rpeasants(r) + n/2);
			}
		}
}

struct building * 
inside_building(const struct unit * u)
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
		--u->faction->no_units;
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
	if (cnt && f) {
		set_number(u, cnt);
		++f->no_units;
	}
}
/* vorsicht Sprüche können u->number == 0 (RS_FARVISION) haben! */
void
set_number(unit * u, int count)
{
	assert (count >= 0);
#ifndef NDEBUG
	assert (u->debug_number == u->number);
	assert (u->faction != 0 || u->number > 0);
#endif
	if (u->faction && u->race != u->faction->race && playerrace(u->race)
	    && old_race(u->race) != RC_SPELL && old_race(u->race) != RC_SPECIAL
			&& !(is_cursed(u->attribs, C_SLAVE, 0))){
		u->faction->num_migrants += count - u->number;
	}

	u->faction->num_people += count - u->number;
	u->number = count;
#ifndef NDEBUG
	u->debug_number = count;
#endif
}

boolean
learn_skill(unit * u, skill_t sk, double chance)
{
	skill * sv = u->skills;
	if (chance < 1.0 && rand()%10000>=chance*10000) return false;
	while (sv != u->skills + u->skill_size) {
		assert (sv->weeks>0);
		if (sv->id == sk) {
			if (sv->weeks<=1) {
				sk_set(sv, sv->level+1);
			} else {
				sv->weeks--;
			}
			return true;
		}
		++sv;
	}
	sv = add_skill(u, sk);
	sk_set(sv, 1);
	return true;
}

void
remove_skill(unit *u, skill_t sk)
{
	skill * sv = u->skills;
	for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
		if (sv->id==sk) {
			skill * sl = u->skills + u->skill_size - 1;
			if (sl!=sv) {
				*sv = *sl;
			}
			--u->skill_size;
			return;
		}
	}
}

skill * 
add_skill(unit * u, skill_t id)
{
	skill * sv = u->skills;
#ifndef NDEBUG
	for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
		assert(sv->id != id);
	}
#endif
	++u->skill_size;
	u->skills = realloc(u->skills, u->skill_size * sizeof(skill));
	sv = (u->skills + u->skill_size - 1);
	sv->level = (unsigned char)0;
	sv->weeks = (unsigned char)1;
	sv->old   = (unsigned char)0;
	sv->id    = (unsigned char)id;
	return sv;
}

skill *
get_skill(const unit * u, skill_t sk)
{
	skill * sv = u->skills;
	while (sv!=u->skills+u->skill_size) {
		if (sv->id==sk) return sv;
		++sv;
	}
	return NULL;
}

boolean
has_skill(const unit * u, skill_t sk)
{
	skill * sv = u->skills;
	while (sv!=u->skills+u->skill_size) {
		if (sv->id==sk) {
			return (sv->level>0);
		}
		++sv;
	}
	return false;
}

static int
item_modification(const unit *u, skill_t sk, int val)
{
	/* Presseausweis: *2 Spionage, 0 Tarnung */
	if(sk == SK_SPY && get_item(u, I_PRESSCARD) >= u->number) {
		val = val * 2;
	}
	if(sk == SK_STEALTH) {
#if NEWATSROI == 1
		if(get_item(u, I_RING_OF_INVISIBILITY)
				+ 100 * get_item(u, I_SPHERE_OF_INVISIBILITY) >= u->number) {
			val += ROIBONUS;
		}
#endif
		if(get_item(u, I_PRESSCARD) >= u->number) {
			val = 0;
		}
	}
#if NEWATSROI == 1
	if(sk == SK_OBSERVATION) {
		if(get_item(u, I_AMULET_OF_TRUE_SEEING) >= u->number) {
			val += ATSBONUS;
		}
	}
#endif
	return val;
}

static int
att_modification(const unit *u, skill_t sk)
{
	int bonus = 0, malus = 0;
	attrib * a;
	int result = 0;
	static boolean init = false;
	static const curse_type * skillmod_ct;
	static const curse_type * gbdream_ct;
	if (!init) { 
		init = true; 
		skillmod_ct = ct_find("skillmod"); 
		gbdream_ct = ct_find("gbdream");
	}

	result += get_curseeffect(u->attribs, C_ALLSKILLS, 0);
	if (skillmod_ct) {
		curse * c = get_cursex(u->attribs, skillmod_ct, (void*)(int)sk, cmp_cursedata);
		result += curse_geteffect(c);
	}

	/* TODO hier kann nicht mit get/iscursed gearbeitet werden, da nur der
	 * jeweils erste vom Typ C_GBDREAM zurückgegen wird, wir aber alle
	 * durchsuchen und aufaddieren müssen */
	a = a_select(u->region->attribs, gbdream_ct, cmp_cursetype);
	while (a) {
		curse * c = (curse*)a->data.v;
		int mod = curse_geteffect(c);
		unit * mage = c->magician;
		/* wir suchen jeweils den größten Bonus und den größten Malus */
		if (mod>0 && (mage==NULL || alliedunit(mage, u->faction, HELP_GUARD))) 
		{
			if (mod > bonus ) bonus = mod;
		} else if (mod < 0 && 
			(mage == NULL || !alliedunit(mage, u->faction, HELP_GUARD)))
		{
			if (mod < malus ) malus = mod;
		}
		a = a_select(a->next, gbdream_ct, cmp_cursetype);
	}
	result = result + bonus + malus;

	return result;
}

int
get_modifier(const unit * u, skill_t sk, int level, const region * r)
{
	int bskill = level;
	int skill = bskill;

	if (r->planep && sk == SK_STEALTH && fval(r->planep, PFL_NOSTEALTH)) return 0;

	assert(r);
	skill += rc_skillmod(u->race, r, sk);
	skill += att_modification(u, sk);

	skill = item_modification(u, sk, skill);
	skill = skillmod(u->attribs, u, r, sk, skill, SMF_ALWAYS);

	if (fspecial(u->faction, FS_TELEPATHY)) {
		switch(sk) {
		case SK_ALCHEMY:
		case SK_HERBALISM:
		case SK_MAGIC:
		case SK_SPY:
		case SK_STEALTH:
		case SK_OBSERVATION:
			break;
		default:
			skill -= 2;
		}
	}

#ifdef HUNGER_REDUCES_SKILL
	if (fval(u, FL_HUNGER)) {
		skill = skill/2;
	}
#endif
	return skill - bskill;
}

int
eff_skill(const unit * u, skill_t sk, const region * r)
{
	int level = get_level(u, sk);
	if (level>0) {
		int mlevel = level + get_modifier(u, sk, level, r);

		if (mlevel>0) return mlevel;
	}
	return 0;
}

int
invisible(const unit *u)
{
#if NEWATSROI == 1
	return 0;
#else
	return get_item(u, I_RING_OF_INVISIBILITY)
		+ 100 * get_item(u, I_SPHERE_OF_INVISIBILITY);

#endif
}

boolean
is_monstrous(const unit * u)
{
	return (boolean) (u->faction->no == MONSTER_FACTION || !playerrace(u->race));
}

