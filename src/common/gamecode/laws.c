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
#include <eressea.h>
#include "laws.h"

#include <modules/gmcmd.h>
#include <modules/infocmd.h>
#ifdef WORMHOLE_MODULE
#include <modules/wormhole.h>
#endif

/* gamecode includes */
#include "economy.h"
#include "monster.h"
#include "randenc.h"
#include "study.h"

/* kernel includes */
#include <alchemy.h>
#include <border.h>
#include <faction.h>
#include <item.h>
#include <magic.h>
#include <message.h>
#include <save.h>
#include <ship.h>
#include <skill.h>
#include <movement.h>
#include <spy.h>
#include <race.h>
#include <battle.h>
#include <region.h>
#include <unit.h>
#include <plane.h>
#include <karma.h>
#include <pool.h>
#include <building.h>
#include <group.h>
#include <race.h>
#include <resources.h>
#ifdef USE_UGROUPS
#  include <ugroup.h>
#endif

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/synonym.h>

/* util includes */
#include <event.h>
#include <base36.h>
#include <goodies.h>
#include <rand.h>
#include <sql.h>
#include <util/message.h>

#include <modules/xecmd.h>
#ifdef ALLIANCES
#include <modules/alliance.h>
#endif

#ifdef AT_OPTION
/* attributes includes */
#include <attributes/option.h>
#endif

/* libc includes */
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <attributes/otherfaction.h>

/* - external symbols ------------------------------------------ */
extern int dropouts[2];
extern int * age;
/* - exported global symbols ----------------------------------- */
boolean nobattle = false;
boolean nomonsters = false;
/* ------------------------------------------------------------- */

static int 
RemoveNMRNewbie(void) {
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "nmr.removenewbie");
    value = str?atoi(str):0;
  }
  return value;
}

static void
restart(unit *u, const race * rc)
{
  faction * oldf = u->faction;
  faction *f = addfaction(oldf->email, oldf->passw, rc, oldf->locale, oldf->subscription);
  unit * nu = addplayer(u->region, f);
  strlist ** o=&u->orders;
  f->subscription = u->faction->subscription;
  fset(f, FFL_RESTART);
  if (f->subscription) fprintf(sqlstream, "UPDATE subscriptions set faction='%s', race='%s' where id=%u;\n",
    itoa36(f->no), dbrace(rc), f->subscription);
  f->magiegebiet = u->faction->magiegebiet;
  f->options = u->faction->options;
  freestrlist(nu->orders);
  nu->orders = u->orders;
  u->orders = NULL;
  while (*o) {
    strlist * S = *o;
    if (igetkeyword(S->s, u->faction->locale) == K_RESTART) {
      *o = S->next;
      S->next = NULL;
      freestrlist(S);
    } else {
      o = &S->next;
    }
  }
  destroyfaction(u->faction);
}

/* ------------------------------------------------------------- */

static void
checkorders(void)
{
	faction *f;

	puts(" - Warne späte Spieler...");
	for (f = factions; f; f = f->next)
		if (f->no!=MONSTER_FACTION && turn - f->lastorders == NMRTimeout() - 1)
			ADDMSG(&f->msgs, msg_message("turnreminder", ""));
}
static boolean
help_money(const unit * u)
{
  if (u->race->ec_flags & GIVEITEM) return true;
  return false;
}

static void
get_food(region *r)
{
	unit *u;
	int peasantfood = rpeasants(r)*10;
  unit * demon = r->units;

	/* 1. Versorgung von eigenen Einheiten. Das vorhandene Silber
	 * wird zunächst so auf die Einheiten aufgeteilt, dass idealerweise
	 * jede Einheit genug Silber für ihren Unterhalt hat. */

	for (u = r->units; u; u = u->next) {
		int need = lifestyle(u);

		/* Erstmal zurücksetzen */
		freset(u, UFL_HUNGER);

		need -= get_money(u);
		if (need > 0) {
			unit *v;

      for (v = r->units; need && v; v = v->next) {
        if (v->faction == u->faction && help_money(v)) {
          int give = get_money(v) - lifestyle(v);
          give = min(need, give);
          if (give>0) {
            change_money(v, -give);
            change_money(u, give);
            need -= give;
          }
        }
      }
		}
	}

	/* 2. Versorgung durch Fremde. Das Silber alliierter Einheiten wird
	 * entsprechend verteilt. */
	for (u = r->units; u; u = u->next) {
		int need = lifestyle(u);
    faction * f = u->faction;

		need -= max(0, get_money(u));

		if (need > 0) {
			unit *v;

			for (v = r->units; need && v; v = v->next) {
				if (v->faction != f && alliedunit(v, f, HELP_MONEY) && help_money(v)) {
					int give = get_money(v) - lifestyle(v);
					give = min(need, give);

					if (give>0) {
						change_money(v, -give);
						change_money(u, give);
						need -= give;
						add_spende(v->faction, u->faction, give, r);
					}
				}
			}

			/* Die Einheit hat nicht genug Geld zusammengekratzt und
			 * nimmt Schaden: */
			if (need) {
				int lspp = lifestyle(u)/u->number;
				if (lspp > 0) {
					int number = (need+lspp-1)/lspp;
					if (hunger(number, u)) fset(u, UFL_HUNGER);
				}
			}
		}
	}

  /* 3. bestimmen, wie viele Bauern gefressen werden. 
   * bei fehlenden Bauern den Dämon hungern lassen
   */
	for (u = r->units; u; u = u->next) {
		if (u->race == new_race[RC_DAEMON]) {
      int hungry = u->number;

      while (demon!=NULL && hungry>0) {
        /* alwayy start with the first known uint that may have some blood */
        static const struct potion_type * pt_blood;
        unit * ud = demon;
        if (pt_blood==NULL) pt_blood = pt_find("peasantblood");
        demon = NULL; /* this will be re-set in the loop */
        while (ud!=NULL) {
          if (ud->race==new_race[RC_DAEMON]) {
            if (get_effect(ud, pt_blood)) {
              /* new best guess for first blood-donor: */
              if (demon==NULL) demon = ud;
              /* if he's in our faction, drain him: */
              if (ud->faction==u->faction) break;
            }
          }
          ud=ud->next;
        }
        if (ud!=NULL) {
          int blut = get_effect(ud, pt_blood);
          blut = min(blut, hungry);
          change_effect(ud, pt_blood, -blut);
          hungry -= blut;
        } else {
          break;
          /* no more help for us */
        }
			}
			if (r->planep == NULL || !fval(r->planep, PFL_NOFEED)) {
				if (peasantfood>=hungry) {
					peasantfood -= hungry;
					hungry = 0;
				} else {
					hungry -= peasantfood;
					peasantfood = 0;
				}
				if (hungry > 0) {
					hunger(hungry, u); /* nicht gefütterte dämonen hungern */
				}
			}
		}
	}
	rsetpeasants(r, peasantfood/10);

	/* 3. Von den überlebenden das Geld abziehen: */
	for (u = r->units; u; u = u->next) {
		int need = min(get_money(u), lifestyle(u));
		change_money(u, -need);
	}
}

static void
live(region * r)
{
  unit **up = &r->units;

  get_food(r);

  while (*up) {
    unit * u = *up;
    /* IUW: age_unit() kann u löschen, u->next ist dann
    * undefiniert, also müssen wir hier schon das nächste
    * Element bestimmen */

    int effect = get_effect(u, oldpotiontype[P_FOOL]);
    if (effect > 0) {	/* Trank "Dumpfbackenbrot" */
      skill * sv = u->skills, * sb = NULL;
      while (sv!=u->skills+u->skill_size) {
        if (sb==NULL || skill_compare(sv, sb)>0) {
          sb = sv;
        }
        ++sv;
      }
      /* bestes Talent raussuchen */
      if (sb!=NULL) {
        int weeks = min(effect, u->number);
        reduce_skill(u, sb, weeks);
        ADDMSG(&u->faction->msgs, msg_message("dumbeffect",
          "unit weeks skill", u, weeks, (skill_t)sb->id));
      }	/* sonst Glück gehabt: wer nix weiß, kann nix vergessen... */
      change_effect(u, oldpotiontype[P_FOOL], -effect);
    }
    age_unit(r, u);
    if (*up==u) up=&u->next;
  }
}

/*
 * This procedure calculates the number of emigrating peasants for the given
 * region r. There are two incentives for peasants to emigrate:
 * 1) They prefer the less crowded areas.
 *    Example: mountains, 700 peasants (max  1000), 70% inhabited
 *             plain, 5000 peasants (max 10000), 50% inhabited
 *             700*(PEASANTSWANDER_WEIGHT/100)*((70-50)/100) peasants wander
 *             from mountains to plain.
 *    Effect : peasents will leave densely populated regions.
 * 2) Peasants prefer richer neighbour regions.
 *    Example: region A,  700 peasants, wealth $10500, $15 per head
 *             region B, 2500 peasants, wealth $25000, $10 per head
 *             Some peasants will emigrate from B to A because $15 > $10
 *             exactly: 2500*(PEASANTSGREED_WEIGHT1/100)*((15-10)/100)
 * Not taken in consideration:
 * - movement because of monsters.
 * - movement because of wars
 * - movement because of low loyalty relating to present parties.
 */

#if NEW_MIGRATION == 1

/* Arbeitsversion */
static void
calculate_emigration(region *r)
{
	direction_t i;
	int overpopulation = rpeasants(r) - maxworkingpeasants(r);
	int weight[MAXDIRECTIONS], weightall;

	/* Bauern wandern nur bei Überbevölkerung, sonst gar nicht */
	if(overpopulation <= 0) return;

	weightall = 0;

	for (i = 0; i != MAXDIRECTIONS; i++) {
		region *rc = rconnect(r,i);
		int w;

		if (rc == NULL || rterrain(rc) == T_OCEAN) {
			w = 0;
		} else {
			w = rpeasants(rc) - maxworkingpeasants(rc);
			w = max(0,w);
			if(rterrain(rc) == T_VOLCANO || rterrain(rc) == T_VOLCANO_SMOKING) {
				w = w/10;
			} 
		}
		weight[i]  = w;
		weightall += w;
	}
	
	if (weightall !=0 ) for (i = 0; i != MAXDIRECTIONS; i++) {
		region *rc;
		if((rc = rconnect(r,i)) != NULL) {
			int wandering_peasants = (overpopulation * weight[i])/weightall;
			if (wandering_peasants > 0) {
				r->land->newpeasants -= wandering_peasants;
				rc->land->newpeasants += wandering_peasants;
			}
		}
	}
}

#else
void
calculate_emigration(region *r)
{
	direction_t i, j;
	int maxpeasants_here;
	int maxpeasants[MAXDIRECTIONS];
	double wfactor, gfactor;

	/* Vermeidung von DivByZero */
	maxpeasants_here = max(maxworkingpeasants(r),1);

	for (i = 0; i != MAXDIRECTIONS; i++) {
		region *c = rconnect(r, i);
		if (c && rterrain(c) != T_OCEAN) {
			maxpeasants[i] = maxworkingpeasants(c);
		} else maxpeasants[i] = 0;
	}

	/* calculate emigration for all directions independently */

	for (i = 0; i != MAXDIRECTIONS; i++) {
		region * c = rconnect(r, i);
		if (!c) continue;
		if (fval(r, RF_ORCIFIED)==fval(c, RF_ORCIFIED)) {
			if (landregion(rterrain(c)) && landregion(rterrain(r))) {
				int wandering_peasants;
				double vfactor;

				/* First let's calculate the peasants who wander off to less inhabited
				 * regions. wfactor indicates the difference of population denity.
				 * Never let more than PEASANTSWANDER_WEIGHT per cent wander off in one
				 * direction. */
				wfactor = ((double) rpeasants(r) / maxpeasants_here -
					((double) rpeasants(c) / maxpeasants[i]));
				wfactor = max(wfactor, 0);
				wfactor = min(wfactor, 1);

				/* Now let's handle the greedy peasants. gfactor indicates the
				 * difference of per-head-wealth. Never let more than
				 * PEASANTSGREED_WEIGHT per cent wander off in one direction. */
				gfactor = (((double) rmoney(c) / max(rpeasants(c), 1)) -
						   ((double) rmoney(r) / max(rpeasants(r), 1))) / 500;
				gfactor = max(gfactor, 0);
				gfactor = min(gfactor, 1);

				/* This calculates the influence of volcanos on peasant
				 * migration. */

				if(rterrain(c) == T_VOLCANO || rterrain(c) == T_VOLCANO_SMOKING) {
						vfactor = 0.10;
				} else {
						vfactor = 1.00;
				}

				for(j=0; j != MAXDIRECTIONS; j++) {
					region *rv = rconnect(c, j);
					if(rv && (rterrain(rv) == T_VOLCANO || rterrain(rv) == T_VOLCANO_SMOKING)) {
						vfactor *= 0.5;
						break;
					}
				}

				wandering_peasants = (int) (rpeasants(r) * (gfactor+wfactor)
						* vfactor * PEASANTSWANDER_WEIGHT / 100.0);

				r->land->newpeasants -= wandering_peasants;
				c->land->newpeasants += wandering_peasants;
			}
		}
	}
}
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void
peasants(region * r)
{

	int glueck;

	/* Das Geld, daß die Bauern erwirtschaftet haben unter expandwork, gibt
	 * den Bauern genug für 11 Bauern pro Ebene ohne Wald. Der Wald
	 * breitet sich nicht in Gebiete aus, die bebaut werden. */

	int peasants, n, i, satiated, money;
#if PEASANTS_DO_NOT_STARVE == 0
 	int dead;
#endif
	attrib * a;

	/* Bauern vermehren sich */

	/* Bis zu 1000 Bauern können Zwillinge bekommen oder 1000 Bauern
	 * wollen nicht! */

	a = a_find(r->attribs, &at_peasantluck);
	if (!a) {
		glueck = 0;
	} else {
		glueck = a->data.i * 1000;
	}

	peasants = rpeasants(r);

	for (n = peasants; n; n--) {
		if (glueck >= 0) {		/* Sonst keine Vermehrung */
			if (rand() % 10000 < PEASANTGROWTH) {
				if ((float) peasants
					/ ((float) production(r) * MAXPEASANTS_PER_AREA)
					< 0.9 || rand() % 100 < PEASANTFORCE) {
					peasants++;
				}
			}
		} else
			glueck++;

		if (glueck > 0) {		/* Doppelvermehrung */
			for(i=0; i<PEASANTLUCK; i++) {
				if (rand() % 10000 < PEASANTGROWTH)
					peasants++;
			}
			glueck--;
		}
	}

	/* Alle werden satt, oder halt soviele für die es auch Geld gibt */

	money = rmoney(r);
	satiated = min(peasants, money / MAINTENANCE);
	rsetmoney(r, money - satiated * MAINTENANCE);

	/* Von denjenigen, die nicht satt geworden sind, verhungert der
	 * Großteil. dead kann nie größer als rpeasants(r) - satiated werden,
	 * so dass rpeasants(r) >= 0 bleiben muß. */

	/* Es verhungert maximal die unterernährten Bevölkerung. */

#if PEASANTS_DO_NOT_STARVE == 0
	dead = 0;
	for (n = min((peasants - satiated), rpeasants(r)); n; n--)
		if (rand() % 100 > STARVATION_SURVIVAL)
			dead++;

	if(dead > 0) {
		message * msg = add_message(&r->msgs, msg_message("phunger", "dead", dead));
		msg_release(msg);
		peasants -= dead;
	}
#endif

	rsetpeasants(r, peasants);
}

