/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/
#include <config.h>
#include "eressea.h"

/* kernel includes */
#include "battle.h"
#include "build.h"
#include "building.h"
#include "faction.h"
#include "item.h"
#include "magic.h"
#include "region.h"
#include "unit.h"
#include "movement.h"
#include "spell.h"
#include "race.h"
#include "skill.h"

/* util includes */
#include <rand.h>
#include <base36.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

#define EFFECT_HEALING_SPELL     5

/* ------------------------------------------------------------------ */
/* Kampfzauberfunktionen */

/* COMBAT */

static const char *
spell_damage(int sp)
{
	switch (sp) {
		case 0:
			/* meist tödlich 20-65 HP */
			return "5d10+15";
		case 1:
			/* sehr variabel 4-48 HP */
			return "4d12";
		case 2:
			/* leicht verwundet 4-18 HP */
			return "2d8+2";
		case 3:
			/* fast immer tödlich 30-50 HP */
			return "5d5+25";
		case 4:
			/* verwundet 11-26 HP */
			return "3d6+8";
		case 5:
			/* leichter Schaden */
			return "2d4";
		default:
			/* schwer verwundet 14-34 HP */
			return "4d6+10";
	}
}

static double
get_force(double power, int formel)
{
	switch (formel) {
		case 0:
			/* (4,8,12,16,20,24,28,32,36,40,44,..)*/
			return (power * 4);
		case 1:
			/* (15,30,45,60,75,90,105,120,135,150,165,..) */
			return (power*15);
		case 2:
			/* (40,80,120,160,200,240,280,320,360,400,440,..)*/
			return (power*40);
		case 3:
			/* (2,8,18,32,50,72,98,128,162,200,242,..)*/
			return (power*power*2);
		case 4:
			/* (4,16,36,64,100,144,196,256,324,400,484,..)*/
			return (power*power*4);
		case 5:
			/* (10,40,90,160,250,360,490,640,810,1000,1210,1440,..)*/
			return (power*power*10);
		case 6:
			/* (6,24,54,96,150,216,294,384,486,600,726,864)*/
			return (power*power*6);
		default:
			return power;
	}
}

/* Generischer Kampfzauber */
int
sp_kampfzauber(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	troop dt;
	troop at;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	int force, enemies;
	int killed = 0;
	const char *damage;
	at.fighter = fi;
	at.index   = 0;

	if (power <= 0) return 0;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));

	switch(sp->id) {
	  	/* lovar halbiert im Schnitt! */
		case SPL_FIREBALL:
			damage = spell_damage(0);
			force = lovar(get_force(power,0));
			break;
		case SPL_HAGEL:
			damage = spell_damage(2);
			force = lovar(get_force(power,4));
			break;
		case SPL_METEORRAIN:
			damage = spell_damage(1);
			force = lovar(get_force(power,1));
			break;
		default:
			damage = spell_damage(10);
			force = lovar(get_force(power,10));
	}

	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	while (force>0 && killed < enemies) {
		dt = select_enemy(b, fi, minrow, maxrow);
		assert(dt.fighter);
		--force;
		killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
	} 

	sprintf(buf, "%d Personen %s getötet",
			killed, killed == 1 ? "wurde" : "wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}

/* Versteinern */
int
sp_versteinern(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	/* Wirkt auf erste und zweite Reihe */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW;
	int force, enemies;
	int stoned = 0;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));

	force = lovar(get_force(power,0));

	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	do {
		troop dt = select_enemy(b, fi, minrow, maxrow);
		fighter * df = dt.fighter;
		unit * du = df->unit;
		if (is_magic_resistant(mage, du, 0) == false) {
			/* person ans ende hinter die lebenden schieben */
			struct person p = dt.fighter->person[dt.index];
			++dt.fighter->removed;
			++dt.fighter->side->removed;
			++stoned;
			dt.fighter->person[dt.index] = dt.fighter->person[df->alive-df->removed];
			dt.fighter->person[(df->alive - df->removed)] = p;
		}
		--force;
	} while (force && stoned < enemies);

	sprintf(buf, "%d Personen %s versteinert.",
			stoned, stoned == 1 ? "wurde" : "wurden");
	battlerecord(b, buf);
	return level;
}

