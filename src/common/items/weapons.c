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

#include "catapultammo.h"

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
	WP_CROSSBOW,
	WP_SPEAR,
	WP_GREATSWORD,
	WP_SWORD,
	WP_AXE,
	WP_LANCE,
	WP_RUSTY_SWORD,
	WP_RUSTY_GREATSWORD,
	WP_RUSTY_AXE,
	WP_RUSTY_HALBERD,
	WP_NONE,
	WP_MAX
};

enum {
	RL_CATAPULT,
	RL_CROSSBOW,
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
	{0.00, "3d10+10", "3d10+10", I_RUNESWORD, SK_SWORD, 2, 2, false, true, { RL_NONE, 0}, CUT },
	/* Flammenschwert */
	{0.30, "3d6+10", "3d6+10", I_FIRESWORD, SK_SWORD, 1, 1, false, false, { RL_NONE, 0}, CUT },
	/* Laenschwert */
	{0.30, "3d6+10", "3d6+10", I_LAENSWORD, SK_SWORD, 1, 1, false, false, { RL_NONE, 0}, CUT },
	/* Katapult */
	{0.00, "3d10+5", "3d10+5", I_CATAPULT, SK_CATAPULT, 0, 0, true, false, { RL_CATAPULT, 5 }, BASH },
	/* Langbogen */
	{0.00, "1d11+1", "1d11+1", I_LONGBOW, SK_LONGBOW, 0, 0, true, false, { RL_NONE, 0 }, PIERCE },
	/* Armbrust */
#if CHANGED_CROSSBOWS == 1
	{0.00, "3d3+5", "3d3+5", I_CROSSBOW, SK_CROSSBOW, 0, 0, true, false, { RL_CROSSBOW, 2 }, PIERCE | ARMORPIERCING },
#else
	{0.00, "3d3+5", "3d3+5", I_CROSSBOW, SK_CROSSBOW, 0, 0, true, false, { RL_CROSSBOW, 1 }, PIERCE },
#endif
	/* Speer */
	{0.00, "1d10+0", "1d12+2", I_SPEAR, SK_SPEAR, 0, 0, false, false, { RL_NONE, 0}, PIERCE },
	/* Zweihänder */
	{0.00, "2d8+3", "2d8+3", I_GREATSWORD, SK_SWORD, -1, -2, false, false, { RL_NONE, 0}, CUT },
	/* Schwert */
	{0.00, "1d9+2", "1d9+2", I_SWORD, SK_SWORD, 0, 0, false, false, { RL_NONE, 0}, CUT },
	/* Kriegsaxt */
	{0.00, "2d6+4", "2d6+4", I_AXE, SK_SWORD, 1, -2, false, false, { RL_NONE, 0}, CUT },
	/* Lanze */
	{0.00, "1d5", "2d6+5", I_LANCE, SK_SPEAR, 0, -2, false, false, { RL_NONE, 0}, PIERCE },
	/* Rostiges Schwert */
	{0.00, "1d9", "1d9", I_RUSTY_SWORD, SK_SWORD, -1, -1, false, false, { RL_NONE, 0}, CUT },
	/* Rostiger Zweihänder */
	{0.00, "2d8", "2d8", I_RUSTY_GREATSWORD, SK_SWORD, -2, -3, false, false, { RL_NONE, 0}, CUT },
	/* Rostige Axt */
	{0.00, "2d6", "2d6", I_RUSTY_AXE, SK_SWORD, 0, -3, false, false, { RL_NONE, 0}, CUT },
	/* Rostige Hellebarde */
	{0.00, "2d6", "2d6", I_RUSTY_HALBERD, SK_SPEAR, -2, 1, false, false, { RL_NONE, 0}, CUT },
	/* Unbewaffnet */
	{0.00, "1d5+0", "1d6+0", I_WOOD, SK_SWORD, 0, 0, false, false, { RL_NONE, 0}, BASH },
	/* Dummy */
	{0.00, "0d0+0", "0d0+0", I_WOOD, SK_SWORD, 0, 0, false, false, { RL_NONE, 0}, 0 }
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
		enemies = count_enemies(fi->side->battle, fi->side, minrow, maxrow);
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
		dt = select_enemy(fi->side->battle, fi, minrow, maxrow);
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
	
	assert(row>=FIGHT_ROW);
	if (row>BEHIND_ROW) return false; /* keine weiteren attacken */
	
	assert(wp->type->itype==olditemtype[I_CATAPULT]);
	assert(af->person[at->index].reload==0);

#if CATAPULT_AMMUNITION
	if(new_get_pooled(au, &rt_catapultammo, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK) <= 0) {
		/* No ammo. Use other weapon if available. */
		return true;
	}
#endif

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