/* ------------------------------------------------------------- */

typedef struct migration {
	struct migration * next;
	region * r;
	int horses;
	int trees;
} migration;

#define MSIZE 1023
migration * migrants[MSIZE];
migration * free_migrants;

static migration *
get_migrants(region * r)
{
	int key = region_hashkey(r);
	int index = key % MSIZE;
	migration * m = migrants[index];
	while (m && m->r != r)
		m = m->next;
	if (m == NULL) {
		/* Es gibt noch keine Migration. Also eine erzeugen
		 */
		m = free_migrants;
		if (!m) m = calloc(1, sizeof(migration));
		else {
			free_migrants = free_migrants->next;
			m->horses = 0;
			m->trees = 0;
		}
		m->r = r;
		m->next = migrants[index];
		migrants[index] = m;
	}
	return m;
}

static void
migrate(region * r)
{
	int key = region_hashkey(r);
	int index = key % MSIZE;
	migration ** hp = &migrants[index];
	fset(r, RF_MIGRATION);
	while (*hp && (*hp)->r != r) hp = &(*hp)->next;
	if (*hp) {
		migration * m = *hp;
		rsethorses(r, rhorses(r) + m->horses);
		/* Was macht das denn hier?
		 * Baumwanderung wird in trees() gemacht.
		 * wer fragt das? Die Baumwanderung war abhängig von der
		 * Auswertungsreihenfolge der regionen,
		 * das hatte ich geändert. jemand hat es wieder gelöscht, toll.
		 * ich habe es wieder aktiviert, muß getestet werden.
		 */
#if GROWING_TREES == 0
		rsettrees(r, rtrees(r) + m->trees);
#endif
		*hp = m->next;
		m->next = free_migrants;
		free_migrants = m;
	}
}

static void
horses(region * r)
{
	int horses, maxhorses;
	direction_t n;

	/* Logistisches Wachstum, Optimum bei halbem Maximalbesatz. */
	maxhorses = maxworkingpeasants(r)/10;
	maxhorses = max(0, maxhorses);
	horses = rhorses(r);
	if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
		rsethorses(r, (int)(horses*0.9));
	} else if (maxhorses > 0) {
		int i;
		int growth = (int)((RESOURCE_QUANTITY * HORSEGROWTH * 200 * (maxhorses-horses))/maxhorses);

		if(a_find(r->attribs, &at_horseluck)) growth *= 2;
		/* printf("Horses: <%d> %d -> ", growth, horses); */
		for(i = 0; i < horses; i++) {
			if(rand()%10000 < growth) horses++;
		}
		/* printf("%d\n", horses); */
		rsethorses(r, horses);
	}

	/* Pferde wandern in Nachbarregionen.
	 * Falls die Nachbarregion noch berechnet
	 * werden muß, wird eine migration-Struktur gebildet,
	 * die dann erst in die Berechnung der Nachbarstruktur einfließt.
	 */

	for(n = 0; n != MAXDIRECTIONS; n++) {
		region * r2 = rconnect(r, n);
		if(r2 && (terrain[r2->terrain].flags & WALK_INTO)) {
			int pt = (rhorses(r) * HORSEMOVE)/100;
			pt = (int)normalvariate(pt, pt/4.0);
			pt = max(0, pt);
			if (fval(r2, RF_MIGRATION))
				rsethorses(r2, rhorses(r2) + pt);
			else {
				migration * nb;
				/* haben wir die Migration schonmal benutzt?
				 * wenn nicht, müssen wir sie suchen.
				 * Wandernde Pferde vermehren sich nicht.
				 */
				nb = get_migrants(r2);
				nb->horses += pt;
			}
			/* Wandernde Pferde sollten auch abgezogen werden */
			rsethorses(r, rhorses(r) - pt);
		}
	}
	assert(rhorses(r) >= 0);
}

#if GROWING_TREES

static int
count_race(const region *r, const race *rc)
{
	unit *u;
	int  c = 0;

	for(u = r->units; u; u=u->next)
		if(u->race == rc) c += u->number;

	return c;
}

extern attrib_type at_germs;

static void
trees(region * r, const int current_season, const int last_weeks_season)
{
	int growth, grownup_trees, i, seeds, sprout, seedchance;
	direction_t d;
	attrib *a;

	if(current_season == SEASON_SUMMER || current_season == SEASON_AUTUMN) {
		int elves = count_race(r,new_race[RC_ELF]);

		a = a_find(r->attribs, &at_germs);
		if(a && last_weeks_season == SEASON_SPRING) {
			/* ungekeimte Samen bleiben erhalten, Sprößlinge wachsen */
			sprout = min(a->data.sa[1], rtrees(r, 1));
			/* aus dem gesamt Sprößlingepool abziehen */
			rsettrees(r, 1, rtrees(r, 1) - sprout);
			/* zu den Bäumen hinzufügen */
			rsettrees(r, 2, rtrees(r, 2) + sprout);

			a_removeall(&r->attribs, &at_germs);
		}

		if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
			rsettrees(r, 1, (int)(rtrees(r, 1) * 0.9));
			rsettrees(r, 2, (int)(rtrees(r, 2) * 0.9));
			return;
		}

		if(production(r) <= 0) return;

		/* Grundchance 1.0% */
		seedchance = (int)(FORESTGROWTH * RESOURCE_QUANTITY);
		/* Jeder Elf in der Region erhöht die Chance um 0.0008%. */
		seedchance += (min(elves, (production(r)*MAXPEASANTS_PER_AREA)/8)) * 8;
		grownup_trees = rtrees(r, 2);
		seeds = 0;

		for(i=0;i<grownup_trees;i++) {
			if(rand()%1000000 < seedchance) seeds++;
		}
		rsettrees(r, 0, rtrees(r, 0) + seeds);

		/* Bäume breiten sich in Nachbarregionen aus. */

		/* Gesamtzahl der Samen:
		 * bis zu 6% (FORESTGROWTH*3) der Bäume samen in die Nachbarregionen */
		seeds = (rtrees(r, 2) * FORESTGROWTH * 3)/1000000;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * r2 = rconnect(r, d);
			if (r2 && (terrain[r2->terrain].flags & WALK_INTO)) {
				/* Eine Landregion, wir versuchen Samen zu verteilen:
				 * Die Chance, das Samen ein Stück Boden finden, in dem sie
				 * keimen können, hängt von der Bewuchsdichte und der
				 * verfügbaren Fläche ab. In Gletschern gibt es weniger
				 * Möglichkeiten als in Ebenen. */
				sprout = 0;
				seedchance = (100 * maxworkingpeasants(r2)) / terrain[r2->terrain].production_max;
				for(i=0; i<seeds/MAXDIRECTIONS; i++) {
					if(rand()%10000 < seedchance) sprout++;
				}
				rsettrees(r2, 0, rtrees(r2, 0) + sprout);
			}
		}

	} else if(current_season == SEASON_SPRING) {

		if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) return;

		/* in at_germs merken uns die Zahl der Samen und Sprößlinge, die
		 * dieses Jahr älter werden dürfen, damit nicht ein Same im selben
		 * Zyklus zum Baum werden kann */
		a = a_find(r->attribs, &at_germs);
		if(!a) {
			a = a_add(&r->attribs, a_new(&at_germs));
			a->data.sa[0] = (short)rtrees(r, 0);
			a->data.sa[1] = (short)rtrees(r, 1);
		}
		/* wir haben 6 Wochen zum wachsen, jeder Same/Sproß hat 18% Chance
		 * zu wachsen, damit sollten nach 5-6 Wochen alle gewachsen sein */
		growth = 1800;

		/* Samenwachstum */

		/* Raubbau abfangen, es dürfen nie mehr Samen wachsen, als aktuell
		 * in der Region sind */
		seeds = min(a->data.sa[0], rtrees(r, 0));
		sprout = 0;

		for(i=0;i<seeds;i++) {
			if(rand()%10000 < growth) sprout++;
		}
		/* aus dem Samenpool dieses Jahres abziehen */
		a->data.sa[0] = (short)(seeds - sprout);
		/* aus dem gesamt Samenpool abziehen */
		rsettrees(r, 0, rtrees(r, 0) - sprout);
		/* zu den Sprößlinge hinzufügen */
		rsettrees(r, 1, rtrees(r, 1) + sprout);

		/* Baumwachstum */

		/* hier gehen wir davon aus, das Jungbäume nicht ohne weiteres aus
		 * der Region entfernt werden können, da Jungbäume in der gleichen
		 * Runde nachwachsen, wir also nicht mehr zwischen diesjährigen und
		 * 'alten' Jungbäumen unterscheiden könnten */
		sprout = min(a->data.sa[1], rtrees(r, 1));
		grownup_trees = 0;

		for(i=0;i<sprout;i++) {
			if(rand()%10000 < growth) grownup_trees++;
		}
		/* aus dem Sprößlingepool dieses Jahres abziehen */
		a->data.sa[1] = (short)(sprout - grownup_trees);
		/* aus dem gesamt Sprößlingepool abziehen */
		rsettrees(r, 1, rtrees(r, 1) - grownup_trees);
		/* zu den Bäumen hinzufügen */
		rsettrees(r, 2, rtrees(r, 2) + grownup_trees);
	}

	/* Jetzt die Kräutervermehrung. Vermehrt wird logistisch:
	 *
	 * Jedes Kraut hat eine Wahrscheinlichkeit von (100-(vorhandene
	 * Kräuter))% sich zu vermehren. */
	if(current_season == SEASON_SPRING || current_season == SEASON_SUMMER
			|| current_season == SEASON_AUTUMN)
	{
		for(i = rherbs(r); i > 0; i--) {
			if (rand()%100 < (100-rherbs(r))) rsetherbs(r, (short)(rherbs(r)+1));
		}
	}
}
#else
static void
trees(region * r)
{
	int i, maxtrees;
	int tree = rtrees(r);
	direction_t d;

	/* Bäume vermehren sich. m ist die Anzahl Bäume, für die es Land
	 * gibt. Der Wald besiedelt keine bebauten Gebiete, wird den Pferden
	 * aber Land wegnehmen. Gibt es zuviele Bauern, werden sie den Wald
	 * nicht fällen, sondern verhungern. Der Wald kann nur von Spielern gefällt
	 * werden! Der Wald wandert nicht. Auch bei magischen Terrainveränderungen
	 * darf m nicht negativ werden! */

	if(production(r) <= 0) return;

	maxtrees = production(r) - rpeasants(r)/MAXPEASANTS_PER_AREA;
	maxtrees = max(0, maxtrees);

	/* Solange es noch freie Plätze gibt, darf jeder Baum versuchen, sich
	 * fortzupflanzen. Da Bäume nicht sofort eingehen, wenn es keinen
	 * Platz gibt (wie zB. die Pferde), darf nicht einfach drauflos vermehrt
	 * werden und dann ein min () gemacht werden, sondern es muß auf diese
	 * Weise vermehrt werden. */

	if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
		tree = (int)(tree*0.9);
	} else if(maxtrees > 0) {
		int growth = (int)((FORESTGROWTH * 200 * ((maxtrees*1.2)-tree))/maxtrees);
		growth = max(FORESTGROWTH*50, growth);
		growth = min(FORESTGROWTH*400, growth);
		/* printf("Trees: <%d> %d -> ", growth, tree); */
		for(i=0;i<tree;i++) {
			if(rand()%10000 < growth) tree++;
		}
		/* printf("%d (max: %d)\n", tree, maxtrees); */
	}

	/* Bäume breiten sich in Nachbarregionen aus.
	 * warnung: früher kamen bäume aus nachbarregionen, heute
	 * gehen sie von der aktuellen in die benachbarten.
	 * Die Chance, das ein Baum in eine Region r2 einwandert, ist
	 * (production-rtrees(r2))/10000.
	 * Die Richtung, in der sich ein Baum vermehrt, ist zufällig.
	 * Es ibt also genausoviel Versuche, in einen Geltscher zu
	 * wandern, wie in eine ebene - nur halt weniger Erfolge.
	 * Die Summe der Versuche ist rtrees(r), jeder Baum
	 * versucht es also einmal pro Woche, mit maximal 10% chance
	 * (wenn ein voller wald von lauter leeren ebenen umgeben ist)
	 */

	for (d=0;d!=MAXDIRECTIONS;++d) {
		region * r2 = rconnect(r, d);
		if (r2
				&& (terrain[r2->terrain].flags & WALK_INTO)
				&& fval(r2, RF_MALLORN) == fval(r, RF_MALLORN))	{
			/* Da hier rtrees(r2) abgefragt wird, macht es einen Unterschied,
			 * ob das wachstum in r2 schon stattgefunden hat, oder nicht.
			 * leider nicht einfach zu verhindern */
			int pt = (production(r2)-rtrees(r2));
			pt = tree*max(0, pt) /
				(MAXDIRECTIONS*terrain[T_PLAIN].production_max*10);
			if (fval(r2, RF_MIGRATION))
				rsettrees(r2, rtrees(r2) + pt);
			else {
				migration * nb;
				/* haben wir die Migration schonmal benutzt?
				 * wenn nicht, müssen wir sie suchen.
				 * Wandernde Pferde vermehren sich nicht.
				 */
				nb = get_migrants(r2);
				nb->trees += pt;
			}
		}
	}
	rsettrees(r, tree);
	assert(tree >= 0);

	/* Jetzt die Kräutervermehrung. Vermehrt wird logistisch:
	 *
	 * Jedes Kraut hat eine Wahrscheinlichkeit von (100-(vorhandene
	 * Kräuter))% sich zu vermehren. */

	for(i = rherbs(r); i > 0; i--) {
		if (rand()%100 < (100-rherbs(r))) rsetherbs(r, (short)(rherbs(r)+1));
	}
}
#endif