/* Benommenheit: eine Runde kein Angriff */
int
sp_stun(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	troop at;
	/* Aus beiden Reihen nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW;
	int force=0, enemies;
	int stunned;
	at.fighter = fi;
	at.index   = 0;

	if (power <= 0) return 0;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));

	switch(sp->id) {
		case SPL_SHOCKWAVE:
			force = lovar(get_force(power,1));
			break;
		default:
			assert(0);
	}

	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	stunned = 0;
	do {
		troop dt = select_enemy(b, fi, minrow, maxrow);
		fighter * df = dt.fighter;
		unit * du = df->unit;

		--force;
		if (is_magic_resistant(mage, du, 0) == false) {
				df->person[dt.index].flags |= FL_STUNNED;
				++stunned;
		}
	} while (force && stunned < enemies);

	sprintf(buf, "%d Krieger %s für einen Moment benommen.",
			stunned, stunned == 1 ? "ist" : "sind");

	scat(".");
	battlerecord(b, buf);
	return level;
}

/* ------------------------------------------------------------- */
/* Für Sprüche 'get_scrambled_list_of_enemys_in_row', so daß man diese
 * Liste nur noch einmal durchlaufen muss, um Flächenzauberwirkungen
 * abzuarbeiten */

/* Rosthauch */
int
sp_combatrosthauch(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	cvector *fgs;
	void **fig;
	int force, enemies;
	int k = 0;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	static const char * msgt[] = {
		"ruft ein fürchterliches Unwetter über seine Feinde, doch es gab niemanden mehr, den dies treffen konnte.",
		"ruft ein fürchterliches Unwetter über seine Feinde, doch der magische Regen zeigt keinen Effekt.",
		"ruft ein fürchterliches Unwetter über seine Feinde, Der magischen Regen lässt alles Eisen rosten."
	};
	unused(sp);

	force = lovar(power * 15);

	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		battlemsg(b, fi->unit, msgt[0]);
		return 0;
	}

	fgs = fighters(b, fi, minrow, maxrow, FS_ENEMY);
	v_scramble(fgs->begin, fgs->end);

	for (fig = fgs->begin; fig != fgs->end; ++fig) {
		fighter *df = *fig;
		if (!force)
			break;

		/* da n min(force, x), sollte force maximal auf 0 sinken */
		assert(force >= 0);

		/* Eisenwaffen: I_SWORD, I_GREATSWORD, I_AXE, I_HALBERD (50%) */

		if (df->weapons) {
			int w;
			for (w=0;df->weapons[w].type!=NULL;++w) {
				weapon * wp = df->weapons;
				int n = min(force, wp->used);
				if (n) {
					requirement * mat = wp->type->itype->construction->materials;
					boolean iron = false;
					while (mat && mat->number>0) {
						if (mat->type==R_IRON) {
							iron = true;
							break;
						}
						mat++;
					}
					if (iron) {
						int p;
						force -=n;
						wp->used -= n;
						k +=n;
						i_change(&df->unit->items, wp->type->itype, -n);
						for (p=0;n && p!=df->unit->number;++p) {
							if (df->person[p].missile==wp) {
								df->person[p].missile = NULL;
								--n;
							}
						}
						for (p=0;n && p!=df->unit->number;++p) {
							if (df->person[p].melee==wp) {
								df->person[p].melee = NULL;
								--n;
							}
						}
					}
				}
			}
		}
	}
	cv_kill(fgs);

	if (k == 0) {
		/* keine Waffen mehr da, die zerstört werden könnten */
		battlemsg(b, fi->unit, msgt[1]);
		fi->magic = 0; /* kämpft nichtmagisch weiter */
		level = 0;
	} else {
		battlemsg(b, fi->unit, msgt[2]);
	}
	return level;
}

int
sp_sleep(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	unit *du;
	troop dt;
	int force, enemies;
	int k = 0;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	force = lovar(power * 25);
	enemies = count_enemies(b, fi->side, minrow, maxrow);

	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	do {
		dt = select_enemy(b, fi, minrow, maxrow);
		assert(dt.fighter);
		du = dt.fighter->unit;
		if (is_magic_resistant(mage, du, 0) == false) {
			dt.fighter->person[dt.index].flags |= FL_SLEEPING;
			++k;
			--enemies;
		}
		--force;
	} while (force && enemies);

	sprintf(buf, "%d Krieger %s in Schlaf versetzt.",
			k, k == 1 ? "wurde" : "wurden");
	battlerecord(b, buf);
	return level;
}


static troop
select_ally_in_row(fighter * af, int minrow, int maxrow)
{
	side *as = af->side;
	battle *b = as->battle;
	troop dt = no_troop;
	fighter *df;
	int allies;

	allies = countallies(as);

	if (!allies)
		return dt;
	allies = rand() % allies;

	cv_foreach(df, b->fighters) {
		side *ds = df->side;
		int dr = get_unitrow(df);

		if (helping(as, ds) && dr >= minrow && dr <= maxrow) {
			if (df->alive - df->removed > allies) {
				dt.index = allies;
				dt.fighter = df;
				break;
			}
			allies -= df->alive;
		}
	}
	cv_next(df);
	return dt;
}

int
sp_speed(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	int force;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW;
	int allies;
	int targets = 0;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));
	scat(":");
	battlerecord(b, buf);

	force = lovar(power * power * 5);

	allies = countallies(fi->side);
	/* maximal 2*allies Versuche ein Opfer zu finden, ansonsten bestände
	 * die Gefahr eine Endlosschleife*/
	allies *= 2;

	do {
		troop dt = select_ally_in_row(fi, minrow, maxrow);
		fighter *df = dt.fighter;
		--allies;

		if (df) {
			if (df->person[dt.index].speed == 1) {
				df->person[dt.index].speed++;
				targets++;
				--force;
			}
		}
	} while (force && allies);

	sprintf(buf, "%d Krieger %s magisch beschleunigt.",
			targets, targets == 1 ? "wurde" : "wurden");
	battlerecord(b, buf);
	return 1;
}

static skill_t
random_skill(unit *u)
{
	int n = 0;
	skill * sv;

  for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
		if (sv->level>0) ++n;
	}

	if(n == 0)
		return NOSKILL;

	n = rand()%n;

  for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
		if (sv->level>0) {
			if (n-- == 0) return sv->id;
		}
	}

	assert(0==1); /* Hier sollte er niemals ankommen. */
	return NOSKILL;
}

