/* vi: set ts=2:
 *
 *	$Id: item.c,v 1.1 2001/01/25 09:37:57 enno Exp $
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
#include <functions.h>

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

attrib_type at_ptype = { "potion_type" };
attrib_type at_wtype = { "weapon_type" };
attrib_type at_ltype = { "luxury_type" };
attrib_type at_itype = { "item_type" };
attrib_type at_htype = { "herb_type" };

extern int hashstring(const char* s);


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
	u->region->land->peasants-=delta;
	return 0;
}

int
res_changeitem(unit * u, const resource_type * rtype, int delta)
{
	if (rtype == oldresourcetype[R_STONE] && u->race==RC_STONEGOLEM && delta<=0) {
		int reduce = delta / GOLEM_STONE;
		if (delta % GOLEM_STONE != 0) --reduce;
		scale_number(u, u->number+reduce);
		return u->number;
	} else if (rtype == oldresourcetype[R_IRON] && u->race==RC_IRONGOLEM && delta<=0) {
		int reduce = delta / GOLEM_IRON;
		if (delta % GOLEM_IRON != 0) --reduce;
		scale_number(u, u->number+reduce);
		return u->number;
	} else {
		const item_type * itype = resource2item(rtype);
		item * i;
		assert(itype!=NULL);
		i = i_change(&u->items, itype, delta);
		if (i==NULL) return 0;
		return i->number;
	}
}

const char *
resourcename(const resource_type * rtype, int flags)
{
	int i = 0;

	if (rtype->name) return rtype->name(rtype, flags);

	if (flags & NMF_PLURAL) i = 1;
	if (flags & NMF_APPEARANCE) return rtype->_appearance[i];
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
		a_add(&itype->rtype->attribs, a_new(&at_itype))->data.v = (void*) itype;
		*p_itype = itype;
		rt_register(itype->rtype);
	}
}

item_type *
new_itemtype(resource_type * rtype,
             int iflags, int weight, int capacity, int minskill, skill_t skill)
{
	item_type * itype;
	assert (resource2item(rtype) == NULL);

	assert(rtype->flags & RTF_ITEM);
	itype = calloc(sizeof(item_type), 1);

	itype->rtype = rtype;
	itype->weight = weight;
	itype->capacity = capacity;
	itype->minskill = minskill;
	itype->skill = skill;
	itype->flags |= iflags;
	it_register(itype);

	rtype->uchange = res_changeitem;

	return itype;
}

void
lt_register(luxury_type * ltype)
{
	a_add(&ltype->itype->rtype->attribs, a_new(&at_ltype))->data.v = (void*) ltype;
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
	a_add(&wtype->itype->rtype->attribs, a_new(&at_wtype))->data.v = (void*) wtype;
	wtype->next = weapontypes;
	weapontypes = wtype;
}

weapon_type *
new_weapontype(item_type * itype,
               int wflags, double magres, const char* damage[], int offmod, int defmod, int reload, skill_t skill, int minskill)
{
	weapon_type * wtype;

	assert(resource2weapon(itype->rtype)==NULL);
	assert(itype->flags & ITF_WEAPON);

	wtype = calloc(sizeof(weapon_type), 1);
	if (damage) {
		wtype->damage[0] = damage[0];
		wtype->damage[1] = damage[1];
	}
	wtype->defmod = defmod;
	wtype->flags |= wflags;
	wtype->itype = itype;
	wtype->magres = magres;
	wtype->minskill = minskill;
	wtype->offmod = offmod;
	wtype->reload = reload;
	wtype->skill = skill;
	wt_register(wtype);

	return wtype;
}


void
pt_register(potion_type * ptype)
{
	a_add(&ptype->itype->rtype->attribs, a_new(&at_ptype))->data.v = (void*) ptype;
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
	a_add(&htype->itype->rtype->attribs, a_new(&at_htype))->data.v = (void*) htype;
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
item2resource(const item_type * itype) {
	return itype->rtype;
}

const item_type *
resource2item(const resource_type * rtype) {
	attrib * a = a_find(rtype->attribs, &at_itype);
	if (a) return (const item_type *)a->data.v;
	return NULL;
}

const herb_type *
resource2herb(const resource_type * rtype) {
	attrib * a = a_find(rtype->attribs, &at_htype);
	if (a) return (const herb_type *)a->data.v;
	return NULL;
}

const weapon_type *
resource2weapon(const resource_type * rtype) {
	attrib * a = a_find(rtype->attribs, &at_wtype);
	if (a) return (const weapon_type *)a->data.v;
	return NULL;
}

const luxury_type *
resource2luxury(const resource_type * rtype) {
	attrib * a = a_find(rtype->attribs, &at_ltype);
	if (a) return (const luxury_type *)a->data.v;
	return NULL;
}

const potion_type *
resource2potion(const resource_type * rtype) {
	attrib * a = a_find(rtype->attribs, &at_ptype);
	if (a) return (const potion_type *)a->data.v;
	return NULL;
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

item_type *
it_find(const char * name)
{
	unsigned int hash = hashstring(name);
	item_type * itype;

	for (itype=itemtypes; itype; itype=itype->next)
		if (itype->rtype->hashkey==hash && strcmp(itype->rtype->_name[0], name) == 0) break;

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
		i = i_new(itype);
		i->next = *pi;
		i->number = delta;
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
i_new(const item_type * itype)
{
	item * i = calloc(1, sizeof(item));
	assert(itype);
	i->type = itype;
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

item_type * olditemtype[MAXITEMS+1];
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
	if (!i)
		i = i_add(&u->items, i_new(type));
	return i->number = value;
}

int
change_item(unit * u, item_t it, int value)
{
	const item_type * type = olditemtype[it];
	item * i = *i_find(&u->items, type);
	if (!i) {
		if (!value) return 0;
		else i = i_add(&u->items, i_new(type));
	}
	return i->number += value;
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
	if (!i)
		i = i_add(&u->items, i_new(type));
	return i->number = value;
}

int
change_herb(unit * u, herb_t h, int value)
{
	const item_type * type = oldherbtype[h]->itype;
	item * i = *i_find(&u->items, type);
	if (!i) {
		if (!value) return 0;
		else i = i_add(&u->items, i_new(type));
	}
	return i->number += value;
}

/*** alte potions ***/