#if NEW_RESOURCEGROWTH == 0
extern attrib_type at_laen;
static void
iron(region * r)
{
	if(is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) return;

#ifndef NO_GROWTH
	if (rterrain(r) == T_MOUNTAIN) {
		rsetiron(r, riron(r) + IRONPERTURN);
		if(a_find(r->attribs, &at_laen)) {
			rsetlaen(r, rlaen(r) + rand() % MAXLAENPERTURN);
		}
	} else if (rterrain(r) == T_GLACIER || rterrain(r) == T_ICEBERG_SLEEP) {
		rsetiron(r, min(MAXGLIRON, riron(r)+GLIRONPERTURN));
	}
#endif

}
#endif /* NEW_RESOURCEGROWTH */
/* ------------------------------------------------------------- */


extern int season(int turn);

void
demographics(void)
{
	region *r;
#if GROWING_TREES
	int current_season = season(turn);
	int last_weeks_season = season(turn-1);
#endif

	for (r = regions; r; r = r->next) {
		live(r);
		/* check_split_dragons(); */

		if (rterrain(r) != T_OCEAN) {
			/* die Nachfrage nach Produkten steigt. */
#ifdef NEW_ITEMS
			struct demand * dmd;
			if (r->land) for (dmd=r->land->demands;dmd;dmd=dmd->next) {
				if (dmd->value>0 && dmd->value < MAXDEMAND) {
					int rise = DMRISE;
					if (buildingtype_exists(r, bt_find("harbour"))) rise = DMRISEHAFEN;
					if (rand() % 100 < rise) dmd->value++;
				}
			}
#else
			item_t n;
			for (n = 0; n != MAXLUXURIES; n++) {
				int d = rdemand(r, n);
				if (d > 0 && d < MAXDEMAND) {
					if (buildingtype_exists(r, &bt_harbour)) {
						if (rand() % 100 < DMRISEHAFEN) {
							d++;
						}
					} else {
						if (rand() % 100 < DMRISE) {
							d++;
						}
					}
				}
				rsetdemand(r, n, (char)d);
			}
#endif
			/* Seuchen erst nachdem die Bauern sich vermehrt haben
			 * und gewandert sind */

			calculate_emigration(r);
			peasants(r);
			plagues(r, false);

			r->age++;
			horses(r);
#if GROWING_TREES
			if(current_season != SEASON_WINTER) {
				trees(r, current_season, last_weeks_season);
			}
#else
			trees(r);
#endif
#if NEW_RESOURCEGROWTH
			update_resources(r);
#else
			iron(r);
#endif
			migrate(r);
		}
	}
	while (free_migrants) {
		migration * m = free_migrants->next;
		free(free_migrants);
		free_migrants = m;
	};
	putchar('\n');

	remove_empty_units();

	puts(" - Einwanderung...");
	for (r = regions; r; r = r->next) {
		if (landregion(rterrain(r))) {
			int rp = rpeasants(r) + r->land->newpeasants;
			rsetpeasants(r, max(0, rp));
			/* Wenn keine Bauer da ist, soll auch kein Geld da sein */
			/* Martin */
			if (rpeasants(r) == 0)
				rsetmoney(r, 0);
		}
	}

	checkorders();
}
/* ------------------------------------------------------------- */

static int
modify(int i)
{
	int c;

	c = i * 2 / 3;

	if (c >= 1) {
		return (c + rand() % c);
	} else {
		return (i);
	}
}

static void
inactivefaction(faction * f)
{
	FILE *inactiveFILE;
	char zText[128];

	sprintf(zText, "%s/%s", datapath(), "/inactive");
	inactiveFILE = fopen(zText, "a");

	fprintf(inactiveFILE, "%s:%s:%d:%d\n",
		factionid(f),
		LOC(default_locale, rc_name(f->race, 1)),
		modify(count_all(f)),
		turn - f->lastorders);

	fclose(inactiveFILE);
}

#ifdef ENHANCED_QUIT
static void
transfer_faction(faction *f, faction *f2)
{
	unit *u, *un;
	
	for (u = f->units; u;) {
		un = u->nextF;
		if(!unit_has_cursed_item(u)
				&& !has_skill(u, SK_MAGIC)
				&& !has_skill(u, SK_ALCHEMY)) {
			u_setfaction(u, f2);
		}
		u = un;
	}
}
#endif

static void
quit(void)
{
	region *r;
	unit *u, *un;
	strlist *S;
	faction *f;
	const race * frace;

	/* Sterben erst nachdem man allen anderen gegeben hat - bzw. man kann
	 * alles machen, was nicht ein dreißigtägiger Befehl ist. */

	for (r = regions; r; r = r->next) {
		for (u = r->units; u;) {
			un = u->next;
			for (S = u->orders; S; S = S->next) {
				if (igetkeyword(S->s, u->faction->locale) == K_QUIT) {
					if (checkpasswd(u->faction, getstrtoken(), false)) {
#ifdef ENHANCED_QUIT
						int f2_id = getid();

						if(f2_id > 0) {
							faction *f2 = findfaction(f2_id);

							f = u->faction;

							if(f2 == NULL) {
								cmistake(u, S->s, 66, MSG_EVENT);
							} else if(f == f2) {
								cmistake(u, S->s, 8, MSG_EVENT);
#ifdef ALLIANCES
							} else if(u->faction->alliance != f2->alliance) {
								cmistake(u, S->s, 315, MSG_EVENT);
#else
#error ENHANCED_QUIT defined without ALLIANCES
#endif
							} else if(!alliedfaction(NULL, f, f2, HELP_MONEY)) {
								cmistake(u, S->s, 316, MSG_EVENT);
							} else {
								transfer_faction(f,f2);
								destroyfaction(f);
							}
						} else {
							destroyfaction(u->faction);
						}
#else
						destroyfaction(u->faction);
#endif
						break;
					} else {
						cmistake(u, S->s, 86, MSG_EVENT);
						log_warning(("STIRB mit falschem Passwort für Partei %s: %s\n",
							   factionid(u->faction), S->s));
					}
				} else if(igetkeyword(S->s, u->faction->locale) == K_RESTART && u->number > 0) {
					const char *s_race, *s_pass;

					if (!landregion(rterrain(r))) {
						cmistake(u, S->s, 242, MSG_EVENT);
						continue;
					}
					s_race = getstrtoken();
					frace = findrace(s_race, u->faction->locale);

					if (!frace) {
						frace = u->faction->race;
						s_pass = s_race;
					} else {
						s_pass = getstrtoken();
					}

					if (frace != u->faction->race && u->faction->age < 81) {
						cmistake(u, S->s, 241, MSG_EVENT);
						continue;
					}
					if (u->faction->age > 3 && fval(u->faction, FFL_RESTART)) {
						cmistake(u, S->s, 314, MSG_EVENT);
						continue;
					}

					if (!playerrace(frace)) {
						cmistake(u, S->s, 243, MSG_EVENT);
						continue;
					}

					if (!checkpasswd(u->faction, s_pass, false)) {
						cmistake(u, S->s, 86, MSG_EVENT);
						log_warning(("NEUSTART mit falschem Passwort für Partei %s: %s\n",
							   factionid(u->faction), S->s));
						continue;
					}
					restart(u, frace);
					break;
				}
			}
			u = un;
		}
	}

	puts(" - beseitige Spieler, die sich zu lange nicht mehr gemeldet haben...");

	remove("inactive");

	for (f = factions; f; f = f->next) {
		if(fval(f, FFL_NOIDLEOUT)) f->lastorders = turn;
		if (NMRTimeout()>0 && turn - f->lastorders >= NMRTimeout()) {
			destroyfaction(f);
			continue;
		}
		if (fval(f, FFL_OVERRIDE)) {
			free(f->override);
			f->override = strdup(itoa36(rand()));
		}
		if (turn!=f->lastorders) {
			char info[256];
			sprintf(info, "%d Einheiten, %d Personen, %d Silber", 
				f->no_units, f->number, f->money);
			if (f->subscription) fprintf(sqlstream, 
				"UPDATE subscriptions SET lastturn=%d, password='%s', info='%s' "
				"WHERE id=%u;\n", 
				f->lastorders, f->override, info, f->subscription);
		} else {
			if (f->subscription) fprintf(sqlstream, 
				"UPDATE subscriptions SET status='ACTIVE', lastturn=%d, password='%s' "
				"WHERE id=%u;\n", 
				f->lastorders, f->override, f->subscription);
		}

		if (NMRTimeout()>0 && turn - f->lastorders >= (NMRTimeout() - 1)) {
			inactivefaction(f);
			continue;
		}
	}
	puts(" - beseitige Spieler, die sich nach der Anmeldung nicht "
		 "gemeldet haben...");

	age = calloc(max(4,turn+1), sizeof(int));
	for (f = factions; f; f = f->next) if (f->no != MONSTER_FACTION) {
		if (RemoveNMRNewbie() && !fval(f, FFL_NOIDLEOUT)) {
			if (f->age>=0 && f->age <= turn) ++age[f->age];
			if (f->age == 2 || f->age == 3) {
				if (f->lastorders == turn - 2) {
					destroyfaction(f);
					++dropouts[f->age-2];
					continue;
				}
			}
		}
#if defined(ALLIANCES) && !defined(ALLIANCEJOIN)
		if (f->alliance==NULL) {
			/* destroyfaction(f); */
			continue;
		}
#endif
	}
	/* Clear away debris of destroyed factions */

	puts(" - beseitige leere Einheiten und leere Parteien...");

	remove_empty_units();

	/* Auskommentiert: Wenn factions gelöscht werden, zeigen
	 * Spendenpointer ins Leere. */
	/* remove_empty_factions(); */
}
/* ------------------------------------------------------------- */

/* HELFE partei [<ALLES | SILBER | GIB | KAEMPFE | WAHRNEHMUNG>] [NICHT] */

static void
set_ally(unit * u, strlist * S)
{
	ally * sf, ** sfp;
	faction *f;
	int keyword, not_kw;
	const char *s;

	f = getfaction();

	if (f == 0 || f->no == 0) {
		cmistake(u, S->s, 66, MSG_EVENT);
		return;
	}
	if (f == u->faction)
		return;

	s = getstrtoken();

	if (!s[0])
		keyword = P_ANY;
	else
		keyword = findparam(s, u->faction->locale);

	sfp = &u->faction->allies;
	{
		attrib * a = a_find(u->attribs, &at_group);
		if (a) sfp = &((group*)a->data.v)->allies;
	}
	for (sf=*sfp; sf; sf = sf->next)
		if (sf->faction == f)
			break;	/* Gleich die passende raussuchen, wenn vorhanden */

	not_kw = getparam(u->faction->locale);		/* HELFE partei [modus] NICHT */

	if (!sf) {
		if (keyword == P_NOT || not_kw == P_NOT) {
			/* Wir helfen der Partei gar nicht... */
			return;
		} else {
			sf = calloc(1, sizeof(ally));
			sf->faction = f;
			sf->status = 0;
			addlist(sfp, sf);
		}
	}
	switch (keyword) {
	case P_NOT:
		sf->status = 0;
		break;

	case NOPARAM:
		cmistake(u, S->s, 137, MSG_EVENT);
		return;

	case P_ANY:
		if (not_kw == P_NOT)
			sf->status = 0;
		else
			sf->status = HELP_ALL;
		break;

	case P_TRAVEL:
		if (not_kw == P_NOT)
			sf->status = sf->status & (HELP_ALL - HELP_TRAVEL);
		else
			sf->status = sf->status | HELP_TRAVEL;
		break;

  case P_GIB:
		if (not_kw == P_NOT)
			sf->status = sf->status & (HELP_ALL - HELP_GIVE);
		else
			sf->status = sf->status | HELP_GIVE;
		break;

	case P_SILVER:
		if (not_kw == P_NOT)
			sf->status = sf->status & (HELP_ALL - HELP_MONEY);
		else
			sf->status = sf->status | HELP_MONEY;
		break;

	case P_KAEMPFE:
		if (not_kw == P_NOT)
			sf->status = sf->status & (HELP_ALL - HELP_FIGHT);
		else
			sf->status = sf->status | HELP_FIGHT;
		break;

	case P_FACTIONSTEALTH:
		if (not_kw == P_NOT)
			sf->status = sf->status & (HELP_ALL - HELP_FSTEALTH);
		else
			sf->status = sf->status | HELP_FSTEALTH;
		break;

	case P_GUARD:
		if (not_kw == P_NOT)
			sf->status = sf->status & (HELP_ALL - HELP_GUARD);
		else
			sf->status = sf->status | HELP_GUARD;
		break;
	}

	if (sf->status == 0) {		/* Alle HELPs geloescht */
		removelist(sfp, sf);
	}
}
/* ------------------------------------------------------------- */