int
sp_mindblast(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	troop dt;
	unit *du;
	skill_t sk;
	int killed = 0;
	int force, enemies;
	int k = 0;
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	force = lovar(power * 25);

	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	do {
		dt = select_enemy(b, fi, minrow, maxrow);
		assert(dt.fighter);
		du = dt.fighter->unit;
		if (humanoidrace(du->race) && is_magic_resistant(mage, du, 0) == false) {
			sk = random_skill(du);
			if (sk != NOSKILL) {
				/* Skill abziehen */
				int n = 30+rand()%61;
				skill * sv = get_skill(du, sk);
				while (n>0) {
					if (n>=30*du->number) {
						reduce_skill(du, sv, 1);
						n-=30;
					} else {
						if (rand()%(30*du->number)<n) reduce_skill(du, sv, 1);
						n = 0;
					}
				}
				--enemies;
			} else {
				troop t;
				/* Keine Skills mehr, Einheit töten */
				t.fighter = dt.fighter;
				t.index = 0;
				while(dt.fighter->alive - dt.fighter->removed) {
					rmtroop(t);
					enemies--;
					killed++;
				}
			}
			k++;
		}
		--force;
	} while (force && enemies);

	sprintf(buf, "%d Krieger %s Erinnerungen",
			k, k == 1 ? "verliert" : "verlieren");

	if (killed > 0) {
		scat(", ");
		icat(killed);
		scat(" Krieger ");
		if (killed == 1) {
			scat("wurde");
		} else {
			scat("wurden");
		}
		scat(" getötet");
	}
	scat(".");

	battlerecord(b, buf);
	return level;
}

