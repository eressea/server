/* vi: set ts=2:
 *
 *	$Id: monster.c,v 1.5 2001/02/03 13:45:30 enno Exp $
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
#include "item.h"

#include "monster.h"
#include "reports.h"
#include "message.h"
#include "region.h"
#include "economy.h"
#include "faction.h"
#include "movement.h"
#include "race.h"
#include "magic.h"
#include "build.h"
#include "pool.h"
#include "names.h"
#include "unit.h"
#include "pathfinder.h"

/* util includes */
#include <attrib.h>
#include <base36.h>
#include <event.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* triggers includes */
#include <triggers/removecurse.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>

#ifdef HAVE_ZLIB
#include <zlib.h>
static gzFile dragonlog;
#else
static FILE * dragonlog;
#endif

#define UNDEAD_REPRODUCTION         0 	/* vermehrung */
#define MOVECHANCE                  25	/* chance fuer bewegung */

/* ------------------------------------------------------------- */

#define MAXILLUSION_TEXTS   3

/* ------------------------------------------------------------- */

static boolean
is_waiting(const unit * u)
{
	if (fval(u, FL_ISNEW)) return true;
	if (strncasecmp(u->lastorder, "WARTEN", 6) == 0) return true;

	return false;
}

static boolean
monster_attack(unit * u, const unit * target)
{
	char zText[20];
	strlist *S;

	if (u->region!=target->region) return false;
	if (!cansee(u->faction, u->region, target, 0)) return false;
	if (is_waiting(u)) return false;

	sprintf(zText, "%s %s",
		keywords[K_ATTACK], unitid(target));
	S = makestrlist(zText);
	addlist(&u->orders, S);
	return true;
}

void
taxed_by_monster(unit * u)
{
	strlist *S;

	S = makestrlist(keywords[K_TAX]);
	addlist(&u->orders, S);
}

static boolean
get_money_for_dragon(region * r, unit * u, int wanted)
{
	unit *u2;
	int n;

	/* attackiere bewachende einheiten */

	for (u2 = r->units; u2; u2 = u2->next)
		if (u2 != u && getguard(u2)&GUARD_TAX)
			monster_attack(u, u2);

	/* treibe steuern bei den bauern ein */

	taxed_by_monster(u);

	/* falls das genug geld ist, bleibt das monster hier */

	if (rmoney(r) >= wanted)
		return true;

	/* attackiere so lange die fremden, alteingesessenen einheiten mit geld
	 * (einfach die reihenfolge der einheiten), bis das geld da ist. n
	 * zaehlt das so erhaltene geld. */

	n = 0;
	for (u2 = r->units; u2; u2 = u2->next)
		if (u2->faction != u->faction && get_money(u2)) {
			if (monster_attack(u, u2)) {
				n += get_money(u2);

				if (n > wanted)
					break;
			}
		}
	/* falls die einnahmen erreicht werden, bleibt das monster noch eine
	 * runde hier. */

	if (n + rmoney(r) >= wanted)
		return true;

	/* falls das geld, das wir haben, und das der angriffe zum leben
	 * reicht, koennen wir uns fortbewegen. deswegen duerfen wir
	 * steuereinnahmen nicht beruecksichtigen - wir bewegen uns ja fort. */

	if (get_money(u) + n >= MAINTENANCE)
		return false;

	/* falls wir auch mit angriffe nicht genug geld zum wandern haben,
	 * muessen wir wohl oder uebel hier bleiben. vielleicht wurden ja genug
	 * steuern eingetrieben */

	return true;
}

int
money(region * r)
{
	unit *u;
	int m;

	m = rmoney(r);
	for (u = r->units; u; u = u->next)
		m += get_money(u);
	return m;
}

