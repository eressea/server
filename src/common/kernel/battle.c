/* vi: set ts=2:
 *
 *	$Id: battle.c,v 1.10 2001/02/10 19:24:05 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#define SHOW_KILLS

#define TACTICS_RANDOM 5 /* define this as 1 to deactivate */
#define CATAPULT_INITIAL_RELOAD 4 /* erster schuss in runde 1 + rand() % INITIAL */
#define CATAPULT_STRUCTURAL_DAMAGE


typedef enum combatmagic {

	DO_PRECOMBATSPELL,

	DO_POSTCOMBATSPELL

} combatmagic_t;


#include <config.h>
#include "eressea.h"
#include "battle.h"

#include "item.h"
#include "alchemy.h"
#include "plane.h"
#include "magic.h"
#include "faction.h"
#include "reports.h"
#include "build.h"
#include "race.h"
#include "movement.h"
#include "movement.h"
#include "names.h"
#include "region.h"
#include "skill.h"
#include "goodies.h"
#include "unit.h"
#include "message.h"
#include "curse.h"
#include "spell.h"
#include "karma.h"
#include "ship.h"
#include "building.h"
#ifdef GROUPS
# include "group.h"
#endif

/* util includes */
#include <base36.h>
#include <cvector.h>
#include <rand.h>

/* attributes includes */
#include <attributes/key.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef _MSC_VER
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif


#ifdef HAVE_ZLIB
# include <zlib.h>
# define dbgprintf(a) gzprintf a;
gzFile bdebug;
#elif HAVE_BZ2LIB
# include <bzlib.h>
# define dbgprintf(a) bz2printf a;
BZFILE *bdebug;
#else
# define dbgprintf(a) fprintf a;
FILE *bdebug;
#endif

/* TODO: header cleanup */
extern int dice_rand(const char *s);
extern item_type it_demonseye;

int  obs_count = 0;
boolean nobattledebug = false;

#define TACTICS_MALUS
#undef  MAGIC_TURNS

#define MINSPELLRANGE 1
#define MAXSPELLRANGE 7

#define COMBATEXP         0

#define COMBAT_TURNS			10

static const double EFFECT_PANIC_SPELL = 0.25;
static const double TROLL_REGENERATION = 0.10;

#define MAX_ADVANTAGE           5

enum {
	SI_DEFENDER,
	SI_ATTACKER
};

extern weapon_type * oldweapontype[];

/* Nach dem alten System: */
static int missile_range[2] = {FIGHT_ROW, BEHIND_ROW};
static int melee_range[2] = {FIGHT_ROW, FIGHT_ROW};

typedef struct armor_type {
	double penalty;
	double magres;
	int prot;
	char shield;
	char item;
} armor_type;

static armor_type armordata[AR_NONE + 1] =
/* penalty; prot; shield; item; */
{
#ifdef COMPATIBILITY
	{-0.80, 5, 0, I_CLOAK_OF_INVULNERABILITY },
#endif
	{ 0.30, 0.00, 5, 0, I_PLATE_ARMOR},
	{ 0.15, 0.00, 3, 0, I_CHAIN_MAIL},
	{ 0.30, 0.00, 3, 0, I_RUSTY_CHAIN_MAIL},
	{-0.15, 0.00, 1, 1, I_SHIELD},
	{ 0.00, 0.00, 1, 1, I_RUSTY_SHIELD},
	{-0.25, 0.30, 2, 1, I_EOGSHIELD},
	{ 0.00, 0.30, 6, 0, I_EOGCHAIN},
	{ 0.00, 0.00, 0, 0, I_SWORD},
	{ 0.00, 0.00, 0, 0, I_SWORD}
};

const troop no_troop = {0, 0};

region *
fleeregion(const unit * u)
{
	const region *r = u->region;
	region *neighbours[MAXDIRECTIONS];
	int c = 0;
	direction_t i;

	if (u->ship && landregion(rterrain(r)))
		return NULL;

	if (u->ship &&
			!(race[u->race].flags & RCF_SWIM) &&
			!(race[u->race].flags & RCF_FLY)) {
		return NULL;
	}

#ifdef FLEE_TO
	/* Hat die Einheit ein NACH, wird die angegebene Richtung bevorzugt */
	if (igetkeyword(u->thisorder) == K_MOVE
				|| igetkeyword(u->thisorder) == K_ROUTE)
	{
		region *r2;
		r2 = movewhere(r, u);
		if (r2) {
			return r2;
		}
	}
#endif

	for (i = 0; i != MAXDIRECTIONS; ++i) {
		region * r2 = rconnect(r, i);
		if (r2) {
			if (can_survive(u,r2) && !move_blocked(u, r, i))
				neighbours[c++] = r2;
		}
	}

	if (!c)
		return NULL;
	return neighbours[rand() % c];
}

static void
brecord(faction * f, region * r, message * m)
{
	if (!f->battles || f->battles->r!=r) {
		struct bmsg * bm = calloc(1, sizeof(struct bmsg));
		bm->next = f->battles;
		f->battles = bm;
		bm->r = r;
	}
	add_message(&f->battles->msgs, m);
}

int
armedmen(const unit * u)
{
	item * itm;
	int n = 0;
	if (!(urace(u)->flags & RCF_NOWEAPONS)) {
		if (urace(u)->ec_flags & CANGUARD) {
			/* kann ohne waffen bewachen: fuer untote und drachen */
			n = u->number;			
		} else {
			/* alle Waffen werden gezaehlt, und dann wird auf die Anzahl
			 * Personen minimiert */
			for (itm=u->items;itm;itm=itm->next) {
				const weapon_type * wtype = resource2weapon(itm->type->rtype);
				if (wtype==NULL) continue;
				if (effskill(u, wtype->skill) >= wtype->minskill) n += itm->number;
				if (n>u->number) break;
			}
			n = min(n, u->number);
		}
	}
	return n;
}

static void
battledebug(const char *s)
{
#if SHOW_DEBUG
	printf("%s\n", translate_regions(s, NULL));
#endif
	if (bdebug) {
		dbgprintf((bdebug, "%s\n", translate_regions(s, NULL)));
	}
}

void
battlerecord(battle * b, const char *s)
{
	bfaction * bf;
#if 0
	static char * lasts = NULL;

	if (lasts!=s) {
		battledebug(s);
		lasts = s;
	}
#endif
	s = gc_add(strdup(s));
	for (bf = b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		message * m = new_message(f, "msg_battle%s:string", s);
		brecord(f, b->region, m);
	}
}

void
battlemsg(battle * b, unit * u, const char * s)
{
	bfaction * bf;

	sprintf(buf, "%s %s", unitname(u), s);
	for (bf=b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		brecord(f, u->region, new_message(f, "msg_battle%s:string", strdup(buf)));
	}
}

static void
fbattlerecord(faction * f, region * r, const char *s)
{
#if 0
	static char * lasts = NULL;

	if (lasts!=s) {
		battledebug(s);
		lasts = s;
	}
#endif
	s = gc_add(strdup(s));
	brecord(f, r, new_message(f, "msg_battle%s:string", s));
}

boolean
enemy(const side * as, const side * ds)
{
	const struct enemy * e = as->enemies[ds->index % 16];
	while (e && e->side!=ds) e=e->nexthash;
	if (e) return true;
	return false;
}

static void
set_enemy(side * as, side * ds, boolean attacking)
{
	struct enemy * e = as->enemies[ds->index % 16];
	while (e && e->side!=ds) e=e->nexthash;
	if (e==NULL) {
		e = calloc(sizeof(struct enemy), 1);
		e->side = ds;
		e->attacking = attacking;
		e->nexthash = as->enemies[ds->index % 16];
		as->enemies[ds->index % 16] = e;

		e = calloc(sizeof(struct enemy), 1);
		e->side = as;
		e->nexthash = ds->enemies[as->index % 16];
		ds->enemies[as->index % 16] = e;
	}
	else e->attacking |= attacking;
}

extern int alliance(const ally * sf, const faction * f, int mode);

static int
allysf(side * s, faction * f)
{
	if (s->bf->faction==f) return true;
#ifdef GROUPS
	if (s->group) return alliance(s->group->allies, f, HELP_FIGHT);
#endif
	return alliance(s->bf->faction->allies, f, HELP_FIGHT);
}

troop
select_corpse(battle * b, fighter * af)
/* Wählt eine Leiche aus, der af hilft. casualties ist die Anzahl der
 * Toten auf allen Seiten (im Array). Wenn af == NULL, wird die
 * Parteizugehörigkeit ignoriert, und irgendeine Leiche genommen. */
{
	troop dt =
	{0, 0};
	int di, maxcasualties = 0;
	fighter *df;
	side *side;

	for_each(side, b->sides) {
		if (!af || (!enemy(af->side, side) && allysf(af->side, side->bf->faction)))
			maxcasualties += side->casualties;
	}
	next(side);

	di = rand() % maxcasualties;
	for_each(df, b->fighters) {
		/* Geflohene haben auch 0 hp, dürfen hier aber nicht ausgewählt
		 * werden! */
		int dead = df->unit->number - (df->alive + df->run_number);
		if (nonplayer(df->unit)) continue;

		if (af && !helping(af->side, df->side))
			continue;
		if (di < dead) {
			dt.fighter = df;
			dt.index = df->unit->number - di;
			break;
		}
		di -= dead;
	}
	next(df);
	return dt;
}

boolean
helping(side * as, side * ds)
{
	if (as->bf->faction==ds->bf->faction) return true;
	return (boolean)(!enemy(as, ds) && allysf(as, ds->bf->faction));
}


/* return the number of live allies warning: this function only considers
 * troops that are still alive, not those that are still fighting although
 * dead. */
int
countallies(side * as)
{
	battle *b = as->battle;
	side *s;
	int count = 0;

	for_each(s, b->sides) {
		if (!helping(as, s)) continue;
		count += s->size[SUM_ROW];
	}
	next(s);
	return count;
}

#ifdef NEW_TACTICS
static int
support(tactics * tac)
{
	return tac ? 100 : 0;
}

static int
tactics_bonus(troop at, troop dt, boolean attacking)
{
	battle *b = dt.fighter->side->battle;
	side *as = at.fighter->side;
	side *ds = dt.fighter->side;
	side *s;
	int abest = 0, dbest = 0;

	for_each(s, b->sides) {
		if (!enemy(s, as) && allysf(s, as->bf->faction)) {
			if (rand() % countallies(as) < support(&as->leader))
				abest = max(s->leader.value, abest);
		} else if (!enemy(s, as) && allysf(s, ds->bf->faction)) {
			if (rand() % countallies(ds) < support(&ds->leader))
				dbest = max(s->leader.value, dbest);
		}
	}
	next(s);
	if (abest > dbest)
		return attacking;
	return 0;
}
#endif /* NEW_TACTICS */

int
get_unitrow(const fighter * af)
{
	static boolean * counted = NULL;
	static size_t csize = 0;

	battle * b = af->side->battle;
	void **si;
	int enemyfront = 0;
	int line, result;
	int retreat = 0;
	int size[NUMROWS];
	int row = af->status + FIRST_ROW;
	int front = 0;
	size_t bsize;

	bsize = cv_size(&b->sides);

	if (csize<bsize) {
		if (counted) free(counted);
		csize=bsize;
		counted = calloc(sizeof(boolean), bsize);
	}
	else memset(counted, 0, bsize*sizeof(boolean));
	memset(size, 0, sizeof(size));
	for (line=FIRST_ROW;line!=NUMROWS;++line) {
		/* how many enemies are there in the first row? */
		for (si = b->sides.begin; si != b->sides.end; ++si) {
			side *s = *si;
			if (s->size[line] && enemy(s, af->side))
			{
				void ** sf;
				enemyfront += s->size[line]; /* - s->nonblockers[line] (nicht, weil angreifer) */
				for (sf = b->sides.begin; sf != b->sides.end; ++sf) {
					side * ally = *sf;
					if (!counted[ally->index] && enemy(s, ally) && !enemy(ally, af->side))
					{
						int i;
						counted[ally->index] = true;
						for (i=0;i!=NUMROWS;++i) size[i] += ally->size[i] - ally->nonblockers[i];
					}
				}
			}
		}
		if (enemyfront) break;
	}
	if (enemyfront) {
		for (line=FIRST_ROW;line!=NUMROWS;++line) {
			front += size[line];
			if (!front || front<enemyfront/10) ++retreat;
			else if (front) break;
		}
	}

	/* every entry in the size[] array means someone trying to defend us.
	 * 'retreat' is the number of rows falling.
	 */
	result = max(FIRST_ROW, row - retreat);

	return result;
}

static int
sort_fighterrow(const fighter ** elem1, const fighter ** elem2)
{
	int a, b;

	a = get_unitrow(*elem1);
	b = get_unitrow(*elem2);
	return (a < b) ? -1 : ((a == b) ? 0 : 1);
}

static void
reportcasualties(battle * b, fighter * fig)
{
	bfaction * bf;
	if (fig->alive == fig->unit->number)
		return;
	fbattlerecord(fig->unit->faction, fig->unit->region, " ");
	for (bf = b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		message * m = new_message(f, "casualties%u:unit%r:runto%i:run%i:alive%i:fallen",
		fig->unit, fig->run_to, fig->run_number, fig->alive, fig->unit->number - fig->alive - fig->run_number);
		brecord(f, fig->unit->region, m);
	}
}


#define BASE_CHANCE  70			/* 70% Überlebenschance */
#define TDIFF_CHANGE 10
static int
contest(int skilldiff, armor_t ar, armor_t sh)
{
	int p, vw = BASE_CHANCE - TDIFF_CHANGE * skilldiff;
	double mod = 1.0;

	/* Hardcodet, muß geändert werden. */

	if (ar != AR_NONE)
		mod *= (1 - armordata[ar].penalty);
	if (sh != AR_NONE)
		mod *= (1 - armordata[sh].penalty);
	vw = (int) (vw * mod);

	do {
		p = rand() % 100;
		vw -= p;
	}
	while (vw > 0 && p >= 90);
	return (vw < 0);
}

static boolean
riding(const troop t) {
	if (t.fighter->building!=NULL) return false;
	if (t.fighter->horses + t.fighter->elvenhorses > t.index) return true;
	return false;
}

