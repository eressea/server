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
#include <eressea.h>
#include "weapons.h"

#include <unit.h>
#include <build.h>
#include <race.h>
#include <item.h>
#include <battle.h>
#include <pool.h>

/* util includes */
#include <functions.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

static weapon_mod wm_bow[] = {
	{ 2, WMF_MISSILE_TARGET },
	{ 0, 0 }
};

static weapon_mod wm_catapult[] = {
	{ 4, WMF_MISSILE_TARGET },
	{ 0, 0 }
};

static weapon_mod wm_spear[] = {
	{ 1, WMF_SKILL|WMF_RIDING|WMF_AGAINST_ANYONE|WMF_OFFENSIVE },
	{ 1, WMF_SKILL|WMF_WALKING|WMF_AGAINST_RIDING|WMF_DEFENSIVE },
	{ 0, 0 }
};

static weapon_mod wm_lance[] = {
	{ 1, WMF_SKILL|WMF_RIDING|WMF_AGAINST_ANYONE|WMF_OFFENSIVE },
	{ 0, 0 }
};

enum {
	WP_RUNESWORD,
	WP_FIRESWORD,
	WP_EOGSWORD,
	WP_CATAPULT,
	WP_LONGBOW,
	WP_SPEAR,
	WP_LANCE,
	WP_NONE,
	WP_MAX
};

enum {
	RL_CATAPULT,
	RL_MAX,
	RL_NONE
};

/* damage types */

#define CUT           (1<<0)
#define PIERCE        (1<<1)
#define BASH          (1<<2)
#define ARMORPIERCING (1<<3)

typedef struct weapondata {
	double magres;
	const char *damfoot;
	const char *damhorse;
	item_t item;
	skill_t skill;
	char attmod;
	char defmod;
	boolean rear;
	boolean is_magic;
	struct reload {
		int type;
		char time;
	} reload;
	char damage_type;
} weapondata;

static weapondata weapontable[WP_MAX + 1] =
/* MagRes, Schaden/Fuß, Schaden/Pferd, Item, Skill, OffMod, DefMod,
 * missile, is_magic */
{
	/* Runenschwert */
	{0.00, "3d10+10", "3d10+10", I_RUNESWORD, SK_MELEE, 2, 2, false, true, { RL_NONE, 0}, CUT },
	/* Flammenschwert */
	{0.30, "3d6+10", "3d6+10", I_FIRESWORD, SK_MELEE, 1, 1, false, false, { RL_NONE, 0}, CUT },
	/* Laenschwert */
	{0.30, "3d6+10", "3d6+10", I_LAENSWORD, SK_MELEE, 1, 1, false, false, { RL_NONE, 0}, CUT },
	/* Katapult */
	{0.00, "3d10+5", "3d10+5", I_CATAPULT, SK_CATAPULT, 0, 0, true, false, { RL_CATAPULT, 5 }, BASH },
	/* Langbogen */
	{0.00, "1d11+1", "1d11+1", I_LONGBOW, SK_LONGBOW, 0, 0, true, false, { RL_NONE, 0 }, PIERCE },
	/* Speer */
	{0.00, "1d10+0", "1d12+2", I_SPEAR, SK_SPEAR, 0, 0, false, false, { RL_NONE, 0}, PIERCE },
	/* Lanze */
	{0.00, "1d5", "2d6+5", I_LANCE, SK_SPEAR, 0, -2, false, false, { RL_NONE, 0}, PIERCE },
	/* Unbewaffnet */
	{0.00, "1d5+0", "1d6+0", I_WOOD, SK_MELEE, 0, 0, false, false, { RL_NONE, 0}, BASH },
	/* Dummy */
	{0.00, "0d0+0", "0d0+0", I_WOOD, SK_MELEE, 0, 0, false, false, { RL_NONE, 0}, 0 }
};

weapon_type * oldweapontype[WP_MAX];

static boolean
attack_firesword(const troop * at, int *casualties, int row)
{
	fighter *fi = at->fighter;
	troop dt;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = FIGHT_ROW;
	int enemies = 0;
	int killed = 0;
	const char *damage = "2d8";
	int force  = 1+rand()%10;

	if (row==FIGHT_ROW) {
		enemies = count_enemies(fi->side->battle, fi->side, minrow, maxrow, true);
	}
	if (!enemies) {
		if (casualties) *casualties = 0;
		return true; /* if no enemy found, no use doing standarad attack */
	}

	if (fi->catmsg == -1) {
		int i, k=0;
		for (i=0;i<=at->index;++i) {
			struct weapon * wp = fi->person[i].melee;
			if (wp!=NULL && wp->type == oldweapontype[WP_FIRESWORD]) ++k;
		}
		sprintf(buf, "%d Kämpfer aus %s benutz%s Flammenschwert%s:", k, unitname(fi->unit),
			(k==1)?"t sein ":"en ihre",(k==1)?"":"er");
		battlerecord(fi->side->battle, buf);
		fi->catmsg = 0;
	}

	do {
		dt = select_enemy(fi->side->battle, fi, minrow, maxrow, true);
		assert(dt.fighter);
		--force;
		killed += terminate(dt, *at, AT_SPELL, damage, 1);
	} while (force && killed < enemies);
	if (casualties) *casualties = killed;
	return true;
}