direction_t
richest_neighbour(region * r, int absolut)
{

	/* m - maximum an Geld, d - Richtung, i - index, t = Geld hier */

	double m;
	double t;
	direction_t d = NODIRECTION, i;

	if (absolut == 1 || rpeasants(r) == 0) {
		m = (double) money(r);
	} else {
		m = (double) money(r) / (double) rpeasants(r);
	}

	/* finde die region mit dem meisten geld */

	for (i = 0; i != MAXDIRECTIONS; i++)
		if (rconnect(r, i) && rterrain(rconnect(r, i)) != T_OCEAN) {
			if (absolut == 1 || rpeasants(r) == 0) {
				t = (double) money(rconnect(r, i));
			} else {
				t = (double) money(rconnect(r, i)) / (double) rpeasants(r);
			}

			if (t > m) {
				m = t;
				d = i;
			}
		}
	return d;
}

boolean
room_for_race_in_region(region *r, race_t rc)
{
	unit *u;
	int  c = 0;

	for(u=r->units;u;u=u->next) {
		if(u->race == rc) c += u->number;
	}

	if(c > (race[rc].splitsize*2))
		return false;

	return true;
}

direction_t
random_neighbour(region * r, unit *u)
{
	direction_t i;
	region *rc;
	int rr, c = 0, c2 = 0;

	/* Nachsehen, wieviele Regionen in Frage kommen */

	for (i = 0; i != MAXDIRECTIONS; i++) {
		rc = rconnect(r, i);
		if (rc && can_survive(u, rc)) {
			if(room_for_race_in_region(rc, u->race)) {
				c++;
			}
			c2++;
		}
	}

	if (c == 0) {
		if(c2 == 0) {
			return NODIRECTION;
		} else {
			c  = c2;
			c2 = 0;		/* c2 == 0 -> room_for_race nicht beachten */
		}
	}

	/* Zufällig eine auswählen */

	rr = rand() % c;

	/* Durchzählen */

	c = -1;
	for (i = 0; i != MAXDIRECTIONS; i++) {
		rc = rconnect(r, i);
		if (rc && can_survive(u, rc)) {
			if(c2 == 0) {
				c++;
			} else if(room_for_race_in_region(rc, u->race)) {
				c++;
			}
			if (c == rr) return i;
		}
	}

	assert(1 == 0);				/* Bis hierhin sollte er niemals kommen. */
	return NODIRECTION;
}

direction_t
treeman_neighbour(region * r)
{
	direction_t i;
	int rr;
	int c = 0;

	/* Nachsehen, wieviele Regionen in Frage kommen */

	for (i = 0; i != MAXDIRECTIONS; i++) {
		if (rconnect(r, i)
			&& rterrain(rconnect(r, i)) != T_OCEAN
			&& rterrain(rconnect(r, i)) != T_GLACIER
			&& rterrain(rconnect(r, i)) != T_DESERT) {
			c++;
		}
	}

	if (c == 0) {
		return NODIRECTION;
	}
	/* Zufällig eine auswählen */

	rr = rand() % c;

	/* Durchzählen */

	c = -1;
	for (i = 0; i != MAXDIRECTIONS; i++) {
		if (rconnect(r, i)
			&& rterrain(rconnect(r, i)) != T_OCEAN
			&& rterrain(rconnect(r, i)) != T_GLACIER
			&& rterrain(rconnect(r, i)) != T_DESERT) {
			c++;
			if (c == rr) {
				return i;
			}
		}
	}

	assert(1 == 0);				/* Bis hierhin sollte er niemals kommen. */
	return NODIRECTION;
}

void
move_monster(region * r, unit * u)
{
	direction_t d = NODIRECTION;
	strlist *S, **SP;

	switch(u->race) {
	case RC_FIREDRAGON:
	case RC_DRAGON:
	case RC_WYRM:
		d = richest_neighbour(r, 1);
		break;
	case RC_TREEMAN:
		d = treeman_neighbour(r);
		break;
	default:
		d = random_neighbour(r,u);
		break;
	}

	/* falls kein geld gefunden wird, zufaellig verreisen, aber nicht in
	 * den ozean */

	if (d == NODIRECTION)
		return;

	sprintf(buf, "%s %s", keywords[K_MOVE], directions[d]);

	SP = &u->orders;
	S = makestrlist(buf);
	addlist2(SP, S);
	*SP = 0;

}