int
get_potion(const unit * u, potion_t p)
{
	const item_type * type = oldpotiontype[p]->itype;
	item * it = *i_find((item**)&u->items, type);
	return it?it->number:0;
}

void use_birthdayamulet(region * r, unit * magician, strlist * cmdstrings);

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
		if (c->flag & CURSE_IMMUN) continue;

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

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagier benutzt werden, erzeugt eine
 * Antimagiezone, die zwei Runden bestehen bleibt */
static void
use_antimagiccrystal(region * r, unit * mage, strlist * cmdstrings)
{
	int effect;
	int power;
	int duration = 2;
	spell *sp = find_spellbyid(SPL_ANTIMAGICZONE);
	unused(cmdstrings);

	/* Reduziert die Stärke jedes Spruchs um effect */
	effect = sp->level*4; /* Stufe 5 =~ 15 */

	/* Hält Sprüche bis zu einem summierten Gesamtlevel von power aus.
	 * Jeder Zauber reduziert die 'Lebenskraft' (vigour) der Antimagiezone
	 * um seine Stufe */
	power = sp->level * 20; /* Stufe 5 =~ 100 */

	power = destroy_curse_crystal(&r->attribs, effect, power);

	if(power) {
		create_curse(mage, &r->attribs, C_ANTIMAGICZONE, 0, power, duration, effect, 0);
	}

	change_item(mage, I_ANTIMAGICCRYSTAL, -1);
	add_message(&mage->faction->msgs,
			new_message(mage->faction, "use_antimagiccrystal%u:unit%r:region", mage, r));
	return;
}

/* ------------------------------------------------------------- */
/* Kann auch von Nichtmagier benutzt werden, modifiziert Taktik für diese
 * Runde um -1 - 4 Punkte. */
static void
use_tacticcrystal(region * r, unit * u, strlist * cmdstrings)
{
	int effect = rand()%6 - 1;
	int duration = 1; /* wirkt nur eine Runde */
	int power = 5; /* Widerstand gegen Antimagiesprüche, ist in diesem
										Fall egal, da der curse für den Kampf gelten soll,
										der vor den Antimagiezaubern passiert */
	unused(cmdstrings);

	create_curse(u, &u->attribs, C_SKILL, SK_TACTICS, power,
			duration, effect, u->number);

	change_item(u, I_TACTICCRYSTAL, -1);
	add_message(&u->faction->msgs,
			new_message(u->faction, "use_tacticcrystal%u:unit%r:region", u, r));
	return;
}

