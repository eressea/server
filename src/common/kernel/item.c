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

/* Neue items in common/items/ anlegen!
 */

#include <config.h>
#include "eressea.h"
#include "item.h"

#include "alchemy.h"
#include "build.h"
#include "faction.h"
#include "message.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "spell.h"
#include "save.h"
#include "skill.h"
#include "unit.h"

/* util includes */
#include <message.h>
#include <functions.h>
#include <goodies.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

resource_type * resourcetypes;
weapon_type * weapontypes;
luxury_type * luxurytypes;
potion_type * potiontypes;
item_type * itemtypes;
herb_type * herbtypes;

#ifdef AT_PTYPE
static attrib_type at_ptype = { "potion_type" };
#endif
#ifdef AT_WTYPE
static attrib_type at_wtype = { "weapon_type" };
#endif
#ifdef AT_LTYPE
static attrib_type at_ltype = { "luxury_type" };
#endif
#ifdef AT_ITYPE
static attrib_type at_itype = { "item_type" };
#endif
#ifdef AT_HTYPE
static attrib_type at_htype = { "herb_type" };
#endif

static int
res_changeaura(unit * u, const resource_type * rtype, int delta)
{
	assert(rtype!=NULL);
	change_spellpoints(u, delta);
	return 0;
}

static int
res_changeperson(unit * u, const resource_type * rtype, int delta)
{
	assert(rtype!=NULL || !"not implemented");
	scale_number(u, u->number+delta);
	return 0;
}

static int
res_changepermaura(unit * u, const resource_type * rtype, int delta)
{
	assert(rtype!=NULL);
	change_maxspellpoints(u, delta);
	return 0;
}

static int
res_changehp(unit * u, const resource_type * rtype, int delta)
{
	assert(rtype!=NULL);
	u->hp+=delta;
	return 0;
}

static int
res_changepeasants(unit * u, const resource_type * rtype, int delta)
{
	assert(rtype!=NULL && u->region->land);
	u->region->land->peasants+=delta;
	return 0;
}

int
res_changeitem(unit * u, const resource_type * rtype, int delta)
{
	int num;
	if (rtype == oldresourcetype[R_STONE] && old_race(u->race)==RC_STONEGOLEM && delta<=0) {
		int reduce = delta / GOLEM_STONE;
		if (delta % GOLEM_STONE != 0) --reduce;
		scale_number(u, u->number+reduce);
		num = u->number;
	} else if (rtype == oldresourcetype[R_IRON] && old_race(u->race) == RC_IRONGOLEM && delta<=0) {
		int reduce = delta / GOLEM_IRON;
		if (delta % GOLEM_IRON != 0) --reduce;
		scale_number(u, u->number+reduce);
		num = u->number;
	} else {
		const item_type * itype = resource2item(rtype);
		item * i;
		assert(itype!=NULL);
		i = i_change(&u->items, itype, delta);
		if (i==NULL) return 0;
		num = i->number;
	}
	return num;
}

const char *
resourcename(const resource_type * rtype, int flags)
{
	int i = 0;

	if (rtype->name) return rtype->name(rtype, flags);

	if (flags & NMF_PLURAL) i = 1;
	if (flags & NMF_APPEARANCE && rtype->_appearance[i]) {
		return rtype->_appearance[i];
	}
	return rtype->_name[i];
}

resource_type *
new_resourcetype(const char ** names, const char ** appearances, int flags)
{
	resource_type * rtype = rt_find(names[0]);

	if (rtype==NULL) {
		int i;
		rtype = calloc(sizeof(resource_type), 1);

		for (i=0; i!=2; ++i) {
			rtype->_name[i] = strdup(names[i]);
			if (appearances) rtype->_appearance[i] = strdup(appearances[i]);
			else rtype->_appearance[i] = NULL;
		}
		rt_register(rtype);
	}
#ifndef NDEBUG
	else {
		/* TODO: check that this is the same type */
	}
#endif
	rtype->flags |= flags;
	return rtype;
}

void
it_register(item_type * itype)
{
	item_type ** p_itype = &itemtypes;
	while (*p_itype && *p_itype != itype) p_itype = &(*p_itype)->next;
	if (*p_itype==NULL) {
#ifdef AT_ITYPE
		a_add(&itype->rtype->attribs, a_new(&at_itype))->data.v = (void*) itype;
#else
		itype->rtype->itype = itype;
#endif
		*p_itype = itype;
		rt_register(itype->rtype);
	}
}

item_type *
new_itemtype(resource_type * rtype,
             int iflags, int weight, int capacity)
{
	item_type * itype;
	assert (resource2item(rtype) == NULL);

	assert(rtype->flags & RTF_ITEM);
	itype = calloc(sizeof(item_type), 1);

	itype->rtype = rtype;
	itype->weight = weight;
	itype->capacity = capacity;
	itype->flags |= iflags;
	it_register(itype);

	rtype->uchange = res_changeitem;

	return itype;
}

void
lt_register(luxury_type * ltype)
{
#ifdef AT_LTYPE
	a_add(&ltype->itype->rtype->attribs, a_new(&at_ltype))->data.v = (void*) ltype;
#else
	ltype->itype->rtype->ltype = ltype;
#endif
	ltype->next = luxurytypes;
	luxurytypes = ltype;
}

luxury_type *
new_luxurytype(item_type * itype, int price)
{
	luxury_type * ltype;

	assert(resource2luxury(itype->rtype) == NULL);
	assert(itype->flags & ITF_LUXURY);

	ltype = calloc(sizeof(luxury_type), 1);
	ltype->itype = itype;
	ltype->price = price;
	lt_register(ltype);

	return ltype;
}

void
wt_register(weapon_type * wtype)
{
#ifdef AT_WTYPE
	a_add(&wtype->itype->rtype->attribs, a_new(&at_wtype))->data.v = (void*) wtype;
#else
	wtype->itype->rtype->wtype = wtype;
#endif
	wtype->next = weapontypes;
	weapontypes = wtype;
}

weapon_type *
new_weapontype(item_type * itype,
               int wflags, double magres, const char* damage[], int offmod, int defmod, int reload, skill_t sk, int minskill)
{
	weapon_type * wtype;

	assert(resource2weapon(itype->rtype)==NULL);
	assert(itype->flags & ITF_WEAPON);

	wtype = calloc(sizeof(weapon_type), 1);
	if (damage) {
		wtype->damage[0] = strdup(damage[0]);
		wtype->damage[1] = strdup(damage[1]);
	}
	wtype->defmod = defmod;
	wtype->flags |= wflags;
	wtype->itype = itype;
	wtype->magres = magres;
	wtype->minskill = minskill;
	wtype->offmod = offmod;
	wtype->reload = reload;
	wtype->skill = sk;
	wt_register(wtype);

	return wtype;
}


void
pt_register(potion_type * ptype)
{
#ifdef AT_PTYPE
	a_add(&ptype->itype->rtype->attribs, a_new(&at_ptype))->data.v = (void*) ptype;
#else
	ptype->itype->rtype->ptype = ptype;
#endif
	ptype->next = potiontypes;
	potiontypes = ptype;
}

potion_type *
new_potiontype(item_type * itype,
               int level)
{
	potion_type * ptype;

	assert(resource2potion(itype->rtype)==NULL);
	assert(itype->flags & ITF_POTION);

	ptype = calloc(sizeof(potion_type), 1);
	ptype->itype = itype;
	ptype->level = level;
	pt_register(ptype);

	return ptype;
}


void
ht_register(herb_type * htype)
{
#ifdef AT_HTYPE
	a_add(&htype->itype->rtype->attribs, a_new(&at_htype))->data.v = (void*) htype;
#else
	htype->itype->rtype->htype = htype;
#endif
	htype->next = herbtypes;
	herbtypes = htype;
}

herb_type *
new_herbtype(item_type * itype,
             terrain_t terrain)
{
	herb_type * htype;

	assert(resource2herb(itype->rtype)==NULL);
	assert(itype->flags & ITF_HERB);

	htype = calloc(sizeof(herb_type), 1);
	htype->itype = itype;
	htype->terrain = terrain;
	ht_register(htype);

	return htype;
}

void
rt_register(resource_type * rtype)
{
	resource_type ** prtype = &resourcetypes;

	if (!rtype->hashkey)
		rtype->hashkey = hashstring(rtype->_name[0]);
	while (*prtype && *prtype!=rtype) prtype=&(*prtype)->next;
	if (*prtype == NULL) {
		*prtype = rtype;
	}
}

const resource_type *
item2resource(const item_type * itype)
{
	return itype->rtype;
}

const item_type *
resource2item(const resource_type * rtype)
{
#ifdef AT_ITYPE
	attrib * a = a_find(rtype->attribs, &at_itype);
	if (a) return (const item_type *)a->data.v;
	return NULL;
#else
	return rtype->itype;
#endif
}

const herb_type *
resource2herb(const resource_type * rtype)
{
#ifdef AT_HTYPE
	attrib * a = a_find(rtype->attribs, &at_htype);
	if (a) return (const herb_type *)a->data.v;
	return NULL;
#else
	return rtype->htype;
#endif
}

const weapon_type *
resource2weapon(const resource_type * rtype) {
#ifdef AT_WTYPE
	attrib * a = a_find(rtype->attribs, &at_wtype);
	if (a) return (const weapon_type *)a->data.v;
	return NULL;
#else
	return rtype->wtype;
#endif
}

const luxury_type *
resource2luxury(const resource_type * rtype)
{
#ifdef AT_LTYPE
	attrib * a = a_find(rtype->attribs, &at_ltype);
	if (a) return (const luxury_type *)a->data.v;
	return NULL;
#else
	return rtype->ltype;
#endif
}

const potion_type *
resource2potion(const resource_type * rtype)
{
#ifdef AT_PTYPE
	attrib * a = a_find(rtype->attribs, &at_ptype);
	if (a) return (const potion_type *)a->data.v;
	return NULL;
#else
	return rtype->ptype;
#endif
}

resource_type *
rt_find(const char * name)
{
	unsigned int hash = hashstring(name);
	resource_type * rtype;

	for (rtype=resourcetypes; rtype; rtype=rtype->next)
		if (rtype->hashkey==hash && !strcmp(rtype->_name[0], name)) break;

	return rtype;
}

static const char * it_aliases[2][2] = { { "Runenschwert", "runesword" }, { NULL, NULL } };
static const char *
it_alias(const char * zname)
{
	int i;
	for (i=0;it_aliases[i][0];++i) {
		if (strcasecmp(it_aliases[i][0], zname)==0) return it_aliases[i][1];
	}
	return zname;
}

item_type *
it_find(const char * zname)
{
	const char * name = it_alias(zname);
	unsigned int hash = hashstring(name);
	item_type * itype;

	for (itype=itemtypes; itype; itype=itype->next)
		if (itype->rtype->hashkey==hash && strcmp(itype->rtype->_name[0], name) == 0) break;
	if (!itype) for (itype=itemtypes; itype; itype=itype->next)
		if (strcmp(itype->rtype->_name[1], name) == 0) break;

	return itype;
}

luxury_type *
lt_find(const char * name)
{
	unsigned int hash = hashstring(name);
	luxury_type * ltype;

	for (ltype=luxurytypes; ltype; ltype=ltype->next)
		if (ltype->itype->rtype->hashkey==hash && !strcmp(ltype->itype->rtype->_name[0], name)) break;

	return ltype;
}

herb_type *
ht_find(const char * name)
{
	unsigned int hash = hashstring(name);
	herb_type * htype;

	for (htype=herbtypes; htype; htype=htype->next)
		if (htype->itype->rtype->hashkey==hash && !strcmp(htype->itype->rtype->_name[0], name)) break;

	return htype;
}

potion_type *
pt_find(const char * name)
{
	unsigned int hash = hashstring(name);
	potion_type * ptype;

	for (ptype=potiontypes; ptype; ptype=ptype->next)
		if (ptype->itype->rtype->hashkey==hash && !strcmp(ptype->itype->rtype->_name[0], name)) break;

	return ptype;
}

item **
i_find(item ** i, const item_type * it)
{
	while (*i && (*i)->type!=it) i = &(*i)->next;
	return i;
}


int
i_get(const item * i, const item_type * it)
{
	i = *i_find((item**)&i, it);
	if (i) return i->number;
	return 0;
}

item *
i_add(item ** pi, item * i)
{
	assert(i && i->type && !i->next);
	while (*pi) {
		int d = strcmp((*pi)->type->rtype->_name[0], i->type->rtype->_name[0]);
		if (d>=0) break;
		pi = &(*pi)->next;
	}
	if (*pi && (*pi)->type==i->type) {
		(*pi)->number += i->number;
		i_free(i);
	} else {
		i->next = *pi;
		*pi = i;
	}
	return *pi;
}