static weapon *
select_weapon(const troop t, boolean attacking, boolean missile)
	/* select the primary weapon for this trooper */
{
	weapon * weapon = NULL;

	/* Im Nahkampf: Verteidiger schalten auf eine Waffe, mit der das besser geht, um. */
	if (!attacking && !missile) weapon = t.fighter->person[t.index].secondary;
	/* Wenn keine Nahkampfwaffe vorhanden, oder nicht Nahampf, dann die normale Waffe nehmen: */
	if (weapon==NULL) weapon = t.fighter->person[t.index].weapon;
	/* Wenn keine Waffe, dann NULL==waffenlos */
	if (weapon==NULL) return NULL;
	else return weapon;
}

/* ------------------------------------------------------------- */

static int
weapon_skill(const weapon_type * wtype, const unit * u, boolean attacking)
	/* the 'pure' skill when using this weapon to attack or defend.
	 * only undiscriminate modifiers (not affected by troops or enemies)
	 * are taken into account, e.g. no horses, magic, etc. */
{
	int skill;

	if (wtype==NULL) {
		skill = effskill(u, SK_WEAPONLESS);
		if (skill==0) {
			/* wenn kein waffenloser kampf, dann den rassen-defaultwert */
			if (attacking) {
				skill = race[u->race].at_default;
			} else {
				skill = race[u->race].df_default;
			}
		}
	} else {
		/* changed: if we own a weapon, we have at least a skill of 0 */
		skill = effskill(u, wtype->skill);
		if (skill < wtype->minskill) skill = 0;
		if (attacking) {
			skill += wtype->offmod;
		} else {
			if (fval(wtype, WTF_MISSILE)) skill = 0;
			else {
				skill += wtype->defmod;
			}
		}
	}
	if (attacking) {
		skill += race[u->race].at_bonus;
	} else {
		skill += race[u->race].df_bonus;
	}

	return skill;
}

static int
weapon_effskill(troop t, troop enemy, const weapon * w, boolean attacking, boolean missile)
	/* effektiver Waffenskill während des Kampfes */
{
	/* In dieser Runde alle die Modifier berechnen, die fig durch die
	 * Waffen bekommt. */
	fighter * tf = t.fighter;
	unit * tu = t.fighter->unit;
	int skill;
	const weapon_type * wtype = w?w->type:NULL;

	if (wtype==NULL) {
		/* Ohne Waffe: Waffenlose Angriffe */
		skill = weapon_skill(NULL, tu, attacking);
	} else {
		if (attacking) {
			skill = w->offskill;
		} else {
			skill = w->defskill;
		}
		if (wtype->modifiers) {
			/* Pferdebonus, Lanzenbonus, usw. */
			int m;
			unsigned int flags = WMF_SKILL|(attacking?WMF_OFFENSIVE:WMF_DEFENSIVE);

			if (riding(t)) flags |= WMF_RIDING;
			else flags |= WMF_WALKING;
			if (riding(enemy)) flags |= WMF_AGAINST_RIDING;
			else flags |= WMF_AGAINST_WALKING;

			for (m=0;wtype->modifiers[m].value;++m) {
				if ((wtype->modifiers[m].flags & flags) == flags) {
					skill += wtype->modifiers[m].value;
				}
			}
		}
	}

	/* Burgenbonus, Pferdebonus. Der Waffenabhängige wird aber * nicht hier
	 * gemacht, sondern in weapon_effskill */
	if (riding(t) && (wtype==NULL || !fval(wtype, WTF_MISSILE))) {
		skill += 2;
#ifdef BETA_CODE
		if (wtype) skill = skillmod(urace(tu)->attribs, tu, tu->region, wtype->skill, skill, SMF_RIDING);
#else
		if (tu->race == RC_TROLL) skill--;
#endif
	}

	if (t.index<tf->elvenhorses) {
		/* Elfenpferde: Helfen dem Reiter, egal ob und welche Waffe. Das ist eleganter, und
		 * vor allem einfacher, sonst muß man noch ein WMF_ELVENHORSE einbauen. */
		skill += 2;
	}

	if (skill>0 && !attacking && missile) {
		/*
		 * Wenn ich verteidige, und nicht direkt meinem Feind gegenüberstehe,
		 * halbiert sich mein Skill: (z.B. gegen Fernkämpfer. Nahkämpfer
		 * können mich eh nicht treffen)
		 */
		skill /= 2;
	}
	return skill;
}

static char
select_armor(troop t)
{
	armor_t a = 0;
	int geschuetzt = 0;

	/* Drachen benutzen keine Rüstungen */

	if (!(race[t.fighter->unit->race].battle_flags & BF_EQUIPMENT))
		return AR_NONE;

	do {
		if (armordata[a].shield == 0) {
			geschuetzt += t.fighter->armor[a];
			if (geschuetzt > t.index)	/* unser Kandidat wird geschuetzt */
				return a;
		}
		++a;
	}
	while (a != AR_MAX);

	return AR_NONE;
}

static char
select_shield(troop t)
{
	armor_t a = 0;
	int geschuetzt = 0;

	/* Drachen benutzen keine Rüstungen */

	if (!(race[t.fighter->unit->race].battle_flags & BF_EQUIPMENT))
		return AR_NONE;

	do {
		if (armordata[a].shield == 1) {
			geschuetzt += t.fighter->armor[a];
			if (geschuetzt > t.index)	/* unser Kandidat wird
										 * * * geschuetzt */
				return a;
		}
		++a;
	}
	while (a != AR_MAX);

	return AR_NONE;
}

/* Hier ist zu beachten, ob und wie sich Zauber und Artefakte, die
 * Rüstungschutz geben, addieren.
 * - Artefakt I_TROLLBELT gibt Rüstung +1
 * - Zauber Rindenhaut gibt Rüstung +3
 */
int
select_magicarmor(troop t)
{
	unit *u = t.fighter->unit;
	int geschuetzt = 0;
	int ma = 0;

	geschuetzt = min(get_item(u, I_TROLLBELT), u->number);

	if (geschuetzt > t.index)	/* unser Kandidat wird geschuetzt */
		ma += 1;

	return ma;
}

/* ------------------------------------------------------------- */

/* Sind side ds und Magier des meffect verbündet, dann return 1*/
boolean
meffect_protection(battle * b, meffect * s, side * ds)
{
	if (!s->magician->alive) return false;
	if (s->duration <= 0) return false;
	if (enemy(s->magician->side, ds)) return false;
	if (allysf(s->magician->side, ds->bf->faction)) return true;
	return false;
}

/* Sind side as und Magier des meffect verfeindet, dann return 1*/
boolean
meffect_blocked(battle *b, meffect *s, side *as)
{
	if (!s->magician->alive) return false;
	if (s->duration <= 0) return false;
	if (enemy(s->magician->side, as)) return true;
	return false;
}

/* ------------------------------------------------------------- */
void
rmtroop(troop dt)
{
	fighter *df = dt.fighter;
	side *ds = df->side;

	--df->alive;
	assert(dt.index >= 0 && dt.index < df->unit->number);
	assert(df->alive < df->unit->number);
	df->person[dt.index] = df->person[df->alive - df->removed];
	df->person[df->alive - df->removed].hp = 0;
	--ds->size[SUM_ROW];
	--ds->size[df->status + FIRST_ROW];
	/* z.B. Schattenritter */
	if (race[df->unit->race].battle_flags & BF_NOBLOCK) {
		--ds->nonblockers[SUM_ROW];
		--ds->nonblockers[df->status + 1];
	}
}

void
remove_troop(troop dt)
{
	fighter *df = dt.fighter;
	unit *du = df->unit;

	rmtroop(dt);

	if (!df->alive) {
		switch (du->race) {
		case RC_FIREDRAGON:
			change_item(du, I_DRACHENBLUT, du->number-df->run_number);
			break;
		case RC_DRAGON:
			change_item(du, I_DRACHENBLUT, (du->number-df->run_number) * 4);
			change_item(du, I_DRAGONHEAD, du->number-df->run_number);
			break;
		case RC_WYRM:
			change_item(du, I_DRACHENBLUT, (du->number-df->run_number) * 10);
			change_item(du, I_DRAGONHEAD, du->number-df->run_number);
			break;
		case RC_SEASERPENT:
			change_item(du, I_DRACHENBLUT, (du->number-df->run_number) * 6);
			change_item(du, I_SEASERPENTHEAD, du->number-df->run_number);
			break;

		}
	}
}

/* ------------------------------------------------------------- */
void
drain_exp(unit *u, int n)
{
	skill_t sk = (skill_t)(rand() % MAXSKILLS);
	skill_t ssk;

	ssk = sk;

	while(get_skill(u, sk) <= 0) {
		sk++;
		if (sk == MAXSKILLS)
			sk = 0;
		if (sk == ssk) {
			sk = NOSKILL;
			break;
		}
	}
	if (sk != NOSKILL) {
		n = min(n, get_skill(u, sk));
		change_skill(u, sk, -n);
	}
}

/* ------------------------------------------------------------- */

const char *
rel_dam(int dam, int hp)
{
	double q = (double)dam/(double)hp;

	if (q > 0.75) {
		return "eine klaffende Wunde";
	} else if (q > 0.5) {
		return "eine schwere Wunde";
	} else if (q > 0.25) {
		return "eine Wunde";
	}
	return "eine kleine Wunde";
}

boolean
terminate(troop dt, troop at, int type, const char *damage, boolean missile)
{
	item ** pitm;
	fighter *df = dt.fighter;
	fighter *af = at.fighter;
	unit *au = af->unit;
	unit *du = df->unit;
	battle *b = df->side->battle;
	int heiltrank = 0;
	int faerie_level;
	char debugbuf[512];
	char smallbuf[512];
	double kritchance;

	/* Schild */
	void **si;
	side *ds = df->side;
	int hp;

	int ar, an, am;
	int armor = select_armor(dt);
	int shield = select_shield(dt);

	int da = dice_rand(damage);
	int rda, sk = 0, sd;
	boolean magic = false;

	const weapon_type *dwtype = NULL;
	const weapon_type *awtype = NULL;
	const weapon * weapon;

	switch (type) {
	case AT_STANDARD:
		weapon = select_weapon(at, true, true);
		sk = weapon_effskill(at, dt, weapon, true, missile);
		if (weapon) awtype = weapon->type;
		if (awtype && fval(awtype, WTF_MAGICAL)) magic = true;
		break;
	case AT_NATURAL:
		sk = weapon_effskill(at, dt, NULL, true, missile);
		break;
	case AT_SPELL:
	case AT_COMBATSPELL:
		magic = true;
		break;
	default:
		break;
	}
	weapon = select_weapon(dt, false, true); /* missile=true to get the unmodified best weapon she has */
	sd = weapon_effskill(dt, at, weapon, false, false);
	if (weapon!=NULL) dwtype=weapon->type;

	/* Magischer Schaden durch Sprüche oder magische Waffen? */

	ar = armordata[armor].prot;
	ar += armordata[shield].prot;
	/* natürliche Rüstung */
	an = race[du->race].armor;
	/* magische Rüstung durch Artefakte oder Sprüche */
	am = select_magicarmor(dt);

	/* natürliche Rüstung ist halbkumulativ */
	if (ar>0) {
		ar += an/2;
	} else {
		ar = an;
	}
	ar += am;

	if (type!=AT_COMBATSPELL && type!=AT_SPELL) /* Kein Zauber, normaler Waffenschaden */
	{
		kritchance = max((sk * 3 - sd) / 200.0, 0.005);

		while (chance(kritchance)) {
			sprintf(debugbuf,
				"%s/%d landet einen kritischen Treffer", unitid(au), at.index);
			battledebug(debugbuf);
			da += dice_rand(damage);
		}

		switch (af->unit->race) {
		case RC_ELF:
			if (awtype!=NULL && fval(awtype, WTF_BOW)) da++;
			break;
		case RC_HALFLING:
			if (awtype!=NULL && dragon(du)) {
				da += 5;
			}
			break;
		}

		da += jihad(au->faction, du->race);

		faerie_level = fspecial(du->faction, FS_FAERIE);
		if (type == AT_STANDARD && faerie_level) {
			int c;

			for (c=0;weapon->type->itype->construction->materials[c].number; c++) {
				if(weapon->type->itype->construction->materials[c].type == R_IRON) {
					da += faerie_level;
					break;
				}
			}
		}

		if (awtype!=NULL && fval(awtype, WTF_MISSILE)) {
			/* Fernkampfschadenbonus */
			da += af->person[at.index].damage_rear;
		} else if (awtype==NULL) {
			/* Waffenloser kampf, bonus von talentwert*/
			da += effskill(au, SK_WEAPONLESS);
		} else {
			/* Nahkampfschadensbonus */
			da += af->person[at.index].damage;
		}

		/* Skilldifferenzbonus */
		da += max(0, sk-sd);
	}


	if (magic) /* Magischer Schaden durch Spruch oder magische Waffe */
	{
		double res = 1.0;

		/* magic_resistance gib x% Resistenzbonus zurück */
		res -= (double)((double)magic_resistance(du)/100.0)*3.0;

		if (race[du->race].battle_flags & BF_EQUIPMENT) {
#ifdef TODO_RUNESWORD
			/* Runenschwert gibt im Kampf 80% Resistenzbonus */
			if (dwp == WP_RUNESWORD) res -= 0.80;
#endif
			/* der Effekt von Laen steigt nicht linear */
			if (armor == AR_EOGCHAIN) res *= (1-armordata[armor].magres);
			if (shield == AR_EOGSHIELD) res *= (1-armordata[shield].magres);
			if (dwtype) res *= (1-dwtype->magres);
		}

		if (res > 0) {
			da = (int) (max(da * res, 0));
		}
		/* gegen Magie wirkt nur natürliche und magische Rüstung */
		ar = an+am;
	}

	rda = max(da - ar,0);

	if ((race[du->race].battle_flags & BF_INV_NONMAGIC) && !magic) rda = 0;
	else {
		unsigned int i = 0;
		if (race[du->race].battle_flags & BF_RES_PIERCE) i |= WTF_PIERCE;
		if (race[du->race].battle_flags & BF_RES_CUT) i |= WTF_CUT;
		if (race[du->race].battle_flags & BF_RES_BASH) i |= WTF_BLUNT;

		if (i && awtype && fval(awtype, i)) rda /= 2;

		/* Schilde */
		for (si = b->meffects.begin; si != b->meffects.end; ++si) {
			meffect *meffect = *si;
			if (meffect_protection(b, meffect, ds) != 0) {
				assert(0 <= rda); /* rda sollte hier immer mindestens 0 sein */
				/* jeder Schaden wird um effect% reduziert bis der Schild duration
				 * Trefferpunkte aufgefangen hat */
				if (meffect->typ == SHIELD_REDUCE) {
					hp = rda * (meffect->effect/100);
					rda -= hp;
					meffect->duration -= hp;
				}
				/* gibt Rüstung +effect für duration Treffer */
				if (meffect->typ == SHIELD_ARMOR) {
					rda = max(rda - meffect->effect, 0);
					meffect->duration--;
				}
			}
		}
	}

	sprintf(debugbuf, "Verursacht %dTP, Rüstung %d: %d -> %d HP",
		da, ar, df->person[dt.index].hp, df->person[dt.index].hp - rda);

	if (b->small) {
		if (rda > 0) {
			sprintf(smallbuf, "Der Treffer verursacht %s",
				rel_dam(rda, df->person[dt.index].hp));
		} else {
			sprintf(smallbuf, "Der Treffer verursacht keinen Schaden");
		}
	}

	assert(dt.index<du->number);
	df->person[dt.index].hp -= rda;

	if (df->person[dt.index].hp > 0) {	/* Hat überlebt */
		battledebug(debugbuf);
		if (au->race == RC_DAEMON) {
#ifdef TODO_RUNESWORD
			if (select_weapon(dt, 0, -1) == WP_RUNESWORD) continue;
#endif
			if (!(df->person[dt.index].flags & FL_HERO)) {
				df->person[dt.index].flags |= FL_DAZZLED;
				df->person[dt.index].defence--;
			}
		}
		df->person[dt.index].flags = (df->person[dt.index].flags & ~FL_SLEEPING);
		if (b->small) {
			strcat(smallbuf, ".");
			battlerecord(b, smallbuf);
		}
		return false;
	}
#ifdef SHOW_KILLS
	++at.fighter->kills;
#endif

	if (b->small) {
		strcat(smallbuf, ", die tödlich ist");
	}

	/* Sieben Leben */
	if (du->race == RC_CAT && (chance(1.0 / 7))) {
		if (b->small) {
			strcat(smallbuf, ", doch die Katzengöttin ist gnädig");
			battlerecord(b, smallbuf);
		} else {
			/* battlemsg(b, du, "verbraucht eins der 7 Leben einer Katze."); */
		}
		assert(dt.index>=0 && dt.index<du->number);
		df->person[dt.index].hp = unit_max_hp(du);
		return false;
	}

	/* Heiltrank schluerfen und hoffen */
	if (get_effect(du, oldpotiontype[P_HEAL]) > 0) {
		change_effect(du, oldpotiontype[P_HEAL], -1);
		heiltrank = 1;
	} else if (i_get(du->items, oldpotiontype[P_HEAL]->itype) > 0) {
		i_change(&du->items, oldpotiontype[P_HEAL]->itype, -1);
		change_effect(du, oldpotiontype[P_HEAL], 3);
		heiltrank = 1;
	}
	if (heiltrank && (chance(0.50))) {
		if (b->small) {
			strcat(smallbuf, ", doch ein Heiltrank bringt Rettung");
			battlerecord(b, smallbuf);
		} else {
			battlemsg(b, du, "konnte durch einen Heiltrank überleben.");
		}
		assert(dt.index>=0 && dt.index<du->number);
		df->person[dt.index].hp = race[du->race].hitpoints;
		return false;
	}

	strcat(debugbuf, ", tot");
	battledebug(debugbuf);
	if (b->small) {
		strcat(smallbuf, ".");
		battlerecord(b, smallbuf);
	}
	for (pitm=&du->items; *pitm; pitm=&(*pitm)->next) {
		const item_type * itype = (*pitm)->type;
		if (!itype->flags & ITF_CURSED && dt.index < (*pitm)->number) {
			/* 25% Grundchance, das ein Item kaputtgeht. */
			if (rand() % 4 < 1) i_change(pitm, itype, -1);
		}
	}
	remove_troop(dt);

	return true;
}