/* Wir machen das mal autoconf-style: */
#ifndef HAVE_DRAND48
#define drand48() (((double)rand()) / RAND_MAX)
#endif

int
dragon_affinity_value(region *r, unit *u)
{
	if(u->race == RC_FIREDRAGON) {
		return (1+rand() % 6) + (int) (count_all_money(r) * (0.1 + drand48()));
	} else {
		return (1+rand() % 6) + (int) (count_all_money(r) * (0.3 + drand48()));
	}
}

attrib *
set_new_dragon_target(unit * u, region * r, int range)
{
	region *r2;
	int x, y;
	int affinity, max_affinity = 0;
	region *max_region = NULL;

	for (x = r->x - range; x < r->x + range; x++) {
		for (y = r->y - range; y < r->y + range; y++) {
			if (koor_distance (r->x, r->y, x, y) <= range
					&& (r2 = findregion(x, y)) != 0
					&&
					path_exists(r, r2, range, allowed_dragon)
				) {
				affinity = dragon_affinity_value(r2, u);
				if(affinity > max_affinity) {
					max_affinity = affinity;
					max_region = r2;
				}
			}
		}
	}

	if (max_region && max_region != r) {
		attrib * a = a_find(u->attribs, &at_targetregion);
		if (!a) {
			a = a_add(&u->attribs, make_targetregion(max_region));
		} else {
			a->data.v = max_region;
		}
		sprintf(buf, "Kommt aus: %s, Will nach: %s", regionid(r), regionid(max_region));
		usetprivate(u, buf);
		if (dragonlog) {
#ifdef HAVE_ZLIB
			gzprintf(dragonlog, "%s entscheidet sich, von %s nach %s zu wandern.\n", unitname(u), f_regionid(r, u->faction), f_regionid(max_region, u->faction));
#elif HAVE_BZ2LIB
			/* TODO */
#else
			fprintf(dragonlog, "%s entscheidet sich, von %s nach %s zu wandern.\n", unitname(u), f_regionid(r, u->faction), f_regionid(max_region, u->faction));
#endif
		}
		return a;
	}
	return NULL;
}

static boolean
set_movement_order(unit * u, const region * target, int moves, boolean (*allowed)(const region *, const region *))
{
	region * r = u->region;
	region ** plan = path_find(r, target, DRAGON_RANGE, allowed);
	int position = 0;
	char * c;

	if (plan==NULL) {
		if (dragonlog) {
#ifdef HAVE_ZLIB
			gzprintf(dragonlog, "%s fand keinen Weg von %s nach %s\n", unitname(u), f_regionid(r, u->faction), f_regionid(target, u->faction));
#elif HAVE_BZ2LIB
			/* TODO */
#else
			fprintf(dragonlog, "%s fand keinen Weg von %s nach %s\n", unitname(u), f_regionid(r, u->faction), f_regionid(target, u->faction));
#endif
		}
		return false;
	}

	strcpy(buf, keywords[K_MOVE]);
	c = buf + strlen(buf);

	while (position!=moves && plan[position+1]) {
		region * prev = plan[position];
		region * next = plan[++position];
		direction_t dir = reldirection(prev, next);
		assert(dir!=NODIRECTION);
		*c++ = ' ';
		strcpy(c, directions[dir]);
		c += strlen(c);
	}

	if (dragonlog) {
#ifdef HAVE_ZLIB
		gzprintf(dragonlog, "%s wandert von %s nach %s: %s\n", unitname(u), f_regionid(r, u->faction), f_regionid(target, u->faction), buf);
#elif HAVE_BZ2LIB
			/* TODO */
#else
		fprintf(dragonlog, "%s wandert von %s nach %s: %s\n", unitname(u), f_regionid(r, u->faction), f_regionid(target, u->faction), buf);
#endif
	}
	set_string(&u->lastorder, buf);
	return true;
}
/* ------------------------------------------------------------- */

