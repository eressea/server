/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

#include <platform.h>
#include <kernel/config.h>
#include "monster.h"

/* gamecode includes */
#include "economy.h"
#include "give.h"

/* triggers includes */
#include <triggers/removecurse.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/hate.h>

/* kernel includes */
#include <kernel/build.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/move.h>
#include <kernel/names.h>
#include <kernel/order.h>
#include <kernel/pathfinder.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/skill.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MOVECHANCE                  25	/* chance fuer bewegung */

#define MAXILLUSION_TEXTS   3

boolean
monster_is_waiting(const unit * u)
{
  if (fval(u, UFL_ISNEW|UFL_MOVED)) return true;
  return false;
}

static void
eaten_by_monster(unit * u)
{
  /* adjustment for smaller worlds */
  static double multi = 0.0;
  int n = 0;
  int horse = 0;

  if (multi==0.0) {
    multi = RESOURCE_QUANTITY * newterrain(T_PLAIN)->size / 10000.0;
  }

  switch (old_race(u->race)) {
    case RC_FIREDRAGON:
      n = rng_int()%80 * u->number;
      horse = get_item(u, I_HORSE);
      break;
    case RC_DRAGON:
      n = rng_int()%200 * u->number;
      horse = get_item(u, I_HORSE);
      break;
    case RC_WYRM:
      n = rng_int()%500 * u->number;
      horse = get_item(u, I_HORSE);
      break;
    default:
      n = rng_int()%(u->number/20+1);
  }

  n = (int)(n * multi);
  if (n > 0) {
    n = lovar(n);
    n = MIN(rpeasants(u->region), n);

    if (n > 0) {
      deathcounts(u->region, n);
      rsetpeasants(u->region, rpeasants(u->region) - n);
      ADDMSG(&u->region->msgs, msg_message("eatpeasants", "unit amount", u, n));
    }
  }
  if (horse > 0) {
    set_item(u, I_HORSE, 0);
    ADDMSG(&u->region->msgs, msg_message("eathorse", "unit amount", u, horse));
  }
}

static void
absorbed_by_monster(unit * u)
{
  int n;

  switch (old_race(u->race)) {
    default:
      n = rng_int()%(u->number/20+1);
  }

  if(n > 0) {
    n = lovar(n);
    n = MIN(rpeasants(u->region), n);
    if (n > 0){
      rsetpeasants(u->region, rpeasants(u->region) - n);
      scale_number(u, u->number + n);
      ADDMSG(&u->region->msgs, msg_message("absorbpeasants", 
        "unit race amount", u, u->race, n));
    }
  }
}

static int
scareaway(region * r, int anzahl)
{
  int n, p, diff = 0, emigrants[MAXDIRECTIONS];
  direction_t d;

  anzahl = MIN(MAX(1, anzahl),rpeasants(r));

  /* Wandern am Ende der Woche (normal) oder wegen Monster. Die
  * Wanderung wird erst am Ende von demographics () ausgefuehrt.
  * emigrants[] ist local, weil r->newpeasants durch die Monster
  * vielleicht schon hochgezaehlt worden ist. */

  for (d = 0; d != MAXDIRECTIONS; d++)
    emigrants[d] = 0;

  p = rpeasants(r);
  assert(p >= 0 && anzahl >= 0);
  for (n = MIN(p, anzahl); n; n--) {
    direction_t dir = (direction_t)(rng_int() % MAXDIRECTIONS);
    region * rc = rconnect(r, dir);

    if (rc && fval(rc->terrain, LAND_REGION)) {
      ++diff;
      rc->land->newpeasants++;
      emigrants[dir]++;
    }
  }
  rsetpeasants(r, p-diff);
  assert(p >= diff);
  return diff;
}

static void
scared_by_monster(unit * u)
{
  int n;

  switch (old_race(u->race)) {
    case RC_FIREDRAGON:
      n = rng_int()%160 * u->number;
      break;
    case RC_DRAGON:
      n = rng_int()%400 * u->number;
      break;
    case RC_WYRM:
      n = rng_int()%1000 * u->number;
      break;
    default:
      n = rng_int()%(u->number/4+1);
  }

  if(n > 0) {
    n = lovar(n);
    n = MIN(rpeasants(u->region), n);
    if(n > 0) {
      n = scareaway(u->region, n);
      if(n > 0) {
        ADDMSG(&u->region->msgs, msg_message("fleescared", 
          "amount unit", n, u));
      }
    }
  }
}

void
monster_kills_peasants(unit * u)
{
  if (!monster_is_waiting(u)) {
    if (u->race->flags & RCF_SCAREPEASANTS) {
      scared_by_monster(u);
    }
    if (u->race->flags & RCF_KILLPEASANTS) {
      eaten_by_monster(u);
    }
    if (u->race->flags & RCF_ABSORBPEASANTS) {
      absorbed_by_monster(u);
    }
  }
}