int
lovar(int n)
{
	assert(n > 0);
	if (n == 1) return rand()%2;
	n /= 2;
	return (rand() % n + 1) + (rand() % n + 1);
}

/* ------------------------------------------------------------- */
int
count_enemies(side * as, int mask, int minrow, int maxrow)
/* new implementation of count_enemies ignores mask, since it was never used */
{
	battle *b = as->battle;
	int i = 0;
	void **si;

	for (si = b->sides.begin; si != b->sides.end; ++si) {
		side *side = *si;
		if (enemy(side, as))
		{
			void **fi;

			for (fi = side->fighters.begin; fi != side->fighters.end; ++fi) {
				fighter *fig = *fi;
				int row;

				if (fig->alive - fig->removed <= 0)
					continue;
				else
					row = get_unitrow(fig);

				if (row >= minrow && row <= maxrow)
					i += fig->alive - fig->removed;
			}
		}
	}
	return i;
}

/* ------------------------------------------------------------- */
troop
select_enemy(fighter * af, int minrow, int maxrow)
{
	side *as = af->side;
	battle *b = as->battle;
	troop dt = no_troop;
	void ** si;
	int enemies;

	enemies = count_enemies(af->side, FS_ENEMY, minrow, maxrow);

	if (!enemies)
		return dt;				/* Niemand ist in der angegebenen Entfernung */
	enemies = rand() % enemies;
	for (si=b->sides.begin;!dt.fighter && si!=b->sides.end;++si) {
		side *ds = *si;
		void ** fi;
		if (!enemy(as, ds)) continue;
		for (fi=ds->fighters.begin;fi!=ds->fighters.end;++fi) {
			fighter * df = *fi;
			int dr = get_unitrow(df);
			if (dr < minrow || dr > maxrow) continue;
			if (df->alive - df->removed > enemies) {
				dt.index = enemies;
				dt.fighter = df;
				enemies = 0;
				break;
			}
			else enemies -= (df->alive - df->removed);
		}
	}
	assert(!enemies);
	return dt;
}

/*
 * Abfrage mit
 *
 * cvector *fgs=fighters(b,af,FIGHT_ROW,BEHIND_ROW, FS_HELP|FS_ENEMY);
 * fighter *fig;
 *
 * Optional: Verwirbeln. Vorsicht: Aufwendig!
 * v_scramble(fgs->begin, fgs->end);
 *
 * for (fig = fgs->begin; fig != fgs->end; ++fig) {
 *     fighter *df = *fig;
 *
 * }
 *
 * cv_kill(fgs); Nicht vergessen
 */

cvector *
fighters(battle *b, fighter *af, int minrow, int maxrow, int mask)
{
	fighter *fig;
	cvector	*fightervp;
	int row;

	fightervp = malloc(sizeof(cvector));
	cv_init(fightervp);

	for_each(fig, b->fighters) {
		if (!fig->alive) continue;

		row = get_unitrow(fig);
		if (row >= minrow && row <= maxrow) {
			switch (mask) {
			case FS_ENEMY:
				if (enemy(fig->side, af->side)) cv_pushback(fightervp, fig);
				break;
			case FS_HELP:
				if (!enemy(fig->side, af->side) && allysf(fig->side, af->side->bf->faction))
					cv_pushback(fightervp, fig);
				break;
			case FS_HELP|FS_ENEMY:
				cv_pushback(fightervp, fig);
				break;
			default:
				assert(0 || !"Ungültiger Allianzstatus in fighters()");
			}

		}
	} next(fig);

	return fightervp;
}

/* ------------------------------------------------------------------ */

void
do_combatmagic(battle *b, combatmagic_t was)
{
	void **fi;
	spell *sp;
	fighter *fig;
	unit *mage;
	region *r = b->region;
	castorder *co;
	castorder *cll[MAX_SPELLRANK];
	int level, power;
	int spellrank;
	int sl;

	for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
		cll[spellrank] = (castorder*)NULL;
	}

	for (fi = b->fighters.begin; fi != b->fighters.end; ++fi) {
		fig = *fi;
		mage = fig->unit;

		if (fig->alive > 0) { /* fighter kann im Kampf getötet worden sein */

			level = eff_skill(mage, SK_MAGIC, r);
			if (level > 0) {
				switch(was) {
				case DO_PRECOMBATSPELL:
					sp = get_combatspell(mage, 0);
					sl = get_combatspelllevel(mage, 0);
					break;
				case DO_POSTCOMBATSPELL:
					sp = get_combatspell(mage, 2);
					sl = get_combatspelllevel(mage, 2);
					break;
				default:
					/* Fehler! */
					return;
				}
				if (sp == NULL)
					continue;
				if (cancast(mage, sp, 1, 1) == false)
					continue;

				level = eff_spelllevel(mage, sp, level, 1);
				if (sl > 0) level = min(sl, level);
				if (level < 0) {
					sprintf(buf, "%s versucht %s zu zaubern, doch der Zauber schlägt "
							"fehl!", unitname(mage), sp->name);
					battlerecord(b, buf);
					continue;
				}

				power = spellpower(r, mage, sp, level);
				if (power <= 0) {	/* Effekt von Antimagie */
					sprintf(buf, "%s versucht %s zu zaubern, doch der Zauber schlägt "
							"fehl!", unitname(mage), sp->name);
					battlerecord(b, buf);
					continue;
				}

				if (fumble(r, mage, sp, sp->level) == true) {
					sprintf(buf, "%s versucht %s zu zaubern, doch der Zauber schlägt "
							"fehl!", unitname(mage), sp->name);
					battlerecord(b, buf);
					pay_spell(mage, sp, level, 1);
					continue;
				}

				co = new_castorder(fig, 0, sp, r, level, power, 0, 0, 0);
				add_castorder(&cll[(int)(sp->rank)], co);
			}
		}
	}
	for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
		for (co = cll[spellrank]; co; co = co->next) {
			fig = (fighter*)co->magician;
			sp = co->sp;
			level = co->level;
			power = co->force;

			level = ((cspell_f)sp->sp_function)(fig, level, power, sp);
			if (level > 0) {
				pay_spell(fig->unit, sp, level, 1);
				if (was == DO_PRECOMBATSPELL) {
					get_mage(fig->unit)->precombataura = level;
				}
			}
		}
	}
	for (spellrank = 0; spellrank < MAX_SPELLRANK; spellrank++) {
		free_castorders(cll[spellrank]);
	}
}


void
do_combatspell(troop at)
{
	spell *sp;
	fighter *fi = at.fighter;
	unit *mage = fi->unit;
	battle *b = fi->side->battle;
	region *r = b->region;
	int level, power;
	int fumblechance = 0;
	void **mg;
	int sl;

	sp = get_combatspell(mage, 1);
	if (sp == NULL) {
		fi->magic = 0; /* Hat keinen Kampfzauber, kämpft nichtmagisch weiter */
		return;
	}
	if (cancast(mage, sp, 1, 1) == false) {
		fi->magic = 0; /* Kann nicht mehr Zaubern, kämpft nichtmagisch weiter */
		return;
	}

	level = eff_spelllevel(mage, sp, fi->magic, 1);
	if ((sl = get_combatspelllevel(mage, 1)) > 0) level = min(level, sl);

	if (fumble(r, mage, sp, sp->level) == true) {
		sprintf(buf, "%s versucht %s zu zaubern, doch der Zauber schlägt "
				"fehl!", unitname(mage), sp->name);
		battlerecord(b, buf);
		pay_spell(mage, sp, level, 1);
		return;
	}

	for (mg = b->meffects.begin; mg != b->meffects.end; ++mg) {
		meffect *mblock = *mg;
		if (mblock->typ == SHIELD_BLOCK) {
			if (meffect_blocked(b, mblock, fi->side) != 0) {
				fumblechance += mblock->duration;
				mblock->duration -= mblock->effect;
			}
		}
	}

	/* Antimagie die Fehlschlag erhöht */
	if (rand()%100 < fumblechance) {
		sprintf(buf, "%s versucht %s zu zaubern, doch der Zauber schlägt "
				"fehl!", unitname(mage), sp->name);
		battlerecord(b, buf);
		return;
	}
	power = spellpower(r, mage, sp, level);
	if (power <= 0) {	/* Effekt von Antimagie */
		sprintf(buf, "%s versucht %s zu zaubern, doch der Zauber schlägt "
				"fehl!", unitname(mage), sp->name);
		battlerecord(b, buf);
		pay_spell(mage, sp, level, 1);
		return;
	}

	level = ((cspell_f)sp->sp_function)(fi, level, power, sp);
	if (level > 0)
		pay_spell(mage, sp, level, 1);
}


/* Sonderattacken: Monster patzern nicht und zahlen auch keine
 * Spruchkosten. Da die Spruchstärke direkt durch den Level bestimmt
 * wird, wirkt auch keine Antimagie (wird sonst in spellpower
 * gemacht) */

void
do_extra_spell(troop at, att *a)
{
	spell *sp;
	fighter *fi = at.fighter;
	unit *au = fi->unit;
	int power;

	sp = find_spellbyid((spellid_t)(a->data.i));
	/* nur bei Monstern können mehrere 'Magier' in einer Einheit sein */
	power = sp->level * au->number;
	((cspell_f)sp->sp_function)(fi, sp->level, power, sp);
}