static void
set_display(region * r, unit * u, strlist * S)
{
	char **s;
  const char *s2;

	s = 0;

	switch (getparam(u->faction->locale)) {
	case P_BUILDING:
	case P_GEBAEUDE:
		if (!u->building) {
			cmistake(u, S->s, 145, MSG_PRODUCE);
			break;
		}
		if (!fval(u, UFL_OWNER)) {
			cmistake(u, S->s, 5, MSG_PRODUCE);
			break;
		}
		if (u->building->type == bt_find("generic")) {
			cmistake(u, S->s, 279, MSG_PRODUCE);
			break;
		}
		if (u->building->type == bt_find("monument") && u->building->display[0] != 0) {
			cmistake(u, S->s, 29, MSG_PRODUCE);
			break;
		}
		if (u->building->type == bt_find("artsculpture") && u->building->display[0] != 0) {
			cmistake(u, S->s, 29, MSG_PRODUCE);
			break;
		}
		s = &u->building->display;
		break;

	case P_SHIP:
		if (!u->ship) {
			cmistake(u, S->s, 144, MSG_PRODUCE);
			break;
		}
		if (!fval(u, UFL_OWNER)) {
			cmistake(u, S->s, 12, MSG_PRODUCE);
			break;
		}
		s = &u->ship->display;
		break;

	case P_UNIT:
		s = &u->display;
		break;

	case P_PRIVAT:
		{
			const char *d = getstrtoken();
			if(d == NULL || *d == 0) {
				usetprivate(u, NULL);
			} else {
				usetprivate(u, d);
			}
		}
		break;

	case P_REGION:
		if (!u->building) {
			cmistake(u, S->s, 145, MSG_EVENT);
			break;
		}
		if (!fval(u, UFL_OWNER)) {
			cmistake(u, S->s, 148, MSG_EVENT);
			break;
		}
		if (u->building != largestbuilding(r,false)) {
			cmistake(u, S->s, 147, MSG_EVENT);
			break;
		}
		s = &r->display;
		break;

	default:
		cmistake(u, S->s, 110, MSG_EVENT);
		break;
	}

	if (!s)
		return;

	s2 = getstrtoken();

	set_string(&(*s), s2);
}

static void
set_prefix(unit * u, strlist *S)
{
	attrib **ap;
	attrib *a, *a2;
	int i;
	const char *s;

	s = getstrtoken();

	if(!*s) {
		a = a_find(u->attribs, &at_group);
		if (a) {
			a_removeall(&((group*)a->data.v)->attribs, &at_raceprefix);
		} else {
			a_removeall(&u->faction->attribs, &at_raceprefix);
		}
		return;
	}

	for(i=0; race_prefixes[i] != NULL; i++) {
		if(strncasecmp(s, LOC(u->faction->locale, race_prefixes[i]), strlen(s)) == 0) {
			break;
		}
	}

	if(race_prefixes[i] == NULL) {
		cmistake(u, S->s, 299, MSG_EVENT);
		return;
	}

  ap = &u->faction->attribs;
	a = a_find(u->attribs, &at_group);
  if (a) ap = &((group*)a->data.v)->attribs;

	a2 = a_find(*ap, &at_raceprefix);
	if(!a2)
		a2 = a_add(ap, a_new(&at_raceprefix));

	a2->data.v = strdup(race_prefixes[i]);

	return;
}

static void
set_synonym(unit * u, strlist *S)
{
	attrib *a;
	int i;
	const char *s;

	a = a_find(u->faction->attribs, &at_synonym);
	if(a) {	/* Kann nur einmal gesetzt werden */
		cmistake(u, S->s, 302, MSG_EVENT);
		return;
	}

	s = getstrtoken();

	if(!s) {
		cmistake(u, S->s, 301, MSG_EVENT);
		return;
	}

	for(i=0; race_synonyms[i].race != -1; i++) {
		if (new_race[race_synonyms[i].race] == u->faction->race
				&& strcasecmp(s, race_synonyms[i].synonyms[0]) == 0) {
			break;
		}
	}

	if(race_synonyms[i].race == -1) {
		cmistake(u, S->s, 300, MSG_EVENT);
		return;
	}

	a = a_add(&u->faction->attribs, a_new(&at_synonym));
	((frace_synonyms *)(a->data.v))->synonyms[0] =
		strdup(race_synonyms[i].synonyms[0]);
	((frace_synonyms *)(a->data.v))->synonyms[1] =
		strdup(race_synonyms[i].synonyms[1]);
	((frace_synonyms *)(a->data.v))->synonyms[2] =
		strdup(race_synonyms[i].synonyms[2]);
	((frace_synonyms *)(a->data.v))->synonyms[3] =
		strdup(race_synonyms[i].synonyms[3]);

	return;
}

static void
set_group(unit * u)
{
	const char * s = getstrtoken();
	join_group(u, s);
}

static void
set_name(region * r, unit * u, strlist * S)
{
  char **s;
	const char *s2;
	int i;
	param_t p;
	boolean foreign = false;

	s = 0;

	p = getparam(u->faction->locale);

	if(p == P_FOREIGN) {
		foreign = true;
		p = getparam(u->faction->locale);
	}

	switch (p) {
	case P_BUILDING:
	case P_GEBAEUDE:
		if (foreign == true) {
			building *b = getbuilding(r);
			unit *uo;

			if (!b) {
				cmistake(u, S->s, 6, MSG_EVENT);
				break;
			}

			if (b->type == bt_find("generic")) {
				cmistake(u, S->s, 278, MSG_EVENT);
				break;
			}

			if(!fval(b,FL_UNNAMED)) {
				cmistake(u, S->s, 246, MSG_EVENT);
				break;
			}

			uo = buildingowner(r, b);
			if (uo) {
				if (cansee(uo->faction, r, u, 0)) {
					add_message(&uo->faction->msgs, new_message(uo->faction,
						"renamed_building_seen%b:building%u:renamer%r:region", b, u, r));
				} else {
					add_message(&uo->faction->msgs, new_message(uo->faction,
						"renamed_building_notseen%b:building%r:region", b, r));
				}
			}
			s = &b->name;
		} else {
			if (!u->building) {
				cmistake(u, S->s, 145, MSG_PRODUCE);
				break;
			}
			if (!fval(u, UFL_OWNER)) {
				cmistake(u, S->s, 148, MSG_PRODUCE);
				break;
			}
			if (u->building->type == bt_find("generic")) {
				cmistake(u, S->s, 278, MSG_EVENT);
				break;
			}
			sprintf(buf, "Monument %d", u->building->no);
			if (u->building->type == bt_find("monument")
				&& !strcmp(u->building->name, buf)) {
				cmistake(u, S->s, 29, MSG_EVENT);
				break;
			}
			if (u->building->type == bt_find("artsculpure")) {
				cmistake(u, S->s, 29, MSG_EVENT);
				break;
			}
			s = &u->building->name;
			freset(u->building, FL_UNNAMED);
		}
		break;

	case P_FACTION:
		if (foreign == true) {
			faction *f;

			f = getfaction();
			if (!f) {
				cmistake(u, S->s, 66, MSG_EVENT);
				break;
			}
			if (f->age < 10) {
				cmistake(u, S->s, 248, MSG_EVENT);
				break;
			}
			if (!fval(f,FL_UNNAMED)) {
				cmistake(u, S->s, 247, MSG_EVENT);
				break;
			}
			if (cansee(f, r, u, 0)) {
				add_message(&f->msgs, new_message(f,
					"renamed_faction_seen%u:renamer%r:region", u, r));
			} else {
				add_message(&f->msgs, new_message(f,
					"renamed_faction_notseen%r:region", r));
			}
			s = &f->name;
		} else {
			s = &u->faction->name;
			freset(u->faction, FL_UNNAMED);
		}
		break;

	case P_SHIP:
		if (foreign == true) {
			ship *sh = getship(r);
			unit *uo;

			if (!sh) {
				cmistake(u, S->s, 20, MSG_EVENT);
				break;
			}
			if (!fval(sh,FL_UNNAMED)) {
				cmistake(u, S->s, 245, MSG_EVENT);
				break;
			}
			uo = shipowner(r,sh);
			if (uo) {
				if (cansee(uo->faction, r, u, 0)) {
					add_message(&uo->faction->msgs, new_message(uo->faction,
						"renamed_ship_seen%h:ship%u:renamer%r:region", sh, u, r));
				} else {
					add_message(&uo->faction->msgs, new_message(uo->faction,
						"renamed_ship_notseen%h:ship%r:region", sh, r));
				}
			}
			s = &sh->name;
		} else {
			if (!u->ship) {
				cmistake(u, S->s, 144, MSG_PRODUCE);
				break;
			}
			if (!fval(u, UFL_OWNER)) {
				cmistake(u, S->s, 12, MSG_PRODUCE);
				break;
			}
			s = &u->ship->name;
			freset(u->ship, FL_UNNAMED);
		}
		break;

	case P_UNIT:
		if (foreign == true) {
			unit *u2 = getunit(r, u->faction);
			if (!u2 || !cansee(u->faction, r, u2, 0)) {
				cmistake(u, S->s, 63, MSG_EVENT);
				break;
			}
			if (!fval(u,FL_UNNAMED)) {
				cmistake(u, S->s, 244, MSG_EVENT);
				break;
			}
			if (cansee(u2->faction, r, u, 0)) {
				add_message(&u2->faction->msgs, new_message(u2->faction,
					"renamed_seen%u:renamer%u:renamed%r:region", u, u2, r));
			} else {
				add_message(&u2->faction->msgs, new_message(u2->faction,
					"renamed_notseen%u:renamed%r:region", u2, r));
			}
			s = &u2->name;
		} else {
			s = &u->name;
			freset(u, FL_UNNAMED);
		}
		break;

	case P_REGION:
		if (!u->building) {
			cmistake(u, S->s, 145, MSG_EVENT);
			break;
		}
		if (!fval(u, UFL_OWNER)) {
			cmistake(u, S->s, 148, MSG_EVENT);
			break;
		}
		if (u->building != largestbuilding(r,false)) {
			cmistake(u, S->s, 147, MSG_EVENT);
			break;
		}
		s = &r->land->name;
		break;

	case P_GROUP:
		{
			attrib * a = a_find(u->attribs, &at_group);
			if (a){
				group * g = (group*)a->data.v;
				s= &g->name;
				break;
			} else {
				cmistake(u, S->s, 109, MSG_EVENT);
				break;
			}
		}
		break;
	default:
		cmistake(u, S->s, 109, MSG_EVENT);
		break;
	}

	if (!s)
		return;

	s2 = getstrtoken();

	if (!s2[0]) {
		cmistake(u, S->s, 84, MSG_EVENT);
		return;
	}
	for (i = 0; s2[i]; i++)
		if (s2[i] == '(')
			break;

	if (s2[i]) {
		cmistake(u, S->s, 112, MSG_EVENT);
		return;
	}
	set_string(&(*s), s2);
}
/* ------------------------------------------------------------- */

static void
deliverMail(faction * f, region * r, unit * u, const char *s, unit * receiver)
{
	if (!receiver) { /* BOTSCHAFT an PARTEI */
		char * message = (char*)gc_add(strdup(s));
		add_message(&f->msgs,
			msg_message("unitmessage", "region unit string", r, u, message));
	}
	else {					/* BOTSCHAFT an EINHEIT */
		unit *emp = receiver;
		if (cansee(f, r, u, 0))
			sprintf(buf, "Eine Botschaft von %s: '%s'", unitname(u), s);
		else
			sprintf(buf, "Eine anonyme Botschaft: '%s'", s);
		addstrlist(&emp->botschaften, strdup(buf));
	}
}

static void
mailunit(region * r, unit * u, int n, strlist * S, const char * s)
{
	unit *u2;		/* nur noch an eine Unit möglich */

	u2=findunitr(r,n);

	if (u2 && cansee(u->faction, r, u2, 0)) {
		deliverMail(u2->faction, r, u, s, u2);
	}
	else
		cmistake(u, S->s, 63, MSG_MESSAGE);
	/* Immer eine Meldung - sonst könnte man so getarnte EHs enttarnen:
	 * keine Meldung -> EH hier. */
}