void
i_merge(item ** pi, item ** si)
{
	item * i = *si;
	while (i) {
		item * itmp;
		while (*pi) {
			int d = strcmp((*pi)->type->rtype->_name[0], i->type->rtype->_name[0]);
			if (d>=0) break;
			pi=&(*pi)->next;
		}
		if (*pi && (*pi)->type==i->type) {
			(*pi)->number += i->number;
			i_free(i_remove(&i, i));
		} else {
			itmp = i->next;
			i->next=*pi;
			*pi = i;
			i = itmp;
		}
	}
	*si=NULL;
}

item *
i_change(item ** pi, const item_type * itype, int delta)
{
	assert(itype);
	while (*pi) {
		int d = strcmp((*pi)->type->rtype->_name[0], itype->rtype->_name[0]);
		if (d>=0) break;
		pi=&(*pi)->next;
	}
	if (!*pi || (*pi)->type!=itype) {
		item * i;
		if (delta==0) return NULL;
		i = i_new(itype, delta);
		i->next = *pi;
		*pi = i;
	} else {
		item * i = *pi;
		i->number+=delta;
		assert(i->number >= 0);
		if (i->number==0) {
			*pi = i->next;
			free(i);
			return NULL;
		}
	}
	return *pi;
}

item *
i_remove(item ** pi, item * i)
{
	assert(i);
	while ((*pi)->type!=i->type) pi = &(*pi)->next;
	assert(*pi);
	*pi = i->next;
	i->next = NULL;
	return i;
}

void
i_free(item * i) {
	assert(!i->next);
	free(i);
}

void
i_freeall(item *i) {
	item *in;

	while(i) {
		in = i->next;
		free(i);
		i = in;
	}
}


item *
i_new(const item_type * itype, int size)
{
	item * i = calloc(1, sizeof(item));
	assert(itype);
	i->type = itype;
	i->number = size;
	return i;
}

#include "region.h"

static boolean
give_horses(const unit * s, const unit * d, const item_type * itype, int n, const char * cmd)
{
	if (d==NULL && itype == olditemtype[I_HORSE])
		rsethorses(s->region, rhorses(s->region) + n);
	return true;
}

#define R_MAXUNITRESOURCE R_HITPOINTS
#define R_MINOTHER R_SILVER
#define R_MINHERB R_PLAIN_1
#define R_MINPOTION R_FAST
#define R_MINITEM R_IRON
#define MAXITEMS MAX_ITEMS
#define MAXRESOURCES MAX_RESOURCES
#define MAXHERBS MAX_HERBS
#define MAXPOTIONS MAX_POTIONS
#define MAXHERBSPERPOTION 6
#define FIRSTLUXURY     (I_BALM)
#define LASTLUXURY      (I_INCENSE +1)
#define MAXLUXURIES (LASTLUXURY - FIRSTLUXURY)

struct item_type * olditemtype[MAXITEMS+1];
resource_type * oldresourcetype[MAXRESOURCES+1];
herb_type * oldherbtype[MAXHERBS+1];
luxury_type * oldluxurytype[MAXLUXURIES+1];
potion_type * oldpotiontype[MAXPOTIONS+1];

/*** alte items ***/

int
get_item(const unit * u, item_t it)
{
	const item_type * type = olditemtype[it];
	item * i = *i_find((item**)&u->items, type);
	if (i) assert(i->number>=0);
	return i?i->number:0;
}

int
set_item(unit * u, item_t it, int value)
{
	const item_type * type = olditemtype[it];
	item * i = *i_find(&u->items, type);
	if (!i) {
		i = i_add(&u->items, i_new(type, value));
	} else {
		i->number = value;
	}
	return value;
}

int
change_item(unit * u, item_t it, int value)
{
	const item_type * type = olditemtype[it];
	item * i = *i_find(&u->items, type);
	if (!i) {
		if (!value) return 0;
		assert(value>0);
		i = i_add(&u->items, i_new(type, value));
	} else {
		i->number += value;
	}
	return i->number;
}

/*** alte herbs ***/

int
get_herb(const unit * u, herb_t h)
{
	const item_type * type = oldherbtype[h]->itype;
	item * it = *i_find((item**)&u->items, type);
	return it?it->number:0;
}

int
set_herb(unit * u, herb_t h, int value)
{
	const item_type * type = oldherbtype[h]->itype;
	item * i = *i_find((item**)&u->items, type);
	if (!i) i = i_add(&u->items, i_new(type, value));
	else i->number = value;
	return value;
}

int
change_herb(unit * u, herb_t h, int value)
{
	const item_type * type = oldherbtype[h]->itype;
	item * i = *i_find(&u->items, type);
	if (!i) {
		if (!value) return 0;
		assert(value>0);
		i = i_add(&u->items, i_new(type, value));
	} else {
		i->number += value;
	}
	return i->number;
}

/*** alte potions ***/

int
get_potion(const unit * u, potion_t p)
{
	const item_type * type = oldpotiontype[p]->itype;
	item * it = *i_find((item**)&u->items, type);
	return it?it->number:0;
}

void use_birthdayamulet(region * r, unit * magician, int amount, strlist * cmdstrings);

enum {
	IS_RESOURCE,
	IS_PRODUCT,
	IS_LUXURY,
	IS_MAGIC,
 /* wird für ZEIGE gebraucht */
	IS_ITEM,
	IS_RACE
};

/* t_item::flags */
#define FL_ITEM_CURSED  (1<<0)
#define FL_ITEM_NOTLOST (1<<1)
#define FL_ITEM_NOTINBAG	(1<<2)	/* nicht im Bag Of Holding */
#define FL_ITEM_ANIMAL	(1<<3)	/* ist ein Tier */
#define FL_ITEM_MOUNT	((1<<4) | FL_ITEM_ANIMAL)	/* ist ein Reittier */

#if 0
/* ------------------------------------------------------------- */
/*   Sprüche auf Artefakten                                      */
/* Benutzung magischer Gegenstände                               */
/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */
/* Antimagie - curse auflösen  - für Kristall */
/* ------------------------------------------------------------- */
static int
destroy_curse_crystal(attrib **alist, int cast_level, int force)
{
	attrib ** ap = alist;

	while (*ap && force > 0) {
		curse * c;
		attrib * a = *ap;
		if (!fval(a->type, ATF_CURSE)) {
			do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
			continue;
		}
		c = (curse*)a->data.v;

		/* Immunität prüfen */
		if (c->flag & CURSE_IMMUN);

		if (cast_level < c->vigour) { /* Zauber ist nicht stark genug */
			int chance;
			/* pro Stufe Unterschied -20% */
			chance = (10 + (cast_level - c->vigour)*2);
			if(rand()%10 >= chance){
				if (c->type->change_vigour)
					c->type->change_vigour(c, -2);
				force -= c->vigour;
				c->vigour -= 2;
				if(c->vigour <= 0) {
					a_remove(alist, a);
				}
			}
		} else { /* Zauber ist stärker als curse */
			if (force >= c->vigour){ /* reicht die Kraft noch aus? */
				force -= c->vigour;
				if (c->type->change_vigour)
					c->type->change_vigour(c, -c->vigour);
				a_remove(alist, a);
			}
		}
		if(*ap) ap = &(*ap)->next;
	}

	return force;
}
#endif

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagiern benutzt werden, erzeugt eine
 * Antimagiezone, die zwei Runden bestehen bleibt */
static void
use_antimagiccrystal(region * r, unit * mage, int amount, strlist * cmdstrings)
{
	int i;
	for (i=0;i!=amount;++i) {
		int effect, force, duration = 2;
		spell *sp = find_spellbyid(SPL_ANTIMAGICZONE);
		attrib ** ap = &r->attribs;
		unused(cmdstrings);

		/* Reduziert die Stärke jedes Spruchs um effect */
		effect = sp->level; 

		/* Hält Sprüche bis zu einem summierten Gesamtlevel von power aus.
		 * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
		 * um seine Stufe */
		force = sp->level * 20; /* Stufe 5 =~ 100 */

		/* Regionszauber auflösen */
		while (*ap && force > 0) {
			curse * c;
			attrib * a = *ap;
			if (!fval(a->type, ATF_CURSE)) {
				do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
				continue;
			}
			c = (curse*)a->data.v;

			/* Immunität prüfen */
			if (c->flag & CURSE_IMMUNE) {
				do { ap = &(*ap)->next; } while (*ap && a->type==(*ap)->type);
				continue;
			}

			force = destr_curse(c, effect, force);
			if(c->vigour <= 0) {
				a_remove(ap, a);
			}
			if(*ap) ap = &(*ap)->next;
		}

		if(force > 0) {
			create_curse(mage, &r->attribs, ct_find("antimagiczone"), force, duration, effect, 0);
		}

	}
	use_pooled(mage, mage->region, R_ANTIMAGICCRYSTAL, amount);
	add_message(&mage->faction->msgs,
			new_message(mage->faction, "use_antimagiccrystal%u:unit%r:region", mage, r));
	return;
}

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagier benutzt werden, modifiziert Taktik für diese
 * Runde um -1 - 4 Punkte. */
static void
use_tacticcrystal(region * r, unit * u, int amount, strlist * cmdstrings)
{
	int i;
	for (i=0;i!=amount;++i) {
		int effect = rand()%6 - 1;
		int duration = 1; /* wirkt nur eine Runde */
		int power = 5; /* Widerstand gegen Antimagiesprüche, ist in diesem
											Fall egal, da der curse für den Kampf gelten soll,
											der vor den Antimagiezaubern passiert */
		curse * c = create_curse(u, &u->attribs, ct_find("skillmod"), power,
			duration, effect, u->number);
		c->data = (void*)SK_TACTICS;
		unused(cmdstrings);
	}
	use_pooled(u, u->region, R_TACTICCRYSTAL, amount);
	add_message(&u->faction->msgs,
			new_message(u->faction, "use_tacticcrystal%u:unit%r:region", u, r));
	return;
}