static int
skilldiff(troop at, troop dt, int dist)
{
	fighter *af = at.fighter, *df = dt.fighter;
	unit *au = af->unit, *du = df->unit;
	int is_protected = 0, skdiff = 0, sk;
	weapon * awp = select_weapon(at, true, dist>1);
	weapon * dwp = select_weapon(dt, false, dist>1);

	/* Effekte von Alchemie */
	if (get_effect(au, oldpotiontype[P_BERSERK]) > at.index)
		++skdiff;

	if (df->person[dt.index].flags & FL_SLEEPING)
		skdiff += 2;

	/* Effekte durch Rassen */
	if (awp!=NULL && au->race == RC_HALFLING &&
		(du->race == RC_FIREDRAGON ||
		du->race == RC_DRAGON ||
		du->race == RC_WYRM))
		skdiff += 5;

	if (au->race == RC_GOBLIN &&
	    af->side->size[SUM_ROW] >= df->side->size[SUM_ROW] * 10)
		skdiff += 1;

	/* TODO this should be a skillmod */
	skdiff += jihad(au->faction, du->race);

	skdiff += af->person[at.index].attack;
	skdiff -= df->person[dt.index].defence;

#ifdef NEW_TACTICS
	/* Effekte von Taktikern */
	skdiff += tactics_bonus(at, dt, true);
	skdiff -= tactics_bonus(dt, at, false);
#endif

	if (df->building) {
		if (df->building->type == &bt_castle) {
			if(fspecial(au->faction, FS_SAPPER)) {
				/* Halbe Schutzwirkung, aufgerundet */
				skdiff -= (buildingeffsize(df->building, false)+1)/2;
			} else {
				skdiff -= buildingeffsize(df->building, false);
			}
			is_protected = 2;
		}
		if (is_cursed(df->building->attribs, C_STRONGWALL, 0)) {
			/* wirkt auf alle Gebäude */
			skdiff -= get_curseeffect(df->building->attribs, C_STRONGWALL, 0);
			is_protected = 2;
		}
		if (is_cursed(df->building->attribs, C_MAGICSTONE, 0)) {
			/* Verdoppelt Burgenbonus */
			skdiff -= buildingeffsize(df->building, false);
		}
	}
	/* Goblin-Verteidigung */

	if (du->race == RC_GOBLIN && dwp == NULL) skdiff -= 2;

	/* Effekte der Waffen */
	skdiff += weapon_effskill(at, dt, awp, true, dist>1);
	if (awp && fval(awp->type, WTF_MISSILE)) {
		skdiff -= is_protected;
		if (awp->type->modifiers) {
			int w;
			for (w=0;awp->type->modifiers[w].value!=0;++w) {
				if (awp->type->modifiers[w].flags & WMF_MISSILE_TARGET) {
					skdiff -= awp->type->modifiers[w].value;
					break;
				}
			}
		}
	}
	sk = weapon_effskill(dt, at, dwp, false, dist>1);
	skdiff -= sk;
	return skdiff;
}

static int
setreload(troop at)
{
	fighter * af = at.fighter;
	const weapon_type * wtype = af->person[at.index].weapon->type;
	if (wtype->reload == 0) return 0;
	return af->person[at.index].reload = wtype->reload;
}

int
getreload(troop at)
{
	return at.fighter->person[at.index].reload;
}

static char *
attack_message(const troop at, const troop dt, const weapon * wp, int dist)
{
	static char smallbuf[512];
	char a_unit[NAMESIZE+8], d_unit[NAMESIZE+8];
	const char *noweap_string[4] = {"schlägt nach",
													"tritt nach",
													"beißt nach",
													"kratzt nach"};

	if (at.fighter->unit->number > 1)
		sprintf(a_unit, "%s/%d", unitname(at.fighter->unit), at.index);
	else
		sprintf(a_unit, "%s", unitname(at.fighter->unit));

	if (dt.fighter->unit->number > 1)
		sprintf(d_unit, "%s/%d", unitname(dt.fighter->unit), dt.index);
	else
		sprintf(d_unit, "%s", unitname(dt.fighter->unit));

	if (wp == NULL) {
		sprintf(smallbuf, "%s %s %s",
			a_unit, noweap_string[rand()%4], d_unit);
		return smallbuf;
	}

	if (dist > 1) {
		sprintf(smallbuf, "%s schießt mit %s auf %s",
			a_unit,
			locale_string(NULL, resourcename(wp->type->itype->rtype, GR_INDEFINITE_ARTICLE)), d_unit);
		return smallbuf;
	}

		sprintf(smallbuf, "%s schlägt mit %s nach %s",
			a_unit,
			locale_string(NULL, resourcename(wp->type->itype->rtype, GR_INDEFINITE_ARTICLE)), d_unit);

	return smallbuf;
}

int
hits(troop at, troop dt, weapon * awp)
{
	battle * b = at.fighter->side->battle;
	fighter *af = at.fighter, *df = dt.fighter;
	unit *au = af->unit, *du = df->unit;
	char debugbuf[512];
	char *smallbuf = NULL;
	armor_t armor, shield;
	int skdiff = 0;
	int dist = get_unitrow(af) + get_unitrow(df) - 1;
	weapon * dwp = select_weapon(dt, false, dist>1);

	if (!df->alive) return 0;
	if (getreload(at)) return 0;
	if (dist>1 && !fval(awp->type, WTF_MISSILE)) return 0;
	if (af->person[at.index].flags & FL_STUNNED) {
			af->person[at.index].flags &= ~FL_STUNNED;
		return 0;
	}
	if ((af->person[at.index].flags & FL_TIRED && rand()%100 < 50)
			|| (af->person[at.index].flags & FL_SLEEPING))
		return 0;
	if (awp && fval(awp->type, WTF_MISSILE)
			&& af->side->battle->reelarrow == true
			&& rand()%100 < 50)
	{
		return 0;
	}

	skdiff = skilldiff(at, dt, dist);

	/* Verteidiger bekommt eine Rüstung */
	armor = select_armor(dt);
	shield = select_shield(dt);
	sprintf(debugbuf, "%.4s/%d [%6s/%d] attackiert %.4s/%d [%6s/%d] mit %d dist %d",
			unitid(au), at.index,
			(awp != NULL) ?
				locale_string(NULL, resourcename(awp->type->itype->rtype, 0)) : "unbewaffnet",
			weapon_effskill(at, dt, awp, true, dist>1),
			unitid(du), dt.index,
			(dwp != NULL) ?
				locale_string(NULL, resourcename(dwp->type->itype->rtype, 0)) : "unbewaffnet",
			weapon_effskill(dt, at, dwp, true, dist>1),
			skdiff, dist);
	if (b->small) {
		smallbuf = attack_message(at, dt, awp, dist);
	}

	if (contest(skdiff, armor, shield)) {
		strcat(debugbuf, " und trifft.");
		battledebug(debugbuf);
		if (b->small) {
			strcat(smallbuf, " und trifft.");
			battlerecord(b,smallbuf);
		}
#ifdef SHOW_KILLS
		++at.fighter->hits;
#endif
		return 1;
	}
	strcat(debugbuf, ".");
	battledebug(debugbuf);
	if (b->small) {
		strcat(smallbuf, ".");
		battlerecord(b,smallbuf);
	}
	return 0;
}

void
dazzle(battle *b, troop *td)
{
	char smallbuf[512];
#ifdef TODO_RUNESWORD
	if (td->fighter->weapon[WP_RUNESWORD].count > td->index) {
		if (b->small) {
			strcpy(smallbuf, "Das Runenschwert glüht kurz auf.");
			battlerecord(b,smallbuf);
		}
		return;
	}
#endif
	if (td->fighter->person[td->index].flags & FL_HERO) {
		if (b->small) {
			sprintf(smallbuf, "Eine kurze Schwäche erfaßt %s/%d, vergeht jedoch "
				"schnell wieder.", unitname(td->fighter->unit), td->index);
			battlerecord(b,smallbuf);
		}
		return;
	}

	if (td->fighter->person[td->index].flags & FL_DAZZLED) {
		if (b->small) {
			sprintf(smallbuf, "Eine kurze Schwäche erfaßt %s/%d, vergeht jedoch "
				"schnell wieder.", unitname(td->fighter->unit), td->index);
			battlerecord(b,smallbuf);
		}
		return;
	}

	if (b->small) {
		sprintf(smallbuf, "%s/%d fühlt sich, als würde Kraft entzogen.",
			unitname(td->fighter->unit), td->index);
		battlerecord(b,smallbuf);
	}

	td->fighter->person[td->index].flags |= FL_DAZZLED;
	td->fighter->person[td->index].defence--;
}

/* TODO: Gebäude/Schiffe sollten auch zerstörbar sein. Schwierig im Kampf,
 * besonders bei Schiffen. */

void
damage_building(battle *b, building *bldg, int damage_abs)
{
	bldg->size = max(1, bldg->size-damage_abs);

	/* Wenn Burg, dann gucken, ob die Leute alle noch in das Gebäude passen. */

	if (bldg->type == &bt_castle) {
		fighter *fi;

		bldg->sizeleft = bldg->size;

		for_each(fi, b->fighters) {
			if (fi->building == bldg) {
				if (bldg->sizeleft >= fi->unit->number) {
					fi->building = bldg;
					bldg->sizeleft -= fi->unit->number;
				} else {
					fi->building = NULL;
				}
			}
		} next(fi);
	}
}

static int
attacks_per_round(troop t)
{
	return t.fighter->person[t.index].speed;
}

static void
attack(battle *b, troop ta, att *a)
{
	fighter *af = ta.fighter;
	troop td;
	unit *au = af->unit;
	int row = get_unitrow(af) - 1;

	switch(a->type) {
	case AT_STANDARD:		/* Waffen, mag. Gegenstände, Kampfzauber */
	case AT_COMBATSPELL:
		if (af->magic > 0) {
			/* Magier versuchen immer erstmal zu zaubern, erst wenn das
			 * fehlschlägt, wird af->magic == 0 und  der Magier kämpft
			 * konventionell weiter */
			do_combatspell(ta);
		} else {
			weapon * wp;

			wp = select_weapon(ta, true, true);
			/* Sonderbehandlungen */

			if (getreload(ta)) {
				ta.fighter->person[ta.index].reload--;
			} else {
				boolean standard_attack = true;
				if (wp && wp->type->attack) {
					int dead;
					standard_attack = wp->type->attack(&ta, &dead);
					af->catmsg += dead;
				}
				if (standard_attack) {
					boolean missile = false;
					if (wp && fval(wp->type, WTF_MISSILE)) missile=true;
					if (missile) td = select_enemy(af, missile_range[0]-row, missile_range[1]-row);
					else td = select_enemy(af, melee_range[0]-row, melee_range[1]-row);
					if (!td.fighter) return;

					if(td.fighter->person[td.index].last_action < b->turn) {
						td.fighter->person[td.index].last_action = b->turn;
						td.fighter->action_counter++;
					}
					if(ta.fighter->person[ta.index].last_action < b->turn) {
						ta.fighter->person[ta.index].last_action = b->turn;
						ta.fighter->action_counter++;
					}

					if (hits(ta, td, wp)) {
						const char * d;
						if (wp == NULL) d = race[au->race].def_damage;
						else if (riding(ta)) d = wp->type->damage[1];
						else d = wp->type->damage[0];
						terminate(td, ta, a->type, d, missile);
					}
				}
				if (wp && wp->type->reload && !getreload(ta)) {
					int i = setreload(ta);
					sprintf(buf, " Nachladen gesetzt: %d", i);
					battledebug(buf);
				}
			}
		}
		break;
	case AT_SPELL:	/* Extra-Sprüche. Kampfzauber in AT_COMBATSPELL! */
		do_extra_spell(ta, a);
		break;
	case AT_NATURAL:
		td = select_enemy(af, FIGHT_ROW-row, FIGHT_ROW-row);
		if (!td.fighter) return;
		if(td.fighter->person[td.index].last_action < b->turn) {
			td.fighter->person[td.index].last_action = b->turn;
			td.fighter->action_counter++;
		}
		if(ta.fighter->person[ta.index].last_action < b->turn) {
			ta.fighter->person[ta.index].last_action = b->turn;
			ta.fighter->action_counter++;
		}
		if (hits(ta, td, NULL)) {
			terminate(td, ta, a->type, a->data.dice, false);
		}
		break;
	case AT_DRAIN_ST:
		td = select_enemy(af, FIGHT_ROW-row, FIGHT_ROW-row);
		if (!td.fighter) return;
		if(td.fighter->person[td.index].last_action < b->turn) {
			td.fighter->person[td.index].last_action = b->turn;
			td.fighter->action_counter++;
		}
		if(ta.fighter->person[ta.index].last_action < b->turn) {
			ta.fighter->person[ta.index].last_action = b->turn;
			ta.fighter->action_counter++;
		}
		if (hits(ta, td, NULL)) {
			int c = dice_rand(a->data.dice);
			while(c > 0) {
				if (rand()%2) {
					td.fighter->person[td.index].attack -= 1;
				} else {
					td.fighter->person[td.index].defence -= 1;
				}
				c--;
			}
		}
		break;
	case AT_DRAIN_EXP:
		td = select_enemy(af, FIGHT_ROW-row, FIGHT_ROW-row);
		if (!td.fighter) return;
		if(td.fighter->person[td.index].last_action < b->turn) {
			td.fighter->person[td.index].last_action = b->turn;
			td.fighter->action_counter++;
		}
		if(ta.fighter->person[ta.index].last_action < b->turn) {
			ta.fighter->person[ta.index].last_action = b->turn;
			ta.fighter->action_counter++;
		}
		if (hits(ta, td, NULL)) {
			drain_exp(td.fighter->unit, dice_rand(a->data.dice));
		}
		break;
	case AT_DAZZLE:
		td = select_enemy(af, FIGHT_ROW-row, FIGHT_ROW-row);
		if (!td.fighter) return;
		if(td.fighter->person[td.index].last_action < b->turn) {
			td.fighter->person[td.index].last_action = b->turn;
			td.fighter->action_counter++;
		}
		if(ta.fighter->person[ta.index].last_action < b->turn) {
			ta.fighter->person[ta.index].last_action = b->turn;
			ta.fighter->action_counter++;
		}
		if (hits(ta, td, NULL)) {
			dazzle(b, &td);
		}
		break;
	case AT_STRUCTURAL:
		td = select_enemy(af, FIGHT_ROW-row, FIGHT_ROW-row);
		if (!td.fighter) return;
		if(ta.fighter->person[ta.index].last_action < b->turn) {
			ta.fighter->person[ta.index].last_action = b->turn;
			ta.fighter->action_counter++;
		}
		if (td.fighter->unit->ship) {
			td.fighter->unit->ship->damage += DAMAGE_SCALE * dice_rand(a->data.dice);
		} else if (td.fighter->unit->building) {
			damage_building(b, td.fighter->unit->building,
											dice_rand(a->data.dice));
		}
	}

	/* Der letzte Katapultschütze setzt die
	 * Ladezeit neu und generiert die Meldung. */
	if (af->catmsg>=0 && ta.index==0) {
		sprintf(buf, "%d Opfer wurde%s getötet.",
				af->catmsg, af->catmsg<=1?"":"n");
		battlerecord(b, buf);
		af->catmsg = -1;
	}
}