static void
mailfaction(unit * u, int n, strlist * S, const char * s)
{
	faction *f;

	f = findfaction(n);
	if (f && n>0)
		deliverMail(f, u->region, u, s, NULL);
	else
		cmistake(u, S->s, 66, MSG_MESSAGE);
}

static void
distributeMail(region * r, unit * u, strlist * S)
{
	unit *u2;
	const char *s;
	int n;

	s = getstrtoken();

	/* Falls kein Parameter, ist das eine Einheitsnummer;
	 * das Füllwort "AN" muß wegfallen, da gültige Nummer! */

	if(strcasecmp(s, "an") == 0)
		s = getstrtoken();

	switch (findparam(s, u->faction->locale)) {

	case P_REGION:				/* können alle Einheiten in der Region sehen */
		s = getstrtoken();
		if (!s[0]) {
			cmistake(u, S->s, 30, MSG_MESSAGE);
			return;
		} else {
			sprintf(buf, "von %s: '%s'", unitname(u), s);
			addmessage(r, 0, buf, MSG_MESSAGE, ML_IMPORTANT);
			return;
		}
		break;

	case P_FACTION:
		{
			boolean see = false;

			n = getfactionid();

			for(u2=r->units; u2; u2=u2->next) {
				if(u2->faction->no == n && seefaction(u->faction, r, u2, 0)) {
					see = true;
					break;
				}
			}

			if(see == false) {
				cmistake(u, S->s, 66, MSG_MESSAGE);
				return;
			}

			s = getstrtoken();
			if (!s[0]) {
				cmistake(u, S->s, 30, MSG_MESSAGE);
				return;
			}
			mailfaction(u, n, S, s);
		}
		break;

	case P_UNIT:
		{
			boolean see = false;
			n = getid();

			for(u2=r->units; u2; u2=u2->next) {
				if(u2->no == n && cansee(u->faction, r, u2, 0)) {
					see = true;
					break;
				}
			}

			if(see == false) {
				cmistake(u, S->s, 63, MSG_MESSAGE);
				return;
			}

			s = getstrtoken();
			if (!s[0]) {
				cmistake(u, S->s, 30, MSG_MESSAGE);
				return;
			}
			mailunit(r, u, n, S, s);
		}
		break;

	case P_BUILDING:
	case P_GEBAEUDE:
		{
			building *b = getbuilding(r);

			if(!b) {
				cmistake(u, S->s, 6, MSG_MESSAGE);
				return;
			}

			s = getstrtoken();

			if (!s[0]) {
				cmistake(u, S->s, 30, MSG_MESSAGE);
				return;
			}

			for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);

			for(u2=r->units; u2; u2=u2->next) {
				if(u2->building == b && !fval(u2->faction, FL_DH)
						&& cansee(u->faction, r, u2, 0)) {
					mailunit(r, u, u2->no, S, s);
					fset(u2->faction, FL_DH);
				}
			}
		}
		break;

	case P_SHIP:
		{
			ship *sh = getship(r);

			if(!sh) {
				cmistake(u, S->s, 20, MSG_MESSAGE);
				return;
			}

			s = getstrtoken();

			if (!s[0]) {
				cmistake(u, S->s, 30, MSG_MESSAGE);
				return;
			}

			for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);

			for(u2=r->units; u2; u2=u2->next) {
				if(u2->ship == sh && !fval(u2->faction, FL_DH)
						&& cansee(u->faction, r, u2, 0)) {
					mailunit(r, u, u2->no, S, s);
					fset(u2->faction, FL_DH);
				}
			}
		}
		break;

	default:
		cmistake(u, S->s, 149, MSG_MESSAGE);
		return;
	}
}

static void
mail(void)
{
	region *r;
	unit *u;
	strlist *S;

	puts(" - verschicke Botschaften...");

	for (r = regions; r; r = r->next)
		for (u = r->units; u; u = u->next)
			for (S = u->orders; S; S = S->next)
				if (igetkeyword(S->s, u->faction->locale) == K_MAIL)
					distributeMail(r, u, S);
}
/* ------------------------------------------------------------- */

static void
report_option(unit * u, const char * sec, char *cmd)
{
	const messageclass * mc;
	const char *s;

	mc = mc_find(sec);

	if (mc == NULL) {
		cmistake(u, findorder(u, cmd), 135, MSG_EVENT);
		return;
	}
	s = getstrtoken();
#ifdef MSG_LEVELS
        if (s[0])
		set_msglevel(&u->faction->warnings, mc->name, atoi(s));
	else
		set_msglevel(&u->faction->warnings, mc->name, -1);
#endif
}

static void
set_passw(void)
{
	region *r;
	unit *u;
	strlist *S;
	const char *s;
	int o, i;
	magic_t mtyp;

	puts(" - setze Passwörter, Adressen und Format, Abstimmungen");

	for (r = regions; r; r = r->next)
		for (u = r->units; u; u = u->next)
			for (S = u->orders; S; S = S->next) {
				switch (igetkeyword(S->s, u->faction->locale)) {
				case NOKEYWORD:
					cmistake(u, S->s, 22, MSG_EVENT);
					break;

				case K_BANNER:
					s = getstrtoken();

					set_string(&u->faction->banner, s);
					add_message(&u->faction->msgs, new_message(u->faction,
						"changebanner%s:value", gc_add(strdup(u->faction->banner))));
					break;

				case K_EMAIL:
					s = getstrtoken();

					if (!s[0]) {
						cmistake(u, S->s, 85, MSG_EVENT);
						break;
					}
					if (strstr(s, "internet:") || strchr(s, ' ')) {
						cmistake(u, S->s, 117, MSG_EVENT);
						break;
					}
					set_string(&u->faction->email, s);
					add_message(&u->faction->msgs, new_message(u->faction,
						"changemail%s:value", gc_add(strdup(u->faction->email))));
					break;

				case K_PASSWORD:
					{
						char pbuf[32];
						int i;

						s = getstrtoken();

						if (!s || !*s) {
							for(i=0; i<6; i++) pbuf[i] = (char)(97 + rand() % 26);
							pbuf[6] = 0;
						} else {
							boolean pwok = true;
							char *c;

							strncpy(pbuf, s, 31);
							pbuf[31] = 0;
							c = pbuf;
							while(*c) {
								if(!isalnum(*c)) pwok = false;
								c++;
							}
							if (pwok == false) {
								cmistake(u, S->s, 283, MSG_EVENT);
								for(i=0; i<6; i++) pbuf[i] = (char)(97 + rand() % 26);
								pbuf[6] = 0;
							}
						}
						set_string(&u->faction->passw, pbuf);
						fset(u->faction, FFL_OVERRIDE);
						ADDMSG(&u->faction->msgs, msg_message("changepasswd",
							"value", gc_add(strdup(u->faction->passw))));
					}
					break;

				case K_REPORT:
					s = getstrtoken();
					i = atoi(s);
					sprintf(buf, "%d", i);
					if (!strcmp(buf, s)) {
						/* int level;
						level = geti();
						not implemented yet. set individual levels for f->msglevels */
					} else {
						report_option(u, s, S->s);
					}
					break;

				case K_SEND:
					s = getstrtoken();
					o = findoption(s, u->faction->locale);
#ifdef AT_OPTION
					/* Sonderbehandlung Zeitungsoption */
					if (o == O_NEWS) {
						attrib *a = a_find(u->faction->attribs, &at_option_news);
						if(a) a->data.i = 0;

						while((s = getstrtoken())) {
							if(findparam(s) == P_NOT) {
								a_removeall(&u->faction->attribs, &at_option_news);
								u->faction->options = u->faction->options & ~O_NEWS;
								break;
							} else {
								int sec = atoi(s);
								if(sec != 0) {
									if(!a) a_add(&u->faction->attribs, a_new(&at_option_news));
									a->data.i = a->data.i & (1<<(sec-1));
									u->faction->options = u->faction->options | O_NEWS;
								}
							}
						}
						break;
					}
#endif
					if (o == -1) {
						cmistake(u, S->s, 135, MSG_EVENT);
					} else {
						if (getparam(u->faction->locale) == P_NOT) {
							if(o == O_COMPRESS || o == O_BZIP2) {
								cmistake(u, S->s, 305, MSG_EVENT);
							} else {
								u->faction->options = u->faction->options & ~((int)pow(2, o));
							}
						} else {
							u->faction->options = u->faction->options | ((int)pow(2,o));
							if(o == O_COMPRESS) u->faction->options &= ~((int)pow(2,O_BZIP2));
							if(o == O_BZIP2) u->faction->options &= ~((int)pow(2,O_COMPRESS));
						}
					}
					break;

				case K_MAGIEGEBIET:
					if(u->faction->magiegebiet != 0) {
						add_message(&u->faction->msgs,
							msg_error(u, S->s, "one_circle_only", ""));
					} else {
						mtyp = getmagicskill();
						if(mtyp == M_NONE) {
							mistake(u, S->s, "Syntax: MAGIEGEBIET <Gebiet>", MSG_EVENT);
						} else {
							region *r2;
							unit   *u2;
							sc_mage *m;
							region *last = lastregion(u->faction);

							u->faction->magiegebiet = mtyp;

							for (r2 = firstregion(u->faction); r2 != last; r2 = r2->next) {
								for (u2 = r->units; u2; u2 = u2->next) {
									if(u2->faction == u->faction
											&& has_skill(u2, SK_MAGIC)) {
										m = get_mage(u2);
										m->magietyp = mtyp;
									}
								}
							}
						}
					}
					break;
				}
			}
}

static boolean
display_item(faction *f, unit *u, const item_type * itype)
{
	FILE *fp;
	char t[NAMESIZE + 1];
	char filename[MAX_PATH];
	const char *name;

	if (u && *i_find(&u->items, itype) == NULL) return false;
	/*
	info = mkname("info", itype->rtype->_name[0]);
	name = LOC(u->faction->locale, info);
	if (strcmp(name, info)==0) {
	*/
		name = resourcename(itype->rtype, 0);
		sprintf(filename, "%s/%s/items/%s", resourcepath(), locale_name(f->locale), name);
		fp = fopen(filename, "r");
		if (!fp) {
			name = locale_string(f->locale, resourcename(itype->rtype, 0));
			sprintf(filename, "%s/%s/items/%s", resourcepath(), locale_name(f->locale), name);
			fp = fopen(filename, "r");
		}
		if (!fp) {
			name = resourcename(itype->rtype, 0);
			sprintf(filename, "%s/%s/items/%s", resourcepath(), locale_name(default_locale), name);
			fp = fopen(filename, "r");
		}
		if (!fp) return false;

		buf[0]='\0';
		while (fgets(t, NAMESIZE, fp) != NULL) {
			if (t[strlen(t) - 1] == '\n') {
				t[strlen(t) - 1] = 0;
			}
			strcat(buf, t);
		}
		fclose(fp);
		name = buf;
/*	} */
	ADDMSG(&f->msgs, msg_message("displayitem", "item description", itype->rtype, strdup(name)));

	return true;
}

static boolean
display_potion(faction *f, unit *u, const potion_type * ptype)
{
	attrib *a;

	if (ptype==NULL) return false;
	else {
		int i = i_get(u->items, ptype->itype);
		if (i==0 && 2*ptype->level > effskill(u,SK_ALCHEMY)) {
			return false;
		}
	}

	a = a_find(f->attribs, &at_showitem);
	while (a && a->data.v != ptype) a=a->next;
	if (!a) {
		a = a_add(&f->attribs, a_new(&at_showitem));
		a->data.v = (void*) ptype->itype;
	}

	return true;
}

static boolean
display_race(faction *f, unit *u, const race * rc)
{
	FILE *fp;
	char t[NAMESIZE + 1];
	char filename[256];
	const char *name;
	int a, at_count;
	char buf2[2048];

	if (u && u->race != rc) return false;
	name = rc_name(rc, 0);

	sprintf(buf, "%s: ", LOC(f->locale, name));

	sprintf(filename, "showdata/%s", LOC(default_locale, name));
	fp = fopen(filename, "r");
	if(fp) {
		while (fgets(t, NAMESIZE, fp) != NULL) {
			if (t[strlen(t) - 1] == '\n') {
				t[strlen(t) - 1] = 0;
			}
			strcat(buf, t);
		}
		fclose(fp);
		strcat(buf, ". ");
	}

	/* hp_p : Trefferpunkte */
	sprintf(buf2, "Trefferpunkte: %d", rc->hitpoints);
	strcat(buf, buf2);
	/* b_armor : Rüstung */
	if (rc->armor > 0){
		sprintf(buf2, ", Rüstung: %d", rc->armor);
		strcat(buf, buf2);
	}
	/* b_attacke : Angriff */
	sprintf(buf2, ", Angriff: %d", (rc->at_default+rc->at_bonus));
	strcat(buf, buf2);

	/* b_defense : Verteidigung */
	sprintf(buf2, ", Verteidigung: %d", (rc->df_default+rc->df_bonus));
	strcat(buf, buf2);

	strcat(buf, ".");

	/* b_damage : Schaden */
	at_count=0;
	for (a = 0; a < 6; a++) {
		if (rc->attack[a].type != AT_NONE){
			at_count++;
		}
	}
	if (rc->battle_flags & BF_EQUIPMENT) {
		strcat(buf, " Kann Waffen benutzen.");
	}
	if (rc->battle_flags & BF_RES_PIERCE) {
		strcat(buf, " Ist durch Stichwaffen, Bögen und Armbrüste schwer zu verwunden.");
	}
	if (rc->battle_flags & BF_RES_CUT) {
		strcat(buf, " Ist durch Hiebwaffen schwer zu verwunden.");
	}
	if (rc->battle_flags & BF_RES_BASH) {
		strcat(buf, " Ist durch Schlagwaffen und Katapulte schwer zu verwunden.");
	}

	sprintf(buf2, " Hat %d Angriff%s", at_count, (at_count>1)?"e":"");
	strcat(buf, buf2);
	for (a = 0; a < 6; a++) {
		if (rc->attack[a].type != AT_NONE){
			if (a!=0){
				strcat(buf, ", ");
			} else {
				strcat(buf, ": ");
			}
			switch(rc->attack[a].type) {
			case AT_STANDARD:
				sprintf(buf2, "ein Angriff mit der Waffe oder macht unbewaffnet %s Schaden", rc->def_damage);
				break;
			case AT_NATURAL:
				sprintf(buf2, "ein Angriff mit Krallen, Zähnen oder Klauen, der %s Schaden macht", rc->attack[a].data.dice);
				break;
			case AT_SPELL:
			case AT_COMBATSPELL:
			case AT_DRAIN_ST:
			case AT_DAZZLE:
				sprintf(buf2, "ein magischer Angriff");
				break;
			case AT_STRUCTURAL:
				sprintf(buf2, "ein Angriff, der %s Gebäudeschaden verursacht", rc->attack[a].data.dice);
			}
			strcat(buf, buf2);
		}
	}

	strcat(buf, ".");

	addmessage(0, f, buf, MSG_EVENT, ML_IMPORTANT);

	return true;
}