int
sp_dragonodem(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	troop dt;
	troop at;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	int force, enemies;
	int killed = 0;
	const char *damage;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));
	/* 11-26 HP */
	damage = spell_damage(4);
	/* Jungdrache 3->54, Drache 6->216, Wyrm 12->864 Treffer */
	force = lovar(get_force(level,6));

	enemies = count_enemies(b, fi->side, minrow, maxrow);

	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	at.fighter = fi;
	at.index = 0;

	do {
		dt = select_enemy(b, fi, minrow, maxrow);
		assert(dt.fighter);
		--force;
		killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
	} while (force && killed < enemies);

	sprintf(buf, "%d Personen %s getötet",
			killed, killed == 1 ? "wurde" : "wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}

/* Feuersturm: Betrifft sehr viele Gegner (in der Regel alle), 
 * macht nur vergleichsweise geringen Schaden */
int
sp_immolation(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	troop dt;
	troop at;
	int minrow = FIGHT_ROW;
	int maxrow = AVOID_ROW;
	int force, enemies;
	int killed = 0;
	const char *damage;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));
	/* 2d4 HP */
	damage = spell_damage(5);
	/* Betrifft alle Gegner */
	force = 99999;

	enemies = count_enemies(b, fi->side, minrow, maxrow);

	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	at.fighter = fi;
	at.index = 0;

	do {
		dt = select_enemy(b, fi, minrow, maxrow);
		assert(dt.fighter);
		--force;
		killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
	} while (force && killed < enemies);

	sprintf(buf, "%d Personen %s getötet",
			killed, killed == 1 ? "wurde" : "wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}

int
sp_drainodem(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	troop dt;
	troop at;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	int force, enemies;
	int drained = 0;
	int killed = 0;
	const char *damage;

	sprintf(buf, "%s zaubert %s", unitname(fi->unit), 
		spell_name(sp, default_locale));
	/* 11-26 HP */
	damage = spell_damage(4);
	/* Jungdrache 3->54, Drache 6->216, Wyrm 12->864 Treffer */
	force = lovar(get_force(level,6));

	enemies = count_enemies(b, fi->side, minrow, maxrow);

	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	at.fighter = fi;
	at.index = 0;

	do {
		dt = select_enemy(b, fi, minrow, maxrow);
		assert(dt.fighter);
		if (hits(at, dt, NULL)) {
			drain_exp(dt.fighter->unit, 90);
			drained++;
		}
		killed += terminate(dt, at, AT_COMBATSPELL, damage, false);
		--force;
	} while (force && drained < enemies);

	sprintf(buf, "%d Person%s wurde ihre Lebenskraft entzogen",
		drained, drained == 1 ? " wurde" : "en wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}

/* ------------------------------------------------------------- */
/* PRECOMBAT */

int
sp_shadowcall(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	region *r = b->region;
	unit *mage = fi->unit;
	attrib *a;
	int force = (int)(get_force(power, 3)/2);
	const race *rc = NULL;
	int num;
	unit *u;

	unused(sp);

	switch(rand()%3) {
	case 0:
		rc  = new_race[RC_SHADOWBAT];
		num = 5000+dice_rand("3d5000");
		break;
	case 1:
		rc = new_race[RC_NIGHTMARE];
		num = 500+dice_rand("3d500");
		break;
	case 2:
		rc = new_race[RC_VAMPUNICORN];
		num = 500+dice_rand("3d500");
		break;
	}
	
	u = createunit(r, mage->faction, force, rc);
	u->status = ST_FIGHT;

	set_string(&u->name, racename(mage->faction->locale, u, u->race));
	set_level(u, SK_WEAPONLESS, (int)(power/2));
	set_level(u, SK_AUSDAUER, (int)(power/2));
	u->hp = u->number * unit_max_hp(u);

	if (fval(mage, UFL_PARTEITARNUNG))
		fset(u, UFL_PARTEITARNUNG);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 0;
	a->data.ca[1] = 100;
	a_add(&u->attribs, a);

	make_fighter(b, u, fi->side, fval(fi, FIG_ATTACKED));
	sprintf(buf, "%s ruft %d %s zu Hilfe", unitname(mage), force,
		racename(default_locale, u, u->race));
	battlerecord(b, buf);
	return level;
}

int
sp_wolfhowl(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	region *r = b->region;
	unit *mage = fi->unit;
	attrib *a;
	int force = (int)(get_force(power, 3)/2);
	unit *u = createunit(r, mage->faction, force, new_race[RC_WOLF]);
	unused(sp);

	u->status = ST_FIGHT;

	set_string(&u->name, racename(mage->faction->locale, u, u->race));
	set_level(u, SK_WEAPONLESS, (int)(power/3));
	set_level(u, SK_AUSDAUER, (int)(power/3));
	u->hp = u->number * unit_max_hp(u);

	if (fval(mage, UFL_PARTEITARNUNG))
		fset(u, UFL_PARTEITARNUNG);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 0;
	a->data.ca[1] = 100;
	a_add(&u->attribs, a);

	make_fighter(b, u, fi->side, fval(fi, FIG_ATTACKED));
	sprintf(buf, "%s ruft %d %s zu Hilfe", unitname(mage), force,
		racename(default_locale, u, u->race));
	battlerecord(b, buf);
	return level;
}

int
sp_shadowknights(fighter * fi, int level, double power, spell * sp)
{
	unit *u;
	battle *b = fi->side->battle;
	region *r = b->region;
	unit *mage = fi->unit;
	attrib *a;
	int force = (int)get_force(power, 3);

	unused(sp);

	u = createunit(r, mage->faction, force, new_race[RC_SHADOWKNIGHT]);
	u->status = ST_FIGHT;

	set_string(&u->name, "Schattenritter");
	u->hp = u->number * unit_max_hp(u);

	if (fval(mage, UFL_PARTEITARNUNG))
		fset(u, UFL_PARTEITARNUNG);

	a = a_new(&at_unitdissolve);
	a->data.ca[0] = 0;
	a->data.ca[1] = 100;
	a_add(&u->attribs, a);

	make_fighter(b, u, fi->side, fval(fi, FIG_ATTACKED));

	sprintf(buf, "%s beschwört Trugbilder herauf", unitname(mage));
	battlerecord(b, buf);
	return level;
}

int
sp_strong_wall(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	building *burg;
	int effect = (int)(power/4);
	static boolean init = false;
	static const curse_type * strongwall_ct;
	if (!init) { init = true; strongwall_ct = ct_find("strongwall"); }

	unused(sp);

	if (!mage->building) {
		sprintf(buf, "%s zaubert nicht, denn dieser Zauber hätte hier keinen "
				"Sinn.", unitname(mage));
		battlerecord(b, buf);
		return 0;
	}
	burg = mage->building;

	if (chance(power-effect)) ++effect;

	create_curse(mage, &burg->attribs, strongwall_ct, power, 1, effect, 0);

	sprintf(buf, "%s Mauern erglühen in einem unheimlichen magischen Licht.",
			buildingname(burg));
	battlerecord(b, buf);
	return level;
}

int
sp_chaosrow(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	cvector *fgs;
	void **fig;
	int n, enemies, row;
	int k = 0;
	int minrow = FIGHT_ROW;
	int maxrow = NUMROWS;

	switch (sp->id) {
		case SPL_CHAOSROW:
			sprintf(buf, "%s murmelt eine düster klingende Formel", unitname(mage));
			power *= 40;
			break;

		case SPL_SONG_OF_CONFUSION:
			sprintf(buf, "%s stimmt einen seltsamen Gesang an", unitname(mage));
			power = get_force(power, 5);
			break;
	}

	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(". ");

	fgs = fighters(b, fi, minrow, maxrow, FS_ENEMY);
	v_scramble(fgs->begin, fgs->end);

	for (fig = fgs->begin; fig != fgs->end; ++fig) {
		fighter *df = *fig;

		if (power<=0.0) break;
		/* force sollte wegen des max(0,x) nicht unter 0 fallen können */

		if (is_magic_resistant(mage, df->unit, 0)) continue;

		n = df->unit->number;

		if (chance(power/n)) {
			row = statusrow(df->status)+FIRST_ROW;
			df->side->size[row] -= df->alive;
			if (df->unit->race->battle_flags & BF_NOBLOCK) {
				df->side->nonblockers[row] -= df->alive;
			}
			row = FIRST_ROW + (rand()%(LAST_ROW-FIRST_ROW));
			switch (row) {
				case FIGHT_ROW:
					df->status = ST_FIGHT;
					break;
				case BEHIND_ROW:
					df->status = ST_CHICKEN;
					break;
				case AVOID_ROW:
					df->status = ST_AVOID;
					break;
				case FLEE_ROW:
					df->status = ST_FLEE;
					break;
				default:
					assert(!"unknown combatrow");
			}
			df->side->size[row] += df->alive;
			if (df->unit->race->battle_flags & BF_NOBLOCK) {
				df->side->nonblockers[row] += df->alive;
			}
			k+=df->alive;
		}
		power = max(0, power-n);
	}
	cv_kill(fgs);

	scat("Ein plötzlicher Tumult entsteht");
	if (k > 0) {
		scat(" und bringt die Kampfaufstellung durcheinander.");
	} else {
		scat(", der sich jedoch schnell wieder legt.");
	}
	battlerecord(b, buf);
	return level;
}

/* Gesang der Furcht (Kampfzauber) */
/* Panik (Präkampfzauber) */

int
sp_flee(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	cvector *fgs;
	void **fig;
	int minrow = FIGHT_ROW;
	int maxrow = AVOID_ROW;
	int force, n;
	int panik = 0;

	switch(sp->id) {
		case SPL_FLEE:
			sprintf(buf, "%s zaubert %s", unitname(mage), 
				spell_name(sp, default_locale));
			force = (int)get_force(power,4);
			break;
		case SPL_SONG_OF_FEAR:
			sprintf(buf, "%s stimmt einen düsteren Gesang an", unitname(mage));
			force = (int)get_force(power,3);
			break;
		case SPL_AURA_OF_FEAR:
			sprintf(buf, "%s ist von dunklen Schatten umgeben", unitname(mage));
			force = (int)get_force(power,5);
			break;
		default:
			force = (int)get_force(power,10);
	}

	if (!count_enemies(b, fi->side, minrow, maxrow)) {
		scat(", aber es gab niemanden mehr, der beeinflusst werden konnte.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	fgs = fighters(b, fi, minrow, maxrow, FS_ENEMY);
	v_scramble(fgs->begin, fgs->end);

	for (fig = fgs->begin; fig != fgs->end; ++fig) {
		fighter *df = *fig;
		for (n = 0; n < df->alive; n++) {
			if (force < 0)
				break;

			if (df->person[n].flags & FL_PANICED) { /* bei SPL_SONG_OF_FEAR möglich */
				df->person[n].attack -= 1;
				--force;
				++panik;
			} else if (!(df->person[n].flags & FL_HERO)
					|| !fval(df->unit->race, RCF_UNDEAD))
			{
				if (is_magic_resistant(mage, df->unit, 0) == false) {
					df->person[n].flags |= FL_PANICED;
					++panik;
				}
				--force;
			}
		}
	}
	cv_kill(fgs);

	sprintf(buf, "%d Krieger %s von Furcht gepackt.", panik,
			panik == 1 ? "wurde" : "wurden");
	battlerecord(b, buf);

	return level;
}

/* Heldenmut */
int
sp_hero(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	int df_bonus = 0;
	int force = 0;
	int allies;
	int targets = 0;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	switch(sp->id) {
		case SPL_HERO:
			df_bonus = (int)(power/5);
			force = lovar(get_force(power, 4));
			break;

		default:
			df_bonus = 1;
			force = (int)power;
	}
	scat(":");
	battlerecord(b, buf);

	allies = countallies(fi->side);
	/* maximal 2*allies Versuche ein Opfer zu finden, ansonsten bestände
	 * die Gefahr eine Endlosschleife*/
	allies *= 2;

	do {
		troop dt = select_ally_in_row(fi, minrow, maxrow);
		fighter *df = dt.fighter;
		--allies;

		if (df) {
			if (!(df->person[dt.index].flags & FL_HERO)) {
				df->person[dt.index].defence += df_bonus;
				df->person[dt.index].flags = df->person[dt.index].flags | FL_HERO;
				targets++;
				--force;
			}
		}
	} while (force && allies);

	sprintf(buf, "%d Krieger %s moralisch gestärkt",
			targets, targets == 1 ? "wurde" : "wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}

int
sp_berserk(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	int at_bonus = 0;
	int df_malus = 0;
	int force = 0;
	int allies = 0;
	int targets = 0;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	switch(sp->id) {
		case SPL_BERSERK:
		case SPL_BLOODTHIRST:
			at_bonus = max(1,level/3);
			df_malus = 2;
			force = (int)get_force(power,2);
			break;

		default:
			at_bonus = 1;
			df_malus = 0;
			force = (int)power;
	}
	scat(":");
	battlerecord(b, buf);

	allies = countallies(fi->side);
	/* maximal 2*allies Versuche ein Opfer zu finden, ansonsten bestände
	 * die Gefahr eine Endlosschleife*/
	allies *= 2;

	do {
		troop dt = select_ally_in_row(fi, minrow, maxrow);
		fighter *df = dt.fighter;
		--allies;

		if (df) {
			if (!(df->person[dt.index].flags & FL_HERO)) {
				df->person[dt.index].attack += at_bonus;
				df->person[dt.index].defence -= df_malus;
				df->person[dt.index].flags = df->person[dt.index].flags | FL_HERO;
				targets++;
				--force;
			}
		}
	} while (force && allies);

	sprintf(buf, "%d Krieger %s in Blutrausch versetzt",
			targets, targets == 1 ? "wurde" : "wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}

int
sp_frighten(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	/* Immer aus der ersten Reihe nehmen */
	int minrow = FIGHT_ROW;
	int maxrow = BEHIND_ROW-1;
	int at_malus = 0;
	int df_malus = 0;
	int force = 0;
	int enemies = 0;
	int targets = 0;

	at_malus = max(1,level - 4);
	df_malus = 2;
	force = (int)get_force(power, 2);

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}
	scat(":");
	battlerecord(b, buf);

	do {
		troop dt = select_enemy(b, fi, minrow, maxrow);
		fighter *df = dt.fighter;
		--enemies;

		if (!df)
			break;

		assert(!helping(fi->side, df->side));

		if (df->person[dt.index].flags & FL_HERO) {
			df->person[dt.index].flags &= ~(FL_HERO);
		}
		if (is_magic_resistant(mage, df->unit, 0) == false) {
			df->person[dt.index].attack -= at_malus;
			df->person[dt.index].defence -= df_malus;
			targets++;
		}
		--force;
	} while (force && enemies);

	sprintf(buf, "%d Krieger %s eingeschüchtert",
			targets, targets == 1 ? "wurde" : "wurden");

	scat(".");
	battlerecord(b, buf);
	return level;
}


int
sp_tiredsoldiers(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	int n = 0;
	int force = (int)(power * power * 4);

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	if (!count_enemies(b, fi->side, FIGHT_ROW, BEHIND_ROW)) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}

	while (force) {
		troop t = select_enemy(b, fi, FIGHT_ROW, BEHIND_ROW);
		fighter *df = t.fighter;

		if (!df)
			break;

		assert(!helping(fi->side, df->side));
		if (!(df->person[t.index].flags & FL_TIRED)) {
			if (is_magic_resistant(mage, df->unit, 0) == false) {
				df->person[t.index].flags = df->person[t.index].flags | FL_TIRED;
				df->person[t.index].defence -= 2;
				++n;
			}
		}
		--force;
	}

	scat(": ");
	if (n == 0) {
		scat("Der Zauber konnte keinen Krieger ermüden.");
	} else if (n == 1) {
		scat("Ein Krieger schleppt sich müde in den Kampf.");
	} else {
		icat(n);
		scat(" Krieger schleppen sich müde in den Kampf.");
	}
	battlerecord(b, buf);
	return level;
}

int
sp_windshield(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	int force, at_malus;
	int enemies;
	/* Immer aus der hinteren Reihe nehmen */
	int minrow = BEHIND_ROW;
	int maxrow = BEHIND_ROW;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	switch(sp->id) {
		case SPL_WINDSHIELD:
			force = (int)get_force(power,4);
			at_malus = level/4;
			break;

		default:
			force = (int)power;
			at_malus = 2;
	}
	enemies = count_enemies(b, fi->side, minrow, maxrow);
	if (!enemies) {
		scat(", aber niemand war in Reichweite.");
		battlerecord(b, buf);
		return 0;
	}

	do {
		troop dt = select_enemy(b, fi, minrow, maxrow);
		fighter *df = dt.fighter;
		--enemies;

		if (!df)
			break;
		assert(!helping(fi->side, df->side));

		if (df->person[dt.index].missile) {
			/* this suxx... affects your melee weapon as well. */
			df->person[dt.index].attack -= at_malus;
			--force;
		}
	} while (force && enemies);

	scat(": ");
	scat("Ein Sturm kommt auf und die Schützen können kaum noch zielen.");
	battlerecord(b, buf);
	return level;
}

int
sp_reeling_arrows(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;

  unused(power);

	b->reelarrow = true;
	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	scat(": ");
	scat("Ein Sturm kommt auf und die Schützen können kaum noch zielen.");
	battlerecord(b, buf);
	return level;
}

int
sp_denyattack(fighter * fi, int level, double power, spell * sp)
{
/* Magier weicht dem Kampf aus. Wenn er sich bewegen kann, zieht er in
 * eine Nachbarregion, wobei ein NACH berücksichtigt wird. Ansonsten
 * bleibt er stehen und nimmt nicht weiter am Kampf teil. */
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	region *r = b->region;
	unused(power);

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	scat(": ");

	/* Fliehende Einheiten verlassen auf jeden Fall Gebäude und Schiffe. */
	leave(r, mage);
	/* und bewachen nicht */
	setguard(mage, GUARD_NONE);
	/* irgendwie den langen befehl sperren */
	fset(fi, FIG_ATTACKED);

	/* Hat der Magier ein NACH, wird die angegebene Richtung bevorzugt */
	if (igetkeyword(mage->thisorder, mage->faction->locale) == K_MOVE
				|| igetkeyword(mage->thisorder, mage->faction->locale) == K_ROUTE)
	{
		fi->run.region = movewhere(r, mage);
		if (!fi->run.region) {
			cmistake(mage, findorder(mage, mage->thisorder), 71, MSG_MOVE);
			fi->run.region = fleeregion(mage);
		}
	} else {
		fi->run.region = fleeregion(mage);
	}
	/* bewegung erst am Ende des Kampfes, zusammen mit den normalen
	 * Flüchtlingen */
	/* travel(r, mage, fi->run.region, 1); */

	/* wir tun so, als wäre die Person geflohen */
	fset(fi, FIG_NOLOOT);
	fi->run.hp = mage->hp;
	fi->run.number = mage->number;
	/* fighter leeren */
	rmfighter(fi, mage->number);

	scat("Das Kampfgetümmel erstirbt und er kann unbehelligt "
			"seines Weges ziehen.");
	battlerecord(b, buf);
	return level;
}

static void
do_meffect(fighter * af, int typ, int effect, int duration)
{
	battle *b = af->side->battle;
	meffect *meffect = calloc(1, sizeof(struct meffect));
	cv_pushback(&b->meffects, meffect);
	meffect->magician = af;
	meffect->typ = typ;
	meffect->effect = effect;
	meffect->duration = duration;
}

int
sp_armorshield(fighter * fi, int level, double power, spell * sp)
{
	int effect;
	int duration;
	unit *mage = fi->unit;
	battle *b = fi->side->battle;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	battlerecord(b, buf);

	/* gibt Rüstung +effect für duration Treffer */

	switch(sp->id) {
		case SPL_ARMORSHIELD:
			effect = level/3;
			duration = (int)(20*power*power);
			break;

		default:
			effect = level/4;
			duration = (int)(power*power);
	}
	do_meffect(fi, SHIELD_ARMOR, effect, duration);
	return level;
}

int
sp_reduceshield(fighter * fi, int level, double power, spell * sp)
{
	int effect;
	int duration;
	unit *mage = fi->unit;
	battle *b = fi->side->battle;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	battlerecord(b, buf);

	/* jeder Schaden wird um effect% reduziert bis der Schild duration
	 * Trefferpunkte aufgefangen hat */

	switch(sp->id) {
		case SPL_REDUCESHIELD:
			effect = 50;
			duration = (int)(50*power*power);
			break;

		default:
			effect = level*3;
			duration = (int)get_force(power,5);
	}
	do_meffect(fi, SHIELD_REDUCE, effect, duration);
	return level;
}

int
sp_fumbleshield(fighter * fi, int level, double power, spell * sp)
{
	int effect;
	int duration;
	unit *mage = fi->unit;
	battle *b = fi->side->battle;

	sprintf(buf, "%s zaubert %s", unitname(mage), 
		spell_name(sp, default_locale));
	battlerecord(b, buf);

	/* der erste Zauber schlägt mit 100% fehl  */

	switch(sp->id) {
		case SPL_DRAIG_FUMBLESHIELD:
		case SPL_GWYRRD_FUMBLESHIELD:
		case SPL_CERRDOR_FUMBLESHIELD:
		case SPL_TYBIED_FUMBLESHIELD:
			duration = 100;
			effect = max(1, 25-level);
			break;

		default:
			duration = 100;
			effect = 10;
	}
	do_meffect(fi, SHIELD_BLOCK, effect, duration);
	return level;
}

/* ------------------------------------------------------------- */
/* POSTCOMBAT */

static int
count_healable(battle *b, fighter *df)
{
	side *s;
	int  healable = 0;

	cv_foreach(s, b->sides) {
		if (helping(df->side, s)) {
			healable += s->casualties;
		}
	} cv_next(s);
	return healable;
}

/* wiederbeleben */
int
sp_reanimate(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	int healable, j=0;
	double c = 0.50;
  double k = EFFECT_HEALING_SPELL * power;

	switch(sp->id) {
		case SPL_REANIMATE:
			sprintf(buf, "%s beginnt ein Ritual der Wiederbelebung",
					unitname(mage));
			c += 0.02 * power;
			break;

		default:
			sprintf(buf, "%s zaubert %s",
					unitname(mage), 
					spell_name(sp, default_locale));
	}
	if (get_item(mage, I_AMULET_OF_HEALING) > 0) {
		scat(" und benutzt das ");
		scat(locale_string(default_locale, resourcename(oldresourcetype[R_AMULET_OF_HEALING], 0)));
		scat(", um den Zauber zu verstärken");
		k *= 2;
		c += 0.10;
	}

	healable = count_healable(b, fi);
	healable = (int)min(k, healable);
	while (healable--) {
		troop t = select_corpse(b, fi);
		if (t.fighter
				&& t.fighter->side->casualties > 0
				&& old_race(t.fighter->unit->race) != RC_DAEMON
				&& (chance(c)))
		{
			assert(t.fighter->alive < t.fighter->unit->number);
			/* t.fighter->person[].hp beginnt mit t.index = 0 zu zählen,
			 * t.fighter->alive ist jedoch die Anzahl lebender in der Einheit,
			 * also sind die hp von t.fighter->alive
			 * t.fighter->hitpoints[t.fighter->alive-1] und der erste Tote
			 * oder weggelaufene ist t.fighter->hitpoints[t.fighter->alive] */
			t.fighter->person[t.fighter->alive].hp = 2;
			++t.fighter->alive;
			++t.fighter->side->size[SUM_ROW];
			++t.fighter->side->size[t.fighter->unit->status + 1];
			++t.fighter->side->healed;
			--t.fighter->side->casualties;
			++j;
		}
	}
	if (j == 0) {
		scat(", kann aber niemanden wiederbeleben.");
		level = 0;
	} else if (j == 1) {
		scat(" und belebt einen Toten wieder.");
		level = 1;
	} else {
		scat(" und belebt ");
		icat(j);
		scat(" Tote wieder.");
	}
	battlerecord(b, buf);

	return level;
}

int
sp_keeploot(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;

	sprintf(buf, "%s zaubert %s.", unitname(fi->unit), 
		spell_name(sp, default_locale));
	battlerecord(b, buf);

	b->keeploot = (int)max(50, b->keeploot + 5*power);

	return level;
}

int
sp_healing(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	int minrow = FIGHT_ROW;
	int maxrow = AVOID_ROW;
	int healhp;
	int hp, wound;
	int n, j = 0;
	cvector *fgs;
	void **fig;

	sprintf(buf, "%s kümmert sich um die Verletzten", unitname(mage));

	/* bis zu 11 Personen pro Stufe (einen HP müssen sie ja noch
	 * haben, sonst wären sie tot) können geheilt werden */
	power *= 200;

	if (get_item(mage, I_AMULET_OF_HEALING) > 0) {
		scat(" und benutzt das ");
		scat(locale_string(default_locale, resourcename(oldresourcetype[R_AMULET_OF_HEALING], 0)));
		scat(", um die Heilzauber zu verstärken");
		power *= 2;
	}

	/* gehe alle denen wir helfen der reihe nach durch, heile verwundete,
	 * bis zu verteilende HP aufgebraucht sind */

	fgs = fighters(b, fi, minrow, maxrow, FS_HELP);
	v_scramble(fgs->begin, fgs->end);

  healhp = (int)power;
	for (fig = fgs->begin; fig != fgs->end; ++fig) {
		fighter *df = *fig;

		if (healhp<=0) break;

		/* wir heilen erstmal keine Monster */
		if (!playerrace(df->unit->race))
			continue;

		hp = unit_max_hp(df->unit);
		for (n = 0; n < df->unit->number; n++) {
			if (!healhp)
				break;
			wound = hp - df->person[n].hp;
			if ( wound > 0 && wound < hp) {
				int heal = min(healhp, wound);
				assert(heal>=0);
				df->person[n].hp += heal;
				healhp = max(0, healhp - heal);
				j++;
			}
		}
	}
	/* haben wir noch HP übrig, so heilen wir nun auch Monster */
	for (fig = fgs->begin; fig != fgs->end; ++fig) {
		fighter *df = *fig;

		if (!healhp)
			break;

		/* Untote kann man nicht heilen */
		if (fval(df->unit->race, RCF_NOHEAL))
			continue;

		hp = unit_max_hp(df->unit);
		for (n = 0; n < df->unit->number; n++) {
			if (!healhp)
				break;
			wound = hp - df->person[n].hp;
			if ( wound > 0 && wound < hp) {
				int heal = min(healhp, wound);
				assert(heal>=0);
				df->person[n].hp += heal;
				healhp = max(0, healhp - heal);
				j++;
			}
		}
	}
	cv_kill(fgs);


	if (j == 0) {
		scat(", doch niemand mußte magisch geheilt werden.");
		level = 0;
	} else if (j == 1) {
		scat(" und heilt einen Verwundeten.");
		level = 1;
	} else {
		scat(" und heilt ");
		icat(j);
		scat(" Verwundete.");
	}
	battlerecord(b, buf);

	return level;
}

int
sp_undeadhero(fighter * fi, int level, double power, spell * sp)
{
	battle *b = fi->side->battle;
	unit *mage = fi->unit;
	region *r = b->region;
	int minrow = FIGHT_ROW;
	int maxrow = AVOID_ROW;
	cvector *fgs;
	void **fig;
	int n, j, undead = 0;
	int k = (int)get_force(power,0);
	double c = 0.50 + 0.02 * power;

	/* Liste aus allen Kämpfern */
	fgs = fighters(b, fi, minrow, maxrow,	FS_ENEMY | FS_HELP );
	v_scramble(fgs->begin, fgs->end);

	for (fig = fgs->begin; fig != fgs->end; ++fig) {
		fighter *df = *fig;
		unit *du = df->unit;

		if (!k)
			break;

		/* keine Monster */
		if (!playerrace(du->race))
			continue;

		if (df->alive + df->run.number < du->number) {
			j = 0;

			/* Wieviele Untote können wir aus dieser Einheit wecken? */
			for (n = df->alive + df->run.number; n <= du->number; n++) {
				if (!k) break;

				if (chance(c)) {
					undead++;
					j++;
					--df->side->casualties;
					--k;
				}
			}

			if (j > 0) {
				int hp = unit_max_hp(du);
				if (j == du->number) {
					/* Einheit war vollständig tot und konnte vollständig zu
					 * Untoten gemacht werden */
					int nr;

					du->race = new_race[RC_UNDEAD];
					setguard(du, GUARD_NONE);
					u_setfaction(du,mage->faction);
					if (fval(mage, UFL_PARTEITARNUNG))
						fset(du, UFL_PARTEITARNUNG);
					df->alive = du->number;
					/* den Toten wieder volle Hitpoints geben */
					for (nr = 0; nr != df->alive; ++nr) {
						df->person[nr].hp = hp;
					}
					/* vereinfachtes loot */
					change_money(mage, get_money(du));
					set_money(du, 0);
				} else {
					unit *u;
					u = createunit(r, mage->faction, 0, new_race[RC_UNDEAD]);
					transfermen(du, u, j);
					sprintf(buf, "%s", du->name);
					set_string(&u->name, buf);
					sprintf(buf, "%s", du->display);
					set_string(&u->display, buf);
					u->status = du->status;
					setguard(u, GUARD_NONE);
					if (fval(mage, UFL_PARTEITARNUNG))
						fset(u, UFL_PARTEITARNUNG);
					set_string(&u->lastorder, du->lastorder);
					/* den Toten volle Hitpoints geben */
					u->hp = u->number * unit_max_hp(u);
				}
			}
		}
	}
	cv_kill(fgs);

	if (undead == 0) {
		sprintf(buf, "%s kann keinen Untoten rufen.", unitname(mage));
		level = 0;
	} else if (undead == 1) {
		sprintf(buf, "%s erweckt einen Untoten.", unitname(mage));
		level = 1;
	} else {
		sprintf(buf, "%s erweckt %d Untote.", unitname(mage), undead);
	}

	battlerecord(b, buf);
	return level;
}


/* ------------------------------------------------------------------ */