unit *
random_unit(const region * r)
{
	int c = 0;
	int n;
	unit *u;

	for (u = r->units; u; u = u->next) {
		if (u->race != RC_SPELL) {
			c += u->number;
		}
	}

	if (c == 0) {
		return NULL;
	}
	n = rand() % c;
	c = 0;
	u = r->units;

	while (u && c < n) {
		if (u->race != RC_SPELL) {
			c += u->number;
		}
		u = u->next;
	}

	return u;
}

static boolean
random_attack_by_monster(const region * r, unit * u)
{
	boolean success = false;
	unit *target;
	int kill, max;
	int tries = 0;
	int attacked = 0;

	switch (u->race) {
	case RC_FIREDRAGON:
		kill = 25;
		max = 50;
		break;
	case RC_DRAGON:
		kill = 100;
		max = 200;
		break;
	case RC_WYRM:
		kill = 400;
		max = 800;
		break;
	default:
		kill = 1;
		max = 1;
	}

	kill *= u->number;
	max *= u->number;

	do {
		tries++;
		target = random_unit(r);
		if (target
		    && target != u
		    && humanoid(target)
			&& !illusionary(target)
		    && target->number <= max) 
		{
			if (monster_attack(u, target)) {
				unit * u2;
				success = true;
				for (u2 = r->units; u2; u2 = u2->next) {
					if (u2->faction->no == MONSTER_FACTION 
						&& rand() % 100 < 75) 
					{
						monster_attack(u2, target);
					}
				}
				attacked += target->number;
			}
		}
	}
	while (attacked < kill && tries < 10);
	return success;
}

void
eaten_by_monster(unit * u)
{
	int n;

	switch (u->race) {
	case RC_FIREDRAGON:
		n = rand()%80 * u->number;
		break;
	case RC_DRAGON:
		n = rand()%200 * u->number;
		break;
	case RC_WYRM:
		n = rand()%500 * u->number;
		break;
	default:
		n = rand()%(u->number/20+1);
	}

	if (n > 0) {
		n = lovar(n);
		n = min(rpeasants(u->region), n);

		if(n > 0) {
			deathcounts(u->region, n);
			rsetpeasants(u->region, rpeasants(u->region) - n);
			add_message(&u->region->msgs, new_message(NULL,
				"eatpeasants%u:unit%i:amount", u, n));
		}
	}
}

void
absorbed_by_monster(unit * u)
{
	int n;

	switch (u->race) {
	default:
		n = rand()%(u->number/20+1);
	}

	if(n > 0) {
		n = lovar(n);
		n = min(rpeasants(u->region), n);
		if(n > 0){
			rsetpeasants(u->region, rpeasants(u->region) - n);
			scale_number(u, u->number + n);
			add_message(&u->region->msgs, new_message(NULL,
				"absorbpeasants%u:unit%i:amount", u, n));
		}
	}
}

int
scareaway(region * r, int anzahl)
{
	int n, p, d = 0, emigrants[MAXDIRECTIONS];
	direction_t i;

	anzahl = min(max(1, anzahl),rpeasants(r));

	/* Wandern am Ende der Woche (normal) oder wegen Monster. Die
	 * Wanderung wird erst am Ende von demographics () ausgefuehrt.
	 * emigrants[] ist local, weil r->newpeasants durch die Monster
	 * vielleicht schon hochgezaehlt worden ist. */

	for (i = 0; i != MAXDIRECTIONS; i++)
		emigrants[i] = 0;

	p = rpeasants(r);
	assert(p >= 0 && anzahl >= 0);
	for (n = min(p, anzahl); n; n--) {
		direction_t dir = (direction_t)(rand() % MAXDIRECTIONS);
		region * c = rconnect(r, dir);

		if (c && landregion(rterrain(c))) {
			d++;
			c->land->newpeasants++;
			emigrants[dir]++;
		}
	}
	rsetpeasants(r, p-d);
	assert(p >= d);
	return d;
}