t_item itemdata[MAXITEMS] = {
	/* name[4]; typ; skill; minskill; material[6]; gewicht; preis;
	 * benutze_funktion; */
	{			/* I_IRON */
		{"Eisen", "Eisen", "Eisen", "Eisen"}, G_N,
		IS_RESOURCE, SK_MINING, 1, {0, 0, 0, 0, 0, 0}, 500, 0, 0, NULL
	},
	{			/* I_WOOD */
		{"Holz", "Holz", "Holz", "Holz"}, G_N,
		IS_RESOURCE, SK_LUMBERJACK, 1, {0, 0, 0, 0, 0, 0}, 500, 0, 0, NULL
	},
	{			/* I_STONE */
		{"Stein", "Steine", "Stein", "Steine"}, G_M,
		IS_RESOURCE, SK_QUARRYING, 1, {0, 0, 0, 0, 0, 0}, 6000, 0, 0, NULL
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
		IS_PRODUCT, SK_CARTMAKER, 5, {0, 10, 0, 0, 0, 0}, 12000, 0, FL_ITEM_NOTINBAG, NULL
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
	{			/* I_EOGSWORD 38 */
		{"Laenschwert", "Laenschwerter", "Laenschwert", "Laenschwerter"}, G_N,
		IS_PRODUCT, SK_WEAPONSMITH, 8, {0, 0, 0, 0, 1, 0}, 100, 0, 0, NULL
	},
	{			/* I_EOGSHIELD 39 */
		{"Laenschild", "Laenschilde", "Laenschild", "Laenschilde"}, G_N,
		IS_PRODUCT, SK_ARMORER, 7, {0, 0, 0, 0, 1, 0}, 0, 0, 0, NULL
	},
	{			/* I_EOGCHAIN 40 */
		{"Laenkettenhemd", "Laenkettenhemden", "Laenkettenhemd", "Laenkettenhemden"}, G_N,
		IS_PRODUCT, SK_ARMORER, 9, {0, 0, 0, 0, 3, 0}, 100, 0, 0, NULL
	},
	{			/* I_EOG 41 */
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
		IS_MAGIC, 0, 0, {0, 0, 0, 0, 0, 0}, 100, 0, 0, NULL
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
	}
};

const item_t matresource[] = {
	I_IRON,
	I_WOOD,
	I_STONE,
	-1,
	I_EOG,
	I_MALLORN
};

#include "movement.h"

static int
mod_elves_only(const unit * u, const region * r, skill_t sk, int value)
{
	if (u->race==RC_ELF) return value;
	unused(r);
	unused(sk);
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
	if (rtype==oldresourcetype[R_EOG]) {
		return rlaen(r);
	} else if (rtype==oldresourcetype[R_IRON]) {
		return riron(r);
	} else if (rtype==oldresourcetype[R_STONE]) {
		return terrain[rterrain(r)].quarries;
	} else if (rtype==oldresourcetype[R_WOOD]) {
		return rtrees(r);
	} else if (rtype==oldresourcetype[R_MALLORN]) {
		return rtrees(r);
	} else if (rtype==oldresourcetype[R_HORSE]) {
		return rhorses(r);
	} else {
		assert(!"das kann man nicht produzieren!");
	}
	return 0;
}


static void
use_oldtypes(region * r, const resource_type * rtype, int norders)
		/* TODO: split into seperate functions. really much nicer. */
{
	assert(norders>0);
	if (rtype==oldresourcetype[R_EOG]) {
		int avail = rlaen(r);
		assert(norders <= avail);
		rsetlaen(r, avail-norders);
	} else if (rtype==oldresourcetype[R_IRON]) {
		int avail = riron(r);
		assert(norders <= avail);
		rsetiron(r, avail-norders);
	} else if (rtype==oldresourcetype[R_WOOD]) {
		int avail = rtrees(r);
		assert(norders <= avail);
		rsettrees(r, avail-norders);
		woodcounts(r, norders);
	} else if (rtype==oldresourcetype[R_MALLORN]) {
		int avail = rtrees(r);
		assert(norders <= avail);
		rsettrees(r, avail-norders);
		woodcounts(r, norders*2);
	} else if (rtype==oldresourcetype[R_HORSE]) {
		int avail = rhorses(r);
		assert(norders <= avail);
		rsethorses(r, avail-norders);
	}
}

static void
init_olditems(void)
{
	item_t i;
	resource_type * rtype;

	for (i=0; i!=MAXITEMS; ++i) {
		int iflags = ITF_NONE;
		int rflags = RTF_ITEM;
		int m, n;
		const char * name[2];
		const char * appearance[2];
		int weight = itemdata[i].gewicht;
		int capacity = 0;
		int skill=0, minskill=0; /* skill required for *using* it */
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
		name[0] = reverse_lookup(NULL, itemdata[i].name[0]);
		name[1] = reverse_lookup(NULL, itemdata[i].name[1]);
		appearance[0] = reverse_lookup(NULL, itemdata[i].name[2]);
		appearance[1] = reverse_lookup(NULL, itemdata[i].name[3]);
		rtype = new_resourcetype(name, appearance, rflags);
		itype = new_itemtype(rtype, iflags, weight, capacity, minskill, skill);

		switch (i) {
		case I_HORSE:
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
				rdata->use = use_oldtypes;
			}
			break;
		}
		itype->construction = con;
		olditemtype[i] = itype;
		oldresourcetype[item2res(i)] = rtype;
	}
}