void
do_attack(fighter * af)
{
	troop ta;
	unit *au = af->unit;
	side *side = af->side;
	battle *b = side->battle;
	int apr;
	int a;

	ta.fighter = af;

	assert(au && au->number);
	/* Da das Zuschlagen auf Einheiten und nicht auf den einzelnen
	 * Kämpfern beruht, darf die Reihenfolge und Größe der Einheit keine
	 * Rolle spielen, Das tut sie nur dann, wenn jeder, der am Anfang der
	 * Runde lebte, auch zuschlagen darf. Ansonsten ist der, der zufällig
	 * mit einer großen Einheit zuerst drankommt, extrem bevorteilt. */
	ta.index = af->fighting;

	while (ta.index--) {
		/* Wir suchen eine beliebige Feind-Einheit aus. An der können
		 * wir feststellen, ob noch jemand da ist. */
		int enemies = count_enemies(af->side, FS_ENEMY, FIGHT_ROW, LAST_ROW);
		if (!enemies) break;

		for (apr=attacks_per_round(ta); apr > 0; apr--) {
			for (a = 0; a < 6; a++) {
				if (race[au->race].attack[a].type != AT_NONE)
					attack(b, ta, &(race[au->race].attack[a]));
			}
		}
	}

}

void
do_regenerate(fighter *af)
{
	troop ta;
	unit *au = af->unit;

	ta.fighter = af;
	ta.index = af->fighting;

	while(ta.index--) {
		af->person[ta.index].hp += effskill(au, SK_AUSDAUER);
		af->person[ta.index].hp =	min(unit_max_hp(au), af->person[ta.index].hp);
	}
}

static void
add_tactics(tactics * ta, fighter * fig, int value)
{
	if (value == 0 || value < ta->value)
		return;
	if (value > ta->value)
		cv_kill(&ta->fighters);
	cv_pushback(&ta->fighters, fig);
	cv_pushback(&fig->side->battle->leaders, fig);
	ta->value = value;
}

double
fleechance(unit * u)
{
	double c = 0.20;			/* Fluchtwahrscheinlichkeit in % */
	region *r = u->region;

	/* Einheit u versucht, dem Getümmel zu entkommen */

	c += (eff_skill(u, SK_STEALTH, r) * 0.05);

	if (get_item(u, I_UNICORN) >= u->number && eff_skill(u, SK_RIDING, r) >= 5)
		c += 0.30;
	else if (get_item(u, I_HORSE) >= u->number && eff_skill(u, SK_RIDING, r) >= 1)
		c += 0.10;

	if (u->race == RC_HALFLING) {
		c += 0.20;
		c = min(c, 0.90);
	} else {
		c = min(c, 0.75);
	}

	return c;
}

int nextside = 0;
side *
#ifdef GROUPS
make_side(battle * b, const faction * f, const group * g, boolean stealth)
#else
make_side(battle * b, const faction * f, boolean stealth)
#endif
{
	side *s1 = calloc(sizeof(struct side), 1);
	bfaction * bf;

	s1->battle = b;
#ifdef GROUPS
	s1->group = g;
#endif
	s1->stealth = stealth;
	cv_pushback(&b->sides, s1);
	for (bf = b->factions;bf;bf=bf->next) {
		faction * f2 = bf->faction;

		if (f2 == f) {
			s1->bf = bf;
			s1->index = nextside++;
			s1->nextF = bf->sides;
			bf->sides = s1;
			break;
		}
	}
	assert(bf);
	return s1;
}

void
loot_items(fighter * corpse)
{
	unit * u = corpse->unit;
	item * itm = u->items;
	u->items = NULL;

	while (itm) {
		const item_type * itype = itm->type;
		int i;
		int keep, give;
		if (corpse->run_number) {
			/* es gibt flüchtlinge */
			if (itype->capacity>0 || fval(itype, ITF_ANIMAL)) {
				/* Regeländerung: Man muß das Tier nicht reiten können,
				 * um es vom Schlachtfeld mitzunehmen, ist ja nur
				 * eine Region weit. * */
				keep = min(corpse->run_number, itm->number);
			} else if (itm->type->weight <= 0) {
				/* if it doesn't weigh anything, it won't slow us down */
				keep = min(corpse->run_number, itm->number);
			}
			else keep = 0;
			give = itm->number-keep;
		}
		else {
			keep = 0;
			give = itm->number;
		}
		if (give) for (i = 10; i != 0; i--) {
			int loot = give / i;
			give -= loot;
			/* Looten tun hier immer nur die Gegner. Das
			 * ist als Ausgleich für die neue Loot-regel
			 * (nur ganz tote Einheiten) fair.
			 * zusätzlich looten auch geflohene, aber
			 * nach anderen Regeln.
			 */
			if (loot>0 && (itm->type->flags & (ITF_CURSED|ITF_NOTLOST)
					|| rand()%100 >= 50 - corpse->side->battle->keeploot)) {
				fighter *fig = select_enemy(corpse, FIGHT_ROW, LAST_ROW).fighter;
				if (fig) {
					item * l = fig->loot;
					while (l && l->type!=itm->type) l=l->next;
					if (!l) {
						l = calloc(sizeof(item), 1);
						l->next = fig->loot;
						fig->loot = l;
						l->type = itm->type;
					}
					l->number += loot;
				}
			}
		}
		if (keep) {
			item * i = i_remove(&itm, itm);
			i->number = keep;
			i_add(&u->items, i);
		} else {
			i_free(i_remove(&itm, itm));
		}
	}
}

void
loot_fleeing(unit* u, unit* runner) {
	/* die einheit runner nimmt von der einheit u soviel sie kann
	 * in die nächste region mit. hier geschieht allerdings nur
	 * der transfer, nicht die eigentliche flucht */
	int money = get_money(u) * runner->number / (u->number + runner->number);

	money = min(money, runner->number * (PERSONCAPACITY(u) - race[u->race].weight) / SILVERWEIGHT);
	set_money(runner, money);
	change_money(u, -money);
	if (effskill(runner, SK_RIDING) > 0) {
		int horses = min(get_item(u, I_HORSE), runner->number);

		change_item(runner, I_HORSE, horses);
		change_item(u, I_HORSE, -horses);
	}
}

static boolean
seematrix(const faction * f, const side * s)
{
	if (f==s->bf->faction) return true;
	if (s->stealth) return false;
	return true;
}

static void
aftermath(battle * b)
{
	int i;
	region *r = b->region;
	ship *sh;
	side *s;
	cvector *fighters = &b->fighters;
	void **fi;
	int is = 0;
	bfaction * bf;
	int dead_peasants;
	boolean battle_was_relevant = (boolean)(b->turn+(b->has_tactics_turn?1:0)>2);

#ifdef TROLLSAVE
	int *trollsave = calloc(2 * cv_size(&b->factions), sizeof(int));
#endif

	for (fi = fighters->begin; fi != fighters->end; ++fi) {
		fighter *df = *fi;
		unit *du = df->unit;
		int dead;
		const attrib *a;
		int pr_mercy = 0;

		for (a = a_find(du->attribs, &at_prayer_effect); a; a = a->nexttype) {
			if (a->data.sa[0] == PR_MERCY) {
				pr_mercy = a->data.sa[1];
			}
		}

		dead = du->number - df->alive;
		dead -= df->run_number;
#ifdef TROLLSAVE
		/* Trolle können regenerieren */
		if (df->alive > 0 && dead && du->race == RC_TROLL)
			for (i = 0; i != dead; ++i)
				if (chance(TROLL_REGENERATION))
					++df->alive;
		trollsave[df->side->index] += dead - du->number + df->alive;
#endif
		/* Regeneration durch PR_MERCY */
		if (dead && pr_mercy)
			for (i = 0; i != dead; ++i)
				if (rand()%100 < pr_mercy)
					++df->alive;

		/* Tote, die wiederbelebt werde können */
		if (!nonplayer(df->unit)) {
			df->side->casualties += dead;
		}
#ifdef SHOW_KILLS
		if (df->hits + df->kills) {
			message * m = new_message(du->faction, "killsandhits%u:unit%i:hits%i:kills", du, df->hits, df->kills);
			brecord(du->faction, b->region, m);
		}
#endif
	}

	/* Wenn die Schlacht kurz war, dann gib Aura für den Präcombatzauber
	 * zurück. Nicht logisch, aber die einzige Lösung, den Verlust der
	 * Aura durch Dummy-Angriffe zu verhindern. */

	for_each(s, b->sides) {
		if (s->bf->lastturn+(b->has_tactics_turn?1:0)<=1) continue;
		/* Prüfung, ob faction angegriffen hat. Geht nur über die Faction */
		if (!s->bf->attacker) {
			fighter *fig;
			for_each(fig, s->fighters) {
				sc_mage * mage = get_mage(fig->unit);
				if (mage)
					mage->spellpoints += mage->precombataura;
			} next(fig);
		}
		/* Alle Fighter durchgehen, Mages suchen, Precombataura zurück */
	} next(s);

	/* POSTCOMBAT */
	do_combatmagic(b, DO_POSTCOMBATSPELL);

	for_each(s, b->sides) {
		fighter *df;
		boolean relevant = false; /* Kampf relevant für dieses Heer? */
		if (s->bf->lastturn+(b->has_tactics_turn?1:0)>1) {
			relevant = true;
		}
		s->alive = 0;
		s->flee = 0;
		s->dead = 0;

		for_each(df, s->fighters) {
			unit *du = df->unit;
			int dead = du->number - df->alive - df->run_number;
			int sum_hp = 0;
			int n;

			if (relevant && df->action_counter >= df->unit->number) {
				fset(df->unit, FL_HADBATTLE);
				/* TODO: das sollte hier weg sobald anderswo üb
				 * erall HADBATTLE getestet wird. */
				set_string(&du->thisorder, "");
			}
			for (n = 0; n != df->alive; ++n) {
				if (df->person[n].hp > 0)
					sum_hp += df->person[n].hp;
			}

			s->dead += dead;
			s->flee += df->run_number;
			s->alive += df->alive;

			if (df->alive == du->number) continue; /* nichts passiert */

			reportcasualties(b, df);
			scale_number(du, df->alive + df->run_number);
			du->hp = sum_hp + df->run_hp;

			/* die weggerannten werden später subtrahiert! */
			assert(du->number >= 0);

			/* Report the casualties */

			if (df->run_hp) {
				/* Sonderbehandlung für sich auflösende Einheiten wie Wölfe,
				 * diese fliehen nicht in eine Nachbarregion sondern kehren
				 * nach dem Kampf zu ihrer Einheit zurück */
				if (a_find(du->attribs, &at_unitdissolve)) {
					/* die Einheit wurde weiter oben ja schon auf die
					 * Überlebenen scaliert und auch die HP aller überlebenden
					 * wurden berücksichtigt. */
					continue;
				}
				if (df->alive == 0) {
					scale_number(du, df->run_number);
					du->hp = df->run_hp;
					/* Distribute Loot */
					if (!fval(df, FIG_NOLOOT)) {
						loot_items(df);
					}
				} else {
					unit *nu = createunit(du->region, du->faction, df->run_number, du->race);
					skill_t sk;

					for (sk = 0; sk != MAXSKILLS; ++sk) {
						int sval = get_skill(du, sk);
						int snew = (sval * nu->number) / du->number;
						if (snew) set_skill(nu, sk, snew);
					}
					sprintf(buf, "%s Flüchtlinge", du->name);
					set_string(&nu->name, buf);
					sprintf(buf, "Sie flohen aus der Schlacht in %s",
							rname(r, nu->faction->locale));
					set_string(&nu->display, buf);
					nu->status = du->status;
					setguard(nu, GUARD_NONE);
					/* Temps von parteigetarnten Einheiten sind
					 * wieder parteigetarnt / Martin */
					if (fval(du, FL_PARTEITARNUNG))
						fset(nu, FL_PARTEITARNUNG);
					/* Daemonentarnung */
					nu->irace = du->irace;
					/* Fliehenden nehmen mit, was sie tragen können*/

					loot_fleeing(du, nu);
					set_string(&nu->lastorder, du->lastorder);

					scale_number(du, df->alive);
					du->hp = sum_hp;
					du = nu;
				}
				set_string(&du->thisorder, "");
				du->hp = df->run_hp;
				setguard(du, GUARD_NONE);
				fset(du, FL_MOVED);
				leave(du->region, du);
				if (df->run_to) travel(r, du, df->run_to, 1);
			}
			else if (df->alive==0) {
				setguard(du, GUARD_NONE);
				scale_number(du, 0);
				/* Distribute Loot */
				if (!fval(df, FIG_NOLOOT)) {
					loot_items(df);
				}
			}

			if (!nonplayer_race(du->race)) {
				/* tote im kampf werden zu regionsuntoten: 
				 * for each o them, a peasant will die as well */
				is += dead;
			}
		} next(df);
	} next(s);
	dead_peasants = min(rpeasants(r), is);
	deathcounts(r, dead_peasants + is);
	chaoscounts(r, dead_peasants / 2);
	rsetpeasants(r, rpeasants(r) - dead_peasants);

	for (bf=b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		fbattlerecord(f, r, " ");
		for_each(s, b->sides) {
			if (seematrix(f, s)) {
				sprintf(buf, "Heer %2d(%s): %d Tote, %d Geflohene, %d Überlebende",
						s->index, abkz(s->bf->faction->name,3), s->dead, s->flee, s->alive);
			} else {
				sprintf(buf, "Heer %2d(Unb): %d Tote, %d Geflohene, %d Überlebende",
						s->index, s->dead, s->flee, s->alive);
			}
			fbattlerecord(f, r, buf);
		} next(s);
	}
#if COMBATEXP
	for (fi = fighters->begin; fi != fighters->end; ++fi) {
		fighter *df = *fi;
		troop t;

		t.fighter = df;
		if (df->alive && df->side->bf->lastturn > 2) {
			/* signifikanter Kampf, dann gibt es XP */
			unit *du = df->unit;
			cvector *tacs = &df->side->leader.fighters;

			if (v_find(tacs->begin, tacs->end, df) != tacs->end)
				change_skill(du, SK_TACTICS, du->number * COMBATEXP);
			if (!fval(df, FIG_COMBATEXP)) {
				t.index = df->alive;
				while (t.index) {
					--t.index;
					change_skill(du, SK_AUSDAUER, COMBATEXP);

					switch (select_weapon(t, 1, 1)) {
					case WP_RUNESWORD:
					case WP_GREATSWORD:
					case WP_EOGSWORD:
					case WP_SWORD:
					case WP_AXE:
						change_skill(du, SK_SWORD, COMBATEXP);
						break;
					case WP_SPEAR:
					case WP_HALBERD:
					case WP_LANCE:
						change_skill(du, SK_SPEAR, COMBATEXP);
						break;
					case WP_CROSSBOW:
						change_skill(du, SK_CROSSBOW, COMBATEXP);
						break;
					case WP_LONGBOW:
					case WP_GREATBOW:
						change_skill(du, SK_LONGBOW, COMBATEXP);
						break;
					case WP_CATAPULT:
						change_skill(du, SK_CATAPULT, COMBATEXP);
						break;
					}

					if (t.index < t.fighter->horses)
						change_skill(du, SK_RIDING, COMBATEXP);

				}

				if (get_skill(du, SK_MAGIC) > 0)
					change_skill(du, SK_MAGIC, COMBATEXP * du->number);

				fset(df, FIG_COMBATEXP);
			}
		}
	}
#endif

	/* Wir benutzen drifted, um uns zu merken, ob ein Schiff
	 * schonmal Schaden genommen hat. (moved und drifted
	 * sollten in flags überführt werden */

	for (sh=r->ships; sh; sh=sh->next) sh->drifted = false;

	for (fi = fighters->begin; fi != fighters->end; ++fi) {
		fighter *df = *fi;
		unit *du = df->unit;
		item * l;

		for (l=df->loot; l; l=l->next) {
			const item_type * itype = l->type;
			sprintf(buf, "%s erbeute%s %d %s.", unitname(du), du->number==1?"t":"n",
				l->number, locale_string(NULL, resourcename(itype->rtype, l->number!=1)));
			fbattlerecord(du->faction, r, buf);
			i_change(&du->items, itype, l->number);
		}

		/* Wenn sich die Einheit auf einem Schiff befindet, wird
		 * dieses Schiff beschädigt. Andernfalls ein Schiff, welches
		 * evt. zuvor verlassen wurde. */

		if (du->ship) sh = du->ship; else sh = leftship(du);

		if (sh && sh->drifted == false && b->turn+(b->has_tactics_turn?1:0)>2) {
			damage_ship(sh, 0.20);
			sh->drifted = true;
		}
	}

	sh = r->ships;
	if (battle_was_relevant) {
		while (sh) {
			sh->drifted = false;
			if (sh->damage >= sh->size * DAMAGE_SCALE) {
				ship * sn = sh->next;
				destroy_ship(sh, r);
				sh = sn;
			} else {
				sh = sh->next;
			}
		}
	}
#ifdef TROLLSAVE
	free(trollsave);
#endif

	sprintf(buf, "The battle lasted %d turns, %s and %s.\n",
			b->turn,
			b->has_tactics_turn==true?"had a tactic turn":"had no tactic turn",
			battle_was_relevant==true?"was relevant":"was not relevant.");
	battledebug(buf);
}