void
scared_by_monster(unit * u)
{
	int n;

	switch (u->race) {
	case RC_FIREDRAGON:
		n = rand()%160 * u->number;
		break;
	case RC_DRAGON:
		n = rand()%400 * u->number;
		break;
	case RC_WYRM:
		n = rand()%1000 * u->number;
		break;
	default:
		n = rand()%(u->number/4+1);
	}

	if(n > 0) {
		n = lovar(n);
		n = min(rpeasants(u->region), n);
		if(n > 0) {
			n = scareaway(u->region, n);
			add_message(&u->region->msgs, new_message(NULL,
				"fleescared%i:amount%u:unit", n, u));
		}
	}
}

const char *
random_growl(void)
{
	switch(rand()%5) {
	case 0:
		return "Groammm";
	case 1:
		return "Roaaarrrr";
	case 2:
		return "Chhhhhhhhhh";
	case 3:
		return "Tschrrrkk";
	case 4:
		return "Schhhh";
	}
	return "";
}

extern attrib_type at_direction;

void
make_ponnuki(void)
{
	int ponn = atoi36("ponn");
	unit * u = ufindhash(ponn);
	region * r = findregion(-67,-5);
	if (u || !r) return;
	u = createunit(r, findfaction(MONSTER_FACTION), 1, RC_ILLUSION);
	u->irace = RC_GOBLIN;
	set_string(&u->name, "Ponnuki");
	u->age=-1000;
	set_string(&u->display, "Go, Ponnuki, Go.");
    uunhash(u);
	u->no = ponn;
	uhash(u);

	/* 'andere' ponnukis mit eigener nummer */
	ponn = atoi36("255x");
	do {
		u = ufindhash(ponn);
		if (!u) break;
		uunhash(u);
		u->no = newunitid();
		uhash(u);
	} while (u);
}

void ponnuki(unit * u)
{
	const char* joke[] = {
		"Ein Bummerang ist, wenn man ihn wegwirft und er kommt nicht wieder, dann war's keiner.",

		"Ein Riese und ein Zwerg sitzen an der Bar und trinken Bier. Der Zwerg spuckt - tftftf - dem Riesen ins Bier.\n"
		"Riese: Hör auf, oder ich reiß Dir ein Ohr ab!\n"
		"Zwerg: Mir egal, bei uns Zwergen wachsen die Ohren nach! - tftftf\n"
		"Riese: Wenn Du nicht sofort aufhörst, reiße ich Dir ein Bein aus!\n"
		"Zwerg: Na und, bei uns Zwergen wachsen Beine nach - tftftf\n"
		"Riese: Jetzt reichts aber wirklich! Schluß, oder ich reiße Dir den\n"
		"Schniedel aus.\n"
		"Zwerg: Probier's doch, wir Zwerge haben gar keinen Schniedel - tftftf\n"
		"Riese (verblüfft): So, und wie pinkelt Ihr dann???\n"
		"Zwerg: tftftf",

		"Ein Ingenieur und ein Mathematiker sitzen zusammen in einem Vortrag über Kulza-Klein Theorie, die sich mit\n"
		"11, 12 und sogar höheren Dimensionen beschäftig. Der Mathematiker geniesst die Vorlesung, während der\n"
		"Ingenieur immer mehr verwirrt aussieht. Als der Vortrag zu ende ist, hat der Ingenieur schreckliche\n"
		"Kopfschmerzen davon.\n"
		"\n"
		"Ingenieur: Wie kannst du nur diesen schrecklichen, abgehobenen Vortrag verstehen?\n"
		"Mathematiker: Ich stelle mir das ganze einfach vor.\n"
		"Ingenieur: Wie kannst du dir einen 11-dimensionalen Raum vorstellen???\n"
		"Mathematiker: Nun, ich stelle mir einen n-dimensionalen Raum vor und lasse dann n gegen 11 gehen..",

		"Merke: Mit Schwabenwitzen soll man ganz sparsam sein.",

		"F: Was bekommt man, wenn man Katzen und Elfen kreuzt?\nA: Elfen ohne Rheuma.",

		"F: Was bekommt man, wenn man Insekten und Katzen kreuzt?\nA: Tiger, die Crisan benutzen.",

		NULL };
	int jokes = 0;

	/*
	attrib * a = a_find(u->attribs, &at_direction);

	if (!a) {
		a_add(&u->attribs, a = a_new(&at_direction));
		a->data.i = D_NORTHWEST;
	}
	if (rand() % 10 == 0) a->data.i = rand() % MAXDIRECTIONS;

	travel(u->region, u, u->region->connect[a->data.i], false);
	*/

	/* Rausgenommen, übergibt irgendwann NULL, das führt zu einem
	 * SegFault. */

	while(joke[jokes]) jokes++;
	if (jokes) addmessage(u->region, 0, joke[rand() % jokes], MSG_MESSAGE, ML_IMPORTANT);

}

