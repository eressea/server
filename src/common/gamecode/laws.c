/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
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
#include <kernel/alchemy.h>
#include <kernel/alliance.h>
#include <kernel/battle.h>
#include <kernel/border.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/magic.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spy.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h> /* for volcanoes in emigration (needs a flag) */
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/synonym.h>
#include <attributes/variable.h>

/* util includes */
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/sql.h>
#include <util/message.h>
#include <util/rng.h>

#include <modules/xecmd.h>

#ifdef AT_OPTION
/* attributes includes */
#include <attributes/option.h>
#endif

/* libc includes */
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>

#include <attributes/otherfaction.h>

/* chance that a peasant dies of starvation: */
#define PEASANT_STARVATION_CHANCE 0.9F
/* Pferdevermehrung */
#define HORSEGROWTH 4
/* Wanderungschance pro Pferd */
#define HORSEMOVE   3
/* Vermehrungschance pro Baum */
#define FORESTGROWTH 10000 /* In Millionstel */

/** Ausbreitung und Vermehrung */
#define MAXDEMAND      25
#define DMRISE         0.1F /* weekly chance that demand goes up */
#define DMRISEHAFEN    0.2F /* weekly chance that demand goes up with harbor */


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
restart_race(unit *u, const race * rc)
{
  faction * oldf = u->faction;
  faction *f = addfaction(oldf->email, oldf->passw, rc, oldf->locale, oldf->subscription);
  unit * nu = addplayer(u->region, f);
  order ** ordp = &u->orders;
  f->subscription = u->faction->subscription;
  f->age = u->faction->age;
  fset(f, FFL_RESTART);
  if (f->subscription) {
    sql_print(("UPDATE subscriptions set faction='%s', race='%s' where id=%u;\n",
               itoa36(f->no), dbrace(rc), f->subscription));
  }
  f->magiegebiet = u->faction->magiegebiet;
  f->options = u->faction->options;
  free_orders(&nu->orders);
  nu->orders = u->orders;
  u->orders = NULL;
  while (*ordp) {
    order * ord = *ordp;
    if (get_keyword(ord) != K_RESTART) {
      *ordp = ord->next;
      ord->next = NULL;
      if (u->thisorder == ord) set_order(&u->thisorder, NULL);
#ifdef LASTORDER
      if (u->lastorder == ord) set_order(&u->lastorder, NULL);
#endif
    } else {
      ordp = &ord->next;
    }
  }
  destroyfaction(u->faction);
}

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
  faction * owner = region_owner(r);

  /* 1. Versorgung von eigenen Einheiten. Das vorhandene Silber
   * wird zunächst so auf die Einheiten aufgeteilt, dass idealerweise
   * jede Einheit genug Silber für ihren Unterhalt hat. */

  for (u = r->units; u; u = u->next) {
    int need = lifestyle(u);

    /* Erstmal zurücksetzen */
    freset(u, UFL_HUNGER);

    /* if the region is owned, and the owner is nice, then we'll get 
     * food from the peasants */
    if (owner!=NULL && (get_alliance(owner, u->faction) & HELP_MONEY)) {
      int rm = rmoney(r);
      int use = min(rm, need);
      rsetmoney(r, rm-use);
      need -= use;
    }

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
      unit * donor = r->units;
      int hungry = u->number;

      while (donor!=NULL && hungry>0) {
        /* always start with the first known unit that may have some blood */
        static const struct potion_type * pt_blood;
        if (pt_blood==NULL) {
          const item_type * it_blood = it_find("peasantblood");
          if (it_blood) pt_blood = it_blood->rtype->ptype;
        }
        if (pt_blood!=NULL) {
          while (donor!=NULL) {
            if (donor->race==new_race[RC_DAEMON]) {
              if (get_effect(donor, pt_blood)) {
                /* if he's in our faction, drain him: */
                if (donor->faction==u->faction) break;
              }
            }
            donor = donor->next;
          }
          if (donor != NULL) {
            int blut = get_effect(donor, pt_blood);
            blut = min(blut, hungry);
            change_effect(donor, pt_blood, -blut);
            hungry -= blut;
          }
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
          /* nicht gefütterte dämonen hungern */
#ifdef PEASANT_HUNGRY_DAEMONS_HAVE_FULL_SKILLS
          /* wdw special rule */
          hunger(hungry, u);
#else
          if (hunger(hungry, u)) fset(u, UFL_HUNGER);
#endif
          /* used to be: hunger(hungry, u); */
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
age_unit(region * r, unit * u)
{
  if (u->race == new_race[RC_SPELL]) {
    if (--u->age <= 0) {
      destroy_unit(u);
    }
  } else {
    ++u->age;
    if (u->number>0 && u->race->age) {
      u->race->age(u);
    }
  }

#ifdef ASTRAL_ITEM_RESTRICTIONS
  if (u->region->planep==get_astralplane()) {
    item ** itemp = &u->items;
    while (*itemp) {
      item * itm = *itemp;
      if ((itm->type->flags & ITF_NOTLOST) == 0) {
        if (itm->type->flags & (ITF_BIG|ITF_ANIMAL|ITF_CURSED)) {
          ADDMSG(&u->faction->msgs, msg_message("itemcrumble", "unit region item amount",
            u, u->region, itm->type->rtype, itm->number));
          i_free(i_remove(itemp, itm));
          continue;
        }
      }
      itemp=&itm->next;
    }
  }
#endif
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
    if (effect > 0) { /* Trank "Dumpfbackenbrot" */
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
      } /* sonst Glück gehabt: wer nix weiß, kann nix vergessen... */
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

    if (rc == NULL || !fval(rc->terrain, LAND_REGION)) {
      w = 0;
    } else {
      w = rpeasants(rc) - maxworkingpeasants(rc);
      w = max(0,w);
      if (rterrain(rc) == T_VOLCANO || rterrain(rc) == T_VOLCANO_SMOKING) {
        w = w/10;
      }
    }
    weight[i]  = w;
    weightall += w;
  }

  if (weightall !=0 ) for (i = 0; i != MAXDIRECTIONS; i++) {
    region *rc = rconnect(r, i);
    if (rc != NULL) {
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
    region * rc = rconnect(r, i);
    if (!rc) continue;
    if (fval(r, RF_ORCIFIED)==fval(rc, RF_ORCIFIED)) {
      if (fval(rc->terrain, LAND_REGION) && fval(r->terrain, LAND_REGION)) {
        int wandering_peasants;
        double vfactor;

        /* First let's calculate the peasants who wander off to less inhabited
         * regions. wfactor indicates the difference of population denity.
         * Never let more than PEASANTSWANDER_WEIGHT per cent wander off in one
         * direction. */
        wfactor = ((double) rpeasants(r) / maxpeasants_here -
          ((double) rpeasants(rc) / maxpeasants[i]));
        wfactor = max(wfactor, 0);
        wfactor = min(wfactor, 1);

        /* Now let's handle the greedy peasants. gfactor indicates the
         * difference of per-head-wealth. Never let more than
         * PEASANTSGREED_WEIGHT per cent wander off in one direction. */
        gfactor = (((double) rmoney(rc) / max(rpeasants(rc), 1)) -
               ((double) rmoney(r) / max(rpeasants(r), 1))) / 500;
        gfactor = max(gfactor, 0);
        gfactor = min(gfactor, 1);

        /* This calculates the influence of volcanos on peasant
         * migration. */

        if(rterrain(rc) == T_VOLCANO || rterrain(rc) == T_VOLCANO_SMOKING) {
            vfactor = 0.10;
        } else {
            vfactor = 1.00;
        }

        for(j=0; j != MAXDIRECTIONS; j++) {
          region *rv = rconnect(rc, j);
          if(rv && (rterrain(rv) == T_VOLCANO || rterrain(rv) == T_VOLCANO_SMOKING)) {
            vfactor *= 0.5;
            break;
          }
        }

        wandering_peasants = (int) (rpeasants(r) * (gfactor+wfactor)
            * vfactor * PEASANTSWANDER_WEIGHT / 100.0);

        r->land->newpeasants -= wandering_peasants;
        rc->land->newpeasants += wandering_peasants;
      }
    }
  }
}
#endif

/** Bauern vermehren sich */

static void
peasants(region * r)
{
  int peasants = rpeasants(r);
  int money = rmoney(r);
  int maxp = production(r) * MAXPEASANTS_PER_AREA;
  int n, satiated;
  int dead = 0;

  /* Bis zu 1000 Bauern können Zwillinge bekommen oder 1000 Bauern
   * wollen nicht! */

  if (peasants>0) {
    int glueck = 0;
    double fraction = peasants * 0.0001F * PEASANTGROWTH;
    int births = (int)fraction;
    attrib * a = a_find(r->attribs, &at_peasantluck);

    if (rng_double()<(fraction-births)) {
      /* because we don't want regions that never grow pga. rounding. */
      ++births;
    }
    if (a!=NULL) {
      glueck = a->data.i * 1000;
    }

    for (n = peasants; n; --n) {
      int chances = 0;

      if (glueck>0) {
        --glueck;
        chances += PEASANTLUCK;
      }

      while (chances--) {
        if (rng_int() % 10000 < PEASANTGROWTH) {
          /* Only raise with 75% chance if peasants have
           * reached 90% of maxpopulation */
          if (peasants/(float)maxp < 0.9 || chance(PEASANTFORCE)) {
            ++births;
          }
        }
      }
    }
    peasants += births;
  }

  /* Alle werden satt, oder halt soviele für die es auch Geld gibt */

  satiated = min(peasants, money / maintenance_cost(NULL));
  rsetmoney(r, money - satiated * maintenance_cost(NULL));

  /* Von denjenigen, die nicht satt geworden sind, verhungert der
   * Großteil. dead kann nie größer als rpeasants(r) - satiated werden,
   * so dass rpeasants(r) >= 0 bleiben muß. */

  /* Es verhungert maximal die unterernährten Bevölkerung. */

  n = min(peasants - satiated, rpeasants(r));
    dead += (int)(n * PEASANT_STARVATION_CHANCE);

  if (dead > 0) {
    message * msg = add_message(&r->msgs, msg_message("phunger", "dead", dead));
    msg_release(msg);
    peasants -= dead;
  }

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
  int key = reg_hashkey(r);
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
  int key = reg_hashkey(r);
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
  if (horses > 0) {
    if (is_cursed(r->attribs, C_CURSED_BY_THE_GODS, 0)) {
      rsethorses(r, (int)(horses*0.9F));
    } else if (maxhorses) {
      int i;
      double growth = (RESOURCE_QUANTITY * HORSEGROWTH * 200 * (maxhorses-horses))/maxhorses;

      if (growth>0) {
        if (a_find(r->attribs, &at_horseluck)) growth *= 2;
        /* printf("Horses: <%d> %d -> ", growth, horses); */
        i = (int)(0.5F + (horses * 0.0001F) * growth);
        /* printf("%d\n", horses); */
        rsethorses(r, horses + i);
      }
    }
  }

  /* Pferde wandern in Nachbarregionen.
   * Falls die Nachbarregion noch berechnet
   * werden muß, wird eine migration-Struktur gebildet,
   * die dann erst in die Berechnung der Nachbarstruktur einfließt.
   */

  for(n = 0; n != MAXDIRECTIONS; n++) {
    region * r2 = rconnect(r, n);
    if (r2 && fval(r2->terrain, WALK_INTO)) {
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

    if (production(r) <= 0) return;

    /* Grundchance 1.0% */
    seedchance = (int)(FORESTGROWTH * RESOURCE_QUANTITY);
    /* Jeder Elf in der Region erhöht die Chance um 0.0008%. */
    seedchance += (min(elves, (production(r)*MAXPEASANTS_PER_AREA)/8)) * 8;
    grownup_trees = rtrees(r, 2);
    seeds = 0;

    if (grownup_trees>0) {
      seeds += (int)(seedchance*0.000001F*grownup_trees);
    }
    rsettrees(r, 0, rtrees(r, 0) + seeds);

    /* Bäume breiten sich in Nachbarregionen aus. */

    /* Gesamtzahl der Samen:
     * bis zu 6% (FORESTGROWTH*3) der Bäume samen in die Nachbarregionen */
    seeds = (rtrees(r, 2) * FORESTGROWTH * 3)/1000000;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      region * r2 = rconnect(r, d);
      if (r2 && fval(r2->terrain, LAND_REGION) && r2->terrain->size) {
        /* Eine Landregion, wir versuchen Samen zu verteilen:
         * Die Chance, das Samen ein Stück Boden finden, in dem sie
         * keimen können, hängt von der Bewuchsdichte und der
         * verfügbaren Fläche ab. In Gletschern gibt es weniger
         * Möglichkeiten als in Ebenen. */
        sprout = 0;
        seedchance = (1000 * maxworkingpeasants(r2)) / r2->terrain->size;
        for(i=0; i<seeds/MAXDIRECTIONS; i++) {
          if(rng_int()%10000 < seedchance) sprout++;
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
      if(rng_int()%10000 < growth) sprout++;
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
      if(rng_int()%10000 < growth) grownup_trees++;
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
      if (rng_int()%100 < (100-rherbs(r))) rsetherbs(r, (short)(rherbs(r)+1));
    }
  }
}

void
demographics(void)
{
  region *r;
  static int last_weeks_season = -1;
  static int current_season = -1;

  if (current_season<0) {
    current_season = get_gamedate(turn, NULL)->season;
    last_weeks_season = get_gamedate(turn-1, NULL)->season;
  }

  for (r = regions; r; r = r->next) {
    ++r->age; /* also oceans. no idea why we didn't always do that */
    live(r);
    /* check_split_dragons(); */

    if (!fval(r->terrain, SEA_REGION)) {
      /* die Nachfrage nach Produkten steigt. */
      struct demand * dmd;
      if (r->land) for (dmd=r->land->demands;dmd;dmd=dmd->next) {
        if (dmd->value>0 && dmd->value < MAXDEMAND) {
          float rise = DMRISE;
          if (buildingtype_exists(r, bt_find("harbour"))) rise = DMRISEHAFEN;
          if (rng_double()<rise) ++dmd->value;
        }
      }
      /* Seuchen erst nachdem die Bauern sich vermehrt haben
       * und gewandert sind */

      calculate_emigration(r);
      peasants(r);
      plagues(r, false);

      horses(r);
      if(current_season != SEASON_WINTER) {
        trees(r, current_season, last_weeks_season);
      }
      update_resources(r);
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
    if (fval(r->terrain, LAND_REGION)) {
      int rp = rpeasants(r) + r->land->newpeasants;
      rsetpeasants(r, max(0, rp));
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
    return (c + rng_int() % c);
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

static int
restart_cmd(unit * u, struct order * ord)
{
  init_tokens(ord);
  skip_token(); /* skip keyword */

  if (!fval(u->region->terrain, LAND_REGION)) {
    cmistake(u, ord, 242, MSG_EVENT);
  } else {
    const char * s_race = getstrtoken(), * s_pass;
    const race * frace = findrace(s_race, u->faction->locale);

    if (!frace) {
      frace = u->faction->race;
      s_pass = s_race;
    } else {
      s_pass = getstrtoken();
    }

    if (u->faction->age > 3 && fval(u->faction, FFL_RESTART)) {
      cmistake(u, ord, 314, MSG_EVENT);
      return 0;
    }

    if (/* frace != u->faction->race && */ u->faction->age < 81) {
      cmistake(u, ord, 241, MSG_EVENT);
      return 0;
    }

    if (!playerrace(frace)) {
      cmistake(u, ord, 243, MSG_EVENT);
      return 0;
    }

    if (!checkpasswd(u->faction, s_pass, false)) {
      cmistake(u, ord, 86, MSG_EVENT);
      log_warning(("NEUSTART mit falschem Passwort für Partei %s: %s\n",
        factionid(u->faction), s_pass));
      return 0;
    }
    restart_race(u, frace);
    return -1;
  }
  return 0;
}

static boolean
EnhancedQuit(void)
{
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "alliance.transferquit");
    value = (str!=0 && strcmp(str, "true")==0);
  }
  return value;
}

static int
quit_cmd(unit * u, struct order * ord)
{
  faction * f = u->faction;
  const char * passwd;

  init_tokens(ord);
  skip_token(); /* skip keyword */

  passwd = getstrtoken();
  if (checkpasswd(f, passwd, false)) {
    if (EnhancedQuit()) {
      const char * token = getstrtoken();
      int f2_id = atoi36(token);
      if (f2_id>0) {
        faction *f2 = findfaction(f2_id);

        if(f2 == NULL) {
          cmistake(u, ord, 66, MSG_EVENT);
          return 0;
        } else if (!u->faction->alliance || u->faction->alliance != f2->alliance) {
          cmistake(u, ord, 315, MSG_EVENT);
          return 0;
        } else if(!alliedfaction(NULL, f, f2, HELP_MONEY)) {
          cmistake(u, ord, 316, MSG_EVENT);
          return 0;
        } else {
          set_variable(&f->attribs, "quit", token);
        }
      }
    }
    fset(f, FFL_QUIT);
  } else {
    cmistake(u, ord, 86, MSG_EVENT);
    log_warning(("STIRB mit falschem Passwort für Partei %s: %s\n",
      factionid(f), passwd));
  }
  return 0;
}

static void
quit(void)
{
  faction ** fptr = &factions;
  while (*fptr) {
    faction * f = *fptr;
    if (f->flags & FFL_QUIT) {
      if (EnhancedQuit()) {
        const char * token = get_variable(f->attribs, "quit");
        if(token != NULL) {
          int f2_id = atoi36(token);
          faction *f2 = findfaction(f2_id);

          assert(f2_id>0);
          assert(f2!=NULL);
          transfer_faction(f, f2);
        }
      }
      destroyfaction(f);
    }
    if (*fptr==f) fptr=&f->next;
  }
}

static void
parse_restart(void)
{
  region *r;
  faction *f;

  /* Sterben erst nachdem man allen anderen gegeben hat - bzw. man kann
  * alles machen, was nicht ein dreißigtägiger Befehl ist. */

  for (r = regions; r; r = r->next) {
    unit * u, * un;
    for (u = r->units; u;) {
      order * ord;

      un = u->next;
      for (ord = u->orders; ord!=NULL; ord = ord->next) {
        if (get_keyword(ord) == K_RESTART) {
          if (u->number > 0) {
            if (restart_cmd(u, ord)!=0) {
              break;
            }
          }
        }
      }
      u = un;
    }
  }

  puts(" - beseitige Spieler, die sich zu lange nicht mehr gemeldet haben...");

  for (f = factions; f; f = f->next) {
    if(fval(f, FFL_NOIDLEOUT)) f->lastorders = turn;
    if (NMRTimeout()>0 && turn - f->lastorders >= NMRTimeout()) {
      destroyfaction(f);
      continue;
    }
    if (fval(f, FFL_OVERRIDE)) {
      free(f->override);
      f->override = strdup(itoa36(rng_int()));
      freset(f, FFL_OVERRIDE);
    }
    if (turn!=f->lastorders) {
      char info[256];
      sprintf(info, "%d Einheiten, %d Personen, %d Silber",
        f->no_units, f->num_total, f->money);
      if (f->subscription) {
        sql_print(("UPDATE subscriptions SET lastturn=%d, password='%s', info='%s' WHERE id=%u;\n",
                   f->lastorders, f->override, info, f->subscription));
      }
    } else {
      if (f->subscription) {
        sql_print(("UPDATE subscriptions SET status='ACTIVE', lastturn=%d, firstturn=greatest(firstturn,%d), password='%s' WHERE id=%u;\n",
                   f->lastorders, f->lastorders-f->age,
                   f->override, f->subscription));
      }
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
#ifndef ALLIANCEJOIN
    if (f->alliance==NULL) {
      /* destroyfaction(f); */
      continue;
    }
#endif
  }
  /* Clear away debris of destroyed factions */

  puts(" - beseitige leere Einheiten und leere Parteien...");
  remove_empty_units();
}
/* ------------------------------------------------------------- */

/* HELFE partei [<ALLES | SILBER | GIB | KAEMPFE | WAHRNEHMUNG>] [NICHT] */

static int
ally_cmd(unit * u, struct order * ord)
{
  ally * sf, ** sfp;
  faction *f;
  int keyword, not_kw;
  const char *s;

  init_tokens(ord);
  skip_token();
  f = getfaction();

  if (f == 0 || f->no == 0) {
    cmistake(u, ord, 66, MSG_EVENT);
    return 0;
  }
  if (f == u->faction) return 0;

  s = getstrtoken();

  if (!s[0])
    keyword = P_ANY;
  else
    keyword = findparam(s, u->faction->locale);

  sfp = &u->faction->allies;
  if (fval(u, UFL_GROUP)) {
    attrib * a = a_find(u->attribs, &at_group);
    if (a) sfp = &((group*)a->data.v)->allies;
  }
  for (sf=*sfp; sf; sf = sf->next)
    if (sf->faction == f)
      break;  /* Gleich die passende raussuchen, wenn vorhanden */

  not_kw = getparam(u->faction->locale);    /* HELFE partei [modus] NICHT */

  if (!sf) {
    if (keyword == P_NOT || not_kw == P_NOT) {
      /* Wir helfen der Partei gar nicht... */
      return 0;
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
    cmistake(u, ord, 137, MSG_EVENT);
    return 0;

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

  if (sf->status == 0) {    /* Alle HELPs geloescht */
    removelist(sfp, sf);
  }
  return 0;
}

static int
prefix_cmd(unit * u, struct order * ord)
{
  attrib **ap;
  int i;
  const char *s;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (!*s) {
    attrib *a = NULL;
    if (fval(u, UFL_GROUP)) {
      a = a_find(u->attribs, &at_group);
    }
    if (a) {
      group * g = (group*)a->data.v;
      a_removeall(&g->attribs, &at_raceprefix);
    } else {
      a_removeall(&u->faction->attribs, &at_raceprefix);
    }
    return 0;
  }

  for(i=0; race_prefixes[i] != NULL; i++) {
    const char * tag = mkname("prefix", race_prefixes[i]);
    if (strncasecmp(s, LOC(u->faction->locale, tag), strlen(s)) == 0) {
      break;
    }
  }

  if (race_prefixes[i] == NULL) {
    cmistake(u, ord, 299, MSG_EVENT);
    return 0;
  }

  ap = &u->faction->attribs;
  if (fval(u, UFL_GROUP)) {
    attrib * a = a_find(u->attribs, &at_group);
    group * g = (group*)a->data.v;
    if (a) ap = &g->attribs;
  }
  set_prefix(ap, race_prefixes[i]);

  return 0;
}

static int
synonym_cmd(unit * u, struct order * ord)
{
  int i;
  const char *s;

  attrib * a = a_find(u->faction->attribs, &at_synonym);
  if (a!=NULL) {  /* Kann nur einmal gesetzt werden */
    cmistake(u, ord, 302, MSG_EVENT);
    return 0;
  }

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (s==NULL) {
    cmistake(u, ord, 301, MSG_EVENT);
    return 0;
  }

  for (i=0; race_synonyms[i].race != -1; i++) {
    if (new_race[race_synonyms[i].race] == u->faction->race
      && strcasecmp(s, race_synonyms[i].synonyms[0]) == 0) {
        break;
      }
  }

  if (race_synonyms[i].race == -1) {
    cmistake(u, ord, 300, MSG_EVENT);
    return 0;
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

  return 0;
}

static int
display_cmd(unit * u, struct order * ord)
{
  building * b = u->building;
  char **s = NULL;
  region * r = u->region;

  init_tokens(ord);
  skip_token();

  switch (getparam(u->faction->locale)) {
  case P_BUILDING:
  case P_GEBAEUDE:
    if (!b) {
      cmistake(u, ord, 145, MSG_PRODUCE);
      break;
    }
    if (!fval(u, UFL_OWNER)) {
      cmistake(u, ord, 5, MSG_PRODUCE);
      break;
    }
    if (b->type == bt_find("generic")) {
      cmistake(u, ord, 279, MSG_PRODUCE);
      break;
    }
    if (b->type == bt_find("monument") && b->display && b->display[0] != 0) {
      cmistake(u, ord, 29, MSG_PRODUCE);
      break;
    }
    if (b->type == bt_find("artsculpture") && b->display && b->display[0] != 0) {
      cmistake(u, ord, 29, MSG_PRODUCE);
      break;
    }
    s = &b->display;
    break;

  case P_SHIP:
    if (!u->ship) {
      cmistake(u, ord, 144, MSG_PRODUCE);
      break;
    }
    if (!fval(u, UFL_OWNER)) {
      cmistake(u, ord, 12, MSG_PRODUCE);
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
    if (!b) {
      cmistake(u, ord, 145, MSG_EVENT);
      break;
    }
    if (!fval(u, UFL_OWNER)) {
      cmistake(u, ord, 148, MSG_EVENT);
      break;
    }
    if (b != largestbuilding(r,false)) {
      cmistake(u, ord, 147, MSG_EVENT);
      break;
    }
    s = &r->display;
    break;

  default:
    cmistake(u, ord, 110, MSG_EVENT);
    break;
  }

  if (s!=NULL) {
    const char * s2 = getstrtoken();

    if (strlen(s2)>=DISPLAYSIZE) {
      char * s3 = strdup(s2);
      s3[DISPLAYSIZE] = 0;
      set_string(s, s3);
      free(s3);
    } else
    set_string(s, s2);
  }

  return 0;
}

static boolean
renamed_building(const building * b)
{
  const struct locale * lang = locales;
  for (;lang;lang=nextlocale(lang)) {
    const char * bdname = LOC(lang, b->type->_name);
    size_t bdlen = strlen(bdname);
    if (strlen(b->name)>=bdlen && strncmp(b->name, bdname, bdlen)==0) {
      return false;
    }
  }
  return true;
}

static int
name_cmd(unit * u, struct order * ord)
{
  building * b = u->building;
  region * r = u->region;
  char **s = NULL;
  param_t p;
  boolean foreign = false;

  init_tokens(ord);
  skip_token();
  p = getparam(u->faction->locale);

  if (p == P_FOREIGN) {
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
        cmistake(u, ord, 6, MSG_EVENT);
        break;
      }

      if (b->type == bt_find("generic")) {
        cmistake(u, ord, 278, MSG_EVENT);
        break;
      } else {
        if (renamed_building(b)) {
          cmistake(u, ord, 246, MSG_EVENT);
          break;
        }
      }

      uo = buildingowner(r, b);
      if (uo) {
        if (cansee(uo->faction, r, u, 0)) {
          ADDMSG(&uo->faction->msgs, msg_message("renamed_building_seen", 
            "building renamer region", b, u, r));
        } else {
          ADDMSG(&uo->faction->msgs, msg_message("renamed_building_notseen", 
            "building region", b, r));
        }
      }
      s = &b->name;
    } else {
      if (!b) {
        cmistake(u, ord, 145, MSG_PRODUCE);
        break;
      }
      if (!fval(u, UFL_OWNER)) {
        cmistake(u, ord, 148, MSG_PRODUCE);
        break;
      }
      if (b->type == bt_find("generic")) {
        cmistake(u, ord, 278, MSG_EVENT);
        break;
      }
      if (b->type == bt_find("monument")) {
        if (renamed_building(b)) {
          cmistake(u, ord, 29, MSG_EVENT);
          break;
        }
      }
      if (b->type == bt_find("artsculpure")) {
        cmistake(u, ord, 29, MSG_EVENT);
        break;
      }
      s = &b->name;
    }
    break;

  case P_FACTION:
    if (foreign == true) {
      faction *f;

      f = getfaction();
      if (!f) {
        cmistake(u, ord, 66, MSG_EVENT);
        break;
      }
      if (f->age < 10) {
        cmistake(u, ord, 248, MSG_EVENT);
        break;
      } else {
        const struct locale * lang = locales;
        for (;lang;lang=nextlocale(lang)) {
          const char * fdname = LOC(lang, "factiondefault");
          size_t fdlen = strlen(fdname);
          if (strlen(f->name)>=fdlen && strncmp(f->name, fdname, fdlen)==0) {
            break;
          }
        }
        if (lang==NULL) {
          cmistake(u, ord, 247, MSG_EVENT);
          break;
        }
      }
      if (cansee(f, r, u, 0)) {
        ADDMSG(&f->msgs, msg_message("renamed_faction_seen", "unit region", u, r));
      } else {
        ADDMSG(&f->msgs, msg_message("renamed_faction_notseen", "", r));
      }
      s = &f->name;
    } else {
      s = &u->faction->name;
    }
    break;

  case P_SHIP:
    if (foreign == true) {
      ship *sh = getship(r);
      unit *uo;

      if (!sh) {
        cmistake(u, ord, 20, MSG_EVENT);
        break;
      } else {
        const struct locale * lang = locales;
        for (;lang;lang=nextlocale(lang)) {
          const char * sdname = LOC(lang, sh->type->name[0]);
          size_t sdlen = strlen(sdname);
          if (strlen(sh->name)>=sdlen && strncmp(sh->name, sdname, sdlen)==0) {
            break;
          }

          sdname = LOC(lang, parameters[P_SHIP]);
          sdlen = strlen(sdname);
          if (strlen(sh->name)>=sdlen && strncmp(sh->name, sdname, sdlen)==0) {
            break;
          }

        }
        if (lang==NULL) {
          cmistake(u, ord, 245, MSG_EVENT);
          break;
        }
      }
      uo = shipowner(sh);
      if (uo) {
        if (cansee(uo->faction, r, u, 0)) {
          ADDMSG(&uo->faction->msgs, msg_message("renamed_ship_seen", 
            "ship renamer region", sh, u, r));
        } else {
          ADDMSG(&uo->faction->msgs, msg_message("renamed_ship_notseen", 
            "ship region", sh, r));
        }
      }
      s = &sh->name;
    } else {
      if (!u->ship) {
        cmistake(u, ord, 144, MSG_PRODUCE);
        break;
      }
      if (!fval(u, UFL_OWNER)) {
        cmistake(u, ord, 12, MSG_PRODUCE);
        break;
      }
      s = &u->ship->name;
    }
    break;

  case P_UNIT:
    if (foreign == true) {
      unit *u2 = getunit(r, u->faction);

      if (!u2 || !cansee(u->faction, r, u2, 0)) {
        cmistake(u, ord, 63, MSG_EVENT);
        break;
      } else {
        const char * udefault = LOC(u2->faction->locale, "unitdefault");
        size_t udlen = strlen(udefault);
        size_t unlen = strlen(u2->name);
        if (unlen>=udlen && strncmp(u2->name, udefault, udlen)!=0) {
          cmistake(u2, ord, 244, MSG_EVENT);
          break;
        }
      }
      if (cansee(u2->faction, r, u, 0)) {
        ADDMSG(&u2->faction->msgs, msg_message("renamed_seen", 
          "renamer renamed region", u, u2, r));
      } else {
        ADDMSG(&u2->faction->msgs, msg_message("renamed_notseen", 
          "renamed region", u2, r));
      }
      s = &u2->name;
    } else {
      s = &u->name;
    }
    break;

  case P_REGION:
    if (!b) {
      cmistake(u, ord, 145, MSG_EVENT);
      break;
    }
    if (!fval(u, UFL_OWNER)) {
      cmistake(u, ord, 148, MSG_EVENT);
      break;
    }
    if (b != largestbuilding(r,false)) {
      cmistake(u, ord, 147, MSG_EVENT);
      break;
    }
    s = &r->land->name;
    break;

  case P_GROUP:
    {
      attrib * a = NULL;
      if (fval(u, UFL_GROUP)) a = a_find(u->attribs, &at_group);
      if (a) {
        group * g = (group*)a->data.v;
        s = &g->name;
        break;
      } else {
        cmistake(u, ord, 109, MSG_EVENT);
        break;
      }
    }
    break;
  default:
    cmistake(u, ord, 109, MSG_EVENT);
    break;
  }

  if (s!=NULL) {
    const char * s2 = getstrtoken();

    if (!s2[0]) {
      cmistake(u, ord, 84, MSG_EVENT);
      return 0;
    }

    if (strchr(s2, '(')!=NULL) {
      cmistake(u, ord, 112, MSG_EVENT);
      return 0;
    }

    if (strlen(s2)>=NAMESIZE) {
      char * s3 = strdup(s2);
      s3[NAMESIZE] = 0;
      set_string(s, s3);
      free(s3);
    } else
      set_string(s, s2);

  }

  return 0;
}
/* ------------------------------------------------------------- */

void
deliverMail(faction * f, region * r, unit * u, const char *s, unit * receiver)
{
  if (!cansee(f, r, u, 0)) {
    u = NULL;
  }
  if (!receiver) { /* BOTSCHAFT an PARTEI */
    ADDMSG(&f->msgs, msg_message("regionmessage", "region sender string", r, u, s));
  } else {          /* BOTSCHAFT an EINHEIT */
    ADDMSG(&f->msgs, msg_message("unitmessage", "region unit sender string", r, receiver, u, s));
  }
}

static void
mailunit(region * r, unit * u, int n, struct order * ord, const char * s)
{
  unit * u2 = findunitr(r,n);

  if (u2 && cansee(u->faction, r, u2, 0)) {
    deliverMail(u2->faction, r, u, s, u2);
    /* now done in prepare_mail_cmd */
  }
  else {
    /* Immer eine Meldung - sonst könnte man so getarnte EHs enttarnen:
    * keine Meldung -> EH hier. */
    cmistake(u, ord, 63, MSG_MESSAGE);
  }
}

static void
mailfaction(unit * u, int n, struct order * ord, const char * s)
{
  faction *f;

  f = findfaction(n);
  if (f && n>0)
    deliverMail(f, u->region, u, s, NULL);
  else
    cmistake(u, ord, 66, MSG_MESSAGE);
}

static int
mail_cmd(unit * u, struct order * ord)
{
  region * r = u->region;
  unit *u2;
  const char *s;
  int n;

  init_tokens(ord);
  skip_token(); /* skip the keyword */
  s = getstrtoken();

  /* Falls kein Parameter, ist das eine Einheitsnummer;
  * das Füllwort "AN" muß wegfallen, da gültige Nummer! */

  if (strcasecmp(s, "to") == 0) s = getstrtoken();
  else if (strcasecmp(s, "an") == 0) s = getstrtoken();

  switch (findparam(s, u->faction->locale)) {
  case P_REGION:
    /* können alle Einheiten in der Region sehen */
    s = getstrtoken();
    if (!s[0]) {
      cmistake(u, ord, 30, MSG_MESSAGE);
      break;
    } else {
      sprintf(buf, "von %s: '%s'", unitname(u), s);
      addmessage(r, 0, buf, MSG_MESSAGE, ML_IMPORTANT);
      break;
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
        cmistake(u, ord, 66, MSG_MESSAGE);
        break;
      }

      s = getstrtoken();
      if (!s[0]) {
        cmistake(u, ord, 30, MSG_MESSAGE);
        break;
      }
      mailfaction(u, n, ord, s);
    }
    break;

  case P_UNIT:
    {
      boolean see = false;
      n = getid();

      for (u2=r->units; u2; u2=u2->next) {
        if (u2->no == n && cansee(u->faction, r, u2, 0)) {
          see = true;
          break;
        }
      }

      if (see == false) {
        cmistake(u, ord, 63, MSG_MESSAGE);
        break;
      }

      s = getstrtoken();
      if (!s[0]) {
        cmistake(u, ord, 30, MSG_MESSAGE);
        break;
      } else {
        attrib * a = a_find(u2->attribs, &at_eventhandler);
        if (a!=NULL) {
          event_arg args[3];
          args[0].data.v = (void*)s;
          args[0].type = "string";
          args[1].data.v = (void*)u;
          args[1].type = "unit";
          args[2].type = NULL;
          handle_event(a, "message", args);
        }

        mailunit(r, u, n, ord, s);
      }
    }
    break;

  case P_BUILDING:
  case P_GEBAEUDE:
    {
      building *b = getbuilding(r);

      if(!b) {
        cmistake(u, ord, 6, MSG_MESSAGE);
        break;
      }

      s = getstrtoken();

      if (!s[0]) {
        cmistake(u, ord, 30, MSG_MESSAGE);
        break;
      }

      for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);

      for(u2=r->units; u2; u2=u2->next) {
        if(u2->building == b && !fval(u2->faction, FL_DH)
          && cansee(u->faction, r, u2, 0)) {
            mailunit(r, u, u2->no, ord, s);
            fset(u2->faction, FL_DH);
          }
      }
    }
    break;

  case P_SHIP:
    {
      ship *sh = getship(r);

      if(!sh) {
        cmistake(u, ord, 20, MSG_MESSAGE);
        break;
      }

      s = getstrtoken();

      if (!s[0]) {
        cmistake(u, ord, 30, MSG_MESSAGE);
        break;
      }

      for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);

      for(u2=r->units; u2; u2=u2->next) {
        if(u2->ship == sh && !fval(u2->faction, FL_DH)
          && cansee(u->faction, r, u2, 0)) {
            mailunit(r, u, u2->no, ord, s);
            fset(u2->faction, FL_DH);
          }
      }
    }
    break;

  default:
    cmistake(u, ord, 149, MSG_MESSAGE);
    break;
  }
  return 0;
}
/* ------------------------------------------------------------- */

static int
banner_cmd(unit * u, struct order * ord)
{
  init_tokens(ord);
  skip_token();

  set_string(&u->faction->banner, getstrtoken());
  add_message(&u->faction->msgs, msg_message("changebanner", "value",
    u->faction->banner));

  return 0;
}

static int
email_cmd(unit * u, struct order * ord)
{
  const char * s;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (!s[0]) {
    cmistake(u, ord, 85, MSG_EVENT);
  } else {
    faction * f = u->faction;
    if (set_email(&f->email, s)!=0) {
      log_error(("Invalid email address for faction %s: %s\n", itoa36(f->no), s));
      ADDMSG(&f->msgs, msg_message("changemail_invalid", "value", s));
    } else {
      ADDMSG(&f->msgs, msg_message("changemail", "value", f->email));
    }
  }
  return 0;
}

static int
password_cmd(unit * u, struct order * ord)
{
  char pbuf[32];
  int i;
  const char * s;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (!s || !*s) {
    for(i=0; i<6; i++) pbuf[i] = (char)(97 + rng_int() % 26);
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
      cmistake(u, ord, 283, MSG_EVENT);
      for(i=0; i<6; i++) pbuf[i] = (char)(97 + rng_int() % 26);
      pbuf[6] = 0;
    }
  }
  set_string(&u->faction->passw, pbuf);
  fset(u->faction, FFL_OVERRIDE);
  ADDMSG(&u->faction->msgs, msg_message("changepasswd",
    "value", u->faction->passw));
  return 0;
}

static int
send_cmd(unit * u, struct order * ord)
{
  const char * s;
  int option;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  option = findoption(s, u->faction->locale);
#ifdef AT_OPTION
  /* Sonderbehandlung Zeitungsoption */
  if (option == O_NEWS) {
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
    return 0;
  }
#endif
  if (option == -1) {
    cmistake(u, ord, 135, MSG_EVENT);
  } else {
    if (getparam(u->faction->locale) == P_NOT) {
      if (option == O_COMPRESS || option == O_BZIP2) {
        cmistake(u, ord, 305, MSG_EVENT);
      } else {
        u->faction->options = u->faction->options & ~((int)pow(2, option));
      }
    } else {
      u->faction->options = u->faction->options | ((int)pow(2, option));
      if(option == O_COMPRESS) u->faction->options &= ~((int)pow(2, O_BZIP2));
      if(option == O_BZIP2) u->faction->options &= ~((int)pow(2, O_COMPRESS));
    }
  }
  return 0;
}

static boolean
display_item(faction *f, unit *u, const item_type * itype)
{
  const char *name;
  const char *info;
  const char *key;

  if (u!=NULL) {
    int i = i_get(u->items, itype);
    if (i==0) {
      if (u->region->land!=NULL) {
        i = i_get(u->region->land->items, itype);
      }
      if (i==0) {
        i = i_get(u->faction->items, itype);
        if (i==0) return false;
      }
    }
  }

  name = resourcename(itype->rtype, 0);
  key = mkname("iteminfo", name);
  info = locale_string(f->locale, key);

  if (info==key || strcmp(info, key)==0) {
    info = locale_string(f->locale, mkname("iteminfo", "no_info"));
  }
  ADDMSG(&f->msgs, msg_message("displayitem", "weight item description",
    itype->weight, itype->rtype, info));

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
  const char *name, *info, *key;
  int a, at_count;
  char buf2[2048];
  char * bufp = buf;
  size_t size = sizeof(buf), rsize;

  if (u && u->race != rc) return false;
  name = rc_name(rc, 0);

  rsize = slprintf(bufp, size, "%s: ", LOC(f->locale, name));
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  key = mkname("raceinfo", rc->_name[0]);
  info = locale_string(f->locale, key);
  if (info==key || strcmp(info, key)==0) {
    info = locale_string(f->locale, mkname("raceinfo", "no_info"));
  }

  rsize = strlcpy(bufp, info, size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  /* hp_p : Trefferpunkte */
  sprintf(buf2, " %d Trefferpunkte", rc->hitpoints);

  rsize = strlcpy(bufp, buf2, size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  /* b_armor : Rüstung */
  if (rc->armor > 0){
    sprintf(buf2, ", Rüstung: %d", rc->armor);
    rsize = strlcpy(bufp, buf2, size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }
  /* b_attacke : Angriff */
  sprintf(buf2, ", Angriff: %d", (rc->at_default+rc->at_bonus));
  rsize = strlcpy(bufp, buf2, size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  /* b_defense : Verteidigung */
  sprintf(buf2, ", Verteidigung: %d", (rc->df_default+rc->df_bonus));
  rsize = strlcpy(bufp, buf2, size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  if (size>1) {
    strcpy(bufp++, ".");
    --size;
  }

  /* b_damage : Schaden */
  at_count=0;
  for (a = 0; a < 6; a++) {
    if (rc->attack[a].type != AT_NONE){
      at_count++;
    }
  }
  if (rc->battle_flags & BF_EQUIPMENT) {
    rsize = strlcpy(bufp, " Kann Waffen benutzen.", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }
  if (rc->battle_flags & BF_RES_PIERCE) {
    rsize = strlcpy(bufp, " Ist durch Stichwaffen, Bögen und Armbrüste schwer zu verwunden.", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }
  if (rc->battle_flags & BF_RES_CUT) {
    rsize = strlcpy(bufp, " Ist durch Hiebwaffen schwer zu verwunden.", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }
  if (rc->battle_flags & BF_RES_BASH) {
    rsize = strlcpy(bufp, " Ist durch Schlagwaffen und Katapulte schwer zu verwunden.", size);
    if (rsize>size) rsize = size-1;
    size -= rsize;
    bufp += rsize;
  }

  sprintf(buf2, " Hat %d Angriff%s", at_count, (at_count>1)?"e":"");
  rsize = strlcpy(bufp, buf2, size);
  if (rsize>size) rsize = size-1;
  size -= rsize;
  bufp += rsize;

  for (a = 0; a < 6; a++) {
    if (rc->attack[a].type != AT_NONE){
      if (size>2) {
        if (a!=0) strcat(bufp, ", ");
        else strcat(bufp, ": ");
        size -= 2;
        bufp += 2;
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
      rsize = strlcpy(bufp, buf2, size);
      if (rsize>size) rsize = size-1;
      size -= rsize;
      bufp += rsize;
    }
  }

  if (size>1) {
    strcat(bufp++, ".");
    --size;
  }

  addmessage(0, f, buf, MSG_EVENT, ML_IMPORTANT);

  return true;
}

static void
reshow(unit * u, struct order * ord, const char * s, param_t p)
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
      if (c == 0) cmistake(u, ord, 285, MSG_EVENT);
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
      sp = get_spellfromtoken(u, s, u->faction->locale);
      if (sp!=NULL && has_spell(u, sp)) {
        attrib *a = a_find(u->faction->attribs, &at_seenspell);
        while (a!=NULL && a->type==&at_seenspell && a->data.v!=sp) a = a->next;
        if (a!=NULL) a_remove(&u->faction->attribs, a);
        break;
      }
      /* last, check if it's a race. */
      rc = findrace(s, u->faction->locale);
      if (rc != NULL) {
        if (display_race(u->faction, u, rc)) break;
      }
      cmistake(u, ord, 21, MSG_EVENT);
      break;
    default:
      cmistake(u, ord, 222, MSG_EVENT);
      break;
  }
}

#ifdef HEROES
static int
promotion_cmd(unit * u, struct order * ord)
{
  int money, people;

  if (maxheroes(u->faction) < countheroes(u->faction)+u->number) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_maxed", "max count",
      maxheroes(u->faction), countheroes(u->faction)));
    return 0;
  }
  if (!playerrace(u->race)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_race", "race",
      u->race));
    return 0;
  }
  people = count_all(u->faction) * u->number;
  money = get_pooled(u, i_silver->rtype, GET_ALL, people);

  if (people>money) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_cost", "cost have",
      people, money));
    return 0;
  }
  use_pooled(u, i_silver->rtype, GET_ALL, people);
  fset(u, UFL_HERO);
  ADDMSG(&u->faction->msgs, msg_message("hero_promotion", "unit cost",
    u, people));
  return 0;
}
#endif

static int
group_cmd(unit * u, struct order * ord)
{
  const char * s;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  join_group(u, s);
  return 0;
}

static int
origin_cmd(unit * u, struct order * ord)
{
  short px, py;

  init_tokens(ord);
  skip_token();

  px = (short)atoi(getstrtoken());
  py = (short)atoi(getstrtoken());

  set_ursprung(u->faction, getplaneid(u->region), px, py);
  return 0;
}

static int
guard_off_cmd(unit * u, struct order * ord)
{
  assert(get_keyword(ord)==K_GUARD);
  init_tokens(ord);
  skip_token();

  if (getparam(u->faction->locale) == P_NOT) {
    setguard(u, GUARD_NONE);
  }
  return 0;
}

static int
reshow_cmd(unit * u, struct order * ord)
{
  const char * s;
  param_t p = NOPARAM;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (findparam(s, u->faction->locale) == P_ANY) {
    p = getparam(u->faction->locale);
    s = NULL;
  }

  reshow(u, ord, s, p);
  return 0;
}

static int
status_cmd(unit * u, struct order * ord)
{
  const char * param;

  init_tokens(ord);
  skip_token();

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
    if (getparam(u->faction->locale) == P_NOT) {
      fset(u, UFL_NOAID);
    } else {
      freset(u, UFL_NOAID);
    }
    break;
  default:
    if (strlen(param)) {
      add_message(&u->faction->msgs,
        msg_feedback(u, ord, "unknown_status", ""));
    } else {
      u->status = ST_FIGHT;
    }
  }
  return 0;
}

static int
combatspell_cmd(unit * u, struct order * ord)
{
  const char * s;
  int level = 0;
  spell * spell;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  /* KAMPFZAUBER [NICHT] löscht alle gesetzten Kampfzauber */
  if (!s || *s == 0 || findparam(s, u->faction->locale) == P_NOT) {
    unset_combatspell(u, 0);
    return 0;
  }

  /* Optional: STUFE n */
  if (findparam(s, u->faction->locale) == P_LEVEL) {
    /* Merken, setzen kommt erst später */
    s = getstrtoken();
    level = atoi(s);
    level = max(0, level);
    s = getstrtoken();
  }

  spell = get_spellfromtoken(u, s, u->faction->locale);

  if(!spell){
    cmistake(u, ord, 173, MSG_MAGIC);
    return 0;
  }

  s = getstrtoken();

  if (findparam(s, u->faction->locale) == P_NOT) {
    /* KAMPFZAUBER "<Spruchname>" NICHT  löscht diesen speziellen
    * Kampfzauber */
    unset_combatspell(u, spell);
    return 0;
  } else {
    /* KAMPFZAUBER "<Spruchname>"  setzt diesen Kampfzauber */
    set_combatspell(u, spell, ord, level);
  }

  return 0;
}

/* ------------------------------------------------------------- */
/* Beachten: einige Monster sollen auch unbewaffent die Region bewachen
 * können */
void
update_guards(void)
{
  region *r;
  unit *u;

  for (r = regions; r; r = r->next)
    for (u = r->units; u; u = u->next) {
      if (getguard(u) && (!armedmen(u) || u->faction->age < NewbieImmunity()))
        setguard(u, GUARD_NONE);
    }
}

static int
guard_on_cmd(unit * u, struct order * ord)
{
  assert(get_keyword(ord)==K_GUARD);

  init_tokens(ord);
  skip_token();

  /* GUARD NOT is handled in goard_off_cmd earlier in the turn */
  if (getparam(u->faction->locale) == P_NOT) return 0;

  if (fval(u->region->terrain, SEA_REGION)) {
    cmistake(u, ord, 2, MSG_EVENT);
  } else {
    if (fval(u, UFL_MOVED)) {
      cmistake(u, ord, 187, MSG_EVENT);
    } else if (fval(u->race, RCF_ILLUSIONARY) || u->race == new_race[RC_SPELL]) {
      cmistake(u, ord, 95, MSG_EVENT);
    } else {
      /* Monster der Monsterpartei dürfen immer bewachen */
      if (u->faction == findfaction(MONSTER_FACTION)) {
        guard(u, GUARD_ALL);
      } else if (!armedmen(u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "unit_unarmed", ""));
      } else if (u->faction->age < NewbieImmunity()) {
        cmistake(u, ord, 304, MSG_EVENT);
      } else {
        guard(u, GUARD_ALL);
      }
    }
  }
  return 0;
}

static void
sinkships(region * r)
{
  ship *sh;

  list_foreach(ship, r->ships, sh) {
    if (fval(r->terrain, SEA_REGION) && (!enoughsailors(sh, r) || get_captain(sh)==NULL)) {
      /* Schiff nicht seetüchtig */
      damage_ship(sh, 0.30);
    }
    if (shipowner(sh)==NULL) {
      damage_ship(sh, 0.05);
    }
    if (sh->damage >= sh->size * DAMAGE_SCALE)
      destroy_ship(sh);
  }
  list_next(sh);
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
    if (f->subscription) {
      sql_print(("UPDATE subscriptions set faction='%s' where id=%u;\n",
                 itoa36(rp->want), f->subscription));
    }
    funhash(f);
    f->no = rp->want;
    fhash(f);
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
    unit ** up=&r->units;
    boolean sorted=false;
    while (*up) {
      unit * u = *up;
      if (!fval(u, FL_MARK)) {
        struct order * ord;
        for (ord = u->orders;ord;ord=ord->next) {
          if (get_keyword(ord)==K_SORT) {
            const char * s;
            param_t p;
            int id;
            unit *v;

            init_tokens(ord);
            skip_token();
            s = getstrtoken();
            p = findparam(s, u->faction->locale);
            id = getid();
            v = findunit(id);

            if (v==NULL || v->faction!=u->faction || v->region!=r) {
              cmistake(u, ord, 258, MSG_EVENT);
            } else if (v->building != u->building || v->ship!=u->ship) {
              cmistake(u, ord, 259, MSG_EVENT);
            } else if (fval(u, UFL_OWNER)) {
              cmistake(u, ord, 260, MSG_EVENT);
            } else if (v == u) {
              cmistake(u, ord, 10, MSG_EVENT);
            } else {
              switch(p) {
              case P_AFTER:
                *up = u->next;
                u->next = v->next;
                v->next = u;
                break;
              case P_BEFORE:
                if (fval(v, UFL_OWNER)) {
                  cmistake(v, ord, 261, MSG_EVENT);
                } else {
                  unit ** vp=&r->units;
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
    if (sorted) {
      unit * u;
      for (u=r->units;u;u=u->next) freset(u, FL_MARK);
    }
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
      for (S = u->orders; S; S = S->next) if (get_keyword(ord)==K_EVICT) {
        int id;
        unit *u2;
        /* Nur der Kapitän bzw Burgherr kann jemanden rausschmeißen */
        if(!fval(u, UFL_OWNER)) {
          /* Die Einheit ist nicht der Eigentümer */
          cmistake(u,ord,49,MSG_EVENT);
          continue;
        }
        init_tokens(ord);
        skip_token();
        id = getid();
        u2 = findunit(id);

        if (u2==NULL) {
          /* Einheit nicht gefunden */
          cmistake(u,ord,63,MSG_EVENT);
          continue;
        }

        if (u->building){
          /* in der selben Burg? */
          if (u->building != u2->building){
            /* nicht in Burg */
            cmistake(u,ord,33,MSG_EVENT);
            continue;
          }
          leave_building(u2);
          /* meldung an beide */
        }

        if (u->ship){
          if (u->ship != u2->ship){
            /* nicht an Bord */
            cmistake(u, ord, 32, MSG_EVENT);
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

#ifdef ENEMIES
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
        switch (get_keyword(ord)) {
        case K_WAR:
          init_tokens(ord);
          skip_token();
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
                ADDMSG(&f->msgs, msg_message("error66", "unit region command", u, r, ord));
              }
            }
          }
          break;
        case K_PEACE:
          init_tokens(ord);
          skip_token();
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
                ADDMSG(&f->msgs, msg_message("error66", "unit region command", u, r, ord));
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

static int
renumber_cmd(unit * u, order * ord)
{
  const char * s;
  int i;
  faction * f = u->faction;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();
  switch(findparam(s, u->faction->locale)) {

    case P_FACTION:
      s = getstrtoken();
      if (s && *s) {
        int id = atoi36(s);
        attrib * a = a_find(f->attribs, &at_number);
        if (!a) a = a_add(&f->attribs, a_new(&at_number));
        a->data.i = id;
      }
      break;

    case P_UNIT:
      s = getstrtoken();
      if (s == NULL || *s == 0) {
        i = newunitid();
      } else {
        i = atoi36(s);
        if (i<=0 || i>MAX_UNIT_NR) {
          cmistake(u, ord, 114, MSG_EVENT);
          break;
        }

        if (forbiddenid(i)) {
          cmistake(u, ord, 116, MSG_EVENT);
          break;
        }

        if (findunitg(i, u->region)) {
          cmistake(u, ord, 115, MSG_EVENT);
          break;
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
      if (!u->ship) {
        cmistake(u, ord, 144, MSG_EVENT);
        break;
      }
      if (u->ship->coast != NODIRECTION) {
        cmistake(u, ord, 116, MSG_EVENT);
        break;
      }
      if (!fval(u, UFL_OWNER)) {
        cmistake(u, ord, 146, MSG_EVENT);
        break;
      }
      s = getstrtoken();
      if(s == NULL || *s == 0) {
        i = newcontainerid();
      } else {
        i = atoi36(s);
        if (i<=0 || i>MAX_CONTAINER_NR) {
          cmistake(u,ord,114,MSG_EVENT);
          break;
        }
        if (findship(i) || findbuilding(i)) {
          cmistake(u, ord, 115, MSG_EVENT);
          break;
        }
      }
      sunhash(u->ship);
      u->ship->no = i;
      shash(u->ship);
      break;
    case P_BUILDING:
    case P_GEBAEUDE:
      if (!u->building) {
        cmistake(u,ord,145,MSG_EVENT);
        break;
      }
      if(!fval(u, UFL_OWNER)) {
        cmistake(u,ord,148,MSG_EVENT);
        break;
      }
      s = getstrtoken();
      if(*s == 0) {
        i = newcontainerid();
      } else {
        i = atoi36(s);
        if (i<=0 || i>MAX_CONTAINER_NR) {
          cmistake(u,ord,114,MSG_EVENT);
          break;
        }
        if(findship(i) || findbuilding(i)) {
          cmistake(u,ord,115,MSG_EVENT);
          break;
        }
      }
      bunhash(u->building);
      u->building->no = i;
      bhash(u->building);
      break;

    default:
      cmistake(u, ord, 239, MSG_EVENT);
  }
  return 0;
}

static building *
age_building(building * b)
{
  static boolean init = false;
  static const building_type * bt_blessed;
  static const curse_type * ct_astralblock;
  if (!init) {
    init = true;
    bt_blessed = bt_find("blessedstonecircle");
    ct_astralblock = ct_find("astralblock");
  }

  /* blesses stone circles create an astral protection in the astral region 
   * above the shield, which prevents chaos suction and other spells. 
   * The shield is created when a magician enters the blessed stone circle,
   * and lasts for as long as his skill level / 2 is, at no mana cost.
   *
   * TODO: this would be nicer in a btype->age function, but we don't have it.
   */
  if (ct_astralblock && bt_blessed && b->type==bt_blessed) {
    region * r = b->region;
    region * rt = r_standard_to_astral(r);
    unit * u, * mage = NULL;

    /* step 1: give unicorns to people in the building,
     * find out if there's a magician in there. */
    for (u=r->units;u;u=u->next) {
      if (b==u->building && inside_building(u)) {
        if (!(u->race->ec_flags & NOGIVE)) {
          int n, unicorns = 0;
          for (n=0; n!=u->number; ++n) {
            if (chance(0.02)) {
              i_change(&u->items, olditemtype[I_UNICORN], 1);
              ++unicorns;
            }
            if (unicorns) {
              ADDMSG(&u->faction->msgs, msg_message("scunicorn", 
                "unit amount rtype", u, unicorns, 
                olditemtype[I_UNICORN]->rtype));
            }
          }
        }
        if (mage==NULL && is_mage(u)) {
          mage = u;
        }
      }
    }

    /* if there's a magician, and a connection to astral space, create the
     * curse. */
    if (rt!=NULL && mage!=NULL) {
      curse * c = get_curse(rt->attribs, ct_astralblock);
      if (c==NULL) {
        if (mage!=NULL) {
          int sk = effskill(mage, SK_MAGIC);
          variant effect;
          effect.i = 100;
          /* the mage reactivates the circle */
          c = create_curse(mage, &rt->attribs, ct_astralblock,
            sk, sk/2, effect, 0);
          ADDMSG(&r->msgs, msg_message("astralshield_activate", 
            "region unit", r, mage));
        }
      } else if (mage!=NULL) {
        int sk = effskill(mage, SK_MAGIC);
        c->duration = max(c->duration, sk/2);
        c->vigour = max(c->vigour, sk);
      }
    }
  }

  a_age(&b->attribs);
  handle_event(b->attribs, "timer", b);

  return b;
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
    handle_event(f->attribs, "timer", f);
  }

  /* Regionen */
  for (r=regions;r;r=r->next) {
    building ** bp;
    unit ** up;
    ship ** sp;

    a_age(&r->attribs);
    handle_event(r->attribs, "timer", r);

    /* Einheiten */
    for (up=&r->units;*up;) {
      unit * u = *up;
      a_age(&u->attribs);
      if (u==*up) handle_event(u->attribs, "timer", u);
      if (u==*up) up = &(*up)->next;
    }

    /* Schiffe */
    for (sp=&r->ships;*sp;) {
      ship * s = *sp;
      a_age(&s->attribs);
      if (s==*sp) handle_event(s->attribs, "timer", s);
      if (s==*sp) sp = &(*sp)->next;
    }

    /* Gebäude */
    for (bp=&r->buildings;*bp;) {
      building * b = *bp;
      b = age_building(b);

      if (b==*bp) bp = &(*bp)->next;
    }
  }
}

static int
maxunits(const faction *f)
{
  if (global.unitsperalliance == true) {
    float mult = 1.0;

#ifdef KARMA_MODULE
    faction *f2;
    for (f2 = factions; f2; f2 = f2->next) {
      if (f2->alliance == f->alliance) {
        mult += 0.4f * fspecial(f2, FS_ADMINISTRATOR);
      }
    }
#endif /* KARMA_MODULE */
    return (int) (global.maxunits * mult);
  }
#ifdef KARMA_MODULE
  return (int) (global.maxunits * (1 + 0.4 * fspecial(f, FS_ADMINISTRATOR)));
#else
  return global.maxunits;
#endif /* KARMA_MODULE */
}

static boolean
checkunitnumber(const faction *f, int add)
{
  if (global.maxunits==0) return true;
  if (global.unitsperalliance == true) {
    /* if unitsperalliance is true, maxunits returns the
    number of units allowed in an alliance */
    faction *f2;
    int unitsinalliance = add;
    int maxu = maxunits(f);

    for (f2 = factions; f2; f2 = f2->next) {
      if (f->alliance == f2->alliance) {
        unitsinalliance += f2->no_units;
      }
      if (unitsinalliance > maxu) return false;
    }

    return true;
  }

  return (f->no_units + add < maxunits(f));
}

static void
new_units (void)
{
  region *r;
  unit *u, *u2;

  /* neue einheiten werden gemacht und ihre befehle (bis zum "ende" zu
  * ihnen rueberkopiert, damit diese einheiten genauso wie die alten
  * einheiten verwendet werden koennen. */

  for (r = regions; r; r = r->next) {
    for (u = r->units; u; u = u->next) {
      order ** ordp = &u->orders;
      while (*ordp) {
        order * makeord = *ordp;
        if (get_keyword(makeord) == K_MAKE) {
          init_tokens(makeord);
          skip_token();
          if (getparam(u->faction->locale) == P_TEMP) {
            const char * token;
            char * name = NULL;
            int g, alias;
            order ** newordersp;

            if (!checkunitnumber(u->faction, 1)) {
							if (global.unitsperalliance) {
                ADDMSG(&u->faction->msgs, msg_message("too_many_units_in_alliance",
                  "command unit region allowed",
                  makeord, u, r, maxunits(u->faction)));
							} else {
                ADDMSG(&u->faction->msgs, msg_message("too_many_units_in_faction",
                  "command unit region allowed",
                  makeord, u, r, maxunits(u->faction)));
							}
              ordp = &makeord->next;

              while (*ordp) {
                order * ord = *ordp;
                if (get_keyword(ord) == K_END) break;
                *ordp = ord->next;
                ord->next = NULL;
                free_order(ord);
              }
              continue;
            }
            alias = getid();

            token = getstrtoken();
            if (token && strlen(token)>0) {
              name = strdup(token);
            }
            u2 = create_unit(r, u->faction, 0, u->faction->race, alias, name, u);
            if (name!=NULL) free(name);
            fset(u2, UFL_ISNEW);

            a_add(&u2->attribs, a_new(&at_alias))->data.i = alias;

            g = getguard(u);
            if (g) setguard(u2, g);
            else setguard(u, GUARD_NONE);

            ordp = &makeord->next;
            newordersp = &u2->orders;
            while (*ordp) {
              order * ord = *ordp;
              if (get_keyword(ord) == K_END) break;
              *ordp = ord->next;
              ord->next = NULL;
              *newordersp = ord;
              newordersp = &ord->next;
            }
          }
        }
        if (*ordp==makeord) ordp=&makeord->next;
      }
    }
  }
}

static void
setdefaults(unit *u)
{
  order *ord;
  boolean trade = false;
  boolean hunger = LongHunger(u);

  freset(u, UFL_LONGACTION);
  if (hunger) {
    /* Hungernde Einheiten führen NUR den default-Befehl aus */
    set_order(&u->thisorder, default_order(u->faction->locale));
  }
#ifdef LASTORDER
  else {
    /* by default the default long order becomes the new long order. */
    u->thisorder = copy_order(u->lastorder);
  }
#endif
  /* check all orders for a potential new long order this round: */
  for (ord = u->orders; ord; ord = ord->next) {
#ifndef LASTORDER
    if (u->old_orders && is_repeated(ord)) {
      /* this new order will replace the old defaults */
      free_orders(&u->old_orders);
      if (hunger) break;
    }
#endif
    if (hunger) continue;

    if (is_exclusive(ord)) {
      /* Über dieser Zeile nur Befehle, die auch eine idle Faction machen darf */
      if (idle(u->faction)) {
        set_order(&u->thisorder, default_order(u->faction->locale));
      } else {
        set_order(&u->thisorder, copy_order(ord));
      }
      break;
    } else {
      keyword_t keyword = get_keyword(ord);
      switch (keyword) {
        /* Wenn gehandelt wird, darf kein langer Befehl ausgeführt
        * werden. Da Handel erst nach anderen langen Befehlen kommt,
        * muß das vorher abgefangen werden. Wir merken uns also
        * hier, ob die Einheit handelt. */
      case NOKEYWORD:
        cmistake(u, ord, 22, MSG_EVENT);
        break;
      case K_BUY:
      case K_SELL:
        /* Wenn die Einheit handelt, muß der Default-Befehl gelöscht
         * werden. */
        trade = true;
        break;
        
      case K_CAST:
        /* dient dazu, das neben Zaubern kein weiterer Befehl
         * ausgeführt werden kann, Zaubern ist ein kurzer Befehl */
        set_order(&u->thisorder, NULL);
        break;
        
      case K_WEREWOLF:
        set_order(&u->thisorder, copy_order(ord));
        break;
        
        /* Wird je diese Ausschliesslichkeit aufgehoben, muss man aufpassen
         * mit der Reihenfolge von Kaufen, Verkaufen etc., damit es Spielern
         * nicht moeglich ist, Schulden zu machen. */
      }
    }
  }

  if (hunger) return;

  /* Wenn die Einheit handelt, muß der Default-Befehl gelöscht
  * werden. */

  if (trade == true) {
    /* fset(u, UFL_LONGACTION); */
    set_order(&u->thisorder, NULL);
  }
  /* thisorder kopieren wir nun nach lastorder. in lastorder steht
  * der DEFAULT befehl der einheit. da MOVE kein default werden
  * darf, wird MOVE nicht in lastorder kopiert. MACHE TEMP wurde ja
  * schon gar nicht erst in thisorder kopiert, so dass MACHE TEMP
  * durch diesen code auch nicht zum default wird Ebenso soll BIETE
  * nicht hierher, da i.A. die Einheit dann ja weg ist (und damit
  * die Einheitsnummer ungueltig). Auch Attackiere sollte nie in
  * den Default übernommen werden */

#ifdef LASTORDER
  switch (get_keyword(u->thisorder)) {
    case K_MOVE:
    case K_ATTACK:
    case K_WEREWOLF:
    case NOKEYWORD:
      /* these can never be default orders */
      break;

    default:
      set_order(&u->lastorder, copy_order(u->thisorder));
  }
#endif
}

static int
use_item(unit * u, const item_type * itype, int amount, struct order * ord)
{
  int i;
  int target = read_unitid(u->faction, u->region);

  i = get_pooled(u, itype->rtype, GET_DEFAULT, amount);

  if (amount>i) {
    amount = i;
  }
  if (amount==0) {
    cmistake(u, ord, 43, MSG_PRODUCE);
    return ENOITEM;
  }

  if (target==-1) {
    if (itype->use==NULL) {
      cmistake(u, ord, 76, MSG_PRODUCE);
      return EUNUSABLE;
    }
    return itype->use(u, itype, amount, ord);
  } else {
    if (itype->useonother==NULL) {
      cmistake(u, ord, 76, MSG_PRODUCE);
      return EUNUSABLE;
    }
    return itype->useonother(u, target, itype, amount, ord);
  }
}


static double
heal_factor(const race *rc)
{
  switch(old_race(rc)) {
    case RC_TROLL:
    case RC_DAEMON:
      return 1.5;
    case RC_GOBLIN:
      return 2.0;
  }
  return 1.0;
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
      double p = 1.0;

      /* hp über Maximum bauen sich ab. Wird zb durch Elixier der Macht
      * oder verändertes Ausdauertalent verursacht */
      if (u->hp > umhp) {
        u->hp -= (int) ceil((u->hp - umhp) / 2.0);
        if (u->hp < umhp) u->hp = umhp;
        continue;
      }

      if (u->race->flags & RCF_NOHEAL) continue;
      if (fval(u, UFL_HUNGER)) continue;

      if (fval(r->terrain, SEA_REGION) && u->ship==NULL && !(canswim(u) || canfly(u))) {
        continue;
      }

#ifdef KARMA_MODULE
      if (fspecial(u->faction, FS_UNDEAD)) continue;

      if(fspecial(u->faction, FS_REGENERATION)) {
        u->hp = umhp;
        continue;
      }
#endif /* KARMA_MODULE */

      p *= heal_factor(u->race);
      if (u->hp < umhp) {
#ifdef NEW_DAEMONHUNGER_RULE
        double maxheal = max(u->number, umhp/20.0);
#else
        double maxheal = max(u->number, umhp/10.0);
#endif
        int addhp;
        struct building * b = inside_building(u);
        const struct building_type * btype = b?b->type:NULL;
        if (btype == bt_find("inn")) {
          p *= 1.5;
        }
        /* pro punkt 5% höher */
        p *= (1.0 + healingcurse * 0.05);

        maxheal = p * maxheal;
        addhp = (int)maxheal;
        maxheal -= addhp;
        if (maxheal>0.0 && chance(maxheal)) ++addhp;

        /* Aufaddieren der geheilten HP. */
        u->hp = min(u->hp + addhp, umhp);

        /* soll man an negativer regeneration sterben können? */
        assert(u->hp > 0);
      }
    }
  }
}

static void
remove_exclusive(order ** ordp) 
{
  while (*ordp) {
    order * ord = *ordp;
    if (is_exclusive(ord)) {
      *ordp = ord->next;
      ord->next = NULL;
      free_order(ord);
    } else {
      ordp = &ord->next;
    }
  }
}

static void
defaultorders (void)
{
  region *r;
  for (r=regions;r;r=r->next) {
    unit *u;
    for (u=r->units;u;u=u->next) {
#ifndef LASTORDER
      boolean neworders = false;
#endif
      order ** ordp = &u->orders;
      while (*ordp!=NULL) {
        order * ord = *ordp;
        if (get_keyword(ord)==K_DEFAULT) {
          char * cmd;
          order * new_order;
          init_tokens(ord);
          skip_token(); /* skip the keyword */
          cmd = strdup(getstrtoken());
          new_order = parse_order(cmd, u->faction->locale);
          *ordp = ord->next;
          ord->next = NULL;
          free_order(ord);
#ifdef LASTORDER
          if (new_order) set_order(&u->lastorder, new_order);
#else
          if (!neworders) {
            /* lange Befehle aus orders und old_orders löschen zu gunsten des neuen */
            remove_exclusive(&u->orders);
            remove_exclusive(&u->old_orders);
            neworders = true;
            ordp = &u->orders; /* we could have broken ordp */
          }
          if (new_order) addlist(&u->old_orders, new_order);
#endif
          free(cmd);
        }
        else ordp = &ord->next;
      }
    }
  }
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
      if (u->faction!=NULL && u->number>0) {
        sc_mage *m = get_mage(u);
        if (u->faction->no != MONSTER_FACTION && m != NULL) {
          if (m->magietyp == M_GRAU) continue;
          updatespelllist(u);
        }
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
    if (f->age < NewbieImmunity()) {
      ADDMSG(&f->msgs, msg_message("newbieimmunity", "turns", 
        NewbieImmunity() - f->age));
    }
  }
}

static int
use_cmd(unit * u, struct order * ord)
{
  const char * t;
  int n;
  const item_type * itype;

  init_tokens(ord);
  skip_token();

  t = getstrtoken();
  n = atoi(t);
  if (n==0) {
    if (findparam(t, u->faction->locale) == P_ANY) {
      /* BENUTZE ALLES Yanxspirit */
      n = INT_MAX;
      t = getstrtoken();
    } else {
      /* BENUTZE Yanxspirit */
      n = 1;
    }
  } else {
    /* BENUTZE 42 Yanxspirit */
    t = getstrtoken();
  }
  itype = finditemtype(t, u->faction->locale);

  if (itype!=NULL) {
    int i = use_item(u, itype, n, ord);
    assert(i<=0 || !"use_item should not return positive values.");
    if (i>0) {
      log_error(("use_item returned a value>0 for %s\n", resourcename(itype->rtype, 0)));
    }     
  } else {
    cmistake(u, ord, 43, MSG_PRODUCE);
  }
  return 0;
}

static int
claim_cmd(unit * u, struct order * ord)
{
  const char * t;
  int n;
  const item_type * itype;

  init_tokens(ord);
  skip_token();

  t = getstrtoken();
  n = atoi(t);
  if (n==0) {
    n = 1;
  } else {
    t = getstrtoken();
  }
  itype = finditemtype(t, u->faction->locale);

  if (itype!=NULL) {
    item ** iclaim = i_find(&u->faction->items, itype);
    if (iclaim!=NULL && *iclaim!=NULL) {
      n = min(n, (*iclaim)->number);
      i_change(iclaim, itype, -n);
      i_change(&u->items, itype, n);
    }
  } else {
    cmistake(u, ord, 43, MSG_PRODUCE);
  }
  return 0;
}

typedef struct processor {
  struct processor * next;
  int priority;
  enum { PR_GLOBAL, PR_REGION, PR_UNIT, PR_ORDER } type;
  union {
    struct {
      keyword_t kword;
      boolean thisorder;
      int (*process)(struct unit *, struct order *);
    } per_order;
    struct {
      void (*process)(struct unit *);
    } per_unit;
    struct {
      void (*process)(struct region *);
    } per_region;
    struct {
      void (*process)(void);
    } global;
  } data;
  const char * name;
} processor;

static processor * processors;

void
add_proc_order(int priority, keyword_t kword, int (*parser)(struct unit *, struct order *), boolean thisorder, const char * name)
{
  processor **pproc = &processors;
  processor *proc;

  while (*pproc) {
    proc = *pproc;
    if (proc->priority>priority) break;
    else if (proc->priority==priority && proc->type>=PR_ORDER) break;
    pproc = &proc->next;
  }

  proc = malloc(sizeof(processor));
  proc->priority = priority;
  proc->type = PR_ORDER;
  proc->data.per_order.process = parser;
  proc->data.per_order.kword = kword;
  proc->data.per_order.thisorder = thisorder;
  proc->name = name;
  proc->next = *pproc;
  *pproc = proc;
}

void
add_proc_global(int priority, void (*process)(void), const char * name)
{
  processor **pproc = &processors;
  processor *proc;

  while (*pproc) {
    proc = *pproc;
    if (proc->priority>priority) break;
    else if (proc->priority==priority && proc->type>PR_GLOBAL) break;
    pproc = &proc->next;
  }

  proc = malloc(sizeof(processor));
  proc->priority = priority;
  proc->type = PR_GLOBAL;
  proc->data.global.process = process;
  proc->name = name;
  proc->next = *pproc;
  *pproc = proc;
}

void
add_proc_region(int priority, void (*process)(region *), const char * name)
{
  processor **pproc = &processors;
  processor *proc;

  while (*pproc) {
    proc = *pproc;
    if (proc->priority>priority) break;
    else if (proc->priority==priority && proc->type>PR_REGION) break;
    pproc = &proc->next;
  }

  proc = malloc(sizeof(processor));
  proc->priority = priority;
  proc->type = PR_REGION;
  proc->data.per_region.process = process;
  proc->name = name;
  proc->next = *pproc;
  *pproc = proc;
}

void
add_proc_unit(int priority, void (*process)(unit *), const char * name)
{
  processor **pproc = &processors;
  processor *proc;

  while (*pproc) {
    proc = *pproc;
    if (proc->priority>priority) break;
    else if (proc->priority==priority && proc->type>PR_UNIT) break;
    pproc = &proc->next;
  }

  proc = malloc(sizeof(processor));
  proc->priority = priority;
  proc->type = PR_UNIT;
  proc->data.per_unit.process = process;
  proc->name = name;
  proc->next = *pproc;
  *pproc = proc;
}

void
process(void)
{
  processor *proc = processors;
  while (proc) {
    int prio = proc->priority;
    region *r;
    processor *pglobal = proc;

    printf("- Step %u\n", prio);
    while (proc && proc->priority==prio) {
      if (proc->name) printf(" - %s\n", proc->name);
      proc = proc->next;
    }

    while (pglobal && pglobal->priority==prio && pglobal->type==PR_GLOBAL) {
      pglobal->data.global.process();
      pglobal = pglobal->next;
    }
    if (pglobal==NULL || pglobal->priority!=prio) continue;

    for (r = regions; r; r = r->next) {
      unit *u;
      processor *pregion = pglobal;

      while (pregion && pregion->priority==prio && pregion->type==PR_REGION) {
        pregion->data.per_region.process(r);
        pregion = pregion->next;
      }
      if (pregion==NULL || pregion->priority!=prio) continue;

      for (u=r->units;u;u=u->next) {
        processor *porder, *punit = pregion;

        while (punit && punit->priority==prio && punit->type==PR_UNIT) {
          punit->data.per_unit.process(u);
          punit = punit->next;
        }
        if (punit==NULL || punit->priority!=prio) continue;

        porder = punit;
        while (porder && porder->priority==prio && porder->type==PR_ORDER) {
          order ** ordp = &u->orders;
          if (porder->data.per_order.thisorder) ordp = &u->thisorder;
          while (*ordp) {
            order * ord = *ordp;
            if (get_keyword(ord) == porder->data.per_order.kword) {
              porder->data.per_order.process(u, ord);
            }
            if (*ordp==ord) ordp=&ord->next;
          }
          porder = porder->next;
        }
      }
    }
  }
}

static void enter_1(region * r) { do_misc(r, false); }
static void enter_2(region * r) { do_misc(r, true); }
static void maintain_buildings_1(region * r) { maintain_buildings(r, false); }
static void maintain_buildings_2(region * r) { maintain_buildings(r,true); }
static void reset_moved(unit * u) { freset(u, UFL_MOVED); }

static void reset_rng(void) {
  if (turn == 0) rng_init((int)time(0));
  else rng_init(turn);
}

void
processorders (void)
{
  region *r;
  int p;

  p = 10;
  add_proc_global(p, &new_units, "Neue Einheiten erschaffen");

  p+=10;
  add_proc_unit(p, &setdefaults, "Default-Befehle");
  add_proc_order(p, K_BANNER, &banner_cmd, false, NULL);
  add_proc_order(p, K_EMAIL, &email_cmd, false, NULL);
  add_proc_order(p, K_PASSWORD, &password_cmd, false, NULL);
  add_proc_order(p, K_SEND, &send_cmd, false, NULL);
  add_proc_order(p, K_GROUP, &group_cmd, false, NULL);

  p+=10;
  add_proc_unit(p, &reset_moved, "Instant-Befehle");
  add_proc_order(p, K_QUIT, &quit_cmd, false, NULL);
  add_proc_order(p, K_URSPRUNG, &origin_cmd, false, NULL);
  add_proc_order(p, K_ALLY, &ally_cmd, false, NULL);
  add_proc_order(p, K_PREFIX, &prefix_cmd, false, NULL);
  add_proc_order(p, K_SYNONYM, &synonym_cmd, false, NULL);
  add_proc_order(p, K_SETSTEALTH, &setstealth_cmd, false, NULL);
  add_proc_order(p, K_STATUS, &status_cmd, false, NULL);
  add_proc_order(p, K_COMBAT, &combatspell_cmd, false, NULL);
  add_proc_order(p, K_DISPLAY, &display_cmd, false, NULL);
  add_proc_order(p, K_NAME, &name_cmd, false, NULL);
  add_proc_order(p, K_GUARD, &guard_off_cmd, false, NULL);
  add_proc_order(p, K_RESHOW, &reshow_cmd, false, NULL);
#ifdef KARMA_MODULE
  add_proc_order(p, K_WEREWOLF, &setwere_cmd, false, NULL);
#endif /* KARMA_MODULE */

  if (alliances!=NULL) {
    p+=10;
    add_proc_global(p, &alliancekick, NULL);
  }

  p+=10;
  add_proc_global(p, &age_factions, "Parteienalter++");
  add_proc_order(p, K_MAIL, &mail_cmd, false, "Botschaften");
  add_proc_order(p, K_CLAIM, &claim_cmd, false, NULL);

  p+=10; /* all claims must be done before we can USE */
  add_proc_region(p, &enter_1, "Kontaktieren & Betreten (1. Versuch)");
  add_proc_order(p, K_USE, &use_cmd, false, "Benutzen");

  if (alliances!=NULL) {
    p+=10; /* in case USE changes it */
    add_proc_global(p, &alliancevictory, "Testen der Allianzbedingungen");
  }

#ifdef INFOCMD_MODULE
  add_proc_global(p, &infocommands, NULL);
#endif
  add_proc_global(p, &gmcommands, "GM Kommandos");

  p += 10; /* in case it has any effects on allincevictories */
  add_proc_order(p, K_LEAVE, &leave_cmd, false, "Verlassen");

  if (!nobattle) {
#ifdef KARMA_MODULE
    p+=10;
    add_proc_global(p, &jihad_attacks, "Jihad-Angriffe");
#endif
    add_proc_region(p, &do_battle, "Attackieren");
  }

  p+=10; /* after combat, reset rng */
  add_proc_global(p, &reset_rng, NULL);
  add_proc_region(p, &do_siege, "Belagern");

  p+=10; /* can't allow reserve before siege (weapons) */
  add_proc_region(p, &enter_2, "Kontaktieren & Betreten (2. Versuch)");
  add_proc_order(p, K_RESERVE, &reserve_cmd, false, "Reservieren");
  add_proc_unit(p, &follow_unit, "Folge auf Einheiten setzen");

  p+=10; /* rest rng again before economics */
  add_proc_global(p, &reset_rng, NULL);
  add_proc_region(p, &economics, "Zerstören, Geben, Rekrutieren, Vergessen");

  p+=10;
  add_proc_region(p, &maintain_buildings_1, "Gebäudeunterhalt (1. Versuch)");

  p+=10; /* QUIT fuer sich alleine */
  add_proc_global(p, &quit, "Sterben");
  add_proc_global(p, &parse_restart, "Neustart");

  p+=10;
  add_proc_global(p, &magic, "Zaubern");

  p+=10;
  if (!global.disabled[K_TEACH]) {
    add_proc_region(p, &teaching, "Lehren");
  }
  add_proc_order(p, K_STUDY, &learn_cmd, true, "Lernen");

  p+=10;
  add_proc_global(p, &produce, "Produktion, Handel, Rekruten");

  p+=10;
  add_proc_region(p, &enter_2, "Kontaktieren & Betreten (3. Versuch)");

  p+=10;
  add_proc_region(p, &sinkships, "Schiffe sinken");

  p+=10;
  add_proc_global(p, &movement, "Bewegungen");

  p+=10;
  add_proc_order(p, K_GUARD, &guard_on_cmd, false, "Bewache (an)");
#ifdef XECMD_MODULE
  /* can do together with guard */
  add_proc_order(p, K_LEAVE, &xecmd, false, "Zeitung");
#endif

  p+=10;
  add_proc_global(p, &encounters, "Zufallsbegegnungen");
  p+=10;
  add_proc_global(p, &reset_rng, NULL);
  add_proc_unit(p, &monsters_kill_peasants, "Monster fressen und vertreiben Bauern");

  p+=10;
  add_proc_global(p, &randomevents, "Zufallsereignisse");

  p+=10;
  add_proc_global(p, &monthly_healing, "Regeneration (HP)");
  add_proc_global(p, &regeneration_magiepunkte, "Regeneration (Aura)");
  add_proc_global(p, &defaultorders, "Defaults setzen");
  add_proc_global(p, &demographics, "Nahrung, Seuchen, Wachstum, Wanderung");

  p+=10;
  add_proc_region(p, &maintain_buildings_2, "Gebäudeunterhalt (2. Versuch)");

  p+=10;
  add_proc_global(p, &reorder, "Einheiten sortieren");
#ifdef KARMA_MODULE
  p+=10;
  add_proc_global(p, &karma, "Jihads setzen");
#endif
#ifdef ALLIANCEJOIN
  p+=10;
  add_proc_global(p, &alliancejoin, "Allianzen");
#endif
#ifdef ENEMIES
  p+=10;
  add_proc_global(p, &declare_war, "Krieg & Frieden");
#endif
#ifdef HEROES
  add_proc_order(p, K_PROMOTION, &promotion_cmd, false, "Heldenbeförderung");
#endif
  add_proc_order(p, K_NUMBER, &renumber_cmd, false, "Neue Nummern (Einheiten)");

  p+=10;
  add_proc_global(p, &renumber_factions, "Neue Nummern");

  process();
  /*************************************************/

  for (r = regions;r;r=r->next) reorder_owners(r);

  puts(" - Attribute altern");
  ageing();
  remove_empty_units();

#ifdef WORMHOLE_MODULE
  create_wormholes();
#endif
  /* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
   * Beschreibungen geändert haben */
  update_spells();
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