static void
reshow(unit * u, const char* cmd, const char * s, param_t p)
{
  int skill, c;
  const potion_type * ptype;
  const item_type * itype;
  const spell * sp;
  const race * rc;

  switch (p) {
    case P_ZAUBER:
      a_removeall(&u->faction->attribs, &at_seenspell);
      break;
    case P_POTIONS:
      skill = effskill(u, SK_ALCHEMY);
      c = 0;
      for (ptype = potiontypes; ptype!=NULL; ptype=ptype->next) {
        if (ptype->level * 2 <= skill) {
          c += display_potion(u->faction, u, ptype);
        }
      }
      if (c == 0) cmistake(u, cmd, 285, MSG_EVENT);
      break;
    case NOPARAM:
      /* check if it's an item */
      itype = finditemtype(s, u->faction->locale);
      if (itype!=NULL) {
        ptype = resource2potion(item2resource(itype));
        if (ptype!=NULL) {
          if (display_potion(u->faction, u, ptype)) break;
        } else {
          if (display_item(u->faction, u, itype)) break;
        }
      }
      /* try for a spell */
      sp = find_spellbyname(u, s, u->faction->locale);
      if (sp!=NULL && has_spell(u, sp)) {
        attrib *a = a_find(u->faction->attribs, &at_seenspell);
        while (a!=NULL && a->data.i!=sp->id) a = a->nexttype;
        if (a!=NULL) a_remove(&u->faction->attribs, a);
        break;
      }
      /* last, check if it's a race. */
      rc = findrace(s, u->faction->locale);
      if (rc != NULL) {
        if (display_race(u->faction, u, rc)) break;
      }
      cmistake(u, cmd, 21, MSG_EVENT);
      break;
    default:
      cmistake(u, cmd, 222, MSG_EVENT);
      break;
  }
}

static void
instant_orders(void)
{
	region *r;
	unit *u;
	strlist *S;
	const char *s;
	const char *param;
	spell *spell;
	faction *f;
	attrib *a;
	int level = 0;	/* 0 = MAX */

	puts(" - Kontakte, Hilfe, Status, Kampfzauber, Texte, Bewachen (aus), Zeigen");

	for (f = factions; f; f = f->next) {
#ifdef NEW_ITEMS
		a=a_find(f->attribs, &at_showitem);
		while(a!=NULL) {
			const item_type * itype = (const item_type *)a->data.v;
			const potion_type * ptype = resource2potion(itype->rtype);
			if (ptype!=NULL) {
				attrib * n = a->nexttype;
				/* potions werden separat behandelt */
				display_item(f, NULL, (const item_type *)a->data.v);
				a_remove(&f->attribs, a);
				a = n;
			} else a = a->nexttype;
		}
#else
		while((a=a_find(f->attribs, &at_show_item_description))!=NULL) {
			display_item(f, NULL, (item_t)(a->data.i));
			a_remove(&f->attribs, a);
		}
#endif
	}

	for (r = regions; r; r = r->next)
		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next)
			{
				if (igetkeyword(S->s, u->faction->locale)==K_GROUP) {
					set_group(u);
				}
			}
			for (S = u->orders; S; S = S->next)

				switch (igetkeyword(S->s, u->faction->locale)) {
				case K_URSPRUNG:
					{
						int px, py;

						px = atoi(getstrtoken());
						py = atoi(getstrtoken());

						set_ursprung(u->faction, getplaneid(r), px, py);
					}
					break;

				case K_ALLY:
					set_ally(u, S);
					break;

				case K_PREFIX:
					set_prefix(u, S);
					break;

				case K_SYNONYM:
					set_synonym(u, S);
					break;

				case K_SETSTEALTH:
					setstealth(u, S);
					break;

				case K_WEREWOLF:
					setwere(u, S);
					break;

				case K_STATUS:
					param = getstrtoken();
					switch (findparam(param, u->faction->locale)) {
					case P_NOT:
						u->status = ST_AVOID;
						break;

					case P_BEHIND:
						u->status = ST_BEHIND;
						break;

					case P_FLEE:
						u->status = ST_FLEE;
						break;

					case P_CHICKEN:
						u->status = ST_CHICKEN;
						break;

					case P_AGGRO:
						u->status = ST_AGGRO;
						break;

					case P_VORNE:
						u->status = ST_FIGHT;
						break;

					case P_HELP:
						param = getstrtoken();
						if( findparam(param, u->faction->locale) == P_NOT) {
							fset(u, UFL_NOAID);
						} else {
							freset(u, UFL_NOAID);
						}
						break;

					default:
						if (strlen(param)) {
							add_message(&u->faction->msgs,
								msg_error(u, S->s, "unknown_status", ""));
						} else {
							u->status = ST_FIGHT;
						}
					}
					break;


				/* KAMPFZAUBER [[STUFE n] "<Spruchname>"] [NICHT] */
				case K_COMBAT:

					s = getstrtoken();

					/* KAMPFZAUBER [NICHT] löscht alle gesetzten Kampfzauber */
					if (!s || *s == 0 || findparam(s, u->faction->locale) == P_NOT) {
						unset_combatspell(u, 0);
						break;
					}

					/* Optional: STUFE n */
					if (findparam(s, u->faction->locale) == P_LEVEL) {
						/* Merken, setzen kommt erst später */
						s = getstrtoken();
						level = atoi(s);
						level = max(0, level);
						s = getstrtoken();
					}
					else level = 0;

					spell = find_spellbyname(u, s, u->faction->locale);

					if(!spell){
						cmistake(u, S->s, 173, MSG_MAGIC);
						break;
					}

					/* KAMPFZAUBER "<Spruchname>" NICHT  löscht diesen speziellen
					 * Kampfzauber */
					s = getstrtoken();
					if(findparam(s, u->faction->locale) == P_NOT){
						unset_combatspell(u,spell);
						break;
					}

					/* KAMPFZAUBER "<Spruchname>"  setzt diesen Kampfzauber */
					set_combatspell(u,spell, S->s, level);
					break;

				case K_DISPLAY:
					set_display(r, u, S);
					break;
				case K_NAME:
					set_name(r, u, S);
					break;

				case K_GUARD:
					if (u->faction->age < IMMUN_GEGEN_ANGRIFF) {
						cmistake(u, S->s, 304, MSG_EVENT);
						break;
					}
					if (fval(u, UFL_HUNGER)) {
						cmistake(u, S->s, 223, MSG_EVENT);
						break;
					}
					if (getparam(u->faction->locale) == P_NOT)
						setguard(u, GUARD_NONE);
					break;

				case K_RESHOW:
					s = getstrtoken();
					if (findparam(s, u->faction->locale) == P_ANY) {
						param_t p = getparam(u->faction->locale);
						reshow(u, S->s, s, p);
					}
					else reshow(u, S->s, s, NOPARAM);
					break;
				}
		}

}
/* ------------------------------------------------------------- */
/* Beachten: einige Monster sollen auch unbewaffent die Region bewachen
 * können */
void
remove_unequipped_guarded(void)
{
	region *r;
	unit *u;

	for (r = regions; r; r = r->next)
		for (u = r->units; u; u = u->next) {
			if (getguard(u) && (!armedmen(u) || u->faction->age < IMMUN_GEGEN_ANGRIFF))
				setguard(u, GUARD_NONE);
		}
}

static void
bewache_an(void)
{
	region *r;
	unit *u;
	strlist *S;

	/* letzte schnellen befehle - bewache */
	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			if (!fval(u, UFL_MOVED)) {
				for (S = u->orders; S; S = S->next) {
					if (igetkeyword(S->s, u->faction->locale) == K_GUARD && getparam(u->faction->locale) != P_NOT) {
						if (rterrain(r) != T_OCEAN) {
							if (!fval(u, RCF_ILLUSIONARY) && u->race != new_race[RC_SPELL]) {
#ifdef WACH_WAFF
								/* Monster der Monsterpartei dürfen immer bewachen */
								if (u->faction == findfaction(MONSTER_FACTION)){
									guard(u, GUARD_ALL);
									continue;
								}

								if (!armedmen(u)) {
									add_message(&u->faction->msgs,
										msg_error(u, S->s, "unit_unarmed", ""));
									continue;
								}
#endif
								guard(u, GUARD_ALL);
							} else {
								cmistake(u, S->s, 95, MSG_EVENT);
							}
						} else {
							cmistake(u, S->s, 2, MSG_EVENT);
						}
					}
				}
			}
		}
	}
}

static void
sinkships(void)
{
	region *r;

	/* Unbemannte Schiffe können sinken */
	for (r = regions; r; r = r->next) {
		ship *sh;

		list_foreach(ship, r->ships, sh) {
			if (rterrain(r) == T_OCEAN && (!enoughsailors(r, sh) || !kapitaen(r, sh))) {
				/* Schiff nicht seetüchtig */
				damage_ship(sh, 0.30);
			}
			if (!shipowner(r, sh)) {
				damage_ship(sh, 0.05);
			}
			if (sh->damage >= sh->size * DAMAGE_SCALE)
				destroy_ship(sh);
		}
		list_next(sh);
	}
}

/* The following functions do not really belong here: */
#include "eressea.h"
#include "build.h"

static attrib_type at_number = {
	"faction_renum",
	NULL, NULL, NULL, NULL, NULL,
	ATF_UNIQUE
};

static void
renumber_factions(void)
	/* gibt parteien neue nummern */
{
	struct renum {
		struct renum * next;
		int want;
		faction * faction;
		attrib * attrib;
	} * renum = NULL, * rp;
	faction * f;
	for (f=factions;f;f=f->next) {
		attrib * a = a_find(f->attribs, &at_number);
		int want;
		struct renum ** rn;
		faction * old;

		if (!a) continue;
		want = a->data.i;
		if (fval(f, FF_NEWID)) {
			sprintf(buf, "NUMMER PARTEI %s: Die Partei kann nicht mehr als einmal ihre Nummer wecheln", itoa36(want));
			addmessage(0, f, buf, MSG_MESSAGE, ML_IMPORTANT);
		}
		old = findfaction(want);
		if (old) {
			a_remove(&f->attribs, a);
			sprintf(buf, "Die Nummer %s wird von einer anderen Partei benutzt.", itoa36(want));
			addmessage(0, f, buf, MSG_MESSAGE, ML_IMPORTANT);
			continue;
		}
		if (!faction_id_is_unused(want)) {
			a_remove(&f->attribs, a);
			sprintf(buf, "Die Nummer %s wurde schon einmal von einer anderen Partei benutzt.", itoa36(want));
			addmessage(0, f, buf, MSG_MESSAGE, ML_IMPORTANT);
			continue;
		}
		for (rn=&renum; *rn; rn=&(*rn)->next) {
			if ((*rn)->want>=want) break;
		}
		if (*rn && (*rn)->want==want) {
			a_remove(&f->attribs, a);
			sprintf(buf, "Die Nummer %s wurde bereits einer anderen Partei zugeteilt.", itoa36(want));
			addmessage(0, f, buf, MSG_MESSAGE, ML_IMPORTANT);
		} else {
			struct renum * r = calloc(sizeof(struct renum), 1);
			r->next = *rn;
			r->attrib = a;
			r->faction = f;
			r->want = want;
			*rn = r;
		}
	}
	for (rp=renum;rp;rp=rp->next) {
		f = rp->faction;
		a_remove(&f->attribs, rp->attrib);
		if (f->subscription) fprintf(sqlstream, "UPDATE subscriptions set faction='%s' where "
			"id=%u;\n", itoa36(rp->want), 
			f->subscription);
		f->no = rp->want;
		register_faction_id(rp->want);
		fset(f, FF_NEWID);
	}
	while (renum) {
		rp = renum->next;
		free(renum);
		renum = rp;
	}
}