static void
battle_punit(unit * u, battle * b)
{
	bfaction * bf;
	strlist *S, *x;

	for (bf = b->factions;bf;bf=bf->next) {
		faction *f = bf->faction;

		S = 0;
		spunit(&S, f, u, 4, see_battle);
		for (x = S; x; x = x->next) {
			fbattlerecord(f, u->region, x->s);
			if (u->faction == f)
				battledebug(x->s);
		}
		if (S)
			freestrlist(S);
	}
}

static void
print_fighters(battle * b, cvector * fighters)
{
	fighter *df;
	int lastrow = -1;

	for_each(df, *fighters) {
		unit *du = df->unit;

		int row = get_unitrow(df);

		if (row != lastrow) {
			char zText[32];
			sprintf(zText, "... in der %d. Kampflinie:", row);
			battlerecord(b, zText);
			lastrow = row;
		}
		battle_punit(du, b);
	}
	next(df);
}

static void
print_header(battle * b)
{
	bfaction * bf;
	void **fi;
	cvector *fighters = &b->fighters;
	boolean * seen = malloc(sizeof(boolean)*cv_size(&b->sides));

	for (bf=b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		const char * lastf = NULL;
		boolean first = false;

		strcpy(buf, "Der Kampf wurde ausgelöst von ");
		memset(seen, 0, sizeof(boolean)*cv_size(&b->sides));

		for (fi = fighters->begin; fi != fighters->end; ++fi) {
			fighter *df = *fi;
			if (!fval(df, FIG_ATTACKED)) continue;
			if (!seen[df->side->index] ) {
				if (first) strcat(buf, ", ");
				if (lastf) {
					strcat(buf, lastf);
					first = true;
				}
				if (seematrix(f, df->side) == true)
					lastf = factionname(df->side->bf->faction);
				else
					lastf = "einer unbekannten Partei";
				seen[df->side->index] = true;
			}
		}
		if (first) strcat(buf, " und ");
		if (lastf) strcat(buf, lastf);
		strcat(buf, ".");
		fbattlerecord(f, b->region, buf);
	}
	free(seen);
}

static void
print_stats(battle * b)
{
	side *s2;
	side *side;
	int i = 0;
	for_each(side, b->sides) {
		bfaction *bf;
		char *k;
		boolean komma;

		++i;

		for (bf=b->factions;bf;bf=bf->next) {
			faction * f = bf->faction;
			struct enemy * e;
			fbattlerecord(f, b->region, " ");
			sprintf(buf, "Heer %d: %s", side->index,
					seematrix(f, side)
					? factionname(side->bf->faction) : "Unbekannte Partei");
			fbattlerecord(f, b->region, buf);
			strcpy(buf, "Kämpft gegen:");
			komma = false;
			for_each(s2, b->sides) {
				if (enemy(s2, side))
				{
					if (seematrix(f, s2) == true) {
						sprintf(buf, "%s%s Heer %d(%s)", buf, komma++ ? "," : "",
								s2->index, abkz(s2->bf->faction->name, 3));
					} else {
						sprintf(buf, "%s%s Heer %d(Unb)", buf, komma++ ? "," : "",
								s2->index);
					}
				}
			}
			next(s2);
			fbattlerecord(f, b->region, buf);
			strcpy(buf, "Attacke gegen:");
			komma = false;
			for (i=0;i!=16;i++)
			for (e=side->enemies[i]; e; e=e->nexthash) if (e->attacking) {
				struct side * s2 = e->side;
				if (seematrix(f, s2) == true) {
					sprintf(buf, "%s%s Heer %d(%s)", buf, komma++ ? "," : "",
							s2->index, abkz(s2->bf->faction->name, 3));
				} else {
					sprintf(buf, "%s%s Heer %d(Unb)", buf, komma++ ? "," : "",
							s2->index);
				}
			}
			fbattlerecord(f, b->region, buf);
		}
		buf[77] = (char)0;
		for (k = buf; *k; ++k) *k = '-';
		battlerecord(b, buf);
		print_fighters(b, &side->fighters);
	}
	next(side);

	battlerecord(b, " ");

	/* Besten Taktiker ermitteln */

	b->max_tactics = 0;

	for_each(side, b->sides) {
		if (cv_size(&side->leader.fighters))
			b->max_tactics = max(b->max_tactics, side->leader.value);
	} next(side);

	if (b->max_tactics > 0) {
		for_each(side, b->sides) {
			if (side->leader.value == b->max_tactics) {
				fighter *tf;
				for_each(tf, side->leader.fighters) {
					unit *u = tf->unit;
					if (!fval(tf, FIG_ATTACKED))
						battlemsg(b, u, "überrascht den Gegner.");
					else
						battlemsg(b, u, "konnte dem Gegner eine Falle stellen.");
				} next(tf);
			}
		} next(side);
	}

}

fighter *
make_fighter(battle * b, unit * u, boolean attack)
{
#define WMAX 16
	weapon weapons[WMAX];
	int owp[WMAX];
	int dwp[WMAX];
	int w = 0;
	region *r = b->region;
	item * itm;
	fighter *fig = NULL;
	int i, t = eff_skill(u, SK_TACTICS, r);
	side *s2, *s1 = NULL;
	int h;
	int berserk;
	int strongmen;
	int speeded, speed;
	boolean pr_aid = false;
	boolean stealth = (boolean)((fval(u, FL_PARTEITARNUNG)!=0)?true:false);
	int rest;
#ifdef GROUPS
	const attrib * a = a_find(u->attribs, &at_group);
	const group * g = a?(const group*)a->data.v:NULL;
#endif
	/* Illusionen und Zauber kaempfen nicht */
	if (u->race == RC_ILLUSION || u->race == RC_SPELL || idle(u->faction))
		return NULL;

	for_each(s2, b->sides) {
		if (s2->bf->faction == u->faction && s2->stealth==stealth) {
#ifdef GROUPS
			if (s2->group==g) {
				s1 = s2;
				break;
			}
#else
			s1 = s2;
			break;
#endif
		}
	}
	next(s2);
	/* aliances are moved out of make_fighter and will be handled later */
#ifdef GROUPS
	if (!s1) s1 = make_side(b, u->faction, g, stealth);
#else
	if (!s1) s1 = make_side(b, u->faction, stealth);
#endif
	/* Zu diesem Zeitpunkt ist attacked noch 0, da die Einheit für noch
	 * keinen Kampf ausgewählt wurde (sonst würde ein fighter existieren) */
	fig = calloc(1, sizeof(struct fighter));

	cv_pushback(&s1->fighters, fig);
	cv_pushback(&b->fighters, fig);

	fig->unit = u;
	/* In einer Burg muß man a) nicht Angreifer sein, und b) drin sein, und
	 * c) noch Platz finden. d) menschlich sein */
	if (attack) fset(fig, FIG_ATTACKED);
	if ((!attack) &&
	    u->building &&
	    u->building->sizeleft >= u->number &&
	    !race[u->race].nonplayer) {
		fig->building = u->building;
		fig->building->sizeleft -= u->number;
	}
	fig->status = u->status;
	fig->side = s1;
	fig->alive = u->number;
	fig->catmsg = -1;

	/* Freigeben nicht vergessen! */
	fig->person = calloc(fig->alive, sizeof(struct person));

	h = u->hp / u->number;
	assert(h);
	rest = u->hp % u->number;
	speeded = get_cursedmen(u->attribs, C_SPEED, 0);
	speed   = get_curseeffect(u->attribs, C_SPEED, 0);

	/* Effekte von Alchemie */
	berserk = get_effect(u, oldpotiontype[P_BERSERK]);

	/* Effekte von Sprüchen */
	/* Effekte von Artefakten */
	strongmen = min(fig->unit->number, get_item(u, I_TROLLBELT));

	for (a = a_find(u->attribs, &at_prayer_effect); a; a = a->nexttype) {
		if (a->data.sa[0] == PR_AID) {
			pr_aid = true;
			break;
		}
	}

	/* Hitpoints, Attack- und Defence-Boni für alle Personen */
	for (i = 0; i < fig->alive; i++) {
		assert(i < fig->unit->number);
		fig->person[i].hp = h;
		if (i < rest)
			fig->person[i].hp++;

		if (i < speeded)
			fig->person[i].speed = speed;
		else
			fig->person[i].speed = 1;

		if (i < berserk) {
			fig->person[i].attack++;
		}
		/* Leute mit einem Aid-Prayer bekommen +1 auf fast alles. */
		if (pr_aid) {
			fig->person[i].attack++;
			fig->person[i].defence++;
			fig->person[i].damage++;
			fig->person[i].damage_rear++;
			fig->person[i].flags |= FL_HERO;
		}
		/* Leute mit Kraftzauber machen +2 Schaden im Nahkampf. */
		if (i < strongmen) {
			fig->person[i].damage += 2;
		}
	}

	/* Für alle Waffengattungne wird bestimmt, wie viele der Personen mit
	 * ihr kämpfen könnten, und was ihr Wert darin ist. */
	if (race[u->race].battle_flags & BF_EQUIPMENT) {
		int oi=0, di=0;
		for (itm=u->items;itm;itm=itm->next) {
			const weapon_type * wtype = resource2weapon(itm->type->rtype);
			if (wtype==NULL || itm->number==0) continue;
			weapons[w].offskill = weapon_skill(wtype, u, true);
			weapons[w].defskill = weapon_skill(wtype, u, false);
			weapons[w].type = wtype;
			weapons[w].used = 0;
			weapons[w].count = itm->number;
			++w;
		}
		fig->weapons = calloc(sizeof(weapon), w+1);
		memcpy(fig->weapons, weapons, w*sizeof(weapon));

		for (i=0; i!=w; ++i) {
			int j, o=0, d=0;
			for (j=0; j!=i; ++j) {
				if (fig->weapons[j].defskill>=fig->weapons[i].defskill) ++d;
				if (fig->weapons[j].offskill>=fig->weapons[i].offskill) ++o;
			}
			for (j=i+1; j!=w; ++j) {
				if (fig->weapons[j].defskill>fig->weapons[i].defskill) ++d;
				if (fig->weapons[j].offskill>fig->weapons[i].offskill) ++o;
			}
			owp[o] = i;
			dwp[d] = i;
		}
		/* jetzt enthalten owp und dwp eine absteigend schlechter werdende Liste der Waffen
		 * oi and di are the current index to the sorted owp/dwp arrays
		 * owp, dwp contain indices to the figther::weapons array */

		/* hand out offensive weapons: */
		for (i=0; i!=fig->alive; ++i) {
			int wpless = weapon_skill(NULL, u, true);
			while (oi!=w && fig->weapons[owp[oi]].used==fig->weapons[owp[oi]].count) {
				++oi;
			}
			if (oi==w) break; /* no more weapons available */
			if (fig->weapons[owp[oi]].offskill<wpless) continue; /* we fight better with bare hands */
			fig->person[i].weapon = &fig->weapons[owp[oi]];
			if (fig->person[i].weapon->type->reload) {
				fig->person[i].reload = rand() % fig->person[i].weapon->type->reload;
			}
			++fig->weapons[owp[oi]].used;
		}
		/* hand out defensive weapons. No missiles, please. */
		for (di=0, i=0; i!=fig->alive; ++i) {
			if (fig->person[i].weapon==NULL) continue;
			while (di!=w && (fig->weapons[dwp[di]].used==fig->weapons[dwp[di]].count || fval(fig->weapons[dwp[di]].type, WTF_MISSILE))) {
				++di;
			}
			if (di==w) break; /* no more weapons available */
			if (fig->weapons[dwp[di]].defskill>fig->person[i].weapon->defskill) {
				fig->person[i].secondary = &fig->weapons[dwp[di]];
				++fig->weapons[dwp[di]].used;
			}
		}
	}

	if (i_get(u->items, &it_demonseye)) {
		char lbuf[80];
		const char * s = race[u->race].name[3];
		char * c = lbuf;
		while (*s) *c++ = (char)toupper(*s++);
	 	*c = 0;
		fig->person[0].hp = unit_max_hp(u) * 3;
		sprintf(buf, "Eine Stimme ertönt über dem Schlachtfeld. 'DIESES %sKIND IST MEIN. IHR SOLLT ES NICHT HABEN.'. Eine leuchtende Aura umgibt %s", lbuf, unitname(u));
		battlerecord(b, buf);
	}

	s1->size[fig->status + FIGHT_ROW] += u->number;
	s1->size[SUM_ROW] += u->number;
	if (race[u->race].battle_flags & BF_NOBLOCK) {
		s1->nonblockers[fig->status + FIGHT_ROW] += u->number;
	}

	if (race[fig->unit->race].flags & RCF_HORSE) {
		fig->horses = fig->unit->number;
		fig->elvenhorses = 0;
	} else {
		fig->horses = get_item(u, I_HORSE);
		fig->elvenhorses = get_item(u, I_UNICORN);
	}

	for (i = 0; i != AR_MAX; ++i)
		if (race[u->race].battle_flags & BF_EQUIPMENT)
			fig->armor[i] = get_item(u, armordata[i].item);


	/* Jetzt muß noch geschaut werden, wo die Einheit die jeweils besten
	 * Werte hat, das kommt aber erst irgendwo später. Ich entscheide
	 * wärend des Kampfes, welche ich nehme, je nach Gegner. Deswegen auch
	 * keine addierten boni. */

	/* Zuerst mal die Spezialbehandlung gewisser Sonderfälle. */
	fig->magic = eff_skill(u, SK_MAGIC, r);

	if (fig->horses) {
		if ((r->terrain != T_PLAIN && r->terrain != T_HIGHLAND
				&& r->terrain != T_DESERT) || rtrees(r) >= 600
				|| eff_skill(u, SK_RIDING, r) < 2 || u->race == RC_TROLL)
			fig->horses = 0;
	}

	if (fig->elvenhorses) {
		if (eff_skill(u, SK_RIDING, r) < 5 || u->race == RC_TROLL)
			fig->elvenhorses = 0;
	}

	/* Schauen, wie gut wir in Taktik sind. */
	if (t > 0 && u->race == RC_INSECT)
		t -= 1 - (int) log10(fig->side->size[SUM_ROW]);
	if (t > 0 && get_unitrow(fig) == FIGHT_ROW)
		t += 1;
#ifdef TACTICS_MALUS
	if (t > 0 && get_unitrow(fig) > BEHIND_ROW)
		t -= 1;
#endif
#if TACTICS_RANDOM
	if (t > 0) {
		int bonus = 0;

		for (i = 0; i < fig->alive; i++) {
			int p_bonus = 0;
			int rnd;

			do {
				rnd = rand()%100;
				if (rnd >= 40 && rnd <= 69)
					p_bonus += 1;
				else if (rnd <= 89)
					p_bonus += 2;
				else
					p_bonus += 3;
			} while(rnd >= 97);
			bonus = max(p_bonus, bonus);
		}
		t += bonus;
	}
	/* Nicht gut, da nicht personenbezogen. */
	/* if (t > 0) t += rand() % TACTICS_RANDOM; */
	/* statt +/- 2 kann man auch random(5) nehmen */
#endif
	add_tactics(&fig->side->leader, fig, t);
	return fig;
}


