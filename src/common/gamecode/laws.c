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
#include <kernel/unit.h>
#ifdef USE_UGROUPS
#  include <kernel/ugroup.h>
#endif

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/synonym.h>

/* util includes */
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/sql.h>
#include <util/message.h>

#include <modules/xecmd.h>

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
    if (get_keyword(ord) == K_RESTART) {
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
      unit * donor = r->units;
      int hungry = u->number;

      while (donor!=NULL && hungry>0) {
        /* alwayy start with the first known uint that may have some blood */
        static const struct potion_type * pt_blood;
        if (pt_blood==NULL) pt_blood = pt_find("peasantblood");
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
    if (u->race->age) {
      u->race->age(u);
    }
  }

#ifdef ASTRAL_ITEM_RESTRICTIONS
  if (u->region->planep==astral_plane) {
    item ** itemp = &u->items;
    while (*itemp) {
      item * itm = *itemp;
      if (itm->type->flags & ITF_NOTLOST == 0) {
        if (itm->type->flags & (ITF_BIG|ITF_ANIMAL|ITF_CURSED)) {
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
    if (glueck >= 0) {    /* Sonst keine Vermehrung */
      if (rand() % 10000 < PEASANTGROWTH) {
        if ((float) peasants
          / ((float) production(r) * MAXPEASANTS_PER_AREA)
          < 0.9 || rand() % 100 < PEASANTFORCE) {
          peasants++;
        }
      }
    } else
      glueck++;

    if (glueck > 0) {   /* Doppelvermehrung */
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

  if (!landregion(rterrain(u->region))) {
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
quit(unit * u, struct order * ord)
{
  const char * passwd;
  init_tokens(ord);
  skip_token(); /* skip keyword */

  passwd = getstrtoken();
  if (checkpasswd(u->faction, passwd, false)) {
    if (EnhancedQuit()) {
      int f2_id = getid();

      if (f2_id > 0) {
        faction *f2 = findfaction(f2_id);
        faction * f = u->faction;

        if(f2 == NULL) {
          cmistake(u, ord, 66, MSG_EVENT);
        } else if(f == f2) {
          cmistake(u, ord, 8, MSG_EVENT);
        } else if (!u->faction->alliance || u->faction->alliance != f2->alliance) {
          cmistake(u, ord, 315, MSG_EVENT);
        } else if(!alliedfaction(NULL, f, f2, HELP_MONEY)) {
          cmistake(u, ord, 316, MSG_EVENT);
        } else {
          transfer_faction(f,f2);
          destroyfaction(f);
        }
      } else {
        destroyfaction(u->faction);
      }
    }
    destroyfaction(u->faction);
    return -1;
  } else {
    cmistake(u, ord, 86, MSG_EVENT);
    log_warning(("STIRB mit falschem Passwort für Partei %s: %s\n",
      factionid(u->faction), passwd));
  }
  return 0;
}

static void
parse_quit(void)
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
      for (ord = u->orders; ord; ord = ord->next) {
        switch (get_keyword(ord)) {
          case K_QUIT:
            if (quit(u, ord)!=0) ord = NULL;
            break;
          case K_RESTART:
            if (u->number > 0) {
              if (restart_cmd(u, ord)!=0) ord = NULL;
            }
            break;
        }
    if (ord==NULL) break;
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
      f->override = strdup(itoa36(rand()));
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
        sql_print(("UPDATE subscriptions SET status='ACTIVE', lastturn=%d, password='%s' WHERE id=%u;\n",
                   f->lastorders, f->override, f->subscription));
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

  /* Auskommentiert: Wenn factions gelöscht werden, zeigen
  * Spendenpointer ins Leere. */
  /* remove_empty_factions(); */
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
  {
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
  attrib *a;
  int i;
  const char *s;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (!*s) {
    a = a_find(u->attribs, &at_group);
    if (a) {
      a_removeall(&((group*)a->data.v)->attribs, &at_raceprefix);
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
  a = a_find(u->attribs, &at_group);
  if (a) ap = &((group*)a->data.v)->attribs;
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
        const struct locale * lang = locales;
        for (;lang;lang=nextlocale(lang)) {
          const char * bdname = LOC(lang, b->type->_name);
          size_t bdlen = strlen(bdname);
          if (strlen(b->name)>=bdlen && strncmp(b->name, bdname, bdlen)==0) {
            break;
          }
        }
        if (lang==NULL) {
          cmistake(u, ord, 246, MSG_EVENT);
          break;
        }
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
      sprintf(buf, "Monument %d", b->no);
      if (b->type == bt_find("monument")
        && !strcmp(b->name, buf)) {
          cmistake(u, ord, 29, MSG_EVENT);
          break;
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
        add_message(&f->msgs, new_message(f,
          "renamed_faction_seen%u:renamer%r:region", u, r));
      } else {
        add_message(&f->msgs, new_message(f,
          "renamed_faction_notseen%r:region", r));
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
        add_message(&u2->faction->msgs, new_message(u2->faction,
          "renamed_seen%u:renamer%u:renamed%r:region", u, u2, r));
      } else {
        add_message(&u2->faction->msgs, new_message(u2->faction,
          "renamed_notseen%u:renamed%r:region", u2, r));
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
      attrib * a = a_find(u->attribs, &at_group);
      if (a){
        group * g = (group*)a->data.v;
        s= &g->name;
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

static void
deliverMail(faction * f, region * r, unit * u, const char *s, unit * receiver)
{
  if (!receiver) { /* BOTSCHAFT an PARTEI */
    char * message = (char*)gc_add(strdup(s));
    add_message(&f->msgs,
      msg_message("unitmessage", "region unit string", r, u, message));
  }
  else {          /* BOTSCHAFT an EINHEIT */
    unit *emp = receiver;
    if (cansee(f, r, u, 0))
      sprintf(buf, "Eine Botschaft von %s: '%s'", unitname(u), s);
    else
      sprintf(buf, "Eine anonyme Botschaft: '%s'", s);
    addstrlist(&emp->botschaften, strdup(buf));
  }
}

static int
prepare_mail_cmd(unit * u, struct order * ord)
{
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
    break;
  case P_FACTION:
    break;
  case P_UNIT:
    {
      region *r = u->region;
      unit *u2;
      boolean see = false;

      n = getid();

      for (u2=r->units; u2; u2=u2->next) {
        if (u2->no == n && cansee(u->faction, r, u2, 0)) {
          see = true;
          break;
        }
      }

      if (see == false) {
        break;
      }

      s = getstrtoken();
      if (!s[0]) {
        break;
      }
      u2 = findunitr(r,n);
      if(u2 && cansee(u->faction, r, u2, 0)) {
        handle_event_va(&u2->attribs, "message", "string unit", s, u);
      }
    }
    break;
  case P_BUILDING:
  case P_GEBAEUDE:
    break;
  case P_SHIP:
    break;
  }
  return 0;
}

static void
mailunit(region * r, unit * u, int n, struct order * ord, const char * s)
{
  unit * u2 = findunitr(r,n);

  if (u2 && cansee(u->faction, r, u2, 0)) {
    deliverMail(u2->faction, r, u, s, u2);
    /* now done in prepare_mail_cmd */
    /* handle_event_va(&u2->attribs, "message", "string unit", s, u); */
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
      }
      mailunit(r, u, n, ord, s);
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

static void
report_option(unit * u, const char * sec, struct order * ord)
{
  const messageclass * mc;
  const char *s;

  mc = mc_find(sec);

  if (mc == NULL) {
    cmistake(u, ord, 135, MSG_EVENT);
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

static int
banner_cmd(unit * u, struct order * ord)
{
  init_tokens(ord);
  skip_token();

  set_string(&u->faction->banner, getstrtoken());
  add_message(&u->faction->msgs, new_message(u->faction,
    "changebanner%s:value", gc_add(strdup(u->faction->banner))));

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
      ADDMSG(&f->msgs, msg_message("changemail_invalid", "value",
        gc_add(strdup(s))));
    } else {
      ADDMSG(&f->msgs, msg_message("changemail", "value",
        gc_add(strdup(f->email))));
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
      cmistake(u, ord, 283, MSG_EVENT);
      for(i=0; i<6; i++) pbuf[i] = (char)(97 + rand() % 26);
      pbuf[6] = 0;
    }
  }
  set_string(&u->faction->passw, pbuf);
  fset(u->faction, FFL_OVERRIDE);
  ADDMSG(&u->faction->msgs, msg_message("changepasswd",
    "value", gc_add(strdup(u->faction->passw))));
  return 0;
}

static int
report_cmd(unit * u, struct order * ord)
{
  const char * s;
  int i;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  i = atoi(s);
  sprintf(buf, "%d", i);
  if (!strcmp(buf, s)) {
    /* int level;
    level = geti();
    not implemented yet. set individual levels for f->msglevels */
  } else {
    report_option(u, s, ord);
  }
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

static void
set_passw(void)
{
  region *r;
  puts(" - setze Passwörter, Adressen und Format, Abstimmungen");

  for (r = regions; r; r = r->next) {
    unit *u;
    for (u = r->units; u; u = u->next) {
      struct order * ord;
      for (ord = u->orders; ord; ord = ord->next) {
        switch (get_keyword(ord)) {
        case NOKEYWORD:
          cmistake(u, ord, 22, MSG_EVENT);
          break;

        case K_BANNER:
          if (banner_cmd(u, ord)!=0) ord = NULL;
          break;

        case K_EMAIL:
          if (email_cmd(u, ord)!=0) ord = NULL;
          break;

        case K_PASSWORD:
          if (password_cmd(u, ord)!=0) ord = NULL;
          break;

        case K_REPORT:
          if (report_cmd(u, ord)!=0) ord = NULL;
          break;

        case K_SEND:
          if (send_cmd(u, ord)!=0) ord = NULL;
          break;
        }
      }
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
  const char *info;
  const char *key;

  if (u && *i_find(&u->items, itype) == NULL) return false;

  name = resourcename(itype->rtype, 0);
  key = mkname("iteminfo", name);
  info = locale_string(f->locale, key);

  if (info==key || strcmp(info, key)==0) {
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
    if (fp!=NULL) {
      buf[0]='\0';
      while (fgets(t, NAMESIZE, fp) != NULL) {
        if (t[strlen(t) - 1] == '\n') {
          t[strlen(t) - 1] = 0;
        }
        strcat(buf, t);
      }
      fclose(fp);
      info = buf;
    } else {
      info = "Keine Informationen.";
    }
  }
  ADDMSG(&f->msgs, msg_message("displayitem", "weight item description",
    itype->weight/1000, itype->rtype, strdup(info)));

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
  money = get_all(u, i_silver->rtype);
  people = count_all(u->faction) * u->number;

  if (people>money) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_cost", "cost have",
      people, money));
    return 0;
  }
  use_all(u, i_silver->rtype, people);
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
  int px, py;

  init_tokens(ord);
  skip_token();

  px = atoi(getstrtoken());
  py = atoi(getstrtoken());

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

  spell = find_spellbyname(u, s, u->faction->locale);

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

static void
instant_orders(void)
{
  region *r;
  faction *f;

  puts(" - Kontakte, Hilfe, Status, Kampfzauber, Texte, Bewachen (aus), Zeigen");

  for (f = factions; f; f = f->next) {
    attrib *a;
#ifdef NEW_ITEMS
    a = a_find(f->attribs, &at_showitem);
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
    a = a_find(f->attribs, &at_show_item_description);
    while (a!=NULL) {
      display_item(f, NULL, (item_t)(a->data.i));
      a_remove(&f->attribs, a);
    }
#endif
  }

  parse(K_GROUP, group_cmd, false);
  for (r = regions; r; r = r->next) {
    unit * u;
    for (u = r->units; u; u = u->next) {
      order * ord;
      freset(u, UFL_MOVED);
      for (ord = u->orders; ord; ord = ord->next) {
        switch (get_keyword(ord)) {
        case K_URSPRUNG:
          if (origin_cmd(u, ord)!=0) ord=NULL;
          break;
        case K_ALLY:
          if (ally_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_PREFIX:
          if (prefix_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_SYNONYM:
          if (synonym_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_SETSTEALTH:
          if (setstealth_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_WEREWOLF:
          if (setwere_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_STATUS:
          /* KAEMPFE [ NICHT | AGGRESSIV | DEFENSIV | HINTEN | FLIEHE ] */
          if (status_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_COMBAT:
          /* KAMPFZAUBER [[STUFE n] "<Spruchname>"] [NICHT] */
          if (combatspell_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_DISPLAY:
          if (display_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_NAME:
          if (name_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_GUARD:
          if (guard_off_cmd(u, ord)!=0) ord = NULL;
          break;
        case K_RESHOW:
          if (reshow_cmd(u, ord)!=0) ord = NULL;
          break;
        }
      }
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
      if (getguard(u) && (!armedmen(u) || u->faction->age < NewbieImmunity()))
        setguard(u, GUARD_NONE);
    }
}

static int
guard_on_cmd(unit * u, struct order * ord)
{
  if (fval(u, UFL_MOVED)) return 0;
  assert(get_keyword(ord)==K_GUARD);

  init_tokens(ord);
  skip_token();

  /* GUARD NOT is handled in goard_off_cmd earlier in the turn */
  if (getparam(u->faction->locale) == P_NOT) return 0;

  if (rterrain(u->region) != T_OCEAN) {
    if (!fval(u, RCF_ILLUSIONARY) && u->race != new_race[RC_SPELL]) {
      /* Monster der Monsterpartei dürfen immer bewachen */
      if (u->faction == findfaction(MONSTER_FACTION)) {
        guard(u, GUARD_ALL);
      } else if (!armedmen(u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "unit_unarmed", ""));
      } else {
        guard(u, GUARD_ALL);
      }
    } else {
      cmistake(u, ord, 95, MSG_EVENT);
    }
  } else {
    cmistake(u, ord, 2, MSG_EVENT);
  }
  return 0;
}

static void
sinkships(void)
{
  region *r;

  /* Unbemannte Schiffe können sinken */
  for (r = regions; r; r = r->next) {
    ship *sh;

    list_foreach(ship, r->ships, sh) {
      if (rterrain(r) == T_OCEAN && (!enoughsailors(sh, r) || get_captain(sh)==NULL)) {
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

static void
renumber(void)
{
  region *r;
  const char *s;
  unit * u;
  int i;

  for (r=regions;r;r=r->next) {
    for (u=r->units;u;u=u->next) {
      faction * f = u->faction;
      struct order * ord;
      for (ord = u->orders; ord; ord = ord->next) if (get_keyword(ord)==K_NUMBER) {
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
            if(s == NULL || *s == 0) {
              i = newunitid();
            } else {
              i = atoi36(s);
              if (i<=0 || i>MAX_UNIT_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                continue;
              }

              if(forbiddenid(i)) {
                cmistake(u, ord, 116, MSG_EVENT);
                continue;
              }

              if(findunitg(i, r)) {
                cmistake(u, ord, 115, MSG_EVENT);
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
              cmistake(u,ord,144,MSG_EVENT);
              continue;
            }
            if(!fval(u, UFL_OWNER)) {
              cmistake(u,ord,146,MSG_EVENT);
              continue;
            }
            s = getstrtoken();
            if(s == NULL || *s == 0) {
              i = newcontainerid();
            } else {
              i = atoi36(s);
              if (i<=0 || i>MAX_CONTAINER_NR) {
                cmistake(u,ord,114,MSG_EVENT);
                continue;
              }
              if (findship(i) || findbuilding(i)) {
                cmistake(u,ord,115,MSG_EVENT);
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
              cmistake(u,ord,145,MSG_EVENT);
              continue;
            }
            if(!fval(u, UFL_OWNER)) {
              cmistake(u,ord,148,MSG_EVENT);
              continue;
            }
            s = getstrtoken();
            if(*s == 0) {
              i = newcontainerid();
            } else {
              i = atoi36(s);
              if (i<=0 || i>MAX_CONTAINER_NR) {
                cmistake(u,ord,114,MSG_EVENT);
                continue;
              }
              if(findship(i) || findbuilding(i)) {
                cmistake(u,ord,115,MSG_EVENT);
                continue;
              }
            }
            bunhash(u->building);
            u->building->no = i;
            bhash(u->building);
            break;

          default:
            cmistake(u,ord,239,MSG_EVENT);
        }
      }
    }
  }
  renumber_factions();
}

static building *
age_building(building * b)
{
  static const building_type * bt_blessed = (const building_type*)0xdeadbeef;
  static const curse_type * ct_astralblock = NULL;
  if (bt_blessed==(const building_type*)0xdeadbeef) {
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
              change_item(u, I_UNICORN, 1);
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
          /* the mage reactivates the circle */
          c = create_curse(mage, &rt->attribs, ct_astralblock,
            sk, sk/2, 100, 0);
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
  handle_event(&b->attribs, "timer", b);

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
      b = age_building(b);

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
            char * name;
            int g, alias;
            int mu = maxunits(u->faction);
            order ** newordersp;

            if(u->faction->no_units >= mu) {
              sprintf(buf, "Eine Partei darf aus nicht mehr als %d "
                "Einheiten bestehen.", mu);
              mistake(u, makeord, buf, MSG_PRODUCE);
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

            name = strdup(getstrtoken());
            if (name && strlen(name)==0) name = NULL;
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
setdefaults (void)
{
  region *r;

  for (r = regions; r; r = r->next) {
    unit *u;

    for (u = r->units; u; u = u->next) {
      order *ord;
      boolean trade = false;

      if (LongHunger(u)) {
        /* Hungernde Einheiten führen NUR den default-Befehl aus */
        set_order(&u->thisorder, default_order(u->faction->locale));
        continue;
      }
#ifdef LASTORDER
      /* by default the default long order becomes the new long order. */
      u->thisorder = copy_order(u->lastorder);
#endif
      /* check all orders for a potential new long order this round: */
      for (ord = u->orders; ord; ord = ord->next) {
#ifndef LASTORDER
        if (u->old_orders && is_repeated(ord)) {
          /* this new order will replace the old defaults */
          free_orders(&u->old_orders);
        }
#endif
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
  }
}

static int
use_item(unit * u, const item_type * itype, int amount, struct order * ord)
{
  int i;
  int target = read_unitid(u->faction, u->region);

  i = new_get_pooled(u, itype->rtype, GET_DEFAULT);

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
      if (fspecial(u->faction, FS_UNDEAD)) continue;

      if (rterrain(r)==T_OCEAN && u->ship==NULL && !canswim(u)) continue;

      if(fspecial(u->faction, FS_REGENERATION)) {
        u->hp = umhp;
        continue;
      }

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

#ifdef LASTORDER
static void
defaultorders (void)
{
  region *r;
  for (r=regions;r;r=r->next) {
    unit *u;
    for (u=r->units;u;u=u->next) {
      order ** ordp = &u->orders;
      while (*ordp!=NULL) {
        order * ord = *ordp;
        if (get_keyword(ord)==K_DEFAULT) {
          char * cmd;
          init_tokens(ord);
          skip_token(); /* skip the keyword */
          cmd = strdup(getstrtoken());
          set_order(&u->lastorder, parse_order(cmd, u->faction->locale));
          free(cmd);
          *ordp = ord->next;
          ord->next = NULL;
          free_order(ord);
        }
        else ordp = &ord->next;
      }
    }
  }
}
#endif

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
    if (f->age < NewbieImmunity()) {
      add_message(&f->msgs, new_message(f,
        "newbieimmunity%i:turns", NewbieImmunity() - f->age));
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
    n = 1;
  } else {
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

void
processorders (void)
{
  region *r;

  set_passw();    /* und pruefe auf illegale Befehle */

  puts(" - neue Einheiten erschaffen...");
  new_units();

  puts(" - Defaults und Instant-Befehle...");
  setdefaults();
  instant_orders();

  if (alliances!=NULL) alliancekick();

  parse(K_MAIL, prepare_mail_cmd, false);
  parse(K_MAIL, mail_cmd, false);
  puts(" - Parteien altern");
  age_factions();

  puts(" - Benutzen");
  parse(K_CLAIM, claim_cmd, false);
  parse(K_USE, use_cmd, false);

  puts(" - Kontaktieren, Betreten von Schiffen und Gebäuden (1.Versuch)");
  do_misc(false);

  if (alliances!=NULL) {
    puts(" - Testen der Allianzbedingungen");
    alliancevictory();
  }

  puts(" - GM Kommandos");
#ifdef INFOCMD_MODULE
  infocommands();
#endif
  gmcommands();

  puts(" - Verlassen");
  parse(K_LEAVE, leave_cmd, false);

  puts(" - Kontakte löschen");
  remove_contacts();

#ifdef KARMA_MODULE
  puts(" - Jihad-Angriffe");
  jihad_attacks();
#endif

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
  parse_quit();

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
  parse(K_GUARD, guard_on_cmd, false);

  puts(" - Zufallsbegegnungen");
  encounters();

  if (turn == 0) srand(time((time_t *) NULL));
  else srand(turn);

  puts(" - Monster fressen und vertreiben Bauern");
  monsters_kill_peasants();

  puts(" - random events");
  randomevents();

  puts(" - newspaper commands");
#ifdef XECMD_MODULE
  xecmd();
#endif

  puts(" - regeneration (healing & aura)");
  monthly_healing();
  regeneration_magiepunkte();

#ifdef LASTORDER
  puts(" - Defaults setzen");
  defaultorders();
#endif

  puts(" - Unterhaltskosten, Nachfrage, Seuchen, Wachstum, Auswanderung");
  demographics();

  puts(" - Gebäudeunterhalt (2. Versuch)");
  maintain_buildings(true);

#ifdef KARMA_MODULE
  puts(" - Jihads setzen");
  karma();
#endif

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

#ifdef HEROES
  puts(" - Heldenbeförderung");
  parse(K_PROMOTION, promotion_cmd, false);
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