static void
reorder(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u, ** up=&r->units;
		boolean sorted=false;
		while (*up) {
			u = *up;
			if (!fval(u, FL_MARK)) {
				strlist * o;
				for (o=u->orders;o;o=o->next) {
					if (igetkeyword(o->s, u->faction->locale)==K_SORT) {
						const char * s = getstrtoken();
						param_t p = findparam(s, u->faction->locale);
						int id = getid();
						unit **vp, *v = findunit(id);
						if (v==NULL || v->faction!=u->faction || v->region!=r) {
							cmistake(u, o->s, 258, MSG_EVENT);
						} else if (v->building != u->building || v->ship!=u->ship) {
							cmistake(u, o->s, 259, MSG_EVENT);
						} else if (fval(u, UFL_OWNER)) {
							cmistake(u, o->s, 260, MSG_EVENT);
						} else if (v == u) {
							cmistake(u, o->s, 10, MSG_EVENT);
						} else {
							switch(p) {
							case P_AFTER:
								*up = u->next;
								u->next = v->next;
								v->next = u;
								break;
							case P_BEFORE:
								if (fval(v, UFL_OWNER)) {
									cmistake(u, o->s, 261, MSG_EVENT);
								} else {
									vp=&r->units;
									while (*vp!=v) vp=&(*vp)->next;
									*vp = u;
									*up = u->next;
									u->next = v;
								}
								break;
							}
							fset(u, FL_MARK);
							sorted = true;
						}
						break;
					}
				}
			}
			if (u==*up) up=&u->next;
		}
		if (sorted) for (u=r->units;u;u=u->next) freset(u, FL_MARK);
	}
}

#if 0
/* Aus Gebäude weisen, VERBANNE */
static void
evict(void)
{
	region *r;
	strlist *S;
	unit * u;

	for (r=regions;r;r=r->next) {
		for (u=r->units;u;u=u->next) {
			for (S = u->orders; S; S = S->next) if (igetkeyword(S->s, u->faction->locale)==K_EVICT) {
				/* Nur der Kapitän bzw Burgherr kann jemanden rausschmeißen */
				if(!fval(u, UFL_OWNER)) {
					/* Die Einheit ist nicht der Eigentümer */
					cmistake(u,S->s,49,MSG_EVENT);
					continue;
				}
				int id = getid();
				unit *u2 = findunit(id);
				if (!u2){
					/* Einheit nicht gefunden */
					cmistake(u,S->s,63,MSG_EVENT);
					continue;
				}

				if (u->building){
					/* in der selben Burg? */
					if (u->building != u2->building){
						/* nicht in Burg */
						cmistake(u,S->s,33,MSG_EVENT);
						continue;
					}
					leave_building(u2);
					/* meldung an beide */
				}

				if (u->ship){
					if (u->ship != u2->ship){
						/* nicht an Bord */
						cmistake(u,S->s,32,MSG_EVENT);
						continue;
					}
					leave_ship(u2);
					/* meldung an beide */
				}
			}
		}
	}
}
#endif

#ifdef REGIONOWNERS
static void
declare_war(void)
{
  region *r;
  for (r=regions;r;r=r->next) {
    unit * u;
    for (u=r->units;u;u=u->next) {
      strlist *S;
      faction * f = u->faction;
      for (S = u->orders; S; S = S->next) {
        switch (igetkeyword(S->s, f->locale)) {
        case K_WAR:
          for (;;) {
            const char * s = getstrtoken();
            if (s[0]==0) break;
            else {
              faction * enemy = findfaction(atoi36(s));
              if (enemy) {
                if (!is_enemy(f, enemy)) {
                  add_enemy(f, enemy);
                  ADDMSG(&enemy->msgs, msg_message("war_notify", "enemy", f));
                  ADDMSG(&f->msgs, msg_message("war_confirm", "enemy", enemy));
                }
              } else {
                ADDMSG(&f->msgs, msg_message("error66", "unit region command", u, r, S->s));
              }
            }
          }
          break;
        case K_PEACE:
          for (;;) {
            const char * s = getstrtoken();
            if (s[0]==0) break;
            else {
              faction * enemy = findfaction(atoi36(s));
              if (enemy) {
                if (is_enemy(f, enemy)) {
                  remove_enemy(f, enemy);
                  ADDMSG(&enemy->msgs, msg_message("peace_notify", "enemy", f));
                  ADDMSG(&f->msgs, msg_message("peace_confirm", "enemy", enemy));
                }
              } else {
                ADDMSG(&f->msgs, msg_message("error66", "unit region command", u, r, S->s));
              }
            }
          }
          break;
        default:
          break;
        }
      }
    }
  }
}
#endif

static void
renumber(void)
{
	region *r;
	const char *s;
	strlist *S;
	unit * u;
	int i;

  for (r=regions;r;r=r->next) {
		for (u=r->units;u;u=u->next) {
			faction * f = u->faction;
			for (S = u->orders; S; S = S->next) if (igetkeyword(S->s, u->faction->locale)==K_NUMBER) {
				s = getstrtoken();
				switch(findparam(s, u->faction->locale)) {

				case P_FACTION:
					s = getstrtoken();
					if (s && *s) {
						int i = atoi36(s);
						attrib * a = a_find(f->attribs, &at_number);
						if (!a) a = a_add(&f->attribs, a_new(&at_number));
						a->data.i = i;
					}
					break;

				case P_UNIT:
					s = getstrtoken();
					if(s == NULL || *s == 0) {
						i = newunitid();
					} else {
						i = atoi36(s);
						if (i<=0 || i>MAX_UNIT_NR) {
							cmistake(u,S->s,114,MSG_EVENT);
							continue;
						}

						if(forbiddenid(i)) {
							cmistake(u,S->s,116,MSG_EVENT);
							continue;
						}

						if(findunitg(i, r)) {
							cmistake(u,S->s,115,MSG_EVENT);
							continue;
						}
					}
					uunhash(u);
					if (!ualias(u)) {
						attrib *a = a_add(&u->attribs, a_new(&at_alias));
						a->data.i = -u->no;
					}
					u->no = i;
					uhash(u);
					break;

				case P_SHIP:
					if(!u->ship) {
						cmistake(u,S->s,144,MSG_EVENT);
						continue;
					}
					if(!fval(u, UFL_OWNER)) {
						cmistake(u,S->s,146,MSG_EVENT);
						continue;
					}
					s = getstrtoken();
					if(s == NULL || *s == 0) {
						i = newcontainerid();
					} else {
						i = atoi36(s);
						if (i<=0 || i>MAX_CONTAINER_NR) {
							cmistake(u,S->s,114,MSG_EVENT);
							continue;
						}
						if (findship(i) || findbuilding(i)) {
							cmistake(u,S->s,115,MSG_EVENT);
							continue;
						}
					}
					sunhash(u->ship);
					u->ship->no = i;
					shash(u->ship);
					break;

				case P_BUILDING:
				case P_GEBAEUDE:
					if(!u->building) {
						cmistake(u,S->s,145,MSG_EVENT);
						continue;
					}
					if(!fval(u, UFL_OWNER)) {
						cmistake(u,S->s,148,MSG_EVENT);
						continue;
					}
					s = getstrtoken();
					if(*s == 0) {
						i = newcontainerid();
					} else {
						i = atoi36(s);
						if (i<=0 || i>MAX_CONTAINER_NR) {
							cmistake(u,S->s,114,MSG_EVENT);
							continue;
						}
						if(findship(i) || findbuilding(i)) {
							cmistake(u,S->s,115,MSG_EVENT);
							continue;
						}
					}
					bunhash(u->building);
					u->building->no = i;
					bhash(u->building);
					break;

				default:
					cmistake(u,S->s,239,MSG_EVENT);
				}
			}
		}
	}
	renumber_factions();
}

static void
ageing(void)
{
	faction *f;
	region *r;

	/* altern spezieller Attribute, die eine Sonderbehandlung brauchen?  */
	for(r=regions;r;r=r->next) {
		unit *u;
		for(u=r->units;u;u=u->next) {
			/* Goliathwasser */
			int i = get_effect(u, oldpotiontype[P_STRONG]);
			if (i > 0){
				change_effect(u, oldpotiontype[P_STRONG], -1 * min(u->number, i));
			}
			/* Berserkerblut*/
			i = get_effect(u, oldpotiontype[P_BERSERK]);
			if (i > 0){
				change_effect(u, oldpotiontype[P_BERSERK], -1 * min(u->number, i));
			}

			if (is_cursed(u->attribs, C_OLDRACE, 0)){
				curse *c = get_curse(u->attribs, ct_find("oldrace"));
				if (c->duration == 1 && !(c->flag & CURSE_NOAGE)) {
					u->race = new_race[curse_geteffect(c)];
					u->irace = new_race[curse_geteffect(c)];
				}
			}
		}
	}

	/* Borders */
	age_borders();

	/* Factions */
	for (f=factions;f;f=f->next) {
		a_age(&f->attribs);
		handle_event(&f->attribs, "timer", f);
	}

	/* Regionen */
	for (r=regions;r;r=r->next) {
		building ** bp;
		unit ** up;
		ship ** sp;

		a_age(&r->attribs);
		handle_event(&r->attribs, "timer", r);
		/* Einheiten */
		for (up=&r->units;*up;) {
			unit * u = *up;
			a_age(&u->attribs);
			if (u==*up) handle_event(&u->attribs, "timer", u);
			if (u==*up) up = &(*up)->next;
		}
		/* Schiffe */
		for (sp=&r->ships;*sp;) {
			ship * s = *sp;
			a_age(&s->attribs);
			if (s==*sp) handle_event(&s->attribs, "timer", s);
			if (s==*sp) sp = &(*sp)->next;
		}
		/* Gebäude */
		for (bp=&r->buildings;*bp;) {
			building * b = *bp;
			a_age(&b->attribs);
			if (b==*bp) handle_event(&b->attribs, "timer", b);
			if (b==*bp) bp = &(*bp)->next;
		}
	}
}

static int
maxunits(faction *f)
{
	return (int) (global.maxunits * (1 + 0.4 * fspecial(f, FS_ADMINISTRATOR)));
}


static void
new_units (void)
{
	region *r;
	unit *u, *u2;
	strlist *S, *S2;

	/* neue einheiten werden gemacht und ihre befehle (bis zum "ende" zu
	 * ihnen rueberkopiert, damit diese einheiten genauso wie die alten
	 * einheiten verwendet werden koennen. */

	for (r = regions; r; r = r->next)
		for (u = r->units; u; u = u->next)
			for (S = u->orders; S;) {
				if ((igetkeyword(S->s, u->faction->locale) == K_MAKE) && (getparam(u->faction->locale) == P_TEMP)) {
					int g;
					const char * name;
					int alias;
					int mu = maxunits(u->faction);

					if(u->faction->no_units >= mu) {
						sprintf(buf, "Eine Partei darf aus nicht mehr als %d "
														"Einheiten bestehen.", mu);
						mistake(u, S->s, buf, MSG_PRODUCE);
						S = S->next;

						while (S) {
							if (igetkeyword(S->s, u->faction->locale) == K_END)
								break;
							S2 = S->next;
							removelist(&u->orders, S);
							S = S2;
						}
						continue;
					}
					alias = getid();

					name = getstrtoken();
					if (name && strlen(name)==0) name = NULL;
					u2 = create_unit(r, u->faction, 0, u->faction->race, alias, name, u);

					a_add(&u2->attribs, a_new(&at_alias))->data.i = alias;

					g = getguard(u);
					if (g) setguard(u2, g);
					else setguard(u, GUARD_NONE);

					S = S->next;

					while (S) {
						if (igetkeyword(S->s, u->faction->locale) == K_END)
							break;
						S2 = S->next;
						translist(&u->orders, &u2->orders, S);
						S = S2;
					}
				}
				if (S)
					S = S->next;
			}

	/* im for-loop wuerde S = S->next ausgefuehrt, bevor S geprueft wird.
	 * Wenn S aber schon 0x0 ist, fuehrt das zu einem Fehler. Und wenn wir
	 * den while (S) ganz durchlaufen, wird S = 0x0 sein! Dh. wir
	 * sicherstellen, dass S != 0, bevor wir S = S->next auszufuehren! */

}