static fighter *
join_battle(battle * b, unit * u, boolean attack)
{
	fighter *c = NULL;
	fighter *fig;

	for_each(fig, b->fighters) {
		if (fig->unit == u) {
			c = fig;
			if (attack) fset(fig, FIG_ATTACKED);
			break;
		}
	}
	next(fig);
	if (!c) c = make_fighter(b, u, attack);
	return c;
}
static const char *
simplename(region * r)
{
	int i;
	static char name[17];
	const char * cp = rname(r, NULL);
	for (i=0;*cp && i!=16;++i, ++cp) {
		while (*cp && !isalpha(*cp) && !isspace(*cp)) ++cp;
		if (isspace(*cp)) name[i] = '_';
		else name[i] = *cp;
	}
	name[i]=0;
	return name;
}

static battle *
make_battle(region * r)
{
	battle *b = calloc(1, sizeof(struct battle));
	unit *u;
	faction *lastf = NULL;
	bfaction * bf;
	static int max_fac_no = 0; /* need this only once */

	if (!nobattledebug) {
		char zText[MAX_PATH];
		char zFilename[MAX_PATH];
		sprintf(zText, "%s/battles", basepath());
		makedir(zText, 0700);
#ifdef HAVE_ZLIB
		sprintf(zFilename, "%s/battle-%d-%s.log.gz", zText, obs_count, simplename(r));
		bdebug = gzopen(zFilename, "w");
#elif HAVE_BZ2LIB
		sprintf(zFilename, "%s/battle-%d-%s.log.bz2", zText, obs_count, simplename(r));
		bdebug = BZ2_bzopen(zFilename, "w");+                       
#else
		sprintf(zFilename, "%s/battle-%d-%s.log", zText, obs_count, simplename(r));
		bdebug = fopen(zFilename, "w");
#endif
		if (!bdebug) fputs("battles können nicht debugged werden\n", stderr);
		else {
			dbgprintf((bdebug, "In %s findet ein Kampf statt:", rname(r, NULL)));
		}
		obs_count++;
	}
	nextside = 0;

	/* cv_init(&b->sides); */
	b->region = r;
	b->plane = getplane(r);
	/* Finde alle Parteien, die den Kampf beobachten können: */
	list_foreach(unit, r->units, u)
		if (u->number > 0) {
		if (lastf != u->faction) {	/* Speed optimiert, aufeinanderfolgende
								 * * gleiche nicht getestet */
			lastf = u->faction;
			for (bf=b->factions;bf;bf=bf->next)
				if (bf->faction==lastf) break;
			if (!bf) {
				bf = calloc(sizeof(bfaction), 1);
				++b->nfactions;
				bf->faction = lastf;
				bf->next = b->factions;
				b->factions = bf;
			}
		}
	}
	list_next(u);

	for (bf=b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		max_fac_no = max(max_fac_no, f->no);
	}
	return b;
}

static void
free_side(side * si)
{
	int i;
	for (i=0;i!=16;++i) while (si->enemies[i]) {
		struct enemy * e = si->enemies[i]->nexthash;
		free(si->enemies[i]);
		si->enemies[i] = e;
	}
	cv_kill(&si->fighters);
}

static void
free_fighter(fighter * fig)
{
	free(fig->person);
}

static void
free_battle(battle * b)
{
	side *side;
	fighter *fighter;
	meffect *meffect;
	bfaction * bf;
	int max_fac_no = 0;

	if (bdebug) {
#ifdef HAVE_ZLIB
		gzclose(bdebug);
#elif HAVE_BZ2LIB
		BZ2_bzclose(bdebug);
#else
		fclose(bdebug);
#endif
	}

	for (bf=b->factions;bf;bf=bf->next) {
		faction * f = bf->faction;
		max_fac_no = max(max_fac_no, f->no);
	}

	for_each(side, b->sides) {
		free_side(side);
		free(side);
	}
	next(side);
	cv_kill(&b->sides);
	for_each(fighter, b->fighters) {
		free_fighter(fighter);
		free(fighter);
	}
	next(fighter);
	cv_kill(&b->fighters);
	for_each(meffect, b->meffects) {
		free(meffect);
	}
	next(meffect);
	cv_kill(&b->meffects);
/*
	cv_kill(&b->factions);
*/
}

static int *
get_alive(battle * b, side * s, faction * vf, boolean see)
{
	static int alive[NUMROWS];
	fighter *f;
	memset(alive, 0, NUMROWS * sizeof(int));
	for_each(f, s->fighters) {
		if (f->alive && seematrix(vf, s)==see)
			alive[get_unitrow(f)] += f->alive;
	} next(f);
	return alive;
}

static int
battle_report(battle * b)
{
	side *s, *s2;
	boolean cont = false;
	boolean komma;
	char buf2[1024];
	bfaction *bf;

	buf[0] = 0;
	for_each(s, b->sides) {
		fighter *f;

		s->alive = 0;
		for_each(f, s->fighters) {
			s->alive += f->alive;
		} next(f);
	} next(s);

	for_each(s, b->sides) {
		for_each(s2, b->sides) {
			if (s->alive && s2->alive && enemy(s, s2))
			{
				cont = true;
				s->bf->lastturn = b->turn;
				s2->bf->lastturn = b->turn;
			}
		} next(s2);
		if (cont) break;
	} next(s);

	printf(" %d", b->turn);
	fflush(stdout);

	for (bf=b->factions;bf;bf=bf->next) {
		faction * fac = bf->faction;
		fbattlerecord(fac, b->region, " ");
		if (cont == true)
			sprintf(buf2, "Einheiten vor der %d. Runde:",
					b->turn);
		else
			sprintf(buf2, "Einheiten nach dem Kampf:");
		fbattlerecord(fac, b->region, buf2);
		buf2[0] = 0;
		komma   = false;
		for_each(s, b->sides) {
			if (s->alive) {
				int r, k = 0, * alive = get_alive(b, s, fac, seematrix(fac, s));
				if (!seematrix(fac, s)) {
					sprintf(buf, "%sHeer %2d(Unb): ",
						komma==true?", ":"", s->index);
				} else {
					sprintf(buf, "%sHeer %2d(%s): ",
						komma==true?", ":"", s->index,abkz(s->bf->faction->name,3));
				}
				{
					int l = FIGHT_ROW;
					for (r=FIGHT_ROW;r!=NUMROWS;++r) {
						if (alive[r]) {
							if (l!=FIGHT_ROW) scat("+");
							while(k--) scat("0+");
							icat(alive[r]);
							k = 0;
							l=r+1;
						} else ++k;
					}
				}

				strcat(buf2, buf);
				if (komma == false) {
					komma = true;
				}
			}
		} next(s);
		fbattlerecord(fac, b->region, buf2);
	}
	return cont;
}

static void
join_allies(battle * b)
{
	region * r = b->region;
	unit * u;
	/* Die Anzahl der Teilnehmer kann sich in dieser Routine ändern.
	 * Deshalb muß das Ende des Vektors vorher gemerkt werden, damit
	 * neue Parteien nicht mit betrachtet werden:
	 */
	int size = cv_size(&b->sides);
	for (u=r->units;u;u=u->next)
		/* Was ist mit Schiffen? */
		if (u->status != ST_FLEE && u->status != ST_AVOID && !fval(u, FL_MOVED) && u->number > 0)
	{
		int si;
		faction * f = u->faction;
		fighter * c = NULL;
		for (si = 0; si != size; ++si) {
			int se;
			side *s = b->sides.begin[si];
#ifdef NOAID
			/* Wenn alle attackierten noch FL_NOAIDF haben, dann kämpfe nicht mit. */
			if (fval(s->bf->faction, FL_NOAIDF)) continue;
#endif
			if (s->bf->faction!=f) {
				/* Wenn wir attackiert haben, kommt niemand mehr hinzu: */
				if (s->bf->attacker) continue;
				/* Wenn alliierte attackiert haben, helfen wir nicht mit: */
				if (s->bf->faction!=f && s->bf->attacker) continue;
				/* alliiert müssen wir schon sein, sonst ist's eh egal : */
				if (!allied(u, s->bf->faction, HELP_FIGHT)) continue;
				/* einen alliierten angreifen dürfen sie nicht, es sei denn, der ist mit einem alliierten verfeindet, der nicht attackiert hat: */
			}
			for (se = 0; se != size; ++se) {
				side * evil = b->sides.begin[se];
				if (u->faction==evil->bf->faction) continue;
				if (allied(u, evil->bf->faction, HELP_FIGHT) &&
					!evil->bf->attacker) continue;
				if (enemy(s, evil)) break;
			}
			if (se==size) continue;
			/* Wenn die Einheit belagert ist, muß auch einer der Alliierten belagert sein: */
			if (besieged(u)) {
				void ** fi;
				boolean siege = false;
				for (fi = s->fighters.begin; !siege && fi != s->fighters.end; ++fi) {
					fighter *ally = *fi;
					if (besieged(ally->unit)) siege = true;
				}
				if (!siege) continue;
			}
			/* keine Einwände, also soll er mitmachen: */
			if (!c) c = join_battle(b, u, false);
			if (!c) continue;
			/* Die Feinde meiner Freunde sind meine Feinde: */
			for (se = 0; se != size; ++se) {
				side * evil = b->sides.begin[se];
				if (evil->bf->faction!=u->faction && enemy(s, evil)) {
					set_enemy(evil, c->side, false);
				}
			}
		}
	}
}