extern void make_ponnuki(void);

void
learn_monster(unit *u)
{
	int c = 0;
	int n;
	skill_t sk;

	/* Monster lernt ein zufälliges Talent aus allen, in denen es schon
	 * Lerntage hat. */

	for(sk=0;sk<MAXSKILLS;sk++)
		if(get_skill(u, sk) > 0) c++;

	if(c == 0) return;

	n = rand()%c + 1;
	c = 0;

	for(sk=0;sk<MAXSKILLS;sk++) {
		if(get_skill(u, sk) > 0) {
			c++;
			if(c == n) {
				sprintf(buf, "%s %s", keywords[K_STUDY], skillnames[sk]);
				set_string(&u->thisorder, buf);
				break;
			}
		}
	}
}

void
monsters_kill_peasants(void)
{
	region *r;
	unit *u;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) if(!fval(u, FL_MOVED)) {
			if(race[u->race].flags & RCF_SCAREPEASANTS) {
				scared_by_monster(u);
			}
			if(race[u->race].flags & RCF_KILLPEASANTS) {
				eaten_by_monster(u);
			}
			if(race[u->race].flags & RCF_ABSORBPEASANTS) {
				absorbed_by_monster(u);
			}
		}
	}
}

void
plan_monsters(void)
{
	region *r;
	faction *f;
	unit *u;
	int ra;
	attrib *ta;
	strlist *S;

	if (!dragonlog) {
		char zText[MAX_PATH];
#ifdef HAVE_ZLIB
		sprintf(zText, "%s/dragonlog.gz", basepath());
		dragonlog = gzopen(zText, "w");
#elif HAVE_BZ2LIB
		sprintf(zText, "%s/dragonlog.bz2", basepath());
		dragonlog = BZ2_bzopen(zText,"w");
#else
		sprintf(zText, "%s/dragonlog", basepath());
		dragonlog = fopen(zText, "w");
#endif
	}
	u = findunitg(atoi36("ponn"), NULL);
	if (!u) make_ponnuki();
	f = findfaction(MONSTER_FACTION);
	if (!f)
		return;

	f->lastorders = turn;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			region * tr = NULL;
			boolean is_moving = false;

			/* Ab hier nur noch Befehle für NPC-Einheiten. */

			if (u->faction->no != MONSTER_FACTION) continue;
			ta = a_find(u->attribs, &at_hate);
			if (ta && strncmp(u->lastorder, "WARTEN", 6) != 0) {
				unit * tu = (unit *)ta->data.v;
				if (tu && tu->region==r) {
					sprintf(buf, "%s %s", keywords[K_ATTACK], itoa36(tu->no));
					S = makestrlist(buf);
					addlist(&u->orders, S);
				} else if (tu) {
					tu = findunitg(ta->data.i, NULL);
					if (tu) set_movement_order(u, tu->region, 2, allowed_walk);
				}
				else a_remove(&u->attribs, ta);
			}
			/* Diese Verkettung ist krank und sollte durch eine 'vernünftige KI'
			 * ersetzt werden. */

			if( (race[u->race].flags & RCF_MOVERANDOM)
					&& (rand()%100<MOVECHANCE || check_overpopulated(u))) {
				move_monster(r, u);
			} else {
				boolean done = false;
				if((race[u->race].flags & RCF_ATTACKRANDOM)
					&& rand()%100<MONSTERATTACK 
					&& is_moving == false) 
				{
					done = random_attack_by_monster(r, u);
				} 
				if (!done) {
					if(u->race == RC_SEASERPENT) {
						set_string(&u->thisorder, keywords[K_PIRACY]);
						set_string(&u->lastorder, keywords[K_PIRACY]);
					} else if(race[u->race].flags & RCF_LEARN) {
						learn_monster(u);
					}
				}
			}

			/* Ab hier noch nicht generalisierte Spezialbehandlungen. */

			switch (u->race) {
			case RC_ILLUSION:
				if (u->no==atoi36("ponn")) ponnuki(u);
				break;
			case RC_FIREDRAGON:
			case RC_DRAGON:
			case RC_WYRM:

				{
					int horses = get_resource(u,R_HORSE);
					if (horses > 0) {
						change_resource(u, R_HORSE, - min(horses,(u->number*2)));
					}
				}
				set_item(u, I_STONE, 0);
				set_item(u, I_WOOD, 0);
				set_item(u, I_IRON, 0);

				if (rand() % 100 < 4) {
					/* dragon gets bored and looks for a different place to go */
					ta = set_new_dragon_target(u, r, DRAGON_RANGE);
				}
				else ta = a_find(u->attribs, &at_targetregion);
				if (ta!=NULL) {
					tr = findregion(ta->data.sa[0], ta->data.sa[1]);
					if (tr==NULL || !path_exists(r, tr, DRAGON_RANGE, allowed_dragon)) {
						ta = set_new_dragon_target(u, r, DRAGON_RANGE);
						if (ta) tr = findregion(ta->data.sa[0], ta->data.sa[1]);
					}
				}
				if (tr!=NULL) {
#ifdef NEW_PATH
					switch(u->race) {
					case RC_FIREDRAGON:
						set_movement_order(u, tr, 4, allowed_dragon);
						break;
					case RC_DRAGON:
						set_movement_order(u, tr, 3, allowed_dragon);
						break;
					case RC_WYRM:
						set_movement_order(u, tr, 1, allowed_dragon);
						break;
					}
#else
					switch(u->race) {
					case RC_FIREDRAGON:
						set_movement_order(u, r, ta->data.sa[0], ta->data.sa[1], 4);
						break;
					case RC_DRAGON:
						set_movement_order(u, r, ta->data.sa[0], ta->data.sa[1], 3);
						break;
					case RC_WYRM:
						set_movement_order(u, r, ta->data.sa[0], ta->data.sa[1], 1);
						break;
					}
#endif
					if (rand()%100 < 15) {
						/* do a growl */
						if (rname(tr, u->faction->locale)) {
							strlist *S;
							sprintf(buf,
								"botschaft an region %s~...~%s~etwas~in~%s.",
								estring(random_growl()), u->number==1?"Ich~rieche":"Wir~riechen",
								estring(rname(tr, u->faction->locale)));
							S = makestrlist(buf);
							addlist(&u->orders, S);
						}
					}
				} else {
					if (!get_money_for_dragon(r, u, income(u))) {
						/* money is gone */
						set_new_dragon_target(u, r, DRAGON_RANGE);
					}
					else if (u->race != RC_FIREDRAGON && r->terrain!=T_OCEAN && r->terrain!=T_FIREWALL) {
						ra = 20 + rand() % 100;
						if (get_money(u) > ra * 50 + 100 && rand() % 100 < 50)
						{
							unit *un;
							un = createunit(r, findfaction(MONSTER_FACTION), ra, RC_DRACOID);
							name_unit(un);
							change_money(u, -un->number * 50);
							set_skill(un, SK_SPEAR, un->number * (180 + rand() % 500));
							set_skill(un, SK_SWORD, un->number * (180 + rand() % 500));
							set_skill(un, SK_LONGBOW, un->number * (90 + rand() % 300));
							switch (rand() % 3) {
							case 0:
								set_item(un, I_LONGBOW, un->number);
								un->status = ST_BEHIND;
								set_string(&un->lastorder, "LERNE BOGENSCHIESSEN");
								break;
							case 1:
								set_item(un, I_SWORD, un->number);
								un->status = ST_FIGHT;
								set_string(&un->lastorder, "LERNE SCHWERTKAMPF");
								break;
							case 2:
								set_item(un, I_SPEAR, un->number);
								un->status = ST_FIGHT;
								set_string(&un->lastorder, "LERNE SPEERKAMPF");
								break;
							}
						}
						if (is_waiting(u)) {
							set_string(&u->thisorder, "LERNE WAHRNEHMUNG");
							set_string(&u->lastorder, "LERNE WAHRNEHMUNG");
						}
					}
				}
				break;
			}
		}
	}
	if (dragonlog) {
#ifdef HAVE_ZLIB
		gzclose(dragonlog);
#elif HAVE_BZ2LIB
		BZ2_bzclose(dragonlog);
#else
		fclose(dragonlog);
#endif
		dragonlog = NULL;
	}
}