#define CATAPULT_ATTACKS 6

static boolean
attack_catapult(const troop * at, int * casualties, int row)
{
	fighter *af = at->fighter;
	unit *au = af->unit;
	battle * b = af->side->battle;
	troop dt;
	int d = 0, n;
	int minrow, maxrow;
	weapon * wp = af->person[at->index].missile;
  static item_type * it_catapultammo = NULL;
  if (it_catapultammo==NULL) {
    it_catapultammo = it_find("catapultammo");
  }
	
	assert(row>=FIGHT_ROW);
	if (row>BEHIND_ROW) {
    /* probiere noch weitere attacken, kann nicht schiessen */
    return true;
	}
	assert(wp->type->itype==olditemtype[I_CATAPULT]);
	assert(af->person[at->index].reload==0);

  if (it_catapultammo!=NULL) {
    if (new_get_pooled(au, it_catapultammo->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK) <= 0) {
      /* No ammo. Use other weapon if available. */
      return true;
    }
  }

	if (af->catmsg == -1) {
		int i, k=0;
		for (i=0;i<=at->index;++i) {
			if (af->person[i].reload==0 && af->person[i].missile == wp) ++k;
		}
		sprintf(buf, "%d Kämpfer aus %s feuer%s Katapult ab:", k, unitname(au), (k==1)?"t sein":"n ihr");
		battlerecord(b, buf);
		af->catmsg = 0;
	}
	minrow = FIGHT_ROW;
	maxrow = FIGHT_ROW;

	n = min(CATAPULT_ATTACKS, count_enemies(b, af->side, minrow, maxrow, true));

  if (it_catapultammo!=NULL) {
    new_use_pooled(au, it_catapultammo->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 1);
  }

	while (--n >= 0) {
		/* Select defender */
		dt = select_enemy(b, af, minrow, maxrow, true);
		if (!dt.fighter)
			break;

		/* If battle succeeds */
		if (hits(*at, dt, wp)) {
			d += terminate(dt, *at, AT_STANDARD, wp->type->damage[0], true);
#ifdef CATAPULT_STRUCTURAL_DAMAGE
			if (dt.fighter->unit->building && rand()%100 < 5) {
				damage_building(b, dt.fighter->unit->building, 1);
			} else if (dt.fighter->unit->ship && rand()%100 < 5) {
				dt.fighter->unit->ship->damage+=DAMAGE_SCALE;
			}
#endif
		}
	}

	if (casualties) *casualties = d;
	return false; /* keine weitren attacken */
}

static void
init_oldweapons(void)
{
	int w;
	for (w=0;w!=WP_MAX && weapontable[w].item!=I_WOOD;++w) {
		const char * damage[2];
		item_type * itype = olditemtype[weapontable[w].item];
		int minskill = 1, wflags = WTF_NONE;
    int m;
		weapon_mod * modifiers = NULL;
		boolean (*attack)(const troop *, int * deaths, int row) = NULL;

		switch (w) {
		case WP_RUNESWORD:
		case WP_FIRESWORD:
			attack = attack_firesword;
			minskill = 7;
			break;
		case WP_LANCE:
			modifiers = wm_lance;
			break;
		case WP_CATAPULT:
			modifiers = wm_catapult;
			attack = attack_catapult;
			break;
		case WP_SPEAR:
			modifiers = wm_spear;
			break;
		case WP_LONGBOW:
			modifiers = wm_bow;
			break;
		}

		assert(itype!=NULL || !"item not initialized");

		itype->flags |= ITF_WEAPON;

		if (weapontable[w].rear) wflags |= WTF_MISSILE;
		if (weapontable[w].is_magic) wflags |= WTF_MAGICAL;
		if (weapontable[w].damage_type & CUT) wflags |= WTF_CUT;
		if (weapontable[w].damage_type & PIERCE) wflags |= WTF_PIERCE;
		if (weapontable[w].damage_type & BASH) wflags |= WTF_BLUNT;
		if (weapontable[w].damage_type & ARMORPIERCING) wflags |= WTF_ARMORPIERCING;

		damage[0] = weapontable[w].damfoot;
		damage[1] = weapontable[w].damhorse;

		oldweapontype[w] = new_weapontype(itype, wflags, weapontable[w].magres, damage, weapontable[w].attmod, weapontable[w].defmod, weapontable[w].reload.time, weapontable[w].skill, minskill);
		oldweapontype[w]->modifiers = modifiers;
		oldweapontype[w]->attack = attack;

#ifdef SCORE_MODULE
    if (itype->construction->materials==NULL) {
      itype->score = 6000;
    } else for (m=0;itype->construction->materials[m].number;++m) {
      const resource_type * rtype = itype->construction->materials[m].rtype;
      int score = rtype->itype?rtype->itype->score:5;
      itype->score = 2*itype->construction->materials[m].number * score;
    }
#endif
  }
}

void
register_weapons(void) {
	register_function((pf_generic)attack_catapult, "attack_catapult");
	register_function((pf_generic)attack_firesword, "attack_firesword");
}

void
init_weapons(void)
{
	init_oldweapons();
}