static void
setdefaults (void)
{
  region *r;

  for (r = regions; r; r = r->next) {
    unit *u;

    for (u = r->units; u; u = u->next) {
      strlist *slist;
      boolean trade = false;

      if (LongHunger() && fval(u, UFL_HUNGER)) {
        /* Hungernde Einheiten führen NUR den default-Befehl aus */
        const char * cmd = locale_string(u->faction->locale, "defaultorder");
        set_string(&u->thisorder, cmd);
        continue;
      }

      /* by default the default long order becomes the new long order. */
      set_string(&u->thisorder, u->lastorder);
      
      /* check all orders for a potential new long order this round: */
      for (slist=u->orders; !trade && slist!=NULL; slist=slist->next) {
        const char * cmd = slist->s;

        keyword_t keyword = igetkeyword(cmd, u->faction->locale);
        switch (keyword) {

          case K_BUY:
          case K_SELL:
            /* Wenn die Einheit handelt, muß der Default-Befehl gelöscht
             * werden. */
            set_string(&u->thisorder, "");
            trade = true;
            break;

          case K_CAST:
            /* dient dazu, das neben Zaubern kein weiterer Befehl
             * ausgeführt werden kann, Zaubern ist ein kurzer Befehl */
            set_string(&u->thisorder, "");
            break;

          case K_MAKE:
            /* Falls wir MACHE TEMP haben, ignorieren wir es. Alle anderen
            * Arten von MACHE zaehlen aber als neue defaults und werden
            * behandelt wie die anderen (deswegen kein break nach case
            * K_MAKE) - und in thisorder (der aktuelle 30-Tage Befehl)
            * abgespeichert). */
            if (getparam(u->faction->locale) == P_TEMP) break;
            /* else fall through */

#if GROWING_TREES
          case K_PFLANZE:
#endif
          case K_BESIEGE:
          case K_ENTERTAIN:
          case K_TAX:
          case K_RESEARCH:
          case K_SPY:
          case K_STEAL:
          case K_SABOTAGE:
          case K_STUDY:
          case K_TEACH:
          case K_ZUECHTE:
          case K_BIETE:
          case K_PIRACY:
            /* Über dieser Zeile nur Befehle, die auch eine idle Faction machen darf */
            if (idle (u->faction)) {
              set_string (&u->thisorder, locale_string(u->faction->locale, "defaultorder"));
              break;
            }
            /* else fall through */

          case K_ROUTE:
          case K_WORK:
          case K_DRIVE:
          case K_MOVE:
          case K_WEREWOLF:
            set_string(&u->thisorder, cmd);
            break;

            /* Wird je diese Ausschliesslichkeit aufgehoben, muss man aufpassen
            * mit der Reihenfolge von Kaufen, Verkaufen etc., damit es Spielern
            * nicht moeglich ist, Schulden zu machen. */
        }
      }

      /* thisorder kopieren wir nun nach lastorder. in lastorder steht
      * der DEFAULT befehl der einheit. da MOVE kein default werden
      * darf, wird MOVE nicht in lastorder kopiert. MACHE TEMP wurde ja
      * schon gar nicht erst in thisorder kopiert, so dass MACHE TEMP
      * durch diesen code auch nicht zum default wird Ebenso soll BIETE
      * nicht hierher, da i.A. die Einheit dann ja weg ist (und damit
      * die Einheitsnummer ungueltig). Auch Attackiere sollte nie in
      * den Default übernommen werden */
      switch (igetkeyword (u->thisorder, u->faction->locale)) {
        case K_MOVE:
        case K_BIETE:
        case K_ATTACK:
        case K_WEREWOLF:
        case NOKEYWORD:
          break;

        default:
          set_string(&u->lastorder, u->thisorder);
      }

      /* Attackiere sollte niemals Default werden */
      if (igetkeyword(u->lastorder, u->faction->locale) == K_ATTACK) {
        set_string(&u->lastorder, locale_string(u->faction->locale, "defaultorder"));
      }
    }
  }
}

static int
use_item(unit * u, const item_type * itype, int amount, const char * cmd)
{
  int i;
  int target = read_unitid(u->faction, u->region);

  i = new_get_pooled(u, itype->rtype, GET_DEFAULT);

  if (amount>i) {
    amount = i;
  }
  if (i==0) {
    cmistake(u, cmd, 43, MSG_PRODUCE);
    return ENOITEM;
  }

  if (target==-1) {
    if (itype->use==NULL) {
      cmistake(u, cmd, 76, MSG_PRODUCE);
      return EUNUSABLE;
    }
    return itype->use(u, itype, amount, cmd);
  } else {
    if (itype->useonother==NULL) {
      cmistake(u, cmd, 76, MSG_PRODUCE);
      return EUNUSABLE;
    }
    return itype->useonother(u, target, itype, amount, cmd);
  }
}


static int
canheal(const unit *u)
{
	switch(old_race(u->race)) {
	case RC_DAEMON:
		return 15;
		break;
	case RC_GOBLIN:
		return 20;
		break;
	case RC_TROLL:
		return 15;
		break;
	case RC_FIREDRAGON:
	case RC_DRAGON:
	case RC_WYRM:
		return 10;
		break;
	}
	if (u->race->flags & RCF_NOHEAL) return 0;
	return 10;
}

static void
monthly_healing(void)
{
  region *r;
  static const curse_type * heal_ct = NULL;
  if (heal_ct==NULL) heal_ct = ct_find("healingzone");

  for (r = regions; r; r = r->next) {
    unit *u;
    int healingcurse = 0;

    if (heal_ct!=NULL) {
      /* bonus zurücksetzen */
      curse * c = get_curse(r->attribs, heal_ct);
      if (c!=NULL) {
        healingcurse = curse_geteffect(c);
      }
    }
    for (u = r->units; u; u = u->next) {
      int umhp = unit_max_hp(u) * u->number;
      int p;

      /* hp über Maximum bauen sich ab. Wird zb durch Elixier der Macht
      * oder verändertes Ausdauertalent verursacht */
      if (u->hp > umhp) {
        u->hp -= (int) ceil((u->hp - umhp) / 2.0);
        if (u->hp < umhp)
          u->hp = umhp;
      }

      if((u->race->flags & RCF_NOHEAL) || fval(u, UFL_HUNGER) || fspecial(u->faction, FS_UNDEAD))
        continue;

      if(rterrain(r) == T_OCEAN && !u->ship && !(canswim(u)))
        continue;

      if(fspecial(u->faction, FS_REGENERATION)) {
        u->hp = umhp;
        continue;
      }

      p = canheal(u);
      if (u->hp < umhp && p > 0) {
        /* Mind 1 HP wird pro Runde geheilt, weil angenommen wird,
        das alle Personen mind. 10 HP haben. */
        int max_unit = max(umhp, u->number * 10);
#ifdef NEW_TAVERN
        struct building * b = inside_building(u);
        const struct building_type * btype = b?b->type:NULL;
        if (btype == bt_find("inn")) {
          max_unit = max_unit * 3 / 2;
        }
#endif
        /* der healing curse verändert den Regenerationsprozentsatz.
        * Wenn dies für negative Heilung benutzt wird, kann es zu
        * negativen u->hp führen! */
        if (healingcurse != 0) {
          p += healingcurse;
        }

        /* Aufaddieren der geheilten HP. */
        u->hp = min(u->hp + max_unit*p/100, umhp);
        if (u->hp < umhp && (rand() % 10 < max_unit % 10)){
          ++u->hp;
        }
        /* soll man an negativer regeneration sterben können? */ 
        if (u->hp <= 0){
          u->hp = 1;
        }
      }
    }
  }
}

static void
defaultorders (void)
{
	region *r;
	unit *u;
	const char * c;
	int i;
	strlist *s;
	list_foreach(region, regions, r) {
		list_foreach(unit, r->units, u) {
			list_foreach(strlist, u->orders, s) {
				switch (igetkeyword(s->s, u->faction->locale)) {
				case K_DEFAULT:
					c = getstrtoken();
					i = atoi(c);
					switch (i) {
					case 0 :
						if (c[0]=='0') set_string(&u->lastorder, getstrtoken());
						else set_string(&u->lastorder, c);
						s->s[0]=0;
						break;
					case 1 :
						sprintf(buf, "%s \"%s\"", locale_string(u->faction->locale, keywords[K_DEFAULT]), getstrtoken());
						set_string(&s->s, buf);
						break;
					default :
						sprintf(buf, "%s %d \"%s\"", locale_string(u->faction->locale, keywords[K_DEFAULT]), i-1, getstrtoken());
						set_string(&s->s, buf);
						break;
					}
				}
			}
			list_next(s);
		}
		list_next(u);
	}
	list_next(r);
}

/* ************************************************************ */
/* GANZ WICHTIG! ALLE GEÄNDERTEN SPRÜCHE NEU ANZEIGEN */
/* GANZ WICHTIG! FÜGT AUCH NEUE ZAUBER IN DIE LISTE DER BEKANNTEN EIN */
/* ************************************************************ */
static void
update_spells(void)
{
	region *r;
	for(r=regions; r; r=r->next) {
		unit *u;
		for(u=r->units;u;u=u->next) {
			sc_mage *m = get_mage(u);
			if (u->faction->no != MONSTER_FACTION && m != NULL) {
				if (m->magietyp == M_GRAU) continue;
				updatespelllist(u);
			}
		}
	}

}

static void
age_factions(void)
{
	faction *f;

	for (f = factions; f; f = f->next) {
		++f->age;
		if (f->age < IMMUN_GEGEN_ANGRIFF) {
			add_message(&f->msgs, new_message(f,
				"newbieimmunity%i:turns", IMMUN_GEGEN_ANGRIFF - f->age));
		}
	}
}

static void
use(void)
{
	region *r;
	unit *u;
	strlist *S;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			for (S = u->orders; S; S = S->next) {
				if (igetkeyword(S->s, u->faction->locale) == K_USE) {
					const char * t = getstrtoken();
					int n = atoi(t);
					const item_type * itype;
					if (n==0) {
						n = 1;
					} else {
						t = getstrtoken();
					}
					itype = finditemtype(t, u->faction->locale);

					if (itype!=NULL) {
					  int i = use_item(u, itype, n, S->s);
					  assert(i<=0 || !"use_item should not return positive values.");
					} else {
					  cmistake(u, S->s, 43, MSG_PRODUCE);
					}
				}
			}
		}
	}
}

void
processorders (void)
{
	region *r;

	if (turn == 0) srand(time((time_t *) NULL));
	else srand(turn);

	puts(" - neue Einheiten erschaffen...");
	new_units();

	puts(" - Monster KI...");
	if (!nomonsters) plan_monsters();
	set_passw();		/* und pruefe auf illegale Befehle */

	puts(" - Defaults und Instant-Befehle...");
	setdefaults();
	instant_orders();
#ifdef ALLIANCES
	alliancekick();
#endif
	mail();
	puts(" - Altern");
	age_factions();

	puts(" - Benutzen");
	use();

	puts(" - Kontaktieren, Betreten von Schiffen und Gebäuden (1.Versuch)");
	do_misc(false);

#ifdef ALLIANCES
	puts(" - Testen der Allianzbedingungen");
	alliancevictory();
#endif

	puts(" - GM Kommandos");
	infocommands();
	gmcommands();

	puts(" - Verlassen");
	do_leave();

	puts(" - Kontakte löschen");
	remove_contacts();

	puts(" - Jihad-Angriffe");
	jihad_attacks();

	puts(" - Attackieren");
	if(nobattle == false) do_battle();
	if (turn == 0) srand(time((time_t *) NULL));
	else srand(turn);

	puts(" - Belagern");
	do_siege();

	puts(" - Initialisieren des Pools, Reservieren");
	init_pool();

	puts(" - Kontaktieren, Betreten von Schiffen und Gebäuden (2.Versuch)");
	do_misc(false);

	puts(" - Folge auf Einheiten ersetzen");
	follow_unit();

	if (turn == 0) srand(time((time_t *) NULL));
	else srand(turn);

	puts(" - Zerstören, Geben, Rekrutieren, Vergessen");
	economics();
	remove_empty_units();

	puts(" - Gebäudeunterhalt (1. Versuch)");
	maintain_buildings(false);

	puts(" - Sterben");
	quit();

	puts(" - Zaubern");
	magic();
	remove_empty_units();

	if (!global.disabled[K_TEACH]) {
		puts(" - Lehren");
		teaching();
	}

	puts(" - Lernen");
	learn();

	puts(" - Produzieren, Geldverdienen, Handeln, Anwerben");
	produce();

	puts(" - Kontaktieren, Betreten von Schiffen und Gebäuden (3.Versuch)");
	do_misc(true);

	puts(" - Schiffe sinken");
 	sinkships();

	puts(" - Bewegungen");
	movement();

	puts(" - Bewache (an)");
	bewache_an();

	puts(" - Zufallsbegegnungen");
	encounters();

	if (turn == 0) srand(time((time_t *) NULL));
	else srand(turn);

	puts(" - Monster fressen und vertreiben Bauern");
	monsters_kill_peasants();

	puts(" - random events");
	randomevents();
	
	puts(" - newspaper commands");
	xecmd();

	puts(" - regeneration (healing & aura)");
	monthly_healing();
	regeneration_magiepunkte();

	puts(" - Defaults setzen");
	defaultorders();

	puts(" - Unterhaltskosten, Nachfrage, Seuchen, Wachstum, Auswanderung");
	demographics();

	puts(" - Gebäudeunterhalt (2. Versuch)");
	maintain_buildings(true);

	puts(" - Jihads setzen");
	set_jihad();

#ifdef USE_UGROUPS
	puts(" - Verbände bilden");
	ugroups();
#endif

	puts(" - Einheiten Sortieren");
	reorder();
#if 0
	puts(" - Einheiten aus Gebäuden/Schiffen weisen");
	evict();
#endif
#ifdef ALLIANCEJOIN
	alliancejoin();
#endif
#ifdef REGIONOWNERS
	puts(" - Krieg & Frieden");
  declare_war();
#endif
	puts(" - Neue Nummern");
	renumber();

	for (r = regions;r;r=r->next) reorder_owners(r);

	puts(" - Attribute altern");
	ageing();

#ifdef WORMHOLE_MODULE
  create_wormholes();
#endif
	/* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
	 * Beschreibungen geändert haben */
	update_spells();
}

int
count_migrants (const faction * f)
{
#ifndef NDEBUG
	unit *u = f->units;
	int n = 0;
	while (u) {
		assert(u->faction == f);
		if (u->race != f->race && u->race != new_race[RC_ILLUSION] && u->race != new_race[RC_SPELL]
			&& !!playerrace(u->race) && !(is_cursed(u->attribs, C_SLAVE, 0)))
		{
			n += u->number;
		}
		u = u->nextF;
	}
	if (f->num_migrants != n)
    log_error(("Anzahl Migranten für (%s) ist falsch: %d statt %d.\n", factionid(f), f->num_migrants, n));
#endif
	return f->num_migrants;
}

int
writepasswd(void)
{
  FILE * F;
  char zText[128];

  sprintf(zText, "%s/passwd", basepath());
  F = cfopen(zText, "w");
  if (F) {
    faction *f;
    puts("Schreibe Passwörter...");

    for (f = factions; f; f = f->next) {
      fprintf(F, "%s:%s:%s:%s:%u\n", 
        factionid(f), f->email, f->passw, f->override, f->subscription);
    }
    fclose(F);
    return 0;
  }
  return 1;
}