void
age_default(unit *u)
{
}

void
age_unit(region * r, unit * u)
{
	if (u->race == RC_SPELL) {
		u->age--;
		if (u->age <= 0)
			destroy_unit(u);
	} else {
		u->age++;

		if(race[u->race].age_function) {
			race[u->race].age_function(u);
		} else {
			age_default(u);
		}
	}
}

void
split_unit(region * r, unit *u)
{
	unit *u2;
	skill_t sk;
	int newsize;
	int n;
	assert(u->number!=1);

	newsize = u->number/2;

	u2 = createunit(r, u->faction, newsize, u->race);
	set_string(&u2->name, u->name);
	set_string(&u2->display, u->display);
	set_string(&u2->thisorder, "WARTEN");
	set_string(&u2->lastorder, "WARTEN");

	for(sk = 0; sk < MAXSKILLS; sk++) {
		int i;
		n = get_skill(u, sk);
		i = (n / u->number) * newsize;
		i += (n % u->number) * newsize / u->number;
		set_skill(u2, sk, i);
		set_skill(u, sk, n-i);
	}
	set_number(u, u->number - newsize);
}

boolean
check_overpopulated(unit *u)
{
	unit *u2;
	int c = 0;

	for(u2 = u->region->units; u2; u2 = u2->next) {
		if(u2->race == u->race && u != u2) c += u2->number;
	}

	if(c > race[u->race].splitsize * 2) return true;

	return false;
}

void
check_split(void)
{
	region *r;
	unit *u;

	for(r=regions;r;r=r->next) {
		for(u=r->units;u;u=u->next) {
			if(u->faction->no == MONSTER_FACTION) {
				if(u->number > race[u->race].splitsize) {
						split_unit(r, u);
				}
			}
		}
	}
}

void
monsters_leave(void)
{
	region *r;
	unit *u;
	int n;

	for(r=regions;r;r=r->next) {
		for(u=r->units;u;u=u->next) {
			for(n = get_item(u, I_PEGASUS); n > 0; n--) {

			}
			for(n = get_item(u, I_UNICORN); n > 0; n--) {

			}
			for(n = get_item(u, I_DOLPHIN); n > 0; n--) {

			}
		}
	}
}