t_item itemdata[MAXITEMS] = {
	/* name[4]; typ; sk; minskill; material[6]; gewicht; preis;
	 * benutze_funktion; */
	{			/* I_IRON */
		{"Eisen", "Eisen", "Eisen", "Eisen"}, G_N,
		IS_RESOURCE, SK_MINING, 1, {0, 0, 0, 0, 0, 0}, 500, 0, FL_ITEM_NOTLOST, NULL
	},
	{			/* I_WOOD */
		{"Holz", "Holz", "Holz", "Holz"}, G_N,
		IS_RESOURCE, SK_LUMBERJACK, 1, {0, 0, 0, 0, 0, 0}, 500, 0, FL_ITEM_NOTLOST, NULL
	},
	{			/* I_STONE */
		{"Stein", "Steine", "Stein", "Steine"}, G_M,
		IS_RESOURCE, SK_QUARRYING, 1, {0, 0, 0, 0, 0, 0}, 6000, 0, FL_ITEM_NOTLOST, NULL
	},
	{			/* I_HORSE */
		{"Pferd", "Pferde", "Pferd", "Pferde"}, G_N,
		IS_RESOURCE, SK_HORSE_TRAINING, 1, {0, 0, 0, 0, 0, 0}, 5000, 0, FL_ITEM_MOUNT | FL_ITEM_ANIMAL | FL_ITEM_NOTINBAG, NULL
	},
	{			/* I_WAGON */
		{"Wagen", "Wagen", "Wagen", "Wagen"}, G_M,
		IS_PRODUCT, SK_CARTMAKER, 1, {0, 5, 0, 0, 0, 0}, 4000, 0, FL_ITEM_NOTINBAG, NULL
	},
	{			/* I_CATAPULT */
		{"Katapult", "Katapulte", "Katapult", "Katapulte"}, G_N,
		IS_PRODUCT, SK_CARTMAKER, 5, {0, 10, 0, 0, 0, 0}, 10000, 0, FL_ITEM_NOTINBAG, NULL
	},
	{			/* I_SWORD */
		{"Schwert", "Schwerter", "Schwert", "Schwerter"}, G_N,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {1, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_SPEAR */
		{"Speer", "Speere", "Speer", "Speere"}, G_M,
		IS_PRODUCT, SK_WEAPONSMITH, 2, {0, 1, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_CROSSBOW */
		{"Armbrust", "Armbrüste", "Armbrust", "Armbrüste"}, G_F,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {0, 1, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_LONGBOW */
		{"Bogen", "Bögen", "Bogen", "Bögen"}, G_M,
		IS_PRODUCT, SK_WEAPONSMITH, 2, {0, 1, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_CHAIN_MAIL */
		{"Kettenhemd", "Kettenhemden", "Kettenhemd", "Kettenhemden"}, G_N,
		IS_PRODUCT, SK_ARMORER, 3, {3, 0, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_PLATE_ARMOR */
		{"Plattenpanzer", "Plattenpanzer", "Plattenpanzer", "Plattenpanzer"}, G_M,
		IS_PRODUCT, SK_ARMORER, 4, {5, 0, 0, 0, 0, 0}, 400, 0, 0, NULL
	},
	{			/* I_BALM */
		{"Balsam", "Balsam", "Balsam", "Balsam"}, G_N,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 200, 4, 0, NULL
	},
	{			/* I_SPICES */
		{"Gewürz", "Gewürz", "Gewürz", "Gewürz"}, G_N,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 200, 5, 0, NULL
	},
	{			/* I_JEWELERY */
		{"Juwel", "Juwelen", "Juwel", "Juwelen"}, G_N,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 7, 0, NULL
	},
	{			/* I_MYRRH */
		{"Myrrhe", "Myrrhe", "Myrrhe", "Myrrhe"}, G_F,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 200, 5, 0, NULL
	},
	{			/* I_OIL */
		{"Öl", "Öl", "Öl", "Öl"}, G_N,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 300, 3, 0, NULL
	},
	{			/* I_SILK */
		{"Seide", "Seide", "Seide", "Seide"}, G_F,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 300, 6, 0, NULL
	},
	{			/* I_INCENSE */
		{"Weihrauch", "Weihrauch", "Weihrauch", "Weihrauch"}, G_M,
		IS_LUXURY, 0, 0, {0, 0, 0, 0, 0, 0}, 200, 4, 0, NULL
	},
#ifdef COMPATIBILITY
	{			/* I_AMULET_OF_DARKNESS */
		{"Amulett der Dunkelheit", "Amulette der Dunkelheit", "Amulett", "Amulette"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_AMULET_OF_DEATH */
		{"Amulett des Todes", "Amulette des Todes", "Amulett", "Amulette"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
#endif
	{			/* I_AMULET_OF_HEALING */
		{"Amulett der Heilung", "Amulette der Heilung", "Amulett", "Amulette"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_AMULET_OF_TRUE_SEEING 22 */
		{"Amulett des wahren Sehens", "Amulette des wahren Sehens", "Amulett",
		"Amulette"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
#ifdef COMPATIBILITY
	{			/* I_CLOAK_OF_INVULNERABILITY 23 */
		{"Mantel der Unverletzlichkeit", "Mäntel der Unverletzlichkeit",
			"Kettenhemd", "Kettenhemden"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
#endif
	{			/* I_RING_OF_INVISIBILITY 24 */
		{"Ring der Unsichtbarkeit", "Ringe der Unsichtbarkeit", "", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_RING_OF_POWER 25 */
		{"Ring der Macht", "Ringe der Macht", "", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_FIRESWORD */
		{"Flammenschwert", "Flammenschwerter",
		 "Flammenschwert", "Flammenschwerter"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
#ifdef COMPATIBILITY
	{			/* I_SHIELDSTONE 27 */
		{"Schildstein", "Schildsteine", "Amulett", "Amulette"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_STAFF_OF_FIRE 28 */
		{"Zauberstab des Feuers", "Zauberstäbe des Feuers", "Zauberstab",
		"Zauberstäbe"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_STAFF_OF_LIGHTNING 29 */
		{"Zauberstab der Blitze", "Zauberstäbe der Blitze", "Zauberstab",
		"Zauberstäbe"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_WAND_OF_TELEPORTATION 30 */
		{"Zauberstab der Teleportation", "Zauberstäbe der Teleportation",
			"Zauberstab", "Zauberstäbe"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_EYE_OF_HORAX 31 */
		{"Kristallauge des Einhorns", "Kristallaugen des Einhorns",
			"Kristallauge des Einhorns", "Kristallaugen des Einhorns"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_TELEPORTCRYSTAL 32 */
		{"Kristall der Weiten Reise", "Kristalle der Weiten Reise",
			"Kristall der Weiten Reise", "Kristalle der Weiten Reise"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
#endif
	{			/* I_DRAGONHEAD 33 */
		{"Drachenkopf", "Drachenköpfe", "Drachenkopf", "Drachenköpfe"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 500, 0, 0, NULL
	},
	{			/* I_CHASTITY_BELT 34 */
		{"Amulett der Keuschheit", "Amulette der Keuschheit",
			"Amulett", "Amulette"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_GREATSWORD 35 */
		{"Bihänder", "Bihänder", "Bihänder", "Bihänder"}, G_M,
		IS_PRODUCT, SK_WEAPONSMITH, 4, {2, 0, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_AXE 36 */
		{"Kriegsaxt", "Kriegsäxte", "Kriegsaxt", "Kriegsäxte"}, G_F,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {1, 1, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_GREATBOW 37 */
		{"Elfenbogen", "Elfenbögen", "Elfenbogen", "Elfenbögen"}, G_M,
		IS_PRODUCT, SK_WEAPONSMITH, 5, {0, 0, 0, 0, 0, 1}, 100, 0, 0, NULL
	},
	{			/* I_LAENSWORD 38 */
		{"Laenschwert", "Laenschwerter", "Laenschwert", "Laenschwerter"}, G_N,
		IS_PRODUCT, SK_WEAPONSMITH, 8, {0, 0, 0, 0, 1, 0}, 100, 0, 0, NULL
	},
	{			/* I_LAENSHIELD 39 */
		{"Laenschild", "Laenschilde", "Laenschild", "Laenschilde"}, G_N,
		IS_PRODUCT, SK_ARMORER, 7, {0, 0, 0, 0, 1, 0}, 0, 0, 0, NULL
	},
	{			/* I_LAENCHAIN 40 */
		{"Laenkettenhemd", "Laenkettenhemden", "Laenkettenhemd", "Laenkettenhemden"}, G_N,
		IS_PRODUCT, SK_ARMORER, 9, {0, 0, 0, 0, 3, 0}, 100, 0, 0, NULL
	},
	{			/* I_LAEN 41 */
		{"Laen", "Laen", "Laen", "Laen"}, G_N,
		IS_RESOURCE, SK_MINING, 7, {0, 0, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_SHIELD 42 */
		{"Schild", "Schilde", "Schild", "Schilde"}, G_N,
		IS_PRODUCT, SK_ARMORER, 2, {1, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_HALBERD 43 */
		{"Hellebarde", "Hellebarden", "Hellebarde", "Hellebarden"}, G_F,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {1, 1, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_LANCE 44 */
		{"Lanze", "Lanzen", "Lanze", "Lanzen"}, G_F,
		IS_PRODUCT, SK_WEAPONSMITH, 2, {0, 2, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_MALLORN 45 */
		{"Mallorn", "Mallorn", "Mallorn", "Mallorn"}, G_N,
		IS_RESOURCE, SK_LUMBERJACK, 2, {0, 0, 0, 0, 0, 0}, 500, 0, 0, NULL
	},
	{			/* I_KEKS 46 *//* Item für Questenzwecke */
		{"Keks", "Kekse", "Keks", "Kekse"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_APFEL 47 *//* Item für Questenzwecke */
		{"Apfel", "Äpfel", "Apfel", "Äpfel"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_NUSS 48 *//* Item für Questenzwecke */
		{"Nuß", "Nüsse", "Nuß", "Nüsse"}, G_F,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_MANDELKERN 49 *//* Item für Questenzwecke */
		{"Mandelkern", "Mandelkerne", "Mandelkern", "Mandelkerne"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
#ifdef COMPATIBILITY
	{			/* I_STAB_DES_SCHICKSALS 53 */
		{"Stab des Schicksals", "Stäbe des Schicksals",
			"Stab des Schicksals", "Stäbe des Schicksals"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_STAB_DER_VERDAMMNIS 54 */
		{"Stab der Verdammnis", "Stäbe der Verdammnis",
			"Stab der Verdammnis", "Stäbe der Verdammnis"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_STAB_DES_TODES 55 */
		{"Stab des Todes", "Stäbe des Todes",
			"Stab des Todes", "Stäbe des Todes"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_STAB_DES_CHAOS 56 */
		{"Stab des Chaos", "Stäbe des Chaos",
			"Stab des Chaos", "Stäbe des Chaos"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_STECKEN_DER_MACHT 57 */
		{"Stecken der Macht", "Stecken der Macht",
			"Stecken der Macht", "Stecken der Macht"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
#endif
	{			/* I_AMULETT_DES_TREFFENS 58 */
		{"Amulett des Treffens", "Amulette des Treffens",
			"Amulett des Treffens", "Amulette des Treffens"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_DRACHENBLUT 59 */
		{"Drachenblut", "Drachenblut", "Drachenblut", "Drachenblut"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_FEENSTIEFEL 60 */
		{"Feenstiefel", "Feenstiefel", "Feenstiefel", "Feenstiefel"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_BIRTHDAYAMULET 69 */
		{"Katzenamulett", "Katzenamulette", "Amulett", "Amulette"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, &use_birthdayamulet
	},
	{			/* I_PEGASUS 60 */
		{"Pegasus", "Pegasi", "Pegasus", "Pegasi" }, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 5000, 0, FL_ITEM_ANIMAL | FL_ITEM_NOTINBAG, NULL
	},
	{			/* I_UNICORN 61 */
		{"Elfenpferd", "Elfenpferde", "Elfenpferd", "Elfenpferde"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 5000, 0, FL_ITEM_ANIMAL | FL_ITEM_NOTINBAG, NULL
	},
	{			/* I_DOLPHIN 62 */
		{"Delphin", "Delphine", "Delphin", "Delphine"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 5000, 0, FL_ITEM_ANIMAL | FL_ITEM_NOTINBAG, NULL
	},
	{			/* I_ANTIMAGICCRYSTAL 63 */
		{"Antimagiekristall", "Antimagiekristalle", "Amulett", "Amulette"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, &use_antimagiccrystal
	},
	{			/* I_RING_OF_NIMBLEFINGER 64 */
		{"Ring der flinken Finger", "Ringe der flinken Finger", "", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_TROLLBELT 65 */
		{"Gürtel der Trollstärke", "Gürtel der Trollstärke", "", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, NULL
	},
	{			/* I_PRESSCARD 67 */
		{"Akkredition des Xontormia-Expreß", "Akkreditionen des Xontormia-Expreß",
		 "Akkredition des Xontormia-Expreß", "Akkreditionen des Xontormia-Expreß"}, G_F,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, FL_ITEM_CURSED, NULL
	},
	{			/* I_RUNESWORD 68 */
		{"Runenschwert", "Runenschwerter", "Runenschwert", "Runenschwerter"}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_AURAKULUM 69 */
		{"Aurafocus", "Aurafocuse", "Amulett", "Amulette"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_SEASERPENTHEAD 70 */
		{"Seeschlangenkopf", "Seeschlangenköpfe",
			"Seeschlangenkopf", "Seeschlangenköpfe"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 500, 0, 0, NULL
	},
	{			/* I_TACTICCRYSTAL 71 */
		{"Traumauge", "Traumaugen",
			"", ""}, G_N,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, &use_tacticcrystal
	},
	{			/* I_RING_OF_REGENERATION 72 */
		{"Ring der Regeneration", "Ringe der Regeneration",
			"", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_TOADSLIME 73 */
		{"Tiegel mit Krötenschleim", "Tiegel mit Krötenschleim",
			"", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{
		{"Zauberbeutel", "Zauberbeutel", "Zauberbeutel", "Zauberbeutel"}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, FL_ITEM_NOTINBAG, NULL
	},
	{	/* I_RUSTY_SWORD */
		{"Schartiges Schwert", "Schartige Schwerter", "Schartiges Schwert", "Schartige Schwerter"}, G_N,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {1, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{	/* I_RUSTY_SHIELD 42 */
		{"Rostiges Schild", "Rostige Schilde", "Rostiges Schild", "Rostige Schilde"}, G_N,
		IS_PRODUCT, SK_ARMORER, 2, {1, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_RUSTY_CHAIN_MAIL */
		{"Rostiges Kettenhemd", "Rostige Kettenhemden", "Rostiges Kettenhemd", "Rostige Kettenhemden"}, G_N,
		IS_PRODUCT, SK_ARMORER, 3, {3, 0, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_SACK_OF_CONSERVATION */
		{"Magischer Kräuterbeutel", "Magische Kräuterbeutel", "", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_SPHERE_OF_INVISIBILITY */
		{"Sphäre der Unsichtbarkeit", "Sphären der Unsichtbarkeit", "", ""}, G_M,
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
	},
	{			/* I_RUSTY_GREATSWORD */
		{"Rostiger Zweihänder", "Rostige Zweihänder", "Rostiger Zweihänder", "Rostige Zweihänder"}, G_M,
		IS_PRODUCT, SK_WEAPONSMITH, 4, {2, 0, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_RUSTY_AXE */
		{"Rostige Kriegsaxt", "Rostige Kriegsäxte", "Rostige Kriegsaxt", "Rostige Kriegsäxte"}, G_F,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {1, 1, 0, 0, 0, 0}, 200, 0, 0, NULL
	},
	{			/* I_RUSTY_HALBERD */
		{"Rostige Hellebarde", "Rostige Hellebarden", "Rostige Hellebarde", "Rostige Hellebarden"}, G_F,
		IS_PRODUCT, SK_WEAPONSMITH, 3, {1, 1, 0, 0, 0, 0}, 200, 0, 0, NULL
	}
};

const item_t matresource[] = {
	I_IRON,
	I_WOOD,
	I_STONE,
	-1,
	I_LAEN,
	I_MALLORN
};

#include "movement.h"

static int
mod_elves_only(const unit * u, const region * r, skill_t sk, int value)
{
	if (old_race(u->race) == RC_ELF) return value;
	unused(r);
	return -118;
}

typedef int material_t;
enum {	/* Vorsicht! Reihenfolge der ersten 3 muß wie I_IRON ... sein! */
	M_EISEN,
	M_HOLZ,
	M_STEIN,
	M_SILBER,
	M_EOG,
	M_MALLORN,
	M_MAX_MAT,
	NOMATERIAL = (material_t) - 1
};

static int
limit_oldtypes(const region * r, const resource_type * rtype)
		/* TODO: split into seperate functions. really much nicer. */
{
	if (rtype==oldresourcetype[R_WOOD]) {
#if GROWING_TREES
		return rtrees(r,2) + rtrees(r,1);
#else
		return rtrees(r);
#endif
#if NEW_RESOURCEGROWTH == 0
	} else if (rtype==oldresourcetype[R_EOG]) {
		return rlaen(r);
	} else if (rtype==oldresourcetype[R_IRON]) {
		return riron(r);
	} else if (rtype==oldresourcetype[R_STONE]) {
		return terrain[rterrain(r)].quarries;
#endif
	} else if (rtype==oldresourcetype[R_MALLORN]) {
#if GROWING_TREES
		return rtrees(r,2) + rtrees(r,1);
#else
		return rtrees(r);
#endif
	} else if (rtype==oldresourcetype[R_HORSE]) {
		return rhorses(r);
	} else {
		assert(!"das kann man nicht produzieren!");
	}
	return 0;
}


static void
use_oldresource(region * r, const resource_type * rtype, int norders)
		/* TODO: split into seperate functions. really much nicer. */
{
	assert(norders>0);
	if (rtype==oldresourcetype[R_WOOD] || rtype==oldresourcetype[R_MALLORN]) {
#if GROWING_TREES
		int avail_grownup = rtrees(r,2);
		int avail_young   = rtrees(r,1);
		int wcount;

		assert(norders <= avail_grownup + avail_young);

		if(norders <= avail_grownup) {
			rsettrees(r, 2, avail_grownup-norders);
			wcount = norders;
		} else {
			rsettrees(r, 2, 0);
			rsettrees(r, 1, avail_young-(norders-avail_grownup));
			wcount = norders * 3;
		}
		if(rtype==oldresourcetype[R_MALLORN]) {
			woodcounts(r, wcount);
		} else {
			woodcounts(r, wcount*2);
		}
#else
		int avail = rtrees(r);
		assert(norders <= avail);
		rsettrees(r, avail-norders);
		woodcounts(r, norders);
#endif
#if NEW_RESOURCEGROWTH == 0
	} else if (rtype==oldresourcetype[R_EOG]) {
		int avail = rlaen(r);
		assert(norders <= avail);
		rsetlaen(r, avail-norders);
	} else if (rtype==oldresourcetype[R_IRON]) {
		int avail = riron(r);
		assert(norders <= avail);
		rsetiron(r, avail-norders);
#endif
	} else if (rtype==oldresourcetype[R_HORSE]) {
		int avail = rhorses(r);
		assert(norders <= avail);
		rsethorses(r, avail-norders);
	} else if (rtype!=oldresourcetype[R_STONE]) {
		assert(!"unknown resource");
	}
}

static int
use_olditem(struct unit * user, const struct item_type * itype, int amount, const char * cmd)
{
	item_t it;
	for (it=0;it!=MAXITEMS;++it) {
		if (olditemtype[it]==itype) {
			strlist * s = makestrlist(cmd);
			itemdata[it].benutze_funktion(user->region, user, amount, s);
			return 0;
		}
	}
	return EUNUSABLE;
}

typedef const char* translate_t[5];
static translate_t translation[] = {
	{ "Delphin", "dolphin", "dolphin_p", "dolphin", "dolphin_p" },
	{ "Holz", "log", "log_p", "log", "log_p" },
	{ "Eisen", "iron", "iron_p", "iron", "iron_p" },
	{ "Drachenblut", "dragonblood", "dragonblood_p", "dragonblood", "dragonblood_p" },
	{ "Feenstiefel", "fairyboot", "fairyboot_p", "fairyboot", "fairyboot_p" },
	{ "Gürtel der Trollstärke", "trollbelt", "trollbelt_p", "trollbelt", "trollbelt_p" },
	{ "Mallorn", "mallorn", "mallorn_p", "mallorn", "mallorn_p" },
	{ "Wagen", "cart", "cart_p", "cart", "cart_p" },
	{ "Plattenpanzer", "plate", "plate_p", "plate", "plate_p" },
	{ "Trollgürtel", "trollbelt", "trollbelt_p", "trollbelt", "trollbelt_p" },
	{ "Balsam", "balm", "balm_p", "balm", "balm_p" },
	{ "Gewürz", "spice", "spice_p", "spice", "spice_p" },
	{ "Myrrhe", "myrrh", "myrrh_p", "myrrh", "myrrh_p" },
	{ "Öl", "oil", "oil_p", "oil", "oil_p" },
	{ "Seide", "silk", "silk_p", "silk", "silk_p" },
	{ "Weihrauch", "incense", "incense_p", "incense", "incense_p" },
	{ "Bihänder", "greatsword", "greatsword_p", "greatsword", "greatsword_p" },
	{ "Laen", "laen", "laen_p", "laen", "laen_p" },
	{ "Goliathwasser", "p1", "p1_p", NULL, NULL },
	{ "Wasser des Lebens", "p2", "p2_p", NULL, NULL },
	{ "Bauernblut", "p5", "p5_p", NULL, NULL },
	{ "Gehirnschmalz", "p6", "p6_p", NULL, NULL },
	{ "Nestwärme", "p8", "p8_p", NULL, NULL },
	{ "Pferdeglück", "p9", "p9_p", NULL, NULL },
	{ "Berserkerblut", "p10", "p10_p", NULL, NULL },
	{ "Bauernlieb", "p11", "p11_p", NULL, NULL },
	{ "Heiltrank", "p14", "p14_p", NULL, NULL },

	{ "Flachwurz", "h0", "h0_p", NULL, NULL },
	{ "Elfenlieb", "h5", "h5_p", NULL, NULL },
	{ "Wasserfinder", "h9", "h9_p", NULL, NULL },
	{ "Windbeutel", "h12", "h12_p", NULL, NULL },
	{ "Steinbeißer", "h15", "h15_p", NULL, NULL },
	{ NULL, NULL, NULL, NULL, NULL }
};

static void
init_olditems(void)
{
	item_t i;
	resource_type * rtype;

	const struct locale * lang = find_locale("de");
	assert(lang);

	for (i=0; i!=MAXITEMS; ++i) {
		int iflags = ITF_NONE;
		int rflags = RTF_ITEM|RTF_POOLED;
		int m, n;
		const char * name[2];
		const char * appearance[2];
		int weight = itemdata[i].gewicht;
		int capacity = 0;
		int price;
		attrib * a;
		skillmod_data * smd;
		item_type * itype;
		construction * con = calloc(sizeof(construction), 1);

		con->minskill = itemdata[i].minskill;
		con->skill = itemdata[i].skill;
		con->maxsize = -1;
		con->reqsize = 1;
		con->improvement = NULL;

		for (m=0, n=0;m!=M_MAX_MAT;++m)
			if (itemdata[i].material[m]>0) ++n;
		if (n>0) {
			con->materials = calloc(sizeof(requirement), n+1);
			for (m=0, n=0;m!=M_MAX_MAT;++m) {
				if (itemdata[i].material[m]>0) {
					con->materials[n].type = matresource[m];
					con->materials[n].number = itemdata[i].material[m];
					con->materials[n].recycle = 0.0;
					++n;
					if (m==M_EISEN) {
					}
				}
			}
		}
		if (itemdata[i].flags & FL_ITEM_CURSED) iflags |= ITF_CURSED;
		if (itemdata[i].flags & FL_ITEM_NOTLOST) iflags |= ITF_NOTLOST;
		if (itemdata[i].flags & FL_ITEM_NOTINBAG) iflags |= ITF_BIG;
		if (itemdata[i].typ == IS_LUXURY) iflags |= ITF_LUXURY;
		if (itemdata[i].flags & FL_ITEM_ANIMAL) iflags |= ITF_ANIMAL;

		name[0]=NULL;
		{
			int ci;
			for (ci=0;translation[ci][0];++ci) {
				if (!strcmp(translation[ci][0], itemdata[i].name[0])) {
					name[0] = translation[ci][1];
					name[1] = translation[ci][2];
					appearance[0] = translation[ci][3];
					appearance[1] = translation[ci][4];
				}
			}
		}
		if (name[0]==NULL) {
			name[0] = reverse_lookup(lang, itemdata[i].name[0]);
			name[1] = reverse_lookup(lang, itemdata[i].name[1]);
			appearance[0] = reverse_lookup(lang, itemdata[i].name[2]);
			appearance[1] = reverse_lookup(lang, itemdata[i].name[3]);
		}
		rtype = new_resourcetype(name, appearance, rflags);
		itype = new_itemtype(rtype, iflags, weight, capacity);

		switch (i) {
		case I_HORSE:
			itype->capacity = HORSECAPACITY;
			itype->give = give_horses;
			break;
		case I_WAGON:
			itype->capacity = WAGONCAPACITY;
			break;
		case I_BAG_OF_HOLDING:
			itype->capacity = BAGCAPACITY;
			break;
		case I_TROLLBELT:
			itype->capacity = STRENGTHCAPACITY;
			break;
		case I_GREATBOW:
			a = a_add(&con->attribs, a_new(&at_skillmod));
			smd = (skillmod_data*)a->data.v;
			smd->skill=NOSKILL;
			smd->special = mod_elves_only;
			break;
		default:
			if (itemdata[i].flags & FL_ITEM_MOUNT) itype->capacity = HORSECAPACITY;
		}

		/* itemdata::typ Analyse. IS_PRODUCT und IS_MAGIC sind so gut wie egal. */
		switch (itemdata[i].typ) {
		case IS_LUXURY:
			price = itemdata[i].preis;
			oldluxurytype[i-FIRSTLUXURY] = new_luxurytype(itype, price);
			break;
		case IS_RESOURCE:
			rtype->flags |= RTF_LIMITED;
			itype->flags |= ITF_NOBUILDBESIEGED;
			a = a_add(&rtype->attribs, a_new(&at_resourcelimit));
			{
				resource_limit * rdata = (resource_limit*)a->data.v;
				rdata->limit = limit_oldtypes;
				rdata->use = use_oldresource;
			}
			break;
		}
		if (itemdata[i].benutze_funktion) {
			itype->use = use_olditem;
		}
		itype->construction = con;
		olditemtype[i] = itype;
		oldresourcetype[item2res(i)] = rtype;
	}
}

const char *herbdata[3][MAXHERBS] =
{
	{
		"Flachwurz",           /* PLAIN_1 */
		"Würziger Wagemut",    /* PLAIN_2 */
		"Eulenauge",           /* PLAIN_3 */
		"Grüner Spinnerich",   /* H_FOREST_1 */
		"Blauer Baumringel",
		"Elfenlieb",
		"Gurgelkraut",         /* SWAMP_1 */
		"Knotiger Saugwurz",
		"Blasenmorchel",		   /* SWAMP_3 */
		"Wasserfinder",
		"Kakteenschwitz",
		"Sandfäule",
		"Windbeutel",			/* HIGHLAND_1 */
		"Fjordwuchs",
		"Alraune",
		"Steinbeißer",	/* MOUNTAIN_1 */
		"Spaltwachs",
		"Höhlenglimm",
		"Eisblume",	/* GLACIER_1 */
		"Weißer Wüterich",
		"Schneekristall"
	},
	{
		"Flachwurz",
		"Würzige Wagemut",
		"Eulenaugen",
		"Grüne Spinneriche",
		"Blaue Baumringel",
		"Elfenlieb",
		"Gurgelkräuter",
		"Knotige Saugwurze",
		"Blasenmorcheln",
		"Wasserfinder",
		"Kakteenschwitze",
		"Sandfäulen",
		"Windbeutel",
		"Fjordwuchse",
		"Alraunen",
		"Steinbeißer",
		"Spaltwachse",
		"Höhlenglimme",
		"Eisblumen",
		"Weiße Wüteriche",
		"Schneekristalle"
	},
	{
		"4",
		"10",
		"7",
		"2",
		"4",
		"1",
		"5",
		"5",
		"4",
		"3",
		"3",
		"5",
		"4",
		"6",
		"1",
		"3",
		"1",
		"5",
		"1",
		"1",
		"3"
	}
};

void
init_oldherbs(void)
{
	herb_t h;
	const char * names[2];
	const char * appearance[2] = { "herbbag", "herbbag" };

	const struct locale * lang = find_locale("de");
	assert(lang);

	for (h=0;h!=MAXHERBS;++h) {
		item_type * itype;
		terrain_t t;
		resource_type * rtype;

		names[0] = NULL;
		{
			int ci;
			for (ci=0;translation[ci][0];++ci) {
				if (!strcmp(translation[ci][0], herbdata[0][h])) {
					names[0] = translation[ci][1];
					names[1] = translation[ci][2];
				}
			}
		}
		if (!names[0]) {
			names[0] = reverse_lookup(lang, herbdata[0][h]);
			names[1] = reverse_lookup(lang, herbdata[1][h]);
		}

		rtype = new_resourcetype(names, appearance, RTF_ITEM|RTF_POOLED);
		itype = new_itemtype(rtype, ITF_HERB, 0, 0);

		t = (terrain_t)(h/3+1);
		if (t>T_PLAIN) --t;
		oldherbtype[h] = new_herbtype(itype, t);
		oldresourcetype[herb2res(h)] = rtype;
	}
}

const char *potionnames[3][MAXPOTIONS] =
{
	{
		/* Stufe 1: */
		"Siebenmeilentee",
		"Goliathwasser",
		"Wasser des Lebens",
		/* Stufe 2: */
		"Schaffenstrunk",
		"Wundsalbe",
		"Bauernblut",
		/* Stufe 3: */
		"Gehirnschmalz",
		"Dumpfbackenbrot",
		"Nestwärme",
		"Pferdeglück",
		"Berserkerblut",
		/* Stufe 4: */
		"Bauernlieb",
		"Trank der Wahrheit",
		"Elixier der Macht",
		"Heiltrank"
	},
	{
		/* Stufe 1: */
		"Siebenmeilentees",
		"Goliathwasser",
		"Wasser des Lebens",
		/* Stufe 2: */
		"Schaffenstrünke",
		"Wundsalben",
		"Bauernblut",
		/* Stufe 3: */
		"Gehirnschmalz",
		"Dumpfbackenbrote",
		"Nestwärme",
		"Pferdeglück",
		"Berserkerblut",
		/* Stufe 4: */
		"Bauernlieb",
		"Tränke der Wahrheit",
		"Elixiere der Macht",
		"Heiltränke"
	},
	{
		/* Stufe 1: */
		"einen Siebenmeilentee",
		"ein Goliathwasser",
		"ein Wasser des Lebens",
		/* Stufe 2: */
		"einen Schaffenstrunk",
		"eine Wundsalbe",
		"ein Bauernblut",
		/* Stufe 3: */
		"ein Gehirnschmalz",
		"ein Dumpfbackenbrot",
		"eine Nestwärme",
		"ein Pferdeglück",
		"ein Berserkerblut",
		/* Stufe 4: */
		"ein Bauernlieb",
		"ein Trank der Wahrheit",
		"ein Elixier der Macht",
		"einen Heiltrank"
	}
};

int potionlevel[MAXPOTIONS] =
{
	1,
	1,
	1,
	2,
	2,
	2,
	3,
	3,
	3,
	3,
	3,
	4,
	1,
	4,
	4
};

#ifdef NEW_RECEIPIES
herb_t potionherbs[MAXPOTIONS][MAXHERBSPERPOTION] =
{								/* Benötigte Kräuter */
	/* Stufe 1: */
	/* Siebenmeilentee: */
	{H_FOREST_2, H_HIGHLAND_1, NOHERB, NOHERB, NOHERB, NOHERB},
	/* Goliathwasser: */
	{H_SWAMP_1, H_HIGHLAND_2, NOHERB, NOHERB, NOHERB, NOHERB},
	/* Wasser des Lebens: */
	{H_FOREST_3, H_SWAMP_2, NOHERB, NOHERB, NOHERB, NOHERB},
	/* Stufe 2: */
	/* Schaffenstrunk: */
	{H_HIGHLAND_3, H_MOUNTAIN_2, H_PLAIN_2, NOHERB, NOHERB, NOHERB},
	/* Wundsalbe: */
	{H_GLACIER_2, H_FOREST_2, H_PLAIN_2, NOHERB, NOHERB, NOHERB},
	/* Bauernblut: */
	{H_MOUNTAIN_3, H_HIGHLAND_2, H_FOREST_2, NOHERB, NOHERB, NOHERB},
	/* Stufe 3: */
	/* Gehirnschmalz: */
	{H_DESERT_1, H_MOUNTAIN_1, H_HIGHLAND_1, H_SWAMP_1, NOHERB, NOHERB},
	/* Dumpfbackenbrote: */
	{H_PLAIN_3, H_FOREST_1, H_MOUNTAIN_3, H_HIGHLAND_2, NOHERB, NOHERB},
	/* Nestwärme: */
	{H_GLACIER_1, H_FOREST_1, H_MOUNTAIN_2, H_DESERT_2, NOHERB, NOHERB},
	/* Pferdeglueck: */
	{H_FOREST_2, H_DESERT_3, H_DESERT_2, H_SWAMP_2, NOHERB, NOHERB},
	/* Berserkerblut: */
	{H_GLACIER_2, H_HIGHLAND_3, H_PLAIN_1, H_DESERT_3, NOHERB, NOHERB},
	/* Stufe 4: */
	/* Bauernlieb: */
	{H_HIGHLAND_3, H_GLACIER_3, H_MOUNTAIN_1, H_SWAMP_3, H_FOREST_3, NOHERB},
	/* Trank der Wahrheit: */
	{H_PLAIN_1, H_HIGHLAND_2, NOHERB, NOHERB, NOHERB, NOHERB},
	/* Elixier der Macht: */
	{H_FOREST_3, H_DESERT_1, H_HIGHLAND_1, H_FOREST_1, H_SWAMP_3, NOHERB},
	/* Heiltrank: */
	{H_SWAMP_1, H_HIGHLAND_1, H_GLACIER_1, H_FOREST_3, H_MOUNTAIN_2, NOHERB}
};
#else
herb_t potionherbs[MAXPOTIONS][MAXHERBSPERPOTION] =
{								/* Benötigte Kräuter */
	/* Stufe 1: */
	/* Siebenmeilentee: */
	{H_PLAIN_2, H_FOREST_1, H_HIGHLAND_1, NOHERB, NOHERB, NOHERB},
	/* Goliathwasser: */
	{H_PLAIN_1, H_SWAMP_3, H_HIGHLAND_2, NOHERB, NOHERB, NOHERB},
	/* Wasser des Lebens: */
	{H_FOREST_2, H_PLAIN_1, H_SWAMP_2, NOHERB, NOHERB, NOHERB},
	/* Stufe 2: */
	/* Schaffenstrunk: */
	{H_PLAIN_1, H_HIGHLAND_2, H_MOUNTAIN_1, H_PLAIN_2, NOHERB, NOHERB},
	/* Scheusalsbier/Wundsalbe: */
	{H_FOREST_2, H_MOUNTAIN_3, H_FOREST_1, H_PLAIN_3, NOHERB, NOHERB},
	/* Duft der Rose/Bauernblut: */
	{H_MOUNTAIN_1, H_HIGHLAND_1, H_FOREST_2, H_PLAIN_2, NOHERB, NOHERB},
	/* Stufe 3: */
	/* Gehirnschmalz: */
	{H_FOREST_1, H_DESERT_1, H_MOUNTAIN_3, H_HIGHLAND_1, H_SWAMP_1, NOHERB},
	/* Dumpfbackenbrote: */
	{H_PLAIN_1, H_FOREST_1, H_MOUNTAIN_2, H_SWAMP_2, H_HIGHLAND_1, NOHERB},
	/* Stahlpasten/Nestwärme: */
	{H_GLACIER_3, H_FOREST_2, H_MOUNTAIN_3, H_DESERT_1, H_SWAMP_3, NOHERB},
	/* Pferdeglueck: */
	{H_FOREST_3, H_DESERT_2, H_HIGHLAND_1, H_MOUNTAIN_1, H_SWAMP_3, NOHERB},
	/* Berserkerblut: */
	{H_GLACIER_2, H_MOUNTAIN_1, H_HIGHLAND_1, H_PLAIN_2, H_DESERT_2, NOHERB},
	/* Stufe 4: */
	/* Bauernlieb: */
 {H_FOREST_1, H_HIGHLAND_2, H_GLACIER_3, H_MOUNTAIN_2, H_SWAMP_3, H_FOREST_3},
	/* Riesengras/Trank der Wahrheit: */
	{H_PLAIN_1, H_SWAMP_3, H_HIGHLAND_1, NOHERB, NOHERB, NOHERB},
	/* Faulobstschnaps/Elixier der Macht: */
	{H_FOREST_2, H_DESERT_3, H_HIGHLAND_3, H_FOREST_1, H_SWAMP_2, H_SWAMP_1},
	/* Heiltrank: */
	{H_SWAMP_1, H_PLAIN_3, H_HIGHLAND_3, H_GLACIER_1, H_FOREST_1, H_MOUNTAIN_3}
};
#endif

const char *potiontext[MAXPOTIONS] =
{
	/* Stufe 1: */
	"Für den Siebenmeilentee koche man einen Blauen Baumringel auf und "
   "gieße dieses Gebräu in einen Windbeutel. Das heraustropfende Wasser "
   "fange man auf, filtere es und verabreiche es alsdann. Durch diesen "
   "Tee können bis zu zehn Menschen schnell wie ein Pferd laufen.",

	"Zuerst brate man das Gurgelkraut leicht an und würze das Zeug mit "
	"ein wenig Fjordwuchs. Man lasse alles so lange kochen, bis fast alle "
	"Flüssigkeit verdampft ist. Diesen Brei stelle man über Nacht raus. "
	"Am nächsten Morgen presse man den Brei aus. Die so gewonnene "
	"Flüssigkeit, Goliathwasser genannt, verleiht bis zu zehn Männern die "
	"Tragkraft eines Pferdes.",

	"Das 'Wasser des Lebens' ist in der Lage, aus gefällten Baumstämmen "
	"wieder lebende Bäume zu machen. Dazu wird ein knotiger Saugwurz zusammen mit einem "
	"Elfenlieb erwärmt, so daß man gerade noch den Finger reinhalten "
	"kann. Dies gieße man in ein Gefäß und lasse es langsam abkühlen. "
	"Der Extrakt reicht für 10 Holzstämme.",

	/* Stufe 2: */
	"Man lasse einen Würzigen Wagemut drei Stunden lang in einem "
	"Liter Wasser köcheln."
	"Dann gebe man eine geriebene Alraune dazu und bestreue "
	"das ganze mit bei Vollmond geerntetem Spaltwachs. Nun lasse man den "
	"Sud drei Tage an einem dunklen und warmen Ort ziehen und seie dann die "
	"Flüssigkeit ab. Dieser Schaffenstrunk erhöht die Kraft und Ausdauer von "
	"zehn Männern, so daß sie doppelt soviel schaffen können wie sonst.",

	"Ist man nach einem einem harten Kampf schwer verwundet, ist es ratsam, "
	"etwas Wundsalbe parat zu haben. Streicht man diese magische Paste auf "
	"die Wunden, schließen sich diese augenblicklich. Für die Herstellung "
	"benötigt der Alchemist nebst einem Blauen "
	"Baumringel einen Würzigen Wagemut und einen Weißen Wüterich. Eine solche Portion "
	"heilt bis zu 400 Lebenspunkte.",

	"Zu den gefährlichsten und geheimsten Wissen der Alchemisten zählt die "
	"Kenntnis um diesen Trank. Den finstersten Höllen entrissen, ermöglicht "
	"die Kenntnis dieser Formel die Herstellung eines Elixiers, welches "
	"Dämonen als Nahrung dient. Von normalen Lebewesen eingenommen, führt "
	"es zu schnellem Tod und ewigen Nichtleben. Die Herstellung benötigt "
	"nebst Fjordwuchs, etwas Höhlenglimm und einem Blauen Baumringel auch einen Bauern aus der Region, "
	"welcher in einem tagelangen blutigen Ritual getötet wird. Ein Fläschchen "
	"des Tranks kann den Hunger von 100 Dämonen für eine Woche stillen.",

	/* Stufe 3: */
	"Für das Gehirnschmalz verrühre man den Saft eines Wasserfinders mit "
	"recht viel geriebenem Windbeutel und ein wenig Gurgelkraut. Dies lasse "
	"man kurz aufwallen. Wenn die Flüssigkeit nur noch handwarm ist, gebe "
	"man etwas Steinbeißer dazu. Das ganze muß "
	"genau siebenmal rechtsherum und siebenmal linksherum mit einem großen "
	"Löffel gerührt werden. Wenn keine Bewegung mehr zu erkennen ist, "
	"fülle man den Saft ab. Der Saft erhöht die Lernfähigkeit von bis zu "
	"zehn Personen um zehn Tage.",

	"Das Dumpfbackenbrot ist eine sehr gemeine Sache, macht es doch jeden "
	"Lernerfolg zunichte oder läßt einen gar Dinge vergessen! Für zehn "
	"Portionen verknete man einen geriebenen Fjordwuchs, einen zerstoßenes "
	"Eulenauge und einen kleingeschnittenen Grünen Spinnerich zu "
	"einem geschmeidigen Teig. Diesen backe man eine Stunde lang bei guter Hitze "
   "und bestreiche das Ergebnis mit etwas Höhlenglimm. "
	"Wer dieses Brot gegessen hat, kann eine Woche lang "
	"nichts lernen, und so er nichts zu lernen versucht, wird er gar "
	"eine Woche seiner besten Fähigkeit vergessen.",

	"Nestwärme erlaubt es einem Insekt, im Winter außerhalb von Wüsten neue "
	"Rekruten anzuwerben. "
	"Zur Zubereitung nimmt der geübte Alchemist einen Kakteenschwitz, "
	"vermischt ihn mit einer Portion Spaltwachs, die in einer sternklaren "
	"Nacht gesammelt wurde, gibt zur Vertreibung des Winters einige "
	"Blütenblätter der Eisblume in den Sud, und rührt alles mit einem grünen "
	"Spinnerich bis es eine "
	"violette Farbe annimmt. Ein Trank reicht eine Woche lang für eine "
	"ganze Region. ",

	"Für das Pferdeglück zerhacke man einen Kakteenschwitz, "
	"einen blauen Baumringel und etwas knotigen Saugwurz und koche das "
	"ganze mit einem Eimer Wasser auf. Dann füge man etwas Sandfäule dazu "
	"und lasse diesen Sud drei Tage lang ziehen. Letztlich gebe man es "
	"den Pferden zu trinken, auf daß sie sich doppelt so schnell vermehren.",

	"Will man seine Krieger zu Höchstleistungen antreiben, sei das "
	"Berserkerblut empfohlen. Um es herzustellen, braucht man einen "
	"Weißen Wüterich, etwas Flachwurz, "
	"Sandfäule und eine Alraune. Alle Zutaten müssen "
	"möglichst klein geschnitten und anschließend zwei Stunden lang gekocht "
	"werden. Den abgekühlten Brei gebe man in ein Tuch und presse ihn aus. "
	"Der so gewonnene Saft reicht aus, um zehn Kämpfer besser angreifen zu "
	"lassen.",

	/* Stufe 4 */
	"Das Bauernlieb betört Mann und Frau gleichwohl und läßt in ihnen "
	"den Wunsch nach Kindern anwachsen. Für eine große Portion höhle "
	"man eine Alraune aus, gebe kleingehackten Blasenmorchel, Elfenlieb "
	"und Schneekristall dazu, streue ein wenig geriebenen Steinbeißer darüber und lasse dieses zwanzig "
	"Stunden lang auf kleiner Flamme kochen. Bis zu 1000 Bauern vermag "
	"der Trank das Glück von Zwillinge zu bescheren.",

	/* Stufe 1, Trank der Wahrheit */
	"Dieses wirkungsvolle einfache Gebräu schärft die Sinne des Trinkers "
	"derart, daß er in der Lage ist, eine Woche lang auch die komplexesten "
	"Illusionen zu durchschauen. Zur Herstellung benötigt ein Alchemist "
	"einen Flachwurz und einen Fjordwuchs.",

	"Eines der seltensten und wertvollsten alchemistischen Elixiere, verleiht "
	"dieser Trank dem Anwender für einige Wochen die Kraft eines Drachen. "
	"Der Trank erhöht die Lebensenergie von maximal zehn "
	"Personen auf das fünffache. Die Wirkung ist direkt nach der Einnahme "
	"am stärksten und klingt danach langsam ab. Zur Herstellung "
	"benötigt der Alchemist ein Elfenlieb, einen Windbeutel, "
	"ein Stück Wasserfinder und einen Grünen Spinnerich. "
	"Über dieses Mischung streue er schließlich einen zerriebenen Blasenmorchel "
	"und rühre dieses Pulver unter etwas Drachenblut.",

	"Für einen Heiltrank nehme man die Schale eines Windbeutels "
	"und etwas Gurgelkraut, rühre eine "
	"kleingehacktes Elfenlieb dazu und bestreue alles mit den "
	"Blüten einer Eisblume. Dies muß vier Tage lang gären, wobei man am "
	"zweiten Tag einen Spaltwachs dazutun muß. Dann ziehe man vorsichtig "
	"den oben schwimmenden Saft ab. Ein solcher Trank gibt vier Männern "
	"(oder einem Mann vier mal) im Kampf eine Chance von 50%, sonst tödliche "
	"Wunden zu überleben. Der Trank wird von ihnen automatisch bei "
	"Verletzung angewandt.",
};

static int
use_warmthpotion(struct unit *u, const struct potion_type *ptype, int amount, const char *cmd)
{
	assert(ptype==oldpotiontype[P_WARMTH]);
	if (old_race(u->faction->race) == RC_INSECT) {
		fset(u, FL_WARMTH);
	} else {
		/* nur für insekten: */
		cmistake(u, cmd, 163, MSG_EVENT);
		return ECUSTOM;
	}
	unused(ptype);
	return 0;
}

static int
use_foolpotion(struct unit *u, int targetno, const struct item_type *itype, int amount, const char *cmd)
{
	unit * target = findunit(targetno);
	if (target==NULL || u->region!=target->region) {
		cmistake(u, cmd, 63, MSG_EVENT);
		return ECUSTOM;
	}
	/* TODO: wahrnehmung-check */
	ADDMSG(&u->faction->msgs, msg_message("givedumb", 
		"unit recipient amount", u, target, amount));
	assert(oldpotiontype[P_FOOL]->itype==itype);
	change_effect(target, oldpotiontype[P_FOOL], amount);
	new_use_pooled(u, itype->rtype, GET_DEFAULT, amount);
	return 0;
}

static int
use_bloodpotion(struct unit *u, const struct potion_type *ptype, int amount, const char *cmd)
{
	int i;
	assert(ptype==oldpotiontype[P_BAUERNBLUT]);
	unused(ptype);
	for (i=0;i!=amount;++i) {
		if (old_race(u->race) == RC_DAEMON) {
			attrib * a = (attrib*)a_find(u->attribs, &at_bauernblut);
			if (!a) a = a_add(&u->attribs, a_new(&at_bauernblut));
			a->data.i += 100;
		} else {
			/* bekommt nicht: */
			cmistake(u, cmd, 165, MSG_EVENT);
			u->race = new_race[RC_GHOUL];
			u_setfaction(u, findfaction(MONSTER_FACTION));
			break;
		}
	}
	return 0;
}

#include <attributes/fleechance.h>
static int
use_mistletoe(struct unit * user, const struct item_type * itype, int amount, const char * cmd)
{
	int mtoes = new_get_pooled(user, itype->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK);

	if (user->number>mtoes) {
		ADDMSG(&user->faction->msgs, msg_message("use_singleperson",
			"unit item region command", user, itype->rtype, user->region, cmd));
		return -1;
	}
	new_use_pooled(user, itype->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, user->number);
	a_add(&user->attribs, make_fleechance((float)1.0));
		ADDMSG(&user->faction->msgs, msg_message("use_item",
			"unit item", user, itype->rtype));
	
	return 0;
}

static int
use_snowball(struct unit * user, const struct item_type * itype, int amount, const char * cmd)
{
	return 0;
}

void
init_oldpotions(void)
{
	potion_t p;
	const char * names[2];
	const char * appearance[2] = { "vial", "vial_p" };

	const struct locale * lang = find_locale("de");
	assert(lang);

	for (p=0;p!=MAXPOTIONS;++p) {
		item_type * itype;
		resource_type * rtype;
		construction * con = calloc(sizeof(construction), 1);
		int i = 0;
		while (i!=MAXHERBSPERPOTION && potionherbs[p][i]!=NOHERB) ++i;
		if (p==P_BAUERNBLUT || p==P_MACHT) ++i;

		con->materials = calloc(sizeof(requirement), i + 1);
		for (i=0;i!=MAXHERBSPERPOTION && potionherbs[p][i]!=NOHERB;++i) {
#ifdef NO_OLD_ITEMS
			con->materials[i].rtype = oldherbtype[potionherbs[p][i]]->itype->rtype;
#else
			con->materials[i].type = herb2res(potionherbs[p][i]);
#endif
			con->materials[i].number = 1;
			con->materials[i].recycle = 0;
		}
		if (p == P_BAUERNBLUT) {
			con->materials[i].number = 1;
			con->materials[i].recycle = 0;
#ifdef NO_OLD_ITEMS
			con->materials[i].rtype = oldresourcetype[R_PEASANTS];
#else
			con->materials[i].type = R_PEASANTS;
#endif
			++i;
		}
		else if (p == P_MACHT) {
			con->materials[i].number = 1;
			con->materials[i].recycle = 0;
#ifdef NO_OLD_ITEMS
			con->materials[i].rtype = oldresourcetype[R_DRACHENBLUT];
#else
			con->materials[i].type = R_DRACHENBLUT;
#endif
			++i;
		}
		con->skill = SK_ALCHEMY;
		con->minskill = potionlevel[p]*2;
		con->maxsize = -1;
		con->reqsize = 1;

		names[0] = NULL;
		{
			int ci;
			for (ci=0;translation[ci][0];++ci) {
				if (!strcmp(translation[ci][0], potionnames[0][p])) {
					names[0] = translation[ci][1];
					names[1] = translation[ci][2];
				}
			}
		}
		if (!names[0]) {
			names[0] = reverse_lookup(lang, potionnames[0][p]);
			names[1] = reverse_lookup(lang, potionnames[1][p]);
		}

		rtype = new_resourcetype(names, appearance, RTF_ITEM|RTF_POOLED);
		if (p==P_FOOL) rtype->flags |= RTF_SNEAK;
		oldresourcetype[potion2res(p)] = rtype;
		itype = new_itemtype(rtype, ITF_POTION, 0, 0);
		itype->construction = con;
		itype->use = use_potion;
		oldpotiontype[p] = new_potiontype(itype, (terrain_t)p/3);
		oldpotiontype[p]->level = potionlevel[p];
		oldpotiontype[p]->text = potiontext[p];
		if (p==P_FOOL) itype->useonother = &use_foolpotion;
	}
	oldpotiontype[P_WARMTH]->use = &use_warmthpotion;
	oldpotiontype[P_BAUERNBLUT]->use = &use_bloodpotion;	
}

resource_type * r_silver;
resource_type * r_aura;
resource_type * r_permaura;
resource_type * r_peasants;
resource_type * r_unit;
resource_type * r_hp;
resource_type * r_person;

resource_type * r_silver;
item_type * i_silver;

static const char * names[] = {
	"money", "money_p",
	"person", "person_p",
	"permaura", "permaura_p",
	"hp", "hp_p",
	"peasant", "peasant_p",
	"aura", "aura_p",
	"unit", "unit_p"
};

#include <xml.h>

typedef struct xml_state {
	struct item_type * itype;
	struct resource_type * rtype;
	struct weapon_type * wtype;
	int wmods;
} xml_state;

static int
tagend(struct xml_stack * stack)
{
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "resource")==0) {
		free(stack->state);
		init_resources();
	}
	return XML_OK;
}

static int
tagbegin(struct xml_stack * stack)
{
	const xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "resource")==0) {
		xml_state * state = stack->state = calloc(sizeof(xml_state), 1);

		char *names[2], *appearance[2];
		const char *tmp;
		unsigned int flags = RTF_NONE;
		if (xml_bvalue(tag, "pooled")) flags |= RTF_POOLED;
		tmp = xml_value(tag, "name");
		assert(tmp || "resource needs a name");
		names[0] = strdup(tmp);
		names[1] = strcat(strcpy((char*)malloc(strlen(tmp)+3), tmp), "_p");
		tmp = xml_value(tag, "appearance");
		if (tmp!=NULL) {
			appearance[0] = strdup(tmp);
			appearance[1] = strcat(strcpy((char*)malloc(strlen(tmp)+3), tmp), "_p");
			state->rtype = new_resourcetype((const char**)names, (const char**)appearance, flags);
			free(appearance[0]);
			free(appearance[1]);
		} else {
			state->rtype = new_resourcetype((const char**)names, NULL, flags);
		}
		free(names[0]);
		free(names[1]);
	} else {
		xml_state * state = stack->state = stack->next->state;
		if (strcmp(tag->name, "item")==0) {
			unsigned int flags = ITF_NONE;
			int weight = xml_ivalue(tag, "weight");
			int capacity = xml_ivalue(tag, "capacity");

			assert(state->itype==NULL);
			if (xml_bvalue(tag, "cursed")) flags |= ITF_CURSED;
			if (xml_bvalue(tag, "notlost")) flags |= ITF_NOTLOST;
			if (xml_bvalue(tag, "big")) flags |= ITF_BIG;
			if (xml_bvalue(tag, "animal")) flags |= ITF_ANIMAL;
			state->rtype->flags |= RTF_ITEM;
			state->itype = new_itemtype(state->rtype, flags, weight, capacity);
		} else if (strcmp(tag->name, "function")==0) {
			const char * semi = xml_value(tag, "name");
			const char * s = xml_value(tag, "value");
			item_type * it = state->itype;
			int i = atoi(s);
			switch (semi[0]) {
			case 'c':
				if (!strcmp(semi, "capacity")) it->capacity=i;
				break;
			case 'f':
				if (!strcmp(semi, "flags")) it->flags=i;
				break;
			case 'g':
				if (!strcmp(semi, "give")) it->give = (boolean (*)(const unit*, const unit*, const struct item_type *, int, const char *))get_function(s);
				break;
			case 'u':
				if (!strcmp(semi, "use")) it->use = (int (*)(unit *, const struct item_type *, int, const char *))get_function(s);
				break;
			case 'w':
				if (!strcmp(semi, "weight")) it->weight=i;
				break;
			}
		} else if (strcmp(tag->name, "weapon")==0) {
			skill_t sk = sk_find(xml_value(tag, "skill"));
			int minskill = xml_ivalue(tag, "minskill");
			int offmod = xml_ivalue(tag, "offmod");
			int defmod = xml_ivalue(tag, "defmod");
			int reload = xml_ivalue(tag, "reload");
			double magres = xml_fvalue(tag, "magres");
			unsigned int flags = WTF_NONE;

			assert(strcmp(stack->next->tag->name, "item")==0);
			assert(state->itype!=NULL);
			state->itype->flags |= ITF_WEAPON;
			state->wtype = new_weapontype(state->itype,
				flags, magres, NULL, offmod, defmod, reload, sk, minskill);
		} else if (strcmp(tag->name, "damage")==0) {
			/* damage of a weapon */
			int pos = 0;
			const char * type = xml_value(tag, "type");

			assert(strcmp(stack->next->tag->name, "weapon")==0);
			if (strcmp(type, "default")!=0) pos = 1;
			state->wtype->damage[pos] = gc_add(strdup(xml_value(tag, "value")));
		} else if (strcmp(tag->name, "modifier")==0) {
			int value = xml_ivalue(tag, "value");
			assert(strcmp(stack->next->tag->name, "weapon")==0);
			if (value!=0) {
				int flags = 0;
				weapon_mod * mods = calloc(sizeof(weapon_mod), state->wmods+2);

				assert(state->wtype);
				if (xml_bvalue(tag, "walking")) flags|=WMF_WALKING;
				if (xml_bvalue(tag, "riding")) flags|=WMF_RIDING;
				if (xml_bvalue(tag, "against_walking")) flags|=WMF_AGAINST_WALKING;
				if (xml_bvalue(tag, "against_riding")) flags|=WMF_AGAINST_RIDING;
				if (xml_bvalue(tag, "offensive")) flags|=WMF_OFFENSIVE;
				if (xml_bvalue(tag, "defensive")) flags|=WMF_DEFENSIVE;
				if (xml_bvalue(tag, "damage")) flags|=WMF_DAMAGE;
				if (xml_bvalue(tag, "skill")) flags|=WMF_SKILL;
				if (xml_bvalue(tag, "missile_target")) flags|=WMF_MISSILE_TARGET;
				if (state->wmods) {
					memcpy(mods, state->wtype->modifiers, sizeof(weapon_mod)*state->wmods);
					free(state->wtype->modifiers);
				}
				mods[state->wmods].value = value;
				mods[state->wmods].flags = flags;
				state->wtype->modifiers = mods;
				++state->wmods;
			}
		} else {
			return XML_USERERROR;
		}
	}
	return XML_OK;
}

static xml_callbacks xml_resource = {
	tagbegin, tagend, NULL
};

void
init_resources(void)
{
	static boolean initialized = false;
	if (initialized) return;
	initialized = true;
	/* silver was never an item: */
	r_silver = new_resourcetype(&names[0], NULL, RTF_ITEM|RTF_POOLED);
	i_silver = new_itemtype(r_silver, ITF_NONE, 1/*weight*/, 0);
	r_silver->uchange = res_changeitem;

	r_person = new_resourcetype(&names[2], NULL, RTF_NONE);
	r_person->uchange = res_changeperson;

	r_permaura = new_resourcetype(&names[4], NULL, RTF_NONE);
	r_permaura->uchange = res_changepermaura;

	r_hp = new_resourcetype(&names[6], NULL, RTF_NONE);
	r_hp->uchange = res_changehp;

	r_peasants = new_resourcetype(&names[8], NULL, RTF_NONE);
	r_peasants->uchange = res_changepeasants;

	r_aura = new_resourcetype(&names[10], NULL, RTF_NONE);
	r_aura->uchange = res_changeaura;

	r_unit = new_resourcetype(&names[12], NULL, RTF_NONE);
	r_unit->uchange = res_changeperson;

	oldresourcetype[R_SILVER] = r_silver;
	oldresourcetype[R_AURA] = r_aura;
	oldresourcetype[R_PERMAURA] = r_permaura;
	oldresourcetype[R_HITPOINTS] = r_hp;
	oldresourcetype[R_PEASANTS] = r_peasants;
	oldresourcetype[R_PERSON] = r_person;
	oldresourcetype[R_UNIT] = r_unit;

	/* alte typen registrieren: */
	init_olditems();
	init_oldherbs();
	init_oldpotions();
}

int
get_money(const unit * u)
{
	const item * i = u->items;
	while (i && i->type!=i_silver) i=i->next;
	if (i==NULL) return 0;
	return i->number;
}

int
set_money(unit * u, int v)
{
	item ** ip = &u->items;
	while (*ip && (*ip)->type!=i_silver) ip = &(*ip)->next;
	if ((*ip)==NULL && v) {
		i_add(&u->items, i_new(i_silver, v));
		return v;
	}
	if ((*ip)!=NULL) {
		if (v) (*ip)->number = v;
		else i_remove(ip, *ip);
	}
	return v;
}

int
change_money(unit * u, int v)
{
	item ** ip = &u->items;
	while (*ip && (*ip)->type!=i_silver) ip = &(*ip)->next;
	if ((*ip)==NULL && v) {
		i_add(&u->items, i_new(i_silver, v));
		return v;
	}
	if ((*ip)!=NULL) {
		item * i = *ip;
		if (i->number + v != 0) {
			i->number += v;
			return i->number;
		}
		else i_free(i_remove(ip, *ip));
	}
	return 0;
}


static local_names * rnames;

const resource_type *
findresourcetype(const char * name, const struct locale * lang)
{
	local_names * rn = rnames;
	void * i;

	while (rn) {
		if (rn->lang==lang) break;
		rn=rn->next;
	}
	if (!rn) {
		const resource_type * rtl = resourcetypes;
		rn = calloc(sizeof(local_names), 1);
		rn->next = rnames;
		rn->lang = lang;
		while (rtl) {
			addtoken(&rn->names, locale_string(lang, rtl->_name[0]), (void*)rtl);
			addtoken(&rn->names, locale_string(lang, rtl->_name[1]), (void*)rtl);
			rtl=rtl->next;
		}
		rnames = rn;
	}

	if (findtoken(&rn->names, name, &i)==E_TOK_NOMATCH) return NULL;
	return (const resource_type*)i;
}

attrib_type at_showitem = {
	"showitem"
};

attrib_type at_seenitem = {
	"showitem",
	NULL,
	NULL,
	NULL,
	a_writedefault,
	a_readdefault
};

static local_names * inames;

const item_type *
finditemtype(const char * name, const struct locale * lang)
{
	local_names * in = inames;
	void * i;

	while (in) {
		if (in->lang==lang) break;
		in=in->next;
	}
	if (!in) {
		const item_type * itl = itemtypes;
		in = calloc(sizeof(local_names), 1);
		in->next = inames;
		in->lang = lang;
		while (itl) {
			addtoken(&in->names, locale_string(lang, itl->rtype->_name[0]), (void*)itl);
			addtoken(&in->names, locale_string(lang, itl->rtype->_name[1]), (void*)itl);
			itl=itl->next;
		}
		inames = in;
	}
	if (findtoken(&in->names, name, &i)==E_TOK_NOMATCH) return NULL;
	return (const item_type*)i;
}

static void
init_resourcelimit(attrib * a)
{
	a->data.v = calloc(sizeof(resource_limit), 1);
}

static void
finalize_resourcelimit(attrib * a)
{
	free(a->data.v);
}

attrib_type at_resourcelimit = {
	"resourcelimit",
	init_resourcelimit,
	finalize_resourcelimit,
};

#define DYNAMIC_TYPES
#ifdef DYNAMIC_TYPES
void
rt_write(FILE * F, const resource_type * rt)
{
	fprintf(F, "RESOURCETYPE %d\n", rt->hashkey);
	a_write(F, rt->attribs); /* scheisse, weil nicht CR. */
	fputc('\n', F);
	fprintf(F, "\"%s\";name_singular\n", rt->_name[0]);
	fprintf(F, "\"%s\";name_plural\n", rt->_name[1]);
	fprintf(F, "\"%s\";appearance_singular\n", rt->_appearance[0]);
	fprintf(F, "\"%s\";appearance_plural\n", rt->_appearance[1]);
	fprintf(F, "%d;flags\n", rt->flags);
	if (rt->uchange) {
		assert(rt->uchange==res_changeitem || !"not implemented");
		fputs("\"res_changeitem\";uchange\n", F);
	}
	if (rt->uget!=NULL) assert(!"not implemented");
	if (rt->name!=NULL) assert(!"not implemented");
	fputs("END RESOURCETYPE\n", F);
}

void
it_write(FILE * F, const item_type * it)
	/* Written at Gate E2 of Amsterdam Airport.
	 * If someone had installed power outlets there,
	 * I would have been able to test this stuff */
{
	fprintf(F, "ITEMTYPE %s\n", it->rtype->_name[0]);
	fprintf(F, "%d;flags\n", it->flags);
	fprintf(F, "%d;weight\n", it->weight);
	fprintf(F, "%d;capacity\n", it->capacity);
	if (it->use!=NULL) {
		const char * name = get_functionname((pf_generic)it->use);
		fprintf(F, "\"%s\";use\n", name);
	}
	if (it->give!=NULL) {
		const char * name = get_functionname((pf_generic)it->give);
		fprintf(F, "\"%s\";give\n", name);
	}
	fputs("END ITEMTYPE\n", F);
}

resource_type *
rt_read(FILE * F)
	/* this function is pretty picky */
{
	resource_type * rt = calloc(sizeof(resource_type), 1);
	unsigned int hash;
	int i = fscanf(F, "%u\n", &hash);
	if (i==0 || i==EOF) {
		free(rt);
		return NULL;
	}
	a_read(F, &rt->attribs); /* scheisse, weil nicht CR. */
	for (;;) {
		char * semi = buf;
		char * s = NULL;
		int i = 0;
		fgets(buf, sizeof(buf), F);
		if (strlen(buf)==1) continue;
		buf[strlen(buf)-1]=0;
		for(;;) {
			char * s = strchr(semi, ';');
			if (s==NULL) break;
			semi = s + 1;
		}
		if (semi==buf) {
			assert(!strcmp(buf, "END RESOURCETYPE"));
			break;
		}
		*(semi-1)=0;
		if (buf[0]=='\"') {
			s = buf+1;
			assert(*(semi-2)=='\"');
			*(semi-2)=0;
		}
		else {
			i = atoi(buf);
		}
		switch (semi[0]) {
		case 'a':
			if (!strcmp(semi, "appearance_singular")) rt->_appearance[0]=strdup(s);
			if (!strcmp(semi, "appearance_plural")) rt->_appearance[1]=strdup(s);
			break;
		case 'f':
			if (!strcmp(semi, "flags")) rt->flags=i;
			break;
		case 'n':
			if (!strcmp(semi, "name_singular")) rt->_name[0]=strdup(s);
			if (!strcmp(semi, "name_plural")) rt->_name[1]=strdup(s);
			break;
		case 'u':
			if (!strcmp(semi, "uchange")) {
				if (!strcmp(s, "res_changeitem")) rt->uchange = res_changeitem;
			}
		}
	}
	rt_register(rt);
	return rt;
}

item_type *
it_read(FILE * F)
	/* this function is pretty picky */
{
	item_type * it = calloc(sizeof(item_type), 1);
	int i = fscanf(F, "%s\n", buf);
	if (i==0 || i==EOF) {
		free(it);
		return NULL;
	}
	it->rtype = rt_find(buf);
	assert(it->rtype || !"resource does not exist");
	for (;;) {
		char * semi = buf;
		char * s = NULL;
		int i = 0;
		fgets(buf, sizeof(buf), F);
		if (strlen(buf)==1) continue;
		buf[strlen(buf)-1]=0;
		for(;;) {
			char * s = strchr(semi, ';');
			if (s==NULL) break;
			semi = s + 1;
		}
		if (semi==buf) {
			assert(!strcmp(buf, "END ITEMTYPE"));
			break;
		}
		*(semi-1)=0;
		if (buf[0]=='\"') {
			s = buf+1;
			assert(*(semi-2)=='\"');
			*(semi-2)=0;
		}
		else {
			i = atoi(buf);
		}
		switch (semi[0]) {
		case 'c':
			if (!strcmp(semi, "capacity")) it->capacity=i;
			break;
		case 'f':
			if (!strcmp(semi, "flags")) it->flags=i;
			break;
		case 'g':
			if (!strcmp(semi, "give")) it->give = (boolean (*)(const unit*, const unit*, const struct item_type *, int, const char *))get_function(s);
			break;
		case 'u':
			if (!strcmp(semi, "use")) it->use = (int (*)(unit *, const struct item_type *, int, const char *))get_function(s);
			break;
		case 'w':
			if (!strcmp(semi, "weight")) it->weight=i;
			break;
		}
	}
	it_register(it);
	return it;
}

#endif

const char*
resname(resource_t res, int index)
{
	const item_type * itype = resource2item(oldresourcetype[res]);
	if (itype!=NULL) {
		return locale_string(NULL, resourcename(oldresourcetype[res], index));
	}
	else if (res == R_AURA) {
		return index==1?"aura":"aura_p";
	} else if (res == R_PERMAURA) {
		return index==1?"permaura":"permaura_p";
	} else if (res == R_PEASANTS) {
		return index==1?"peasant":"peasant_p";
	} else if (res == R_UNIT) {
		return index==1?"unit":"unit_p";
	} else if (res == R_PERSON) {
		return index==1?"person":"person_p";
	} else if (res == R_HITPOINTS) {
		return index==1?"hp":"hp_p";
	}
	return NULL;
}

void
register_resources(void)
{

	register_function((pf_generic)limit_oldtypes, "limit_oldtypes");
	register_function((pf_generic)mod_elves_only, "mod_elves_only");
	register_function((pf_generic)res_changeitem, "changeitem");
	register_function((pf_generic)res_changeperson, "changeperson");
	register_function((pf_generic)res_changepeasants, "changepeasants");
	register_function((pf_generic)res_changepermaura, "changepermaura");
	register_function((pf_generic)res_changehp, "changehp");
	register_function((pf_generic)res_changeaura, "changeaura");

	register_function((pf_generic)use_oldresource, "useoldresource");
	register_function((pf_generic)use_olditem, "useolditem");
	register_function((pf_generic)use_potion, "usepotion");
	register_function((pf_generic)use_tacticcrystal, "usetacticcrystal");
	register_function((pf_generic)use_birthdayamulet, "usebirthdayamulet");
	register_function((pf_generic)use_antimagiccrystal, "useantimagiccrystal");
	register_function((pf_generic)use_warmthpotion, "usewarmthpotion");
	register_function((pf_generic)use_bloodpotion, "usebloodpotion");
	register_function((pf_generic)use_foolpotion, "usefoolpotion");
	register_function((pf_generic)use_mistletoe, "usemistletoe");
	register_function((pf_generic)use_snowball, "usesnowball");

	register_function((pf_generic)give_horses, "givehorses");

	/* xml reader */
	xml_register(&xml_resource, "eressea resource", 0);
}

int
xml_writeitems(const char * file)
{
	FILE * stream = fopen(file, "w+");
	resource_type * rt = resourcetypes;
	item_type * it = itemtypes;
	weapon_type * wt = weapontypes;

/*
	luxury_type * lt = luxurytypes;
	potion_type * pt = potiontypes;
	herb_type * ht = herbtypes;
*/

	if (stream==NULL) return -1;
	fputs("<resources>\n", stream);
	while (rt) {
		fprintf(stream, "\t<resource name=\"%s\"", rt->_name[0]);
		if (fval(rt, RTF_SNEAK)) fputs(" sneak", stream);
		if (fval(rt, RTF_LIMITED)) fputs(" limited", stream);
		if (fval(rt, RTF_POOLED)) fputs(" pooled", stream);
		fputs(">\n", stream);

		if (rt->uchange) {
			const char * name = get_functionname((pf_generic)rt->uchange);
			assert(name);
			fprintf(stream, "\t\t<function name=\"change\" value=\"%s\"></function>\n",
				name);
		}
		if (rt->uget) {
			const char * name = get_functionname((pf_generic)rt->uget);
			assert(name);
			fprintf(stream, "\t\t<function name=\"get\" value=\"%s\"></function>\n",
				name);
		}
		if (rt->name) {
			const char * name = get_functionname((pf_generic)rt->name);
			assert(name);
			fprintf(stream, "\t\t<function name=\"name\" value=\"%s\"></function>\n",
				name);
		}

		fputs("\t</resource>\n", stream);
		rt = rt->next;
	}
	fputs("</resources>\n\n", stream);

	fputs("<items>\n", stream);
	while (it) {
		const construction *ic = it->construction;
		if (ic && ic->improvement) {
			log_printf("construction::improvement not implemented, not writing item '%s'", it->rtype->_name[0]);
			it=it->next;
			continue;
		}
		if (ic && ic->attribs) {
			log_printf("construction::attribs not implemented, not writing item '%s'", it->rtype->_name[0]);
			it=it->next;
			continue;
		}
		fprintf(stream, "\t<item resource=\"%s\"", it->rtype->_name[0]);
		if (it->weight) fprintf(stream, " weight=\"%d\"", it->weight);
		if (it->capacity) fprintf(stream, " capacity=\"%d\"", it->capacity);
	/*
		if (fval(it, ITF_HERB)) fputs(" herb", stream);
		if (fval(it, ITF_WEAPON)) fputs(" weapon", stream);
		if (fval(it, ITF_LUXURY)) fputs(" luxury", stream);
		if (fval(it, ITF_POTION)) fputs(" potion", stream);
	*/
		if (fval(it, ITF_CURSED)) fputs(" cursed", stream);
		if (fval(it, ITF_NOTLOST)) fputs(" notlost", stream);
		if (fval(it, ITF_BIG)) fputs(" big", stream);
		if (fval(it, ITF_ANIMAL)) fputs(" animal", stream);
		if (fval(it, ITF_NOBUILDBESIEGED)) fputs(" nobuildbesieged", stream);
		if (fval(it, ITF_DYNAMIC)) fputs(" dynamic", stream);
		fputs(">\n", stream);

		if (ic) {
			requirement * cm = ic->materials;
			fputs("\t\t<construction", stream);
			if (ic->skill!=NOSKILL) {
				fprintf(stream, " sk=\"%s\"", skillname(ic->skill, NULL));
				if (ic->minskill) fprintf(stream, " minskill=\"%d\"", ic->minskill);
			}
			if (ic->reqsize!=1) {
				fprintf(stream, " reqsize=\"%d\"", ic->reqsize);
			}
			if (ic->maxsize!=-1) {
				fprintf(stream, " maxsize=\"%d\"", ic->maxsize);
			}
			fputs(">\n", stream);
			while (cm && cm->number) {
#ifdef NO_OLD_ITEMS
				resource_type * rtype = cm->rtype;
#else
				resource_type * rtype = oldresourcetype[cm->type];
#endif
				fprintf(stream, "\t\t\t<material resource=\"%s\" size=\"%d\"",
					rtype->_name[0], cm->number);
				if (cm->recycle!=0.0)
					fprintf(stream, " recycle=\"%.2f\"", cm->recycle);
				fputs("></material>\n", stream);
				++cm;
			}
			fputs("\t\t</construction>\n", stream);
		}
		if (it->use) {
			const char * name = get_functionname((pf_generic)it->use);
			assert(name);
			fprintf(stream, "\t\t<function name=\"use\" value=\"%s\"></function>\n",
				name);
		}
		if (it->give) {
			const char * name = get_functionname((pf_generic)it->give);
			assert(name);
			fprintf(stream, "\t\t<function name=\"give\" value=\"%s\"></function>\n",
				name);
		}

		fputs("\t</item>\n", stream);
		fflush(stream);
		it = it->next;
	}
	fputs("</items>\n\n", stream);

	fputs("<weapons>\n", stream);
	while (wt) {
		weapon_mod * wm = wt->modifiers;
		fprintf(stream, "\t<weapon resource=\"%s\"", wt->itype->rtype->_name[0]);
		if (wt->minskill) fprintf(stream, " minskill=\"%d\"", wt->minskill);
		fprintf(stream, " sk=\"%s\"", skillname(wt->skill, NULL));
		if (wt->defmod) fprintf(stream, " offmod=\"%d\"", wt->offmod);
		if (wt->offmod) fprintf(stream, " defmod=\"%d\"", wt->defmod);
		if (wt->reload!=0) fprintf(stream, " reload=\"%d\"", wt->reload);
		if (wt->magres!=0.0) fprintf(stream, " magres=\"%.2f\"", wt->magres);

		if (fval(wt, WTF_MISSILE)) fputs(" missile", stream);
		if (fval(wt, WTF_MAGICAL)) fputs(" magical", stream);
		if (fval(wt, WTF_PIERCE)) fputs(" pierce", stream);
		if (fval(wt, WTF_CUT)) fputs(" cut", stream);
		if (fval(wt, WTF_BLUNT)) fputs(" blunt", stream);
		if (fval(wt, WTF_BOW)) fputs(" bow", stream);

		fputs(">\n", stream);
		while (wm && wm->value) {
			fprintf(stream, "\t\t<modifier value=\"%d\"", wm->value);
			if (fval(wm, WMF_WALKING)) fputs(" walking", stream);
			if (fval(wm, WMF_RIDING)) fputs(" riding", stream);
			if (fval(wm, WMF_AGAINST_RIDING)) fputs(" against_riding", stream);
			if (fval(wm, WMF_AGAINST_WALKING)) fputs(" against_walking", stream);
			if (fval(wm, WMF_OFFENSIVE)) fputs(" offensive", stream);
			if (fval(wm, WMF_DEFENSIVE)) fputs(" defensive", stream);
			if (fval(wm, WMF_DAMAGE)) fputs(" damage", stream);
			if (fval(wm, WMF_SKILL)) fputs(" sk", stream);
			fputs("></modifier>\n", stream);
			++wm;
		}
		fprintf(stream, "\t\t<damage type=\"default\" value=\"%s\"></damage>\n", wt->damage[0]);
		if (strcmp(wt->damage[0], wt->damage[1])!=0) {
			fprintf(stream, "\t\t<damage type=\"riding\" value=\"%s\"></damage>\n", wt->damage[1]);
		}

		if (wt->attack) {
			const char * name = get_functionname((pf_generic)wt->attack);
			assert(name);
			fprintf(stream, "\t\t<function name=\"attack\" value=\"%s\"></function>\n",
				name);
		}

		fputs("\t</weapon>\n", stream);
		fflush(stream);
		wt = wt->next;
	}
	fputs("</weapons>\n\n", stream);
	fclose(stream);
	return 0;
}