	n = min(CATAPULT_ATTACKS, count_enemies(b, af->side, minrow, maxrow));

#if CATAPULT_AMMUNITION
	new_use_pooled(au, &rt_catapultammo, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, 1);
#endif

	while (--n >= 0) {
		/* Select defender */
		dt = select_enemy(b, af, minrow, maxrow);
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
	}
}

/** begin mallornspear **/
resource_type rt_mallornspear = {
	{ "mallornspear", "mallornspear_p" },
	{ "mallornspear", "mallornspear_p" },
	RTF_ITEM,
	&res_changeitem
};
static requirement mat_mallornspear[] = {
	{I_MALLORN, 1},
	{0, 0}
};
static construction cons_mallornspear = {
	SK_WEAPONSMITH, 5,       /* skill, minskill */
	-1, 1, mat_mallornspear   /* maxsize, reqsize, materials */
};
item_type it_mallornspear = {
	&rt_mallornspear,        /* resourcetype */
	ITF_WEAPON, 100, 0,      /* flags, weight, capacity */
	&cons_mallornspear       /* construction */
};
weapon_type wt_mallornspear = {
	&it_mallornspear,        /* item_type */
	{ "1d10+1", "1d12+3" },  /* on foot, on horse */
	WTF_PIERCE,              /* flags */
	SK_SPEAR, 5,             /* skill, minskill */
	0, 0, 0.15, 0,           /* offmod, defmod, magres, reload */
	wm_spear                 /* modifiers */
};
/** end mallornspear **/

/** begin mallornlance **/
resource_type rt_mallornlance = {
	{ "mallornlance", "mallornlance_p" },
	{ "mallornlance", "mallornlance_p" },
	RTF_ITEM,
	&res_changeitem
};
static requirement mat_mallornlance[] = {
	{I_MALLORN, 2},
	{0, 0}
};
static construction cons_mallornlance = {
	SK_WEAPONSMITH, 5,       /* skill, minskill */
	-1, 1, mat_mallornlance   /* maxsize, reqsize, materials */
};
item_type it_mallornlance = {
	&rt_mallornlance,        /* resourcetype */
	ITF_WEAPON, 100, 0,      /* flags, weight, capacity */
	&cons_mallornlance       /* construction */
};
weapon_type wt_mallornlance = {
	&it_mallornlance,        /* item_type */
	{ "1d5+1", "2d6+2" },    /* on foot, on horse */
	WTF_PIERCE,              /* flags */
	SK_SPEAR, 5,             /* skill, minskill */
	0, 0, 0.15, 0,            /* offmod, defmod, magres, reload */
	wm_lance                 /* modifiers */
};
/** end mallornlance **/

/** begin mallorncrossbow **/
resource_type rt_mallorncrossbow = {
	{ "mallorncrossbow", "mallorncrossbow_p" },
	{ "mallorncrossbow", "mallorncrossbow_p" },
	RTF_ITEM,
	&res_changeitem
};
static requirement mat_mallorncrossbow[] = {
	{I_MALLORN, 1},
	{0, 0}
};
static construction cons_mallorncrossbow = {
	SK_WEAPONSMITH, 5,          /* skill, minskill */
	-1, 1, mat_mallorncrossbow  /* maxsize, reqsize, materials */
};
item_type it_mallorncrossbow = {
	&rt_mallorncrossbow,   /* resourcetype */
	ITF_WEAPON, 100, 0,    /* flags, weight, capacity */
	&cons_mallorncrossbow  /* construction */
};
weapon_type wt_mallorncrossbow = {
	&it_mallorncrossbow,    /* item_type */
	{ "3d3+5", "3d3+5" },   /* on foot, on horse */
	WTF_MISSILE|WTF_PIERCE, /* flags */
	SK_CROSSBOW, 5,         /* skill, minskill */
	0, 0, 0.15, 0            /* offmod, defmod, magres, reload */
};
/** end mallorncrossbow **/

void
register_weapons(void) {
	rt_register(&rt_mallornspear);
	it_register(&it_mallornspear);
	wt_register(&wt_mallornspear);

	rt_register(&rt_mallornlance);
	it_register(&it_mallornlance);
	wt_register(&wt_mallornlance);

	rt_register(&rt_mallorncrossbow);
	it_register(&it_mallorncrossbow);
	wt_register(&wt_mallorncrossbow);

	register_function((pf_generic)attack_catapult, "attack_catapult");
	register_function((pf_generic)attack_firesword, "attack_firesword");
}

void
init_weapons(void)
{
	init_oldweapons();
}