void
do_battle(void)
{
	region *r;
	char smallbuf[512];

	list_foreach(region, regions, r) {
		unit *u;
		battle *b = NULL;
		int sides = 0;
		side *s;
		void **fi;
		building *bu;

#ifdef NOAID
		for (u = r->units; u != NULL; u = u->next) {
			fset(u->faction, FL_NOAIDF);
		}
#endif

		/* list_foreach geht nicht, wegen flee() */
		for (u = r->units; u != NULL; u = u->next) {
			if (fval(u, FL_HADBATTLE)) continue;
			if (u->number > 0) {
				strlist *sl;

				list_foreach(strlist, u->orders, sl) {
					if (igetkeyword(sl->s) == K_ATTACK) {
						unit *u2;
						fighter *c1, *c2;

						if(r->planep && fval(r->planep, PFL_NOATTACK)) {
							cmistake(u, sl->s, 271, MSG_BATTLE);
							list_continue(sl);
						}

						/* Fehlerbehandlung Angreifer */
						if (is_spell_active(r, C_PEACE)) {
							sprintf(buf, "Hier ist es so schön friedlich, %s möchte "
									"hier niemanden angreifen.", unitname(u));
							mistake(u, sl->s, buf, MSG_BATTLE);
							list_continue(sl);
						}

						if (fval(u, FL_HUNGER)) {
							cmistake(u, sl->s, 225, MSG_BATTLE);
							list_continue(sl);
						}

						if (u->status == ST_AVOID || u->status == ST_FLEE) {
							cmistake(u, sl->s, 226, MSG_BATTLE);
							list_continue(sl);
						}

						/* ist ein Flüchtling aus einem andern Kampf */
						if (fval(u, FL_MOVED)) list_continue(sl);

						if (is_cursed(u->attribs, C_SLAVE, 0)) {
							sprintf(buf, "%s kämpft nicht.", unitname(u));
							mistake(u, sl->s, buf, MSG_BATTLE);
							list_continue(sl);
						}

						/* Fehler: "Das Schiff muß erst verlassen werden" */
						if (u->ship != NULL && rterrain(r) != T_OCEAN) {
							cmistake(u, sl->s, 19, MSG_BATTLE);
							list_continue(sl);
						}

						if (leftship(u)) {
							cmistake(u, sl->s, 234, MSG_BATTLE);
							list_continue(sl);
						}

						/* Ende Fehlerbehandlung Angreifer */

						/* attackierte Einheit ermitteln */
						u2 = getunit(r, u);

						/* Beginn Fehlerbehandlung */
						/* Fehler: "Die Einheit wurde nicht gefunden" */
						if (!u2 || fval(u2, FL_MOVED) || u2->number == 0
								|| !cansee(u->faction, u->region, u2, 0)) {
							cmistake(u, sl->s, 64, MSG_BATTLE);
							list_continue(sl);
						}
						/* Fehler: "Die Einheit ist eine der unsrigen" */
						if (u2->faction == u->faction) {
							cmistake(u, sl->s, 45, MSG_BATTLE);
							list_continue(sl);
						}
						/* Fehler: "Die Einheit ist mit uns alliert" */
						if (allied(u, u2->faction, HELP_FIGHT)) {
							cmistake(u, sl->s, 47, MSG_BATTLE);
							list_continue(sl);
						}
						/* xmas */
						if (u2->no==atoi36("xmas") && u2->irace==RC_GNOME) {
							a_add(&u->attribs, a_new(&at_key))->data.i = atoi36("coal");
							sprintf(buf, "%s ist böse gewesen...", unitname(u));
							mistake(u, sl->s, buf, MSG_BATTLE);
							list_continue(sl);
						}if (u2->faction->age < IMMUN_GEGEN_ANGRIFF) {
							sprintf(buf, "Eine Partei muß mindestens %d "
									"Wochen alt sein, bevor sie angegriffen werden kann",
									IMMUN_GEGEN_ANGRIFF);
							mistake(u, sl->s, buf, MSG_BATTLE);
							list_continue(sl);
						}
						/* Fehler: "Die Einheit ist mit uns alliert" */
						if (is_cursed(u->attribs, C_CALM, u2->faction->unique_id)) {
							cmistake(u, sl->s, 47, MSG_BATTLE);
							list_continue(sl);
						}
						/* Ende Fehlerbehandlung */

						if (!b) {
							b = make_battle(r);
						}
						c1 = join_battle(b, u, true);
						c2 = join_battle(b, u2, false);

#ifdef NOAID
						/* Hat die attackierte Einheit keinen Noaid-Status,
						 * wird das Flag von der Faction genommen, andere
						 * Einheiten greifen ein. */
						if (!fval(u2, FL_NOAID)) freset(u2->faction, FL_NOAIDF);
#endif
						if (c1 && c2) {
							/* Merken, wer Angreifer ist, für die Rückzahlung der
							 * Präcombataura bei kurzem Kampf. */
							c1->side->bf->attacker = true;

							set_enemy(c1->side, c2->side, true);
							if (!enemy(c1->side, c2->side)) {
								sprintf(buf, "%s attackiert %s", factionname(c1->side->bf->faction),
										factionname(c2->side->bf->faction));
								battledebug(buf);
							}
							sides = 1;
						}
					}
				}
				list_next(sl);
			}
		}
		if (!b)
			list_continue(r);

		/* Bevor wir die alliierten hineinziehen, sollten wir schauen, *
		 * Ob jemand fliehen kann. Dann erübrigt sich das ganze ja
		 * vielleicht schon. */
		print_header(b);
		if (!sides) {
			/* Niemand mehr da, Kampf kann nicht stattfinden. */
			battlerecord(b, "Der Kampf wurde abgebrochen, da alle "
					"Verteidiger flohen.");
			free_battle(b);
			free(b);
			list_continue(r);
		}
		join_allies(b);

		/* Jetzt reinitialisieren wir die Matrix nochmal, um auch die passiv
		 * in den Kampf gezogenen korrekt zu behandeln. */

		/* Jetzt wird gekämpft, je COMBAT_TURNS Runden lang, plus
		 * evtl.taktik: */

		/* Alle Mann raus aus der Burg! */
		list_foreach(building, rbuildings(r), bu) {
			bu->sizeleft = bu->size;
		}
		list_next(bu);

		/* Gibt es eine Taktikrunde ? */
		b->turn = 1;
#ifndef NEW_TACTICS
		if (cv_size(&b->leaders)) {
			b->turn=0;
			b->has_tactics_turn = true;
		} else {
			b->has_tactics_turn = false;
		}
#endif

		/* PRECOMBATSPELLS */
		do_combatmagic(b, DO_PRECOMBATSPELL);

		/* Nun erstellen wir eine Liste von allen Kämpfern, die wir
		 * dann scramblen */
		/* Zuerst werden sie wild gemischt, und dann wird (stabil) *
		 * nach der Kampfreihe sortiert */
		v_scramble(b->fighters.begin, b->fighters.end);
		v_sort(b->fighters.begin, b->fighters.end, (v_sort_fun) sort_fighterrow);
		for_each(s, b->sides) {
			v_sort(s->fighters.begin, s->fighters.end, (v_sort_fun) sort_fighterrow);
		}
		next(side);

		print_stats(b); /* gibt die Kampfaufstellung aus */
		printf("%s (%d, %d) : ", rname(r, NULL), r->x, r->y);

		b->dh = 0;
		/* Kämpfer zählen und festlegen, ob es ein kleiner Kampf ist */
		for (fi = b->fighters.begin; fi != b->fighters.end; ++fi) {
			fighter *fig = *fi;
			b->dh += fig->unit->number;
		}
		if (b->dh <= 30) {
			b->small = true;
		} else {
			b->small = false;
		}

		for (;battle_report(b) && b->turn<=COMBAT_TURNS;++b->turn) {
			int flee_ops = 1;
			int i;

			/* Der doppelte Versuch immer am Anfang der 1. Kampfrunde, nur
			 * ein Versuch vor der Taktikerrunde. */
			if (b->has_tactics_turn?b->turn==0:b->turn==1)
				flee_ops = 2;

			for (i=1;i<=flee_ops;i++) {
				for (fi = b->fighters.begin; fi != b->fighters.end; ++fi) {
					fighter *fig = *fi;
					unit *u = fig->unit;
					troop dt;
					int runners = 0;
					/* Flucht nicht bei mehr als 600 HP. Damit Wyrme tötbar bleiben. */
					int runhp = min(600,max(4, unit_max_hp(u)/5));
					side *side = fig->side;
					region *r = NULL;
					if (is_undead(u) || u->race == RC_SHADOWKNIGHT) continue;
					if (u->ship) continue;
					dt.fighter = fig;
					if (!fig->run_to) fig->run_to = fleeregion(u);
					r = fig->run_to;
					if (!r)
						continue;
					dt.index = fig->alive - fig->removed;
					while (side->size[SUM_ROW] && dt.index != 0) {
						--dt.index;
						assert(dt.index>=0 && dt.index<fig->unit->number);
						assert(fig->person[dt.index].hp > 0);

						/* Versuche zu fliehen, wenn
						 * - Kampfstatus fliehe
						 * - schwer verwundet
						 * - in panik (Zauber)
						 * aber nicht, wenn der Zaubereffekt Held auf dir liegt!
						 */
						if ((u->status == ST_FLEE
									|| fig->person[dt.index].hp <= runhp
									|| (fig->person[dt.index].flags & FL_PANICED))
								&& !(fig->person[dt.index].flags & FL_HERO))
						{
							double ispaniced = 0.0;
							if (fig->person[dt.index].flags & FL_PANICED) {
								ispaniced = EFFECT_PANIC_SPELL;
							}
							if (chance(min(fleechance(u)+ispaniced, 0.90))) {
								++runners;
								fig->run_hp += fig->person[dt.index].hp;
								++fig->run_number;
								remove_troop(dt);
								if (b->small) {
									sprintf(smallbuf, "%s/%d gelingt es, vom Schlachtfeld zu entkommen.",
										unitname(fig->unit), dt.index);
									battlerecord(b, smallbuf);
								}
							} else if (b->small) {
								sprintf(smallbuf, "%s/%d versucht zu fliehen, wird jedoch aufgehalten.",
										unitname(fig->unit), dt.index);
								battlerecord(b, smallbuf);
							}
						}
					}
				}
			}
			for (fi = b->fighters.begin; fi != b->fighters.end; ++fi) {
				fighter *fig = *fi;

				/* Kämpfer diese Runde: */
				fig->fighting = fig->alive - fig->removed;
				/* ist in dieser Einheit noch jemand handlungsfähig? */
				if (fig->fighting == 0) continue;

				/* Taktikrunde: */
				if (b->turn == 0) {
					side    *side;
					boolean yes = false;

					for_each(side, b->sides) {
						if (b->max_tactics > 0
							&& side->leader.value == b->max_tactics
							&& helping(side, fig->side))
						{
							yes = true;
							break;
						}
					} next(side);
					if (!yes)
						continue;
				}
				/* Handle the unit's attack on someone */
				do_attack(fig);
			}

			/* Regeneration */
			for (fi = b->fighters.begin; fi != b->fighters.end; ++fi) {
				fighter *fig = *fi;

				if(fspecial(fig->unit->faction, FS_REGENERATION)) {
					fig->fighting = fig->alive - fig->removed;
					if(fig->fighting == 0) continue;
					do_regenerate(fig);
				}
			}
		}

		printf("\n");

		/* Auswirkungen berechnen: */
		aftermath(b);
		/*
#if MALLOCDBG
		assert(_CrtCheckMemory());
#endif
		*/
		/* Hier ist das Gefecht beendet, und wir können die
		 * Hilfsstrukturen * wieder löschen: */

		if (b) {
			free_battle(b);
			free(b);
		}
	}
	list_next(r);
}

/* ------------------------------------------------------------- */

/* Funktionen, die außerhalb von battle.c verwendet werden. */
int
nb_armor(unit *u, int index)
{
	int a, av = 0;
	int geschuetzt = 0;

	if (!(race[u->race].battle_flags & BF_EQUIPMENT)) return AR_NONE;

	/* Normale Rüstung */

	a = 0;
	do {
		if (armordata[a].shield == 0) {
			geschuetzt += get_item(u, armordata[a].item);
			if (geschuetzt > index)
				av += armordata[a].prot;
		}
		++a;
	}
	while (a != AR_MAX);

	/* Schild */

	a = 0;
	do {
		if (armordata[a].shield == 1) {
			geschuetzt += get_item(u, armordata[a].item);
			if (geschuetzt > index)
				av += armordata[a].prot;
		}
		++a;
	}
	while (a != AR_MAX);

	return av;
}


int
damage_unit(unit *u, const char *dam, boolean armor, boolean magic)
{
	int *hp = malloc(u->number * sizeof(int));
	int   h = u->hp/u->number;
	int   i, dead = 0, hp_rem = 0, heiltrank;

	/* HP verteilen */
	for (i=0; i<u->number; i++) hp[i] = h;
	h = u->hp - (u->number * h);
	for (i=0; i<h; i++) hp[i]++;

	/* Schaden */
	for (i=0; i<u->number; i++) {
		int damage = dice_rand(dam);
		if (magic) damage = (int)(damage * (100.0 - magic_resistance(u))/100.0);
		if (armor) damage -= nb_armor(u, i);
		hp[i] -= damage;
	}

	/* Auswirkungen */
	for (i=0; i<u->number; i++) {
		if (hp[i] < 0){
			heiltrank = 0;

			/* Sieben Leben */
			if (u->race == RC_CAT && (chance(1.0 / 7))) {
				hp[i] = u->hp/u->number;
				hp_rem += hp[i];
				continue;
			}

			/* Heiltrank */
			if (get_effect(u, oldpotiontype[P_HEAL]) > 0) {
				change_effect(u, oldpotiontype[P_HEAL], -1);
				heiltrank = 1;
			} else if (get_potion(u, P_HEAL) > 0) {
				i_change(&u->items, oldpotiontype[P_HEAL]->itype, -1);
				change_effect(u, oldpotiontype[P_HEAL], 3);
				heiltrank = 1;
			}
			if (heiltrank && (chance(0.50))) {
				hp[i] = u->hp/u->number;
				hp_rem += hp[i];
				continue;
			}

		 	dead++;
		}	else {
			hp_rem += hp[i];
		}
	}

	scale_number(u, u->number - dead);
	u->hp = hp_rem;

	free(hp);

	return dead;
}
