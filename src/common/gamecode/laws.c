/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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
#include <kernel/eressea.h>
#include "laws.h"

#include <modules/gmcmd.h>
#include <modules/infocmd.h>
#include <modules/wormhole.h>

/* gamecode includes */
#include "economy.h"
#include "monster.h"
#include "randenc.h"
#include "spy.h"
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
#include <kernel/move.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/spell.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h> /* for volcanoes in emigration (needs a flag) */
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/object.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/sql.h>
#include <util/umlaut.h>
#include <util/message.h>
#include <util/rng.h>

#include <modules/xecmd.h>

#ifdef AT_OPTION
/* attributes includes */
#include <attributes/option.h>
#endif

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string.h>
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

  if (verbosity>=1) puts(" - Warne spaete Spieler...");
  for (f = factions; f; f = f->next)
    if (!is_monsters(f) && turn - f->lastorders == NMRTimeout() - 1)
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
  faction * owner = region_get_owner(r);

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
      int use = MIN(rm, need);
      rsetmoney(r, rm-use);
      need -= use;
    }

    need -= get_money(u);
    if (need > 0) {
      unit *v;

      for (v = r->units; need && v; v = v->next) {
        if (v->faction == u->faction && help_money(v)) {
          int give = get_money(v) - lifestyle(v);
          give = MIN(need, give);
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

    need -= MAX(0, get_money(u));

    if (need > 0) {
      unit *v;

      for (v = r->units; need && v; v = v->next) {
        if (v->faction != f && alliedunit(v, f, HELP_MONEY) && help_money(v)) {
          int give = get_money(v) - lifestyle(v);
          give = MIN(need, give);

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
            blut = MIN(blut, hungry);
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
    int need = MIN(get_money(u), lifestyle(u));
    change_money(u, -need);
  }
}

static void
age_unit(region * r, unit * u)
{
  if (u->race == new_race[RC_SPELL]) {
    if (--u->age <= 0) {
      remove_unit(&r->units, u);
    }
  } else {
    ++u->age;
    if (u->number>0 && u->race->age) {
      u->race->age(u);
    }
  }
#ifdef ASTRAL_ITEM_RESTRICTIONS
  if (u->region && is_astral(u->region)) {
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
    /* IUW: age_unit() kann u loeschen, u->next ist dann
    * undefiniert, also muessen wir hier schon das nächste
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
        int weeks = MIN(effect, u->number);
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

#define MAX_EMIGRATION(p) ((p)/MAXDIRECTIONS)
#define MAX_IMMIGRATION(p) ((p)*2/3)

static void
calculate_emigration(region *r)
{
  int i;
  int maxp = maxworkingpeasants(r);
  int rp = rpeasants(r);
  int max_immigrants = MAX_IMMIGRATION(maxp-rp);

  if (r->terrain == newterrain(T_VOLCANO) || r->terrain == newterrain(T_VOLCANO_SMOKING)) {
    max_immigrants = max_immigrants/10;
  }

  for (i = 0; max_immigrants>0 && i != MAXDIRECTIONS; i++) {
    int dir = (turn+i) % MAXDIRECTIONS;
    region *rc = rconnect(r, (direction_t)dir);

    if (rc != NULL && fval(rc->terrain, LAND_REGION)) {
      int rp2 = rpeasants(rc);
      int maxp2 = maxworkingpeasants(rc);
      int max_emigration = MAX_EMIGRATION(rp2-maxp2);
      
      if (max_emigration>0) {
        max_emigration = MIN(max_emigration, max_immigrants);
        r->land->newpeasants += max_emigration;
        rc->land->newpeasants -= max_emigration;
        max_immigrants -= max_emigration;
      }
    }
  }
}

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

  satiated = MIN(peasants, money / maintenance_cost(NULL));
  rsetmoney(r, money - satiated * maintenance_cost(NULL));

  /* Von denjenigen, die nicht satt geworden sind, verhungert der
   * Großteil. dead kann nie größer als rpeasants(r) - satiated werden,
   * so dass rpeasants(r) >= 0 bleiben muß. */

  /* Es verhungert maximal die unterernährten Bevölkerung. */

  n = MIN(peasants - satiated, rpeasants(r));
    dead += (int)(0.5F + n * PEASANT_STARVATION_CHANCE);

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
  maxhorses = MAX(0, maxhorses);
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
      pt = MAX(0, pt);
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

extern struct attrib_type at_germs;

static void
trees(region * r, const int current_season, const int last_weeks_season)
{
  int growth, grownup_trees, i, seeds, sprout;
  direction_t d;
  attrib *a;

  if(current_season == SEASON_SUMMER || current_season == SEASON_AUTUMN) {
    double seedchance = 0.01F * RESOURCE_QUANTITY;
    int elves = count_race(r,new_race[RC_ELF]);

    a = a_find(r->attribs, &at_germs);
    if(a && last_weeks_season == SEASON_SPRING) {
      /* ungekeimte Samen bleiben erhalten, Sprößlinge wachsen */
      sprout = MIN(a->data.sa[1], rtrees(r, 1));
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
    /* Jeder Elf in der Region erhöht die Chance um 0.0008%. */
    seedchance += (MIN(elves, (production(r)*MAXPEASANTS_PER_AREA)/8)) * 0.0008;
    grownup_trees = rtrees(r, 2);
    seeds = 0;

    if (grownup_trees>0) {
      double remainder = seedchance*grownup_trees;
      seeds = (int)(remainder);
      remainder -= seeds;
      if (chance(remainder)) {
        ++seeds;
      }
      if (seeds>0) {
        seeds += rtrees(r, 0);
        rsettrees(r, 0, seeds);
      }
    }

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
    seeds = MIN(a->data.sa[0], rtrees(r, 0));
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
    sprout = MIN(a->data.sa[1], rtrees(r, 1));
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
    gamedate date;
    get_gamedate(turn, &date);
    current_season = date.season;
    get_gamedate(turn-1, &date);
    last_weeks_season = date.season;
  }

  for (r = regions; r; r = r->next) {
    ++r->age; /* also oceans. no idea why we didn't always do that */
    live(r);
    /* check_split_dragons(); */

    if (!fval(r->terrain, SEA_REGION)) {
      /* die Nachfrage nach Produkten steigt. */
      struct demand * dmd;
      if (r->land) {
        for (dmd=r->land->demands;dmd;dmd=dmd->next) {
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
        if (current_season != SEASON_WINTER) {
          trees(r, current_season, last_weeks_season);
        }
      }

      update_resources(r);
      if (r->land) migrate(r);
    }
  }
  while (free_migrants) {
    migration * m = free_migrants->next;
    free(free_migrants);
    free_migrants = m;
  };
  if (verbosity>=1) putchar('\n');

  remove_empty_units();

  if (verbosity>=1) puts(" - Einwanderung...");
  for (r = regions; r; r = r->next) {
    if (r->land && r->land->newpeasants) {
      int rp = rpeasants(r) + r->land->newpeasants;
      rsetpeasants(r, MAX(0, rp));
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
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_onlandonly", ""));
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

    if (!checkpasswd(u->faction, (const char *)s_pass, false)) {
      cmistake(u, ord, 86, MSG_EVENT);
      log_warning(("RESTART with wrong password, faction %s, pass %s\n",
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
  if (checkpasswd(f, (const char *)passwd, false)) {
    if (EnhancedQuit()) {
      int f2_id = getid();
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
          variant var;
          var.i = f2_id;
          object_create("quit", TINTEGER, var);
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
        attrib * a = a_find(f->attribs, &at_object);
        if (a) {
          variant var;
          object_type type;
          var.i = 0;
          object_get(a, &type, &var);
          assert(var.i && type==TINTEGER);
          if (var.i) {
            int f2_id = var.i;
            faction *f2 = findfaction(f2_id);

            assert(f2_id>0);
            assert(f2!=NULL);
            transfer_faction(f, f2);
          }
        }
      }
      destroyfaction(f);
    }
    if (*fptr==f) fptr=&f->next;
  }
}

int dropouts[2];
int * age = NULL;

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

  if (verbosity>=1) puts(" - beseitige Spieler, die sich zu lange nicht mehr gemeldet haben...");

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
  if (verbosity>=1) {
    puts(" - beseitige Spieler, die sich nach der Anmeldung nicht "
      "gemeldet haben...");
  }

  age = calloc(MAX(4,turn+1), sizeof(int));
  for (f = factions; f; f = f->next) if (!is_monsters(f)) {
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

  if (verbosity>=1) puts(" - beseitige leere Einheiten und leere Parteien...");
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

  if (f==NULL || is_monsters(f)) {
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

  case P_GIVE:
    if (not_kw == P_NOT)
      sf->status = sf->status & (HELP_ALL - HELP_GIVE);
    else
      sf->status = sf->status | HELP_GIVE;
    break;

  case P_MONEY:
    if (not_kw == P_NOT)
      sf->status = sf->status & (HELP_ALL - HELP_MONEY);
    else
      sf->status = sf->status | HELP_MONEY;
    break;

  case P_FIGHT:
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

  sf->status &= HelpMask();

  if (sf->status == 0) {    /* Alle HELPs geloescht */
    removelist(sfp, sf);
  }
  return 0;
}

static struct local_names * pnames;

static void
init_prefixnames(void)
{
  int i;
  for (i=0;localenames[i];++i) {
    const struct locale * lang = find_locale(localenames[i]);
    boolean exist = false;
    struct local_names * in = pnames;

    while (in!=NULL) {
      if (in->lang==lang) {
        exist = true;
        break;
      }
      in = in->next;
    }
    if (in==NULL) in = calloc(sizeof(local_names), 1);
    in->next = pnames;
    in->lang = lang;

    if (!exist) {
      int key;
      for (key=0;race_prefixes[key];++key) {
        variant var;
        const char * pname = locale_string(lang, mkname("prefix", race_prefixes[key]));
        if (findtoken(&in->names, pname, &var)==E_TOK_NOMATCH || var.i!=key) {
          var.i = key;
          addtoken(&in->names, pname, var);
          addtoken(&in->names, locale_string(lang, mkname("prefix", race_prefixes[key])), var);
        }
      }
    }
    pnames = in;
  }
}

static int
prefix_cmd(unit * u, struct order * ord)
{
  attrib **ap;
  const char *s;
  local_names * in = pnames;
  variant var;
  const struct locale * lang = u->faction->locale;

  while (in!=NULL) {
    if (in->lang==lang) break;
    in = in->next;
  }
  if (in==NULL) {
    init_prefixnames();
    for (in=pnames;in->lang!=lang;in=in->next) ;
  }

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

  if (findtoken(&in->names, s, &var)==E_TOK_NOMATCH) {
    return 0;
  } else if (race_prefixes[var.i] == NULL) {
    cmistake(u, ord, 299, MSG_EVENT);
  } else {
    ap = &u->faction->attribs;
    if (fval(u, UFL_GROUP)) {
      attrib * a = a_find(u->attribs, &at_group);
      group * g = (group*)a->data.v;
      if (a) ap = &g->attribs;
    }
    set_prefix(ap, race_prefixes[var.i]);
  }
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
    if (!fval(b->type, BTF_NAMECHANGE) && b->display && b->display[0] != 0) {
      cmistake(u, ord, 278, MSG_EVENT);
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
    if (b != largestbuilding(r, &is_castle, false)) {
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

    free(*s);
    *s = strdup(s2);
    if (strlen(s2)>=DISPLAYSIZE) {
      (*s)[DISPLAYSIZE] = 0;
    }
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
      if (!fval(b->type, BTF_NAMECHANGE)) {
        cmistake(u, ord, 278, MSG_EVENT);
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
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
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
    if (b != largestbuilding(r, &is_castle, false)) {
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

    /* TODO: Validate to make sure people don't have illegal characters in
     * names, phishing-style? () come to mind. */

    free(*s);
    *s = strdup(s2);
    if (strlen(s2)>=NAMESIZE) {
      (*s)[NAMESIZE] = 0;
    }
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
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
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
  int n, cont;

  init_tokens(ord);
  skip_token(); /* skip the keyword */
  s = getstrtoken();

  /* Falls kein Parameter, ist das eine Einheitsnummer;
  * das Füllwort "AN" muß wegfallen, da gültige Nummer! */

  do {
    cont = 0;
    switch (findparam(s, u->faction->locale)) {
    case P_REGION:
      /* können alle Einheiten in der Region sehen */
      s = getstrtoken();
      if (!s[0]) {
        cmistake(u, ord, 30, MSG_MESSAGE);
        break;
      } else {
        ADDMSG(&r->msgs, msg_message("mail_result", "unit message", u, s));
        return 0;
      }

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
        return 0;
      }

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
          ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
          return 0;
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
        return 0;
      }

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

        for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);

        for (u2=r->units; u2; u2=u2->next) {
          if(u2->building == b && !fval(u2->faction, FFL_SELECT)
            && cansee(u->faction, r, u2, 0)) {
              mailunit(r, u, u2->no, ord, s);
              fset(u2->faction, FFL_SELECT);
            }
        }
        return 0;
      }

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

        for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FFL_SELECT);

        for(u2=r->units; u2; u2=u2->next) {
          if(u2->ship == sh && !fval(u2->faction, FFL_SELECT) && cansee(u->faction, r, u2, 0)) {
            mailunit(r, u, u2->no, ord, s);
            fset(u2->faction, FFL_SELECT);
          }
        }
        return 0;
      }

    default:
      /* possibly filler token? */
      s = getstrtoken();
      if (s && *s) cont = 1;
      break;
    }
  } while (cont);
  cmistake(u, ord, 149, MSG_MESSAGE);
  return 0;
}
/* ------------------------------------------------------------- */

static int
banner_cmd(unit * u, struct order * ord)
{
  init_tokens(ord);
  skip_token();

  free(u->faction->banner);
  u->faction->banner = strdup(getstrtoken());
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
    if (set_email(&f->email, (const char *)s)!=0) {
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
  char pwbuf[32];
  int i;
  const char * s;
  boolean pwok = true;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (!s || !*s) {
    for(i=0; i<6; i++) pwbuf[i] = (char)(97 + rng_int() % 26);
    pwbuf[6] = 0;
  } else {
    char *c;

    strlcpy(pwbuf, (const char *)s, 31);
    pwbuf[31] = 0;
    c = pwbuf;
    while (*c && pwok) {
      if (!isalnum(*(unsigned char*)c)) pwok = false;
      c++;
    }
  }
  free(u->faction->passw);
  if (pwok == false) {
    cmistake(u, ord, 283, MSG_EVENT);
    u->faction->passw = strdup(itoa36(rng_int()));
  } else {
    u->faction->passw = strdup(pwbuf);
  }
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
        u->faction->options = u->faction->options & ~(1<<option);
      }
    } else {
      u->faction->options = u->faction->options | (1<<option);
      if(option == O_COMPRESS) u->faction->options &= ~(1<<O_BZIP2);
      if(option == O_BZIP2) u->faction->options &= ~(1<<O_COMPRESS);
    }
  }
  return 0;
}

static boolean
display_item(faction *f, unit *u, const item_type * itype)
{
  const char *name;
  const char *key;
  const char *info;

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
  info = locale_getstring(f->locale, key);

  if (info==NULL) {
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
  const char *name, *key;
  const char *info;
  int a, at_count;
  char buf[2048], * bufp = buf;
  size_t size = sizeof(buf) - 1;
  int bytes;

  if (u && u->race != rc) return false;
  name = rc_name(rc, 0);

  bytes = slprintf(bufp, size, "%s: ", LOC(f->locale, name));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  key = mkname("raceinfo", rc->_name[0]);
  info = locale_getstring(f->locale, key);
  if (info==NULL) {
    info = locale_string(f->locale, mkname("raceinfo", "no_info"));
  }

  bytes = (int)strlcpy(bufp, info, size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  /* hp_p : Trefferpunkte */
  bytes = snprintf(bufp, size, " %d %s", rc->hitpoints, LOC(f->locale, "stat_hitpoints"));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  /* b_attacke : Angriff */
  bytes = snprintf(bufp, size, ", %s: %d", LOC(f->locale, "stat_attack"), (rc->at_default+rc->at_bonus));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  /* b_defense : Verteidigung */
  bytes = snprintf(bufp, size, ", %s: %d", LOC(f->locale, "stat_defense"), (rc->df_default+rc->df_bonus));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  /* b_armor : Rüstung */
  if (rc->armor > 0) {
    bytes = snprintf(bufp, size, ", %s: %d", LOC(f->locale, "stat_armor"), rc->armor);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }

  if (size>1) {
    *bufp++ ='.';
    --size;
  } else WARN_STATIC_BUFFER();

  /* b_damage : Schaden */
  at_count=0;
  for (a = 0; a < 6; a++) {
    if (rc->attack[a].type != AT_NONE){
      at_count++;
    }
  }
  if (rc->battle_flags & BF_EQUIPMENT) {
    bytes = snprintf(bufp, size, " %s", LOC(f->locale, "stat_equipment"));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  if (rc->battle_flags & BF_RES_PIERCE) {
    bytes = snprintf(bufp, size, " %s", LOC(f->locale, "stat_pierce"));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  if (rc->battle_flags & BF_RES_CUT) {
    bytes = snprintf(bufp, size, " %s", LOC(f->locale, "stat_cut"));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  if (rc->battle_flags & BF_RES_BASH) {
    bytes = snprintf(bufp, size, " %s", LOC(f->locale, "stat_bash"));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }

  bytes = snprintf(bufp, size, " %d %s", at_count, LOC(f->locale, (at_count==1)?"stat_attack":"stat_attacks"));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  for (a = 0; a < 6; a++) {
    if (rc->attack[a].type != AT_NONE){
      if (a!=0) bytes = (int)strlcpy(bufp, ", ", size);
      else bytes = (int)strlcpy(bufp, ": ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

      switch(rc->attack[a].type) {
      case AT_STANDARD:
        bytes = snprintf(bufp, size, "%s (%s)", LOC(f->locale, "attack_standard"), rc->def_damage);
        break;
      case AT_NATURAL:
        bytes = snprintf(bufp, size, "%s (%s)", LOC(f->locale, "attack_natural"), rc->attack[a].data.dice);
        break;
      case AT_SPELL:
      case AT_COMBATSPELL:
      case AT_DRAIN_ST:
      case AT_DAZZLE:
        bytes = snprintf(bufp, size, "%s", LOC(f->locale, "attack_magical"));
        break;
      case AT_STRUCTURAL:
        bytes = snprintf(bufp, size, "%s (%s)", LOC(f->locale, "attack_structural"), rc->attack[a].data.dice);
        break;
      default:
        bytes = 0;
      }

      if (bytes && wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }

  if (size>1) {
    *bufp++ = '.';
    --size;
  } else WARN_STATIC_BUFFER();

  *bufp = 0;
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

static int
promotion_cmd(unit * u, struct order * ord)
{
  int money, people;

  if (fval(u, UFL_HERO)) {
    /* TODO: message "is already a hero" */
    return 0;
  }

  if (maxheroes(u->faction) < countheroes(u->faction)+u->number) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "heroes_maxed", "max count",
      maxheroes(u->faction), countheroes(u->faction)));
    return 0;
  }
  if (u->race!=u->faction->race) {
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

  px = (short)getint();
  py = (short)getint();

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
    setstatus(u, ST_AVOID);
    break;
  case P_BEHIND:
    setstatus(u, ST_BEHIND);
    break;
  case P_FLEE:
    setstatus(u, ST_FLEE);
    break;
  case P_CHICKEN:
    setstatus(u, ST_CHICKEN);
    break;
  case P_AGGRO:
    setstatus(u, ST_AGGRO);
    break;
  case P_VORNE:
    setstatus(u, ST_FIGHT);
    break;
  case P_HELP:
    if (getparam(u->faction->locale) == P_NOT) {
      fset(u, UFL_NOAID);
    } else {
      freset(u, UFL_NOAID);
    }
    break;
  default:
    if (param[0]) {
      add_message(&u->faction->msgs,
        msg_feedback(u, ord, "unknown_status", ""));
    } else {
      setstatus(u, ST_FIGHT);
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
    level = getint();
    level = MAX(0, level);
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

enum { E_GUARD_OK, E_GUARD_UNARMED, E_GUARD_NEWBIE, E_GUARD_FLEEING };

static int
can_start_guarding(const unit * u)
{
  if (u->status>=ST_FLEE) return E_GUARD_FLEEING;
  if (fval(u->race, RCF_UNARMEDGUARD)) return E_GUARD_OK;
  if (!armedmen(u, true)) return E_GUARD_UNARMED;
  if (u->faction->age < NewbieImmunity()) return E_GUARD_NEWBIE;
  return E_GUARD_OK;
}

void
update_guards(void)
{
  const region *r;

  for (r = regions; r; r = r->next) {
    unit *u;
    for (u = r->units; u; u = u->next) {
      if (can_start_guarding(u)!=E_GUARD_OK) {
        setguard(u, GUARD_NONE);
      }
    }
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
      if (is_monsters(u->faction)) {
        guard(u, GUARD_ALL);
      } else {
        int err = can_start_guarding(u);
        if (err==E_GUARD_OK) {
          guard(u, GUARD_ALL);
        } else if (err==E_GUARD_UNARMED) {
          ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "unit_unarmed", ""));
        } else if (err==E_GUARD_FLEEING) {
          cmistake(u, ord, 320, MSG_EVENT);
        } else if (err==E_GUARD_NEWBIE) {
          cmistake(u, ord, 304, MSG_EVENT);
        }
      }
    }
  }
  return 0;
}

static void
sinkships(region * r)
{
  ship **shp = &r->ships;

  while (*shp) {
    ship * sh = *shp;
    if (fval(r->terrain, SEA_REGION) && (!enoughsailors(sh, r) || get_captain(sh)==NULL)) {
      /* Schiff nicht seetüchtig */
      damage_ship(sh, 0.30);
    }
    if (shipowner(sh)==NULL) {
      damage_ship(sh, 0.05);
    }
    if (sh->damage >= sh->size * DAMAGE_SCALE) {
      remove_ship(shp, sh);
    }
    if (*shp==sh) shp=&sh->next;
  }
}

/* The following functions do not really belong here: */
#include <kernel/eressea.h>
#include <kernel/build.h>

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
    if (fval(f, FFL_NEWID)) {
      ADDMSG(&f->msgs, msg_message("renumber_twice", "id", want));
      continue;
    }
    old = findfaction(want);
    if (old) {
      a_remove(&f->attribs, a);
      ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
      continue;
    }
    if (!faction_id_is_unused(want)) {
      a_remove(&f->attribs, a);
      ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
      continue;
    }
    for (rn=&renum; *rn; rn=&(*rn)->next) {
      if ((*rn)->want>=want) break;
    }
    if (*rn && (*rn)->want==want) {
      ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
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
    renumber_faction(f, rp->want);
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
      if (!fval(u, UFL_MARK)) {
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
              fset(u, UFL_MARK);
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
      for (u=r->units;u;u=u->next) freset(u, UFL_MARK);
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
          ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found", ""));
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
        int id = atoi36((const char *)s);
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
        i = atoi36((const char *)s);
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
      if (s == NULL || *s == 0) {
        i = newcontainerid();
      } else {
        i = atoi36((const char *)s);
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
        i = atoi36((const char *)s);
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

    if (fval(rt->terrain, FORBIDDEN_REGION)) rt = NULL;
    /* step 1: give unicorns to people in the building,
     * find out if there's a magician in there. */
    for (u=r->units;u;u=u->next) {
      if (b==u->building && inside_building(u)) {
        if (!(u->race->ec_flags & GIVEITEM)==0) {
          int n, unicorns = 0;
          for (n=0; n!=u->number; ++n) {
            if (chance(0.02)) {
              i_change(&u->items, olditemtype[I_ELVENHORSE], 1);
              ++unicorns;
            }
            if (unicorns) {
              ADDMSG(&u->faction->msgs, msg_message("scunicorn", 
                "unit amount rtype", u, unicorns, 
                olditemtype[I_ELVENHORSE]->rtype));
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
            (float)sk, sk/2, effect, 0);
          ADDMSG(&r->msgs, msg_message("astralshield_activate", 
            "region unit", r, mage));
        }
      } else if (mage!=NULL) {
        int sk = effskill(mage, SK_MAGIC);
        c->duration = MAX(c->duration, sk/2);
        c->vigour = MAX(c->vigour, sk);
      }
    }
  }

  a_age(&b->attribs);
  handle_event(b->attribs, "timer", b);

  return b;
}

static void age_region(region * r) 
{
  a_age(&r->attribs);
  handle_event(r->attribs, "timer", r);

  if (r->land && r->land->ownership) {
    int stability = r->land->ownership->since_turn;
    int morale = MORALE_TAKEOVER + stability/10;
    if (r->land->ownership->owner && r->land->morale<MORALE_MAX) {
      r->land->morale = (short)MIN(morale, MORALE_MAX);
    }
    if (!r->land->ownership->owner && r->land->morale<MORALE_DEFAULT) {
      r->land->morale = (short)MIN(morale, MORALE_DEFAULT);
    }
  }
}

static void
ageing(void)
{
  faction *f;
  region *r;

  /* altern spezieller Attribute, die eine Sonderbehandlung brauchen?  */
  for(r=regions;r;r=r->next) {
    unit *u;

    for (u=r->units;u;u=u->next) {
      /* Goliathwasser */
      int i = get_effect(u, oldpotiontype[P_STRONG]);
      if (i > 0){
        change_effect(u, oldpotiontype[P_STRONG], -1 * MIN(u->number, i));
      }
      /* Berserkerblut*/
      i = get_effect(u, oldpotiontype[P_BERSERK]);
      if (i > 0){
        change_effect(u, oldpotiontype[P_BERSERK], -1 * MIN(u->number, i));
      }

      if (is_cursed(u->attribs, C_OLDRACE, 0)){
        curse *c = get_curse(u->attribs, ct_find("oldrace"));
        if (c->duration == 1 && !(c_flags(c) & CURSE_NOAGE)) {
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
    building * blargest = NULL;

    age_region(r);

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
    }

    blargest = largestbuilding(r, &is_tax_building, false);
    if (blargest) {
      /* region owners update? */
      faction * f = region_get_owner(r);
      unit * u = buildingowner(r, blargest);
      if (u==NULL) {
        region_set_owner(r, NULL, turn);
      } else if (u->faction!=f) {
        region_set_owner(r, f, turn);
      }
    }
  }
}

static int
maxunits(const faction *f)
{
  if (global.unitsperalliance == true) {
    float mult = 1.0;

#if KARMA_MODULE
    faction *f2;
    for (f2 = factions; f2; f2 = f2->next) {
      if (f2->alliance == f->alliance) {
        mult += 0.4f * fspecial(f2, FS_ADMINISTRATOR);
      }
    }
#endif /* KARMA_MODULE */
    return (int) (global.maxunits * mult);
  }
#if KARMA_MODULE
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
            int alias;
            ship * sh;
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
            if (token && token[0]) {
              name = strdup(token);
            }
            u2 = create_unit(r, u->faction, 0, u->faction->race, alias, name, u);
            if (name!=NULL) free(name);
            fset(u2, UFL_ISNEW);

            a_add(&u2->attribs, a_new(&at_alias))->data.i = alias;
            sh = leftship(u);
            if (sh) set_leftship(u2, sh);
            setstatus(u2, u->status);

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
  /* check all orders for a potential new long order this round: */
  for (ord = u->orders; ord; ord = ord->next) {
    if (u->old_orders && is_repeated(ord)) {
      /* this new order will replace the old defaults */
      free_orders(&u->old_orders);
      if (hunger) break;
    }
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
    /* fset(u, UFL_LONGACTION|UFL_NOTMOVING); */
    set_order(&u->thisorder, NULL);
  }
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

#if KARMA_MODULE
      if (fspecial(u->faction, FS_UNDEAD)) continue;

      if(fspecial(u->faction, FS_REGENERATION)) {
        u->hp = umhp;
        continue;
      }
#endif /* KARMA_MODULE */

      p *= heal_factor(u->race);
      if (u->hp < umhp) {
#ifdef NEW_DAEMONHUNGER_RULE
        double maxheal = MAX(u->number, umhp/20.0);
#else
        double maxheal = MAX(u->number, umhp/10.0);
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
        u->hp = MIN(u->hp + addhp, umhp);

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
      boolean neworders = false;
      order ** ordp = &u->orders;
      while (*ordp!=NULL) {
        order * ord = *ordp;
        if (get_keyword(ord)==K_DEFAULT) {
          char lbuf[8192];
          order * new_order;
          init_tokens(ord);
          skip_token(); /* skip the keyword */
          strcpy(lbuf, getstrtoken());
          new_order = parse_order(lbuf, u->faction->locale);
          *ordp = ord->next;
          ord->next = NULL;
          free_order(ord);
          if (!neworders) {
            /* lange Befehle aus orders und old_orders löschen zu gunsten des neuen */
            remove_exclusive(&u->orders);
            remove_exclusive(&u->old_orders);
            neworders = true;
            ordp = &u->orders; /* we could have broken ordp */
          }
          if (new_order) addlist(&u->old_orders, new_order);
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
        if (!is_monsters(u->faction) && m != NULL) {
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
  n = atoi((const char *)t);
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
  n = atoi((const char *)t);
  if (n==0) {
    n = 1;
  } else {
    t = getstrtoken();
  }
  itype = finditemtype(t, u->faction->locale);

  if (itype!=NULL) {
    item ** iclaim = i_find(&u->faction->items, itype);
    if (iclaim!=NULL && *iclaim!=NULL) {
      n = MIN(n, (*iclaim)->number);
      i_change(iclaim, itype, -n);
      i_change(&u->items, itype, n);
    }
  } else {
    cmistake(u, ord, 43, MSG_PRODUCE);
  }
  return 0;
}

enum {
  PROC_THISORDER = 1<<0,
  PROC_LONGORDER = 1<<1
};
typedef struct processor {
  struct processor * next;
  int priority;
  enum { PR_GLOBAL, PR_REGION_PRE, PR_UNIT, PR_ORDER, PR_REGION_POST } type;
  unsigned int flags;
  union {
    struct {
      keyword_t kword;
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

processor *
add_proc(int priority, const char * name, int type)
{
  processor **pproc = &processors;
  processor *proc;

  while (*pproc) {
    proc = *pproc;
    if (proc->priority>priority) break;
    else if (proc->priority==priority && proc->type>=type) break;
    pproc = &proc->next;
  }

  proc = malloc(sizeof(processor));
  proc->priority = priority;
  proc->type = type;
  proc->name = name;
  proc->next = *pproc;
  *pproc = proc;
  return proc;
}

void
add_proc_order(int priority, keyword_t kword, int (*parser)(struct unit *, struct order *), unsigned int flags, const char * name)
{
  if (!global.disabled[kword]) {
    processor * proc = add_proc(priority, name, PR_ORDER);
    if (proc) {
      proc->data.per_order.process = parser;
      proc->data.per_order.kword = kword;
      proc->flags = flags;
    }
  }
}

void
add_proc_global(int priority, void (*process)(void), const char * name)
{
  processor * proc = add_proc(priority, name, PR_GLOBAL);
  if (proc) {
    proc->data.global.process = process;
  }
}

void
add_proc_region(int priority, void (*process)(region *), const char * name)
{
  processor * proc = add_proc(priority, name, PR_REGION_PRE);
  if (proc) {
    proc->data.per_region.process = process;
  }
}

void
add_proc_postregion(int priority, void (*process)(region *), const char * name)
{
  processor * proc = add_proc(priority, name, PR_REGION_POST);
  if (proc) {
    proc->data.per_region.process = process;
  }
}

void
add_proc_unit(int priority, void (*process)(unit *), const char * name)
{
  processor * proc = add_proc(priority, name, PR_UNIT);
  if (proc) {
    proc->data.per_unit.process = process;
  }
}

/* per priority, execute processors in order from PR_GLOBAL down to PR_ORDER */
void
process(void)
{
  processor *proc = processors;
  faction * f;

  while (proc) {
    int prio = proc->priority;
    region *r;
    processor *pglobal = proc;

    if (verbosity>=3) printf("- Step %u\n", prio);
    while (proc && proc->priority==prio) {
      if (proc->name && verbosity>=1) log_stdio(stdout, " - %s\n", proc->name);
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

      while (pregion && pregion->priority==prio && pregion->type==PR_REGION_PRE) {
        pregion->data.per_region.process(r);
        pregion = pregion->next;
      }
      if (pregion==NULL || pregion->priority!=prio) continue;

      if (r->units) {
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
            if (porder->flags & PROC_THISORDER) ordp = &u->thisorder;
            while (*ordp) {
              order * ord = *ordp;
              if (get_keyword(ord) == porder->data.per_order.kword) {
                if (porder->flags & PROC_LONGORDER) {
                  if (u->number==0) {
                    ord = NULL;
                  } else if (u->race == new_race[RC_INSECT] && r_insectstalled(r) && !is_cursed(u->attribs, C_KAELTESCHUTZ,0)) {
                    ord = NULL;
                  } else if (LongHunger(u)) {
                    cmistake(u, ord, 224, MSG_MAGIC);
                    ord = NULL;
                  } else if (fval(u, UFL_LONGACTION)) {
                    cmistake(u, ord, 52, MSG_PRODUCE);
                    ord = NULL;
                  } else if (fval(r->terrain, SEA_REGION) && u->race != new_race[RC_AQUARIAN] && !(u->race->flags & RCF_SWIM)) {
                    /* error message disabled by popular demand */
                    ord = NULL;
                  }
                }
                if (ord) {
                  porder->data.per_order.process(u, ord);
                }
              }
              if (!ord || *ordp==ord) ordp=&(*ordp)->next;
            }
            porder = porder->next;
          }
        }
      }

      while (pregion && pregion->priority==prio && pregion->type!=PR_REGION_POST) {
        pregion = pregion->next;
      }

      while (pregion && pregion->priority==prio && pregion->type==PR_REGION_POST) {
        pregion->data.per_region.process(r);
        pregion = pregion->next;
      }
      if (pregion==NULL || pregion->priority!=prio) continue;

    }
  }

  if (verbosity>=3) printf("\n - Leere Gruppen loeschen...\n");
  for (f=factions; f; f=f->next) {
    group ** gp = &f->groups;
    while (*gp) {
      group * g = *gp;
      if (g->members==0) {
        *gp = g->next;
        free_group(g);
      } else
        gp = &g->next;
    }
  }

}

static void enter_1(region * r) { do_misc(r, false); }
static void enter_2(region * r) { do_misc(r, true); }
static void maintain_buildings_1(region * r) { maintain_buildings(r, false); }
static void maintain_buildings_2(region * r) { maintain_buildings(r,true); }
static void reset_moved(unit * u) { freset(u, UFL_MOVED); }

/** warn about passwords that are not US ASCII.
 * even though passwords are technically UTF8 strings, the server receives
 * them as part of the Subject of an email when reports are requested.
 * This means that we need to limit them to ASCII characters until that
 * mechanism has been changed.
 */
static int
warn_password(void)
{
  faction * f = factions;
  while (f) {
    boolean pwok = true;
    const char * c = f->passw;
    while (*c && pwok) {
      if (!isalnum((unsigned char)*c)) pwok = false;
      c++;
    }
    if (!pwok) {
      free(f->passw);
      f->passw = strdup(itoa36(rng_int()));
      ADDMSG(&f->msgs, msg_message("illegal_password", "newpass", f->passw));
    }
    f = f->next;
  }
  return 0;
}

void
init_processor(void)
{
  int p;

  p = 10;
  add_proc_global(p, &new_units, "Neue Einheiten erschaffen");

  p+=10;
  add_proc_unit(p, &setdefaults, "Default-Befehle");
  add_proc_order(p, K_BANNER, &banner_cmd, 0, NULL);
  add_proc_order(p, K_EMAIL, &email_cmd, 0, NULL);
  add_proc_order(p, K_PASSWORD, &password_cmd, 0, NULL);
  add_proc_order(p, K_SEND, &send_cmd, 0, NULL);
  add_proc_order(p, K_GROUP, &group_cmd, 0, NULL);

  p+=10;
  add_proc_unit(p, &reset_moved, "Instant-Befehle");
  add_proc_order(p, K_QUIT, &quit_cmd, 0, NULL);
  add_proc_order(p, K_URSPRUNG, &origin_cmd, 0, NULL);
  add_proc_order(p, K_ALLY, &ally_cmd, 0, NULL);
  add_proc_order(p, K_PREFIX, &prefix_cmd, 0, NULL);
  add_proc_order(p, K_SETSTEALTH, &setstealth_cmd, 0, NULL);
  add_proc_order(p, K_STATUS, &status_cmd, 0, NULL);
  add_proc_order(p, K_COMBATSPELL, &combatspell_cmd, 0, NULL);
  add_proc_order(p, K_DISPLAY, &display_cmd, 0, NULL);
  add_proc_order(p, K_NAME, &name_cmd, 0, NULL);
  add_proc_order(p, K_GUARD, &guard_off_cmd, 0, NULL);
  add_proc_order(p, K_RESHOW, &reshow_cmd, 0, NULL);
#if KARMA_MODULE
  add_proc_order(p, K_WEREWOLF, &setwere_cmd, 0, NULL);
#endif /* KARMA_MODULE */

  if (get_param_int(global.parameters, "rules.alliances", 0)==1) {
    p+=10;
    add_proc_global(p, &alliance_cmd, NULL);
  }

  p+=10;
  add_proc_global(p, &age_factions, "Parteienalter++");
  add_proc_order(p, K_MAIL, &mail_cmd, 0, "Botschaften");

  p+=10; /* all claims must be done before we can USE */
  add_proc_region(p, &enter_1, "Kontaktieren & Betreten (1. Versuch)");
  add_proc_order(p, K_USE, &use_cmd, 0, "Benutzen");

#if INFOCMD_MODULE
  if (!global.disabled[K_INFO]) {
    add_proc_global(p, &infocommands, NULL);
  }
#endif
  if (!global.disabled[K_GM]) {
    add_proc_global(p, &gmcommands, "GM Kommandos");
  }

  p += 10; /* in case it has any effects on alliance victories */
  add_proc_order(p, K_LEAVE, &leave_cmd, 0, "Verlassen");

  if (!nobattle) {
#if KARMA_MODULE
    p+=10;
    add_proc_global(p, &jihad_attacks, "Jihad-Angriffe");
#endif
    add_proc_region(p, &do_battle, "Attackieren");
  }

  if (!global.disabled[K_BESIEGE]) {
    p+=10;
    add_proc_region(p, &do_siege, "Belagern");
  }

  p+=10; /* can't allow reserve before siege (weapons) */
  add_proc_region(p, &enter_1, "Kontaktieren & Betreten (2. Versuch)");
  add_proc_order(p, K_RESERVE, &reserve_cmd, 0, "Reservieren");
  add_proc_order(p, K_CLAIM, &claim_cmd, 0, NULL);
  add_proc_unit(p, &follow_unit, "Folge auf Einheiten setzen");

  p+=10; /* rest rng again before economics */
  add_proc_region(p, &economics, "Zerstoeren, Geben, Rekrutieren, Vergessen");

  p+=10;
  add_proc_region(p, &maintain_buildings_1, "Gebaeudeunterhalt (1. Versuch)");

  p+=10; /* QUIT fuer sich alleine */
  add_proc_global(p, &quit, "Sterben");
  if (!global.disabled[K_RESTART]) {
    add_proc_global(p, &parse_restart, "Neustart");
  }

  if (!global.disabled[K_CAST]) {
    p+=10;
    add_proc_global(p, &magic, "Zaubern");
  }

  p+=10;
  add_proc_order(p, K_TEACH, &teach_cmd, PROC_THISORDER|PROC_LONGORDER, "Lehren");
  p+=10;
  add_proc_order(p, K_STUDY, &learn_cmd, PROC_THISORDER|PROC_LONGORDER, "Lernen");

  p+=10;
  add_proc_global(p, &produce, "Arbeiten, Handel, Rekruten");
  add_proc_order(p, K_MAKE, &make_cmd, PROC_THISORDER|PROC_LONGORDER, "Produktion");
  add_proc_postregion(p, &split_allocations, "Produktion II");

  p+=10;
  add_proc_region(p, &enter_2, "Kontaktieren & Betreten (3. Versuch)");

  p+=10;
  add_proc_region(p, &sinkships, "Schiffe sinken");

  p+=10;
  add_proc_global(p, &movement, "Bewegungen");

  if (get_param_int(global.parameters, "work.auto", 0)) {
    p+=10;
    add_proc_region(p, &auto_work, "Arbeiten (auto)");
  }

  p+=10;
  add_proc_order(p, K_GUARD, &guard_on_cmd, 0, "Bewache (an)");
#if XECMD_MODULE
  /* can do together with guard */
  add_proc_order(p, K_XE, &xecmd, 0, "Zeitung");
#endif

  p+=10;
  add_proc_global(p, &encounters, "Zufallsbegegnungen");
  p+=10;
  add_proc_unit(p, &monsters_kill_peasants, "Monster fressen und vertreiben Bauern");

  p+=10;
  add_proc_global(p, &randomevents, "Zufallsereignisse");

  p+=10;
  add_proc_global(p, &monthly_healing, "Regeneration (HP)");
  add_proc_global(p, &regeneration_magiepunkte, "Regeneration (Aura)");
  if (!global.disabled[K_DEFAULT]) {
    add_proc_global(p, &defaultorders, "Defaults setzen");
  }
  add_proc_global(p, &demographics, "Nahrung, Seuchen, Wachstum, Wanderung");

  p+=10;
  add_proc_region(p, &maintain_buildings_2, "Gebaeudeunterhalt (2. Versuch)");

  if (!global.disabled[K_SORT]) {
    p+=10;
    add_proc_global(p, &reorder, "Einheiten sortieren");
  }
#if KARMA_MODULE
  p+=10;
  add_proc_global(p, &karma, "Jihads setzen");
#endif
  add_proc_order(p, K_PROMOTION, &promotion_cmd, 0, "Heldenbefoerderung");
  if (!global.disabled[K_NUMBER]) {
    add_proc_order(p, K_NUMBER, &renumber_cmd, 0, "Neue Nummern (Einheiten)");
    p+=10;
    add_proc_global(p, &renumber_factions, "Neue Nummern");
  }
}

void
processorders (void)
{
  static int init = 0;

  if (!init) {
    init_processor();
    init = 1;
  }
  process();
  /*************************************************/

  if (verbosity>=1) puts(" - Attribute altern");
  ageing();
  remove_empty_units();

  if (get_param_int(global.parameters, "modules.wormholes", 0)) {
    create_wormholes();
  }

  /* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
   * Beschreibungen geändert haben */
  update_spells();
  warn_password();
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
    puts("writing passwords...");

    for (f = factions; f; f = f->next) {
      fprintf(F, "%s:%s:%s:%s:%u\n",
        factionid(f), f->email, f->passw, f->override, f->subscription);
    }
    fclose(F);
    return 0;
  }
  return 1;
}

void
update_subscriptions(void)
{
  FILE * F;
  char zText[MAX_PATH];
  faction * f;
  strcat(strcpy(zText, basepath()), "/subscriptions");
  F = fopen(zText, "r");
  if (F==NULL) {
    log_warning((0, "could not open %s.\n", zText));
    return;
  }
  for (;;) {
    char zFaction[5];
    int subscription, fno;
    if (fscanf(F, "%d %s", &subscription, zFaction)<=0) break;
    fno = atoi36(zFaction);
    f = findfaction(fno);
    if (f!=NULL) {
      f->subscription=subscription;
    }
  }
  fclose(F);

  sprintf(zText, "subscriptions.%u", turn);
  F = fopen(zText, "w");
  for (f=factions;f!=NULL;f=f->next) {
    fprintf(F, "%s:%u:%s:%s:%s:%u:\n",
      itoa36(f->no), f->subscription, f->email, f->override,
      dbrace(f->race), f->lastorders);
  }
  fclose(F);
}