static void
init_items(void) {
	init_olditems();
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
	for (h=0;h!=MAXHERBS;++h) {
		item_type * itype;
		terrain_t t;
		resource_type * rtype;

		names[0] = reverse_lookup(NULL, herbdata[0][h]);
		names[1] = reverse_lookup(NULL, herbdata[1][h]);

		rtype = new_resourcetype(names, appearance, RTF_ITEM);
		itype = new_itemtype(rtype, ITF_HERB, 0, 0, 0, NOSKILL);

		t = (terrain_t)(h/3+1);
#ifdef NO_FOREST
		if (t>T_PLAIN) --t;
#endif
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
	"den Pferden zu trinken, auf daß sich ca. 50 Pferde besser vermehren.",

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

void
init_oldpotions(void)
{
	potion_t p;
	const char * names[2];
	const char * appearance[2] = { "vial", "vials" };
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
		con->maxsize = 1;
		con->reqsize = 1;

		names[0] = reverse_lookup(NULL, potionnames[0][p]);
		names[1] = reverse_lookup(NULL, potionnames[1][p]);

		rtype = new_resourcetype(names, appearance, RTF_ITEM);
		if (p==P_FOOL) rtype->flags |= RTF_SNEAK;
		oldresourcetype[potion2res(p)] = rtype;
		itype = new_itemtype(rtype, ITF_POTION, 0, 0, 0, NOSKILL);
		itype->construction = con;
		itype->use = use_potion;
		oldpotiontype[p] = new_potiontype(itype, (terrain_t)p/3);
		oldpotiontype[p]->level = potionlevel[p];
		oldpotiontype[p]->text = potiontext[p];
	}
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

static const char * names[] = { "money", "moneys", "person", "persons", "permaura", "permauras", "hp", "hps", "peasant", "peasants", "aura", "auras", "unit", "units" };

void
init_resources(void)
{
	/* silver was never an item: */
	r_silver = new_resourcetype(&names[0], NULL, RTF_ITEM);
	i_silver = new_itemtype(r_silver, ITF_NONE, 1/*weight*/, 0, 0, NOSKILL);
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
	init_items();
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

int set_money(unit * u, int v)
{
	item ** ip = &u->items;
	while (*ip && (*ip)->type!=i_silver) ip = &(*ip)->next;
	if ((*ip)==NULL && v) {
		i_add(&u->items, i_new(i_silver))->number = v;
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
		i_add(&u->items, i_new(i_silver))->number = v;
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
findresourcetype(const char * name, const locale * lang)
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
	i = findtoken(&rn->names, name);
	if (i==E_TOK_NOMATCH) return NULL;
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
finditemtype(const char * name, const locale * lang)
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
	i = findtoken(&in->names, name);
	if (i==E_TOK_NOMATCH) return NULL;
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
	fprintf(F, "%d;minskill\n", it->minskill);
	fprintf(F, "%d;skill\n", it->skill);
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
	int i = fscanf(F, "%d\n", &hash);
	if (i==0 || i==EOF) {
		free(rt);
		return NULL;
	}
	a_read(F, &rt->attribs); /* scheisse, weil nicht CR. */
	for (;;) {
		char * semi = buf;
		char * s = NULL;
		int i = 0;
		fgets(buf, 1024, F);
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
	assert(rt->hashkey==hash);
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
		fgets(buf, 1024, F);
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
			if (!strcmp(semi, "give")) it->give = (int (*)(const unit*, const unit*, const struct item_type *, int, const char *))get_function(s);
			break;
		case 'm':
			if (!strcmp(semi, "minskill")) it->minskill=i;
			break;
		case 's':
			if (!strcmp(semi, "skill")) it->skill=i;
			break;
		case 'u':
			if (!strcmp(semi, "use")) it->use = (int (*)(unit *, const struct item_type *, const char *))get_function(s);
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
		return "Aura";
	} else if (res == R_PERMAURA) {
		return "permanente Aura";
	} else if (res == R_PEASANTS) {
		return index==1?"Bauer":"Bauern";
	} else if (res == R_UNIT) {
		return index==1?"Einheit":"Einheiten";
	} else if (res == R_PERSON) {
		return index==1?"Person":"Personen";
	} else if (res == R_HITPOINTS) {
		return index==1?"Trefferpunkt":"Trefferpunkte";
	}
	return NULL;
}

