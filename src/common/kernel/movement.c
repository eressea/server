/* vi: set ts=2:
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
#include "eressea.h"
#include "movement.h"

#include "alchemy.h"
#include "border.h"
#include "build.h"
#include "building.h"
#include "calendar.h"
#include "curse.h"
#include "faction.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "order.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "render.h"
#include "spell.h"
#include "ship.h"
#include "skill.h"
#include "teleport.h"
#include "unit.h"

/* util includes */
#include <util/goodies.h>
#include <util/base36.h>
#include <util/language.h>
#include <util/rand.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/targetregion.h>
#include <attributes/at_movement.h>
#include <attributes/otherfaction.h>

extern border_type bt_wisps;
int * storms;

typedef struct traveldir {
	int no;
	direction_t dir;
	int age;
} traveldir;

static attrib_type at_traveldir = {
	"traveldir",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,					/* Weil normales Aging an ungünstiger Stelle */
	DEFAULT_WRITE,
	DEFAULT_READ
};

typedef struct follower {
  struct follower * next;
  unit * uf;
  unit * ut;
  region_list * route_end;
} follower;

static void
get_followers(unit * target, region * r, region_list * route_end, follower ** followers)
{
  unit * uf;
  for (uf=r->units;uf;uf=uf->next) {
    if (fval(uf, UFL_FOLLOWING) && !fval(uf, UFL_LONGACTION)) {
      const attrib * a = a_findc(uf->attribs, &at_follow);
      if (a && a->data.v == target) {
        follower * fnew = malloc(sizeof(follower));
        fnew->uf = uf;
        fnew->ut = target;
        fnew->route_end = route_end;
        fnew->next = *followers;
        *followers = fnew;
      }
    }
  }
}

static void
shiptrail_init(attrib *a)
{
	a->data.v = calloc(1, sizeof(traveldir));
}

static void
shiptrail_finalize(attrib *a)
{
	free(a->data.v);
}

static int
shiptrail_age(attrib *a)
{
	traveldir *t = (traveldir *)(a->data.v);

	t->age--;
	if(t->age == 0) return 0;
	return 1;
}

static int
shiptrail_read(attrib *a, FILE *f)
{
	traveldir *t = (traveldir *)(a->data.v);
	int       no, age, dir;

	fscanf(f, "%d %d %d", &no, &dir, &age);
	t->no  = no;
	t->dir = (direction_t)dir;
	t->age = age;
	return AT_READ_OK;
}

static void
shiptrail_write(const attrib *a, FILE *f)
{
	  traveldir *t = (traveldir *)(a->data.v);
	  fprintf(f, "%d %d %d ", t->no, (int)t->dir, t->age);
}

attrib_type at_shiptrail = {
	"traveldir_new",
	shiptrail_init,
	shiptrail_finalize,
	shiptrail_age,
	shiptrail_write,
	shiptrail_read
};

static int
age_speedup(attrib *a)
{
  if (a->data.sa[0] > 0) {
    a->data.sa[0] = a->data.sa[0] - a->data.sa[1];
  }
  return a->data.sa[0]>0;
}

attrib_type at_speedup = {
  "speedup",
  NULL, NULL,
  age_speedup,
  a_writedefault,
  a_readdefault
};

/* ------------------------------------------------------------- */

direction_t
getdirection(const struct locale * lang)
{
	return finddirection(getstrtoken(), lang);
}
/* ------------------------------------------------------------- */

static attrib_type at_driveweight = {
	"driveweight", NULL, NULL, NULL, NULL, NULL
};

static boolean
entrance_allowed(const struct unit * u, const struct region * r)
{
#ifdef REGIONOWNERS
  unit * owner = region_owner(r);
  if (owner==NULL || u->faction==owner->faction) return true;
  if (alliedunit(owner, u->faction, HELP_TRAVEL)) return true;
  if (is_enemy(u->faction, owner->faction)) return true;
  return false;
#endif
  return true;
}

int
personcapacity(const unit *u)
{
#if RACE_CAPACITY
	int cap = u->race->weight+u->race->capacity;
#else
	int cap = u->race->weight+540;

	if (u->race == new_race[RC_TROLL])
		cap += 540;
#if RACE_ADJUSTMENTS
	else if (u->race == new_race[RC_GOBLIN])
		cap -= 100;
#endif
#endif
	
	if (fspecial(u->faction, FS_QUICK))
		cap -= 200;

	return cap;
}

static int
eff_weight(const unit *u)
{
	attrib *a = a_find(u->attribs, &at_driveweight);

	if (a) return weight(u) + a->data.i;

	return weight(u);
}

static int
ridingcapacity(unit * u)
{
	int n;
	int wagen, pferde;

	n = 0;

	/* Man trägt sein eigenes Gewicht plus seine Kapazität! Die Menschen
	 * tragen nichts (siehe walkingcapacity). Ein Wagen zählt nur, wenn er
	 * von zwei Pferden gezogen wird */

  pferde = get_item(u, I_HORSE) + get_item(u, I_UNICORN);
  pferde = min(pferde, effskill(u, SK_RIDING) * u->number * 2);
	if (fval(u->race, RCF_HORSE)) pferde += u->number;

	/* maximal diese Pferde können zum Ziehen benutzt werden */
	wagen = min(pferde / HORSESNEEDED, get_item(u, I_WAGON));

	n = wagen * WAGONCAPACITY + pferde * HORSECAPACITY;
	return n;
}

int
walkingcapacity(const struct unit * u)
{
	int n, tmp, personen, pferde_fuer_wagen;
	int wagen, wagen_ohne_pferde, wagen_mit_pferden, wagen_mit_trollen;
	/* Das Gewicht, welches die Pferde tragen, plus das Gewicht, welches
	 * die Leute tragen */

	int pferde = get_item(u, I_HORSE) + get_item(u, I_UNICORN);
	if (fval(u->race, RCF_HORSE)) {
		pferde += u->number;
		personen = 0;
	} else {
		personen = u->number;
	}
	wagen = get_item(u, I_WAGON);

	pferde_fuer_wagen = min(pferde, effskill(u, SK_RIDING) * u->number * 4);

	/* maximal diese Pferde können zum Ziehen benutzt werden */

	wagen_mit_pferden = min(wagen, pferde_fuer_wagen / HORSESNEEDED);

	n = wagen_mit_pferden * WAGONCAPACITY;

	if (u->race == new_race[RC_TROLL]) {
		/* 4 Trolle ziehen einen Wagen. */
		/* Unbesetzte Wagen feststellen */
		wagen_ohne_pferde = wagen - wagen_mit_pferden;

		/* Genug Trolle, um die Restwagen zu ziehen? */
		wagen_mit_trollen = min(u->number / 4, wagen_ohne_pferde);

		/* Wagenkapazität hinzuzählen */
		n += wagen_mit_trollen * WAGONCAPACITY;
		wagen_ohne_pferde -= wagen_mit_trollen;
	}

	n += pferde * HORSECAPACITY;
	n += personen * personcapacity(u);
	/* Goliathwasser */
  tmp = get_effect(u, oldpotiontype[P_STRONG]);
	n += min(personen, tmp) * (HORSECAPACITY - personcapacity(u));
  /* change_effect wird in ageing gemacht */
  tmp = get_item(u, I_TROLLBELT);
	n += min(personen, tmp) * (STRENGTHMULTIPLIER-1) * personcapacity(u);

	return n;
}

enum {
  E_CANWALK_OK = 0,
  E_CANWALK_TOOMANYHORSES,
  E_CANWALK_TOOMANYCARTS,
  E_CANWALK_TOOHEAVY
};

static int
canwalk(unit * u)
{
	int wagen, maxwagen;
	int pferde, maxpferde;

	/* workaround: monsters are too stupid to drop items, therefore they have
	 * infinite carrying capacity */

	if (u->faction->no == 0) return E_CANWALK_OK;

	wagen = get_item(u, I_WAGON);
	pferde = get_item(u, I_HORSE) + get_item(u, I_UNICORN);

	maxwagen = effskill(u, SK_RIDING) * u->number * 2;
	if (u->race == new_race[RC_TROLL]) {
		maxwagen = max(maxwagen, u->number / 4);
	}
	maxpferde = effskill(u, SK_RIDING) * u->number * 4 + u->number;

	if (pferde > maxpferde)
		return E_CANWALK_TOOMANYHORSES;

	if (walkingcapacity(u) - eff_weight(u) >= 0)
		return E_CANWALK_OK;

	/* Stimmt das Gewicht, impliziert dies hier, daß alle Wagen ohne
	 * Zugpferde/-trolle als Fracht aufgeladen wurden: zu viele Pferde hat
	 * die Einheit nicht zum Ziehen benutzt, also nicht mehr Wagen gezogen
	 * als erlaubt. */

	if (wagen > maxwagen)
		return E_CANWALK_TOOMANYCARTS;
	/* Es muß nicht zwingend an den Wagen liegen, aber egal... (man
	 * könnte z.B. auch 8 Eisen abladen, damit ein weiterer Wagen als
	 * Fracht draufpaßt) */

	return E_CANWALK_TOOHEAVY;
}

boolean
canfly(unit *u)
{
	if (get_movement(&u->attribs, MV_CANNOTMOVE)) return false;

	if (get_item(u, I_HORSE) || get_item(u, I_UNICORN)) return false;

	if (get_item(u, I_PEGASUS) >= u->number && effskill(u, SK_RIDING) >= 4)
		return true;

	if (fval(u->race, RCF_FLY)) return true;

	if (get_movement(&u->attribs, MV_FLY)) return true;

	return false;
}

boolean
canswim(unit *u)
{
	if (get_movement(&u->attribs, MV_CANNOTMOVE)) return false;

	if (get_item(u, I_HORSE) || get_item(u, I_UNICORN)) return false;

	if (get_item(u, I_DOLPHIN) >= u->number && effskill(u, SK_RIDING) >= 4)
		return true;

	if(fspecial(u->faction, FS_AMPHIBIAN)) return true;

	if (u->race->flags & RCF_FLY) return true;

	if (u->race->flags & RCF_SWIM) return true;

	if (get_movement(&u->attribs, MV_FLY)) return true;

	if (get_movement(&u->attribs, MV_SWIM)) return true;

	return false;
}

static int
canride(unit * u)
{
	int pferde, maxpferde, unicorns, maxunicorns;
	int skill = effskill(u, SK_RIDING);

	unicorns = get_item(u, I_UNICORN);
	pferde = get_item(u, I_HORSE);
	maxunicorns = (skill/5) * u->number;
	maxpferde = skill * u->number * 2;

	if(!(u->race->flags & RCF_HORSE)
		&& ((pferde == 0 && unicorns == 0)
			|| pferde > maxpferde || unicorns > maxunicorns)) {
		return 0;
	}

	if (ridingcapacity(u) - eff_weight(u) >= 0) {
		if(pferde == 0 && unicorns >= u->number && !(u->race->flags & RCF_HORSE)) {
			return 2;
		}
		return 1;
	}

	return 0;
}

static boolean
cansail(const region * r, ship * sh)
{
	int n = 0, p = 0;

	 /* sonst ist construction:: size nicht ship_type::maxsize */
	assert(!sh->type->construction || sh->type->construction->improvement==NULL);

	if (sh->type->construction && sh->size!=sh->type->construction->maxsize)
	  return false;
	getshipweight(sh, &n, &p);
  n = ((n+99)/100) * 100; /* Silberreste aufrunden */

  if( is_cursed(sh->attribs, C_SHIP_FLYING, 0) ) {
		if (sh->type->cargo>500*100)
			assert(!"Ein Schiff wurde verzaubert, das zu groß ist");
		if (n > 10000) return false;
	}
	if (n > shipcapacity(sh)) return false;
	if (p > sh->type->cabins) return false;

	return true;
}

int
enoughsailors(const ship * sh, const region * r)
{
	int n;
	unit *u;

	n = 0;

	for (u = r->units; u; u = u->next) {
		if (u->ship == sh)
			n += eff_skill(u, SK_SAILING, r) * u->number;
  }
	return n >= sh->type->sumskill;
}
/* ------------------------------------------------------------- */

static ship *
do_maelstrom(region *r, unit *u)
{
  int damage;

  damage = rand()%150 - eff_skill(u, SK_SAILING, r)*5;

  if(damage <= 0) {
    ADDMSG(&u->faction->msgs, msg_message("entermaelstrom", 
      "region ship damage sink", r, u->ship, damage, 1));
    return u->ship;
  }

  damage_ship(u->ship, 0.01*damage);

  if (u->ship->damage >= u->ship->size * DAMAGE_SCALE) {
    ADDMSG(&u->faction->msgs, msg_message("entermaelstrom",
      "region ship damage sink", r, u->ship, damage, 1));
    destroy_ship(u->ship);
    return NULL;
  }
  ADDMSG(&u->faction->msgs, msg_message("entermaelstrom",
    "region ship damage sink", r, u->ship, damage, 0));
  return u->ship;
}

/** sets a marker in the region telling that the unit has travelled through it
 * this is used for two distinctly different purposes:
 * - to report that a unit has travelled through. the report function
 *   makes sure to only report the ships of travellers, not the travellers 
 *   themselves
 * - to report the region to the traveller
 */
void
travelthru(const unit * u, region * r)
{
  attrib *ru = a_add(&r->attribs, a_new(&at_travelunit));

  ru->data.v = (void*)u;

  /* the first and last region of the faction gets reset, because travelthrough
   * could be in regions that are located before the [first, last] interval,
   * and recalculation is needed */
#ifdef SMART_INTERVALS
  update_interval(u->faction, r);
#endif
}

static void
leave_trail(ship * sh, region * from, region_list *route)
{
  region * r = from;
  
  while (route!=NULL) {
    region * rn = route->data;
    direction_t dir = reldirection(r, rn);

    /* TODO: we cannot leave a trail into special directions 
    * if we use this kind of direction-attribute */
    if (dir<MAXDIRECTIONS && dir>=0) {
      traveldir * td = NULL;
      attrib * a = a_find(r->attribs, &at_shiptrail);

      while (a!=NULL) {
        td = (traveldir *)a->data.v;
        if (td->no == sh->no) break;
        a = a->nexttype;
      }

      if (a==NULL) {
        a = a_add(&(r->attribs), a_new(&at_shiptrail));
        td = (traveldir *)a->data.v;
        td->no = sh->no;
      }
      td->dir = dir;
      td->age = 2;
    }
    route = route->next;
    r = rn;
  }
}

static void
mark_travelthru(const unit * u, region * r, region_list * route, region_list * route_end)
{
  /* kein travelthru in der letzten region! */
  while (route!=route_end) {
    travelthru(u, r);
    r = route->data;
    route = route->next;
  }
}

ship *
move_ship(ship * sh, region * from, region * to, region_list * route)
{
  unit **iunit = &from->units;
  unit **ulist = &to->units;
  boolean trail = (route==NULL);

  if (from!=to) {
    translist(&from->ships, &to->ships, sh);
    sh->region = to;
  }

  while (*iunit!=NULL) {
    unit *u = *iunit;
    assert(u->region==from);

    if (u->ship == sh) {
      if (!trail) {
        leave_trail(sh, from, route);
        trail = true;
      }
      if (route!=NULL) mark_travelthru(u, from, route, NULL);
      if (from!=to) {
        u->ship = NULL;		/* damit move_unit() kein leave() macht */
        move_unit(u, to, ulist);
        ulist = &u->next;
        u->ship = sh;
      }
      if (route && eff_skill(u, SK_SAILING, from) >= 1) {
        produceexp(u, SK_SAILING, u->number);
      }
    }
    if (*iunit==u) iunit=&u->next;
  }
  
#ifndef NDEBUG
  if (rterrain(sh->region)!=T_OCEAN && sh->coast!=NODIRECTION) {
    region * rcoast = rconnect(sh->region, sh->coast);
    if (rterrain(rcoast)!=T_OCEAN) {
      log_error(("ship %s sailed into a coast with no ocean neighbours.\n",
                 shipid(sh)));
    }
  }
#endif
  
  return sh;
}

static boolean
check_working_buildingtype(const region * r, const building_type * bt)
{
	building *b;

	for (b = rbuildings(r); b; b = b->next) {
		if (b->type == bt) {
			if (b->size >= bt->maxsize && fval(b, BLD_WORKING)) {
				return true;
			}
		}
	}

	return false;
}

static boolean
is_freezing(const unit * u)
{
  if (u->race!=new_race[RC_INSECT]) return false;
  if (is_cursed(u->attribs, C_KAELTESCHUTZ, 0)) return false;
  return true;
}

static boolean
ship_allowed(const struct ship * sh, const region * r)
{
  int c = 0;
  terrain_t t = rterrain(r);
  static const building_type * bt_harbour=NULL;

  if (bt_harbour==NULL) bt_harbour=bt_find("harbour");

  if (r_insectstalled(r)) {
    /* insekten dürfen nicht hier rein. haben wir welche? */
    unit * u;

    for (u=sh->region->units;u!=NULL;u=u->next) {
      if (u->ship!=sh) continue;

      if (is_freezing(u)) {
        unit * captain = shipowner(sh);
        ADDMSG(&captain->faction->msgs, msg_message("detectforbidden", 
          "unit region", u, r));

        return false;
      }
    }
  }

  if (check_working_buildingtype(r, bt_harbour)) return true;
  for (c=0;sh->type->coast[c]!=NOTERRAIN;++c) {
    if (sh->type->coast[c]==t) return true;
  }

  return false;
}

static boolean
flying_ship(const ship * sh)
{
  if (sh->type->flags & SFL_FLY) return true;
  if (is_cursed(sh->attribs, C_SHIP_FLYING, 0)) return true;
  return false;
}

static void
set_coast(ship * sh, region * r, region * rnext)
{
  if (rnext->terrain != T_OCEAN && !flying_ship(sh)) {
    sh->coast = reldirection(rnext, r);
    assert(rterrain(r)==T_OCEAN);
  } else {
    sh->coast = NODIRECTION;
  }
}

static void
drifting_ships(region * r)
{
  direction_t d;

  if (rterrain(r) == T_OCEAN) {
    ship** shp = &r->ships;
    while (*shp) {
      ship * sh = *shp;
      region * rnext = NULL;
      region_list * route = NULL;
      unit * captain;
      int d_offset;

      /* Schiff schon abgetrieben oder durch Zauber geschützt? */
      if (fval(sh, SF_DRIFTED) || is_cursed(sh->attribs, C_SHIP_NODRIFT, 0)) {
        shp = &sh->next;
        continue;
      }

      /* Kapitän bestimmen */
      for (captain = r->units; captain; captain = captain->next) {
        if (captain->ship != sh) continue;
        if (eff_skill(captain, SK_SAILING, r) >= sh->type->cptskill) {
          break;
        }
      }
      /* Kapitän da? Beschädigt? Genügend Matrosen?
      * Genügend leicht? Dann ist alles OK. */

      assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
      if (captain && sh->size==sh->type->construction->maxsize && enoughsailors(sh, r) && cansail(r, sh)) {
        shp = &sh->next;
        continue;
      }

      /* Auswahl einer Richtung: Zuerst auf Land, dann
      * zufällig. Falls unmögliches Resultat: vergiß es. */
      d_offset = rand() % MAXDIRECTIONS;
      for (d = 0; d != MAXDIRECTIONS; ++d) {
        region * rn = rconnect(r, (direction_t)((d + d_offset) % MAXDIRECTIONS));
        terrain_t t = rterrain(rn);
        if (rn!=NULL && (terrain[t].flags & SAIL_INTO) && ship_allowed(sh, rn)) {
          rnext = rn;
          if (t!=T_OCEAN) break;
        }
      }

      if (rnext==NULL) {
        shp = &sh->next;
        continue;
      }

      /* Das Schiff und alle Einheiten darin werden nun von r
      * nach rnext verschoben. Danach eine Meldung. */
      add_regionlist(&route, rnext);

      set_coast(sh, r, rnext);
      sh = move_ship(sh, r, rnext, route);
      free_regionlist(route);

      if (sh!=NULL) {

        fset(sh, SF_DRIFTED);

        damage_ship(sh, 0.02);

        if (sh->damage>=sh->size * DAMAGE_SCALE) {
          destroy_ship(sh);
        }
      }

      if (*shp == sh) shp = &sh->next;
    }
  }
}

static boolean
present(region * r, unit * u)
{
	return (boolean) (u && u->region == r);
}

static void
caught_target(region * r, unit * u)
{
	attrib * a = a_find(u->attribs, &at_follow);

	/* Verfolgungen melden */
	/* Misserfolgsmeldung, oder bei erfolgreichem Verfolgen unter
	 * Umstaenden eine Warnung. */

	if (a) {
		unit * target = (unit*)a->data.v;

    if (!present(r, target)) {
      ADDMSG(&u->faction->msgs, msg_message("followfail", "unit follower",
        target, u));
    } else if (!alliedunit(target, u->faction, HELP_ALL)
      && cansee(target->faction, r, u, 0))
    {
      ADDMSG(&target->faction->msgs, msg_message("followdetect", 
        "unit follower", target, u));
    }
	}
}

/* TODO: Unsichtbarkeit bedenken ! */

static unit *
bewegung_blockiert_von(unit * reisender, region * r)
{
	unit *u;
	int perception = 0;
	boolean contact = false;
	unit * guard = NULL;

	if (fval(reisender->race, RCF_ILLUSIONARY)) return NULL;
	for (u=r->units;u && !contact;u=u->next) {
		if (getguard(u) & GUARD_TRAVELTHRU) {
			int sk = eff_skill(u, SK_OBSERVATION, r);
			if (invisible(reisender) >= reisender->number &&
				!get_item(u, I_AMULET_OF_TRUE_SEEING)) continue;
			if (u->faction==reisender->faction) contact = true;
      else if (ucontact(u, reisender)) contact = true;
			else if (alliedunit(u, reisender->faction, HELP_GUARD)) contact = true;
			else if (sk>=perception) {
				perception = sk;
				guard = u;
			}
		}
	}
	if (!contact && guard) {
		double prob = 0.3; /* 30% base chance */
		prob += 0.1 * (perception - eff_stealth(reisender, r));
		prob += 0.1 * min(guard->number, get_item(guard, I_AMULET_OF_TRUE_SEEING));

		if (chance(prob)) {
			return guard;
		}
	}
	return NULL;
}

static boolean
is_guardian(unit * u2, unit *u, unsigned int mask)
{
	if (u2->faction == u->faction) return false;
	if ((getguard(u2) & mask) == 0) return false;
  if (u2->number==0) return false;
  if (alliedunit(u2, u->faction, HELP_GUARD)) return false;
  if (ucontact(u2, u)) return false;
  if (besieged(u2)) return false;
  if (!armedmen(u2)) return false;
  if (!cansee(u2->faction, u->region, u, 0)) return false;
  
  return true;
}

unit *
is_guarded(region * r, unit * u, unsigned int mask)
{
	unit *u2;

	for (u2 = r->units; u2; u2 = u2->next)
		if (is_guardian(u2, u, mask)) return u2;
	return NULL;
}

static const char *shortdirections[MAXDIRECTIONS] =
{
	"dir_nw",
	"dir_ne",
	"dir_east",
	"dir_se",
	"dir_sw",
	"dir_west"
};

static void
cycle_route(order * ord, unit *u, int gereist)
{
	int cm = 0;
	char tail[1024];
	char neworder[2048];
	const char *token;
	direction_t d = NODIRECTION;
	boolean paused = false;
	boolean pause;
  order * norder;

	if (get_keyword(ord) != K_ROUTE) return;
	tail[0] = '\0';

	strcpy(neworder, locale_string(u->faction->locale, keywords[K_ROUTE]));

  init_tokens(ord);
  skip_token();

	for (cm=0;;++cm) {
		const struct locale * lang = u->faction->locale;
		pause = false;
		token = getstrtoken();
		d = finddirection(token, lang);
		if(d == D_PAUSE) {
			pause = true;
		} else if (d==NODIRECTION) {
			break;
		}
		if (cm<gereist) {
			/* hier sollte keine PAUSE auftreten */
			assert(!pause);
			if (!pause) strcat(strcat(tail, " "), LOC(lang, shortdirections[d]));
		}
		else if (strlen(neworder)>sizeof(neworder)/2) break;
		else if (cm==gereist && !paused && pause) {
			strcat(strcat(tail, " "), LOC(lang, parameters[P_PAUSE]));
			paused=true;
		}
		else if (pause) {
			/* da PAUSE nicht in ein shortdirections[d] umgesetzt wird (ist
			 * hier keine normale direction), muss jede PAUSE einzeln
			 * herausgefiltert und explizit gesetzt werden */
			strcat(strcat(neworder, " "), LOC(lang, parameters[P_PAUSE]));
		} else {
			strcat(strcat(neworder, " "), LOC(lang, shortdirections[d]));
		}
	}

	strcat(neworder, tail);
  norder = parse_order(neworder, u->faction->locale);
#ifdef LASTORDER
	set_order(&u->lastorder, norder);
#else
  replace_order(&u->orders, ord, norder);
  free_order(norder);
#endif
}

static boolean 
transport(unit * ut, unit * u)
{
  order * ord;

  if (LongHunger(u) || ut->region->terrain==T_OCEAN) {
    return false;
  }

  for (ord = ut->orders; ord; ord = ord->next) {
    if (get_keyword(ord) == K_TRANSPORT) {
      init_tokens(ord);
      skip_token();
      if (getunit(ut->region, ut->faction) == u) {
        return true;
      }
    }
  }
  return false;
}

static boolean
can_move(const unit * u)
{
  if (u->race->flags & RCF_CANNOTMOVE) return false;
  if (get_movement(&u->attribs, MV_CANNOTMOVE)) return false;
  return true;
}

static void
init_transportation(void)
{
  region *r;

  for (r=regions; r; r=r->next) {
    unit *u;

    /* This is just a simple check for non-corresponding K_TRANSPORT/
    * K_DRIVE. This is time consuming for an error check, but there
    * doesn't seem to be an easy way to speed this up. */
    for (u=r->units; u; u=u->next) {
      if (get_keyword(u->thisorder) == K_DRIVE && can_move(u) && !fval(u, UFL_LONGACTION) && !LongHunger(u)) {
        unit * ut;

        init_tokens(u->thisorder);
        skip_token();
        ut = getunit(r, u->faction);
        if (ut==NULL) {
          cmistake(u, u->thisorder, 63, MSG_MOVE);
          continue;
        }
        if (!transport(ut, u)) {
          if (cansee(u->faction, r, ut, 0)) {
            cmistake(u, u->thisorder, 286, MSG_MOVE);
          } else {
            cmistake(u, u->thisorder, 63, MSG_MOVE);
          }
        }
      }
    }

    /* This calculates the weights of all transported units and
     * adds them to an internal counter which is used by travel () to
     * calculate effective weight and movement. */
    for (u=r->units; u; u=u->next) {
      order * ord;
      int w = 0;

      for (ord = u->orders; ord; ord = ord->next) {
        if (get_keyword(ord) == K_TRANSPORT) {
          unit * ut;

          init_tokens(ord);
          skip_token();
          ut = getunit(r, u->faction);
          if (ut==NULL) continue;

          if (get_keyword(ut->thisorder) == K_DRIVE && can_move(u) && !fval(ut, UFL_LONGACTION) && !LongHunger(ut)) {
            init_tokens(ut->thisorder);
            skip_token();
            if (getunit(r, ut->faction) == u) {
              w += weight(ut);
              break;
            }
          }
        }
      }
      if (w > 0) a_add(&u->attribs, a_new(&at_driveweight))->data.i = w;
    }
  }
}

static boolean
roadto(const region * r, direction_t dir)
{
  /* wenn es hier genug strassen gibt, und verbunden ist, und es dort
  * genug strassen gibt, dann existiert eine strasse in diese richtung */
  region * r2;
  static const curse_type * roads_ct = NULL;

  if (roads_ct==NULL) roads_ct = ct_find("magicstreet"); 
  if (roads_ct!=NULL) {
    curse *c = get_curse(r->attribs, roads_ct);
    if (c!=NULL) return true;
  }
  
  if (dir>=MAXDIRECTIONS || dir<0) return false;
  r2 = rconnect(r, dir);
  if (r==NULL || r2==NULL) return false;

  if (terrain[rterrain(r)].roadreq==0) return false;
  if (terrain[rterrain(r2)].roadreq==0) return false;
  if (rroad(r, dir) < terrain[rterrain(r)].roadreq) return false;
  if (rroad(r2, dir_invert(dir)) < terrain[rterrain(r2)].roadreq) return false;
  return true;
}

static region_list *
cap_route(region * r, region_list * route, region_list * route_end, int speed)
{
  region * current = r;
  int moves = speed;
  region_list * iroute = route;
  while (iroute!=route_end) {
    region * next = iroute->data;
    direction_t reldir = reldirection(current, next);

    /* adjust the range of the unit */
    if (roadto(current, reldir)) moves -= BP_ROAD;
    else moves -= BP_NORMAL;
    if (moves<0) break;
    iroute = iroute->next;
    current = next;
  }
  return iroute;
}

static void
make_route(unit * u, order * ord, region_list ** routep)
{
  region_list **iroute = routep;
  region * current = u->region;
  region * next = NULL;
  const char * token = getstrtoken();
  int error = movewhere(u, token, current, &next);

  if (error!=E_MOVE_OK) {
    message * msg = movement_error(u, token, ord, error);
    if (msg!=NULL) {
      add_message(&u->faction->msgs, msg);
      msg_release(msg);
    }
    next = NULL;
  }

  while (next!=NULL) {
    border * b = get_borders(current, next);
    direction_t reldir;

    if (current==next) {
      /* PAUSE */
      break;
    }
    while (b!=NULL) {
      if (b->type->move) {
        region * rto = b->type->move(b, u, current, next, true);
        if (rto!=next) {
          /* the target region was changed (bt_wisps, for example). check the
           * new target region for borders */
          next = rto;
          b = get_borders(current, next);
          continue;
        }
      }
      b = b->next;
    }
    
    reldir = reldirection(current, next);
    
    add_regionlist(iroute, next);
    iroute = &(*iroute)->next;

    current = next;
    token = getstrtoken();
    error = movewhere(u, token, current, &next);
    if (error) {
      message * msg = movement_error(u, token, ord, error);
      if (msg!=NULL) {
        add_message(&u->faction->msgs, msg);
        msg_release(msg);
      }
      next = NULL;
    }
  }
}

/** calculate the speed of a unit
 *
 * zu Fuß reist man 1 Region, zu Pferd 2 Regionen. Mit Straßen reist
 * man zu Fuß 2, mit Pferden 3 weit.
 *
 * Berechnet wird das mit BPs. Zu Fuß hat man 4 BPs, zu Pferd 6.
 * Normalerweise verliert man 3 BP pro Region, bei Straßen nur 2 BP.
 * Außerdem: Wenn Einheit transportiert, nur halbe BP 
 */
static int 
movement_speed(unit * u)
{
  int mp;
  static const curse_type * speed_ct;
  static boolean init = false;
  double dk = u->race->speed;

  /* dragons have a fixed speed, and no other effects work on them: */
  switch (old_race(u->race)) {
    case RC_DRAGON:
    case RC_WYRM:
    case RC_FIREDRAGON:
    case RC_BIRTHDAYDRAGON:
    case RC_PSEUDODRAGON:
      return BP_DRAGON;
  }

  if (!init) { 
    init = true; 
    speed_ct = ct_find("speed"); 
  }
  if (speed_ct) {
    curse *c = get_curse(u->attribs, speed_ct);
    if (c!=NULL) {
      int men = get_cursedmen(u, c);
      dk *= 1.0 + (double)men/(double)u->number;
    }
  }

  switch (canride(u)) {

  case 1:		/* Pferd */
    mp = BP_RIDING;
    break;

  case 2:		/* Einhorn */
    mp = BP_UNICORN;
    break;

  default:
    mp = BP_WALKING;
    /* faction special */
    if (fspecial(u->faction, FS_QUICK)) mp = BP_RIDING;

    /* Siebenmeilentee */
    if (u->region->terrain!=T_OCEAN && get_effect(u, oldpotiontype[P_FAST]) >= u->number) {
      mp *= 2;
      change_effect(u, oldpotiontype[P_FAST], -u->number);
    }

    /* unicorn in inventory */
    if (u->number <= get_item(u, I_FEENSTIEFEL)) {
      mp *= 2;
    }

    /* Im Astralraum sind Tyb und Ill-Magier doppelt so schnell.
    * Nicht kumulativ mit anderen Beschleunigungen! */
    if (mp*dk <= BP_WALKING*u->race->speed && getplane(u->region) == get_astralplane() && is_mage(u)) {
      sc_mage * mage = get_mage(u);
      if (mage->magietyp == M_ASTRAL || mage->magietyp == M_TRAUM) {
        mp *= 2;
      }
    }
    break;
  }
  return (int)(dk*mp);
}

enum {
  TRAVEL_NORMAL,
  TRAVEL_FOLLOWING,
  TRAVEL_TRANSPORTED,
  TRAVEL_RUNNING
};

static region_list *
travel_route(unit * u, region_list * route_begin, region_list * route_end, order * ord, int mode)
{
  region * r = u->region;
  region * current = u->region;
  region_list * iroute = route_begin;
  int steps = 0;

  while (iroute && iroute!=route_end) {
    region * next = iroute->data;
    direction_t reldir = reldirection(current, next);
    border * b = get_borders(current, next);

    /* check if we are caught by guarding units */
    if (iroute!=route_begin && mode!=TRAVEL_RUNNING && mode!=TRAVEL_TRANSPORTED) {
      unit * wache = bewegung_blockiert_von(u, current);
      if (wache!=NULL) {
        ADDMSG(&u->faction->msgs, msg_message("moveblockedbyguard", 
          "unit region guard", u, current, wache));
        break;
      }
    }

    /* check movement from/to oceans. 
     * aquarian special, flying units, horses, the works */
    if ((u->race->flags & RCF_FLY) == 0) {
      if (rterrain(current)==T_OCEAN && rterrain(next) != T_OCEAN) {
        int moving = u->race->flags & (RCF_SWIM|RCF_WALK|RCF_COASTAL);
        /* Die Einheit kann nicht fliegen, ist im Ozean, und will an Land */
        if (moving != (RCF_SWIM|RCF_WALK) && (moving|RCF_COASTAL)==0) {
          /* can't swim+walk and isn't allowed to enter coast from sea */
          if (ord!=NULL) cmistake(u, ord, 44, MSG_MOVE);
          break;
        }
      }

      if (rterrain(current)==T_OCEAN || rterrain(next) == T_OCEAN) {
        /* trying to enter or exit ocean with horses, are we? */
        if (get_item(u, I_HORSE) > 0 || get_item(u, I_UNICORN) > 0) {
          /* tries to do it with horses */
          if (ord!=NULL) cmistake(u, ord, 67, MSG_MOVE);
          break;
        }
      }

      /* Ozeanfelder können nur von Einheiten mit Schwimmen und ohne
       * Pferde betreten werden. */
      if (rterrain(next) == T_OCEAN && !canswim(u)) {
        ADDMSG(&u->faction->msgs, msg_message("detectocean",
          "unit region", u, next));
        break;
      }
    }

    /* movement blocked by a wall */
    if (reldir>=0 && move_blocked(u, current, next)) {
      ADDMSG(&u->faction->msgs, msg_message("leavefail", 
        "unit region", u, next));
      break;
    }

    /* region ownership only: region owned by enemies */
    if (!entrance_allowed(u, next)) {
      ADDMSG(&u->faction->msgs, msg_message("regionowned", 
        "unit region target", u, current, next));
      break;
    }

    /* illusionary units disappear in antimagic zones */
    if (fval(u->race, RCF_ILLUSIONARY)) {
      curse * c = get_curse(next->attribs, ct_find("antimagiczone"));
      if (curse_active(c)) {
        curse_changevigour(&next->attribs, c, - u->number);
        ADDMSG(&u->faction->msgs, msg_message("illusionantimagic", "unit", u));
        set_number(u, 0);
        break;
      }
    }

    /* terrain is marked as forbidden (curse, etc) */
    if (terrain[rterrain(next)].flags & FORBIDDEN_LAND) {
      ADDMSG(&u->faction->msgs, msg_message("detectforbidden", 
        "unit region", u, next));
      break;
    }

    /* unit is an insect and cannot move into a glacier */
    if (u->race==new_race[RC_INSECT]) {
      if (r_insectstalled(next) && is_freezing(u)) {
        ADDMSG(&u->faction->msgs, msg_message("detectforbidden",
          "unit region", u, next));
        break;
      }
    }

    /* effect of borders */
    while (b!=NULL) {
      if (b->type->move) {
        b->type->move(b, u, current, next, false);
      }
      b = b->next;
    }

    current = next;
    iroute = iroute->next;
    ++steps;
    if (u->number==0) break;
  }

  if (iroute!=route_begin) {
    /* the unit has moved at least one region */
    int walkmode;
    region_list *rlist = route_begin;
    char * p = buf;
    region * next = r;

    setguard(u, GUARD_NONE);
    cycle_route(ord, u, steps);

    if (mode==TRAVEL_RUNNING) {
      walkmode = 0;
    } if (canride(u)) {
      walkmode = 1;
      produceexp(u, SK_RIDING, u->number);
    } else {
      walkmode = 2;
    }

    /* Berichte über Durchreiseregionen */

    *p = 0;
    while (rlist!=iroute) {
      if (next!=u->region && next!=current) {
        MSG(("travelthru_trail", "region", next), 
          p, sizeof(buf) - (p-buf), u->faction->locale, u->faction);
        if (rlist->data!=current) {
          if (rlist->next->data==current) scat(" und ");
          else scat(", ");
        }
        p += strlen(p);
      }
      next = rlist->data;
      rlist = rlist->next;
    }
    if (mode!=TRAVEL_TRANSPORTED) {
      ADDMSG(&u->faction->msgs, msg_message("travel", 
        "unit mode start end regions", u, walkmode, r, current, strdup(buf)));
    }

    mark_travelthru(u, r, route_begin, iroute);
    move_unit(u, current, NULL);

    /* make orders for the followers */
  }
  fset(u, UFL_LONGACTION);
  setguard(u, GUARD_NONE);
  assert(u->region==current);
  return iroute;
}

static boolean
ship_ready(const region * r, unit * u)
{
	if (!fval(u, UFL_OWNER)) {
		cmistake(u, u->thisorder, 146, MSG_MOVE);
		return false;
	}
	if (eff_skill(u, SK_SAILING, r) < u->ship->type->cptskill) {
		sprintf(buf, "Der Kapitän muß mindestens Segeln %d haben, "
				"um %s zu befehligen ", u->ship->type->cptskill,
				LOC(u->faction->locale, u->ship->type->name[1]));
		mistake(u, u->thisorder, buf, MSG_MOVE);
		return false;
	}
	assert(u->ship->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
	if (u->ship->size!=u->ship->type->construction->maxsize) {
		cmistake(u, u->thisorder, 15, MSG_MOVE);
		return false;
	}
	if (!enoughsailors(u->ship, r)) {
		cmistake(u, u->thisorder, 1, MSG_MOVE);
/*		mistake(u, u->thisorder,
				"Auf dem Schiff befinden sich zuwenig erfahrene Seeleute.", MSG_MOVE); */
		return false;
	}
	if (!cansail(r, u->ship)) {
		if( is_cursed(u->ship->attribs, C_SHIP_FLYING, 0) )
			cmistake(u, u->thisorder, 17, MSG_MOVE);
		else
			cmistake(u, u->thisorder, 18, MSG_MOVE);
		return false;
	}
	return true;
}

unit *
owner_buildingtyp(const region * r, const building_type * bt)
{
	building *b;
	unit *owner;

	for (b = rbuildings(r); b; b = b->next) {
		owner = buildingowner(r, b);
		if (b->type == bt && owner != NULL) {
			if (b->size >= bt->maxsize) {
				return owner;
			}
		}
	}

	return NULL;
}

boolean
buildingtype_exists(const region * r, const building_type * bt)
{
	building *b;

	for (b = rbuildings(r); b; b = b->next) {
		if (b->type == bt) {
			if (b->size >= bt->maxsize) {
				return true;
			}
		}
	}

	return false;
}

/* Prüft, ob Ablegen von einer Küste in eine der erlaubten Richtungen erfolgt. */

static boolean
check_takeoff(ship *sh, region *from, region *to)
{
  if (rterrain(from)!=T_OCEAN && sh->coast != NODIRECTION) {
    direction_t coast = sh->coast;
    direction_t dir   = reldirection(from, to);
    direction_t coastr  = (direction_t)((coast+1) % MAXDIRECTIONS);
    direction_t coastl  = (direction_t)((coast+MAXDIRECTIONS-1) % MAXDIRECTIONS);

    if (dir!=coast && dir!=coastl && dir!=coastr
      && !check_working_buildingtype(from, bt_find("harbour")))
    {
      return false;
    }
  }

  return true;
}

static void
sail(unit * u, order * ord, boolean move_on_land, region_list **routep)
{
  region *starting_point = u->region;
  region *current_point, *last_point;
  int k, step = 0;
  region_list **iroute = routep;
  ship * sh = u->ship;
  faction * f = u->faction;
  region * next_point = NULL;
  int error;
  const char * token = getstrtoken();

  if (routep) *routep = NULL;

  error = movewhere(u, token, starting_point, &next_point);
  if (error) {
    message * msg = movement_error(u, token, ord, error);
    if (msg!=NULL) {
      add_message(&u->faction->msgs, msg);
      msg_release(msg);
    }
    return;
  }

  if (!ship_ready(starting_point, u)) return;

  /* Wir suchen so lange nach neuen Richtungen, wie es geht. Diese werden
  * dann nacheinander ausgeführt. */

  k = shipspeed(sh, u);

  last_point = starting_point;
  current_point = starting_point;

  /* die nächste Region, in die man segelt, wird durch movewhere () aus der
  * letzten Region bestimmt.
  *
  * Anfangen tun wir bei starting_point. next_point ist beim ersten
  * Durchlauf schon gesetzt (Parameter!). current_point ist die letzte gültige,
  * befahrene Region. */

  while (next_point && current_point!=next_point && step < k) {
    const char * token;
    int error;
    terrain_t tthis = rterrain(current_point);
    /* these values need to be updated if next_point changes (due to storms): */
    terrain_t tnext = rterrain(next_point); 
    direction_t dir = reldirection(current_point, next_point);

    assert(sh==u->ship || !"ship has sunk, but we didn't notice it");

    if (terrain[tnext].flags & FORBIDDEN_LAND) {
      plane *pl = getplane(next_point);
      if (pl && fval(pl, PFL_NOCOORDS)) {
        ADDMSG(&f->msgs,  msg_message("sailforbiddendir", 
          "ship direction", sh, dir));
      } else {
        ADDMSG(&f->msgs, msg_message("sailforbidden", 
          "ship region", sh, next_point));
      }
      break;
    }

    if (!flying_ship(sh)) {
      static int stormyness = -1;
      int stormchance;

      if (stormyness==-1) {
        int thismonth = get_gamedate(turn, 0)->month;
        stormyness = storms[thismonth] * 5;
      }

      /* storms should be the first thing we do. */
      stormchance = stormyness / shipspeed(sh, u);
      if (check_leuchtturm(next_point, NULL)) stormchance /= 3;

      if (rand()%10000 < stormchance && current_point->terrain == T_OCEAN) {
        if (!is_cursed(sh->attribs, C_SHIP_NODRIFT, 0)) {
          region * rnext = NULL;
          boolean storm = true;
          int d_offset = rand() % MAXDIRECTIONS;
          direction_t d;
          /* Sturm nur, wenn nächste Region Hochsee ist. */
          for (d=0;d!=MAXDIRECTIONS;++d) {
            direction_t dnext = (direction_t)((d + d_offset) % MAXDIRECTIONS);
            region * rn = rconnect(current_point, dnext);

            if (rn!=NULL) {
              terrain_t t = rterrain(rn);

              if (terrain[t].flags & FORBIDDEN_LAND) continue;
              if (t!=T_OCEAN) {
                storm = false;
                break;
              }
              if (rn!=next_point) rnext = rn;
            }
          }
          if (storm && rnext!=NULL) {
            ADDMSG(&f->msgs, msg_message("storm", "ship region sink", 
              sh, current_point, sh->damage>=sh->size * DAMAGE_SCALE));

            /* damage the ship. we handle destruction in the end */
            damage_ship(sh, 0.02);
            if (sh->damage>=sh->size * DAMAGE_SCALE) break;

            next_point = rnext;
            /* these values need to be updated if next_point changes (due to storms): */
            tnext = rterrain(next_point); 
            dir = reldirection(current_point, next_point);
          }
        }
      }

      if (tthis != T_OCEAN) {
        if (tnext != T_OCEAN) {
          if (!move_on_land) {
            /* check that you're not traveling from one land region to another. */
            plane *pl = getplane(next_point);

            if (pl!=NULL && fval(pl, PFL_NOCOORDS)) {
              /* we don't have that case yet, but hey... */
              sprintf(buf, "Die %s entdeckt, daß im %s Festland ist.",
                shipname(sh), locale_string(u->faction->locale, directions[dir]));
            } else {
              sprintf(buf, "Die %s entdeckt, daß (%d,%d) Festland ist.",
                shipname(sh), region_x(next_point,u->faction),
                region_y(next_point,u->faction));
            }
            addmessage(0, u->faction, buf, MSG_MOVE, ML_WARN);
            break;
          }
        } else {
          if (check_takeoff(sh, current_point, next_point) == false) {
            /* Schiff kann nicht ablegen */
            cmistake(u, ord, 182, MSG_MOVE);
            break;
          }
        }
      } else if (tnext==T_OCEAN) {
        /* target region is an ocean, and we're not leaving a shore */
        if (!(sh->type->flags & SFL_OPENSEA)) {
          /* ship can only stay close to shore */
          direction_t d;
          
          for (d=0;d!=MAXDIRECTIONS;++d) {
            region * rc = rconnect(next_point, d);
            if (rterrain(rc) != T_OCEAN) break;
          }
          if (d==MAXDIRECTIONS) {
            /* Schiff kann nicht aufs offene Meer */
            cmistake(u, ord, 249, MSG_MOVE);
            break;
          }
        }
      }
    
      if (!ship_allowed(sh, next_point)) {
        /* for some reason or another, we aren't allowed in there.. */
        if (check_leuchtturm(current_point, NULL)) {
          ADDMSG(&f->msgs, msg_message("sailnolandingstorm", "ship", sh));
        } else {
          damage_ship(sh, 0.10);
          /* we handle destruction at the end */
        }
        break;
      }

      if (is_cursed(next_point->attribs, C_MAELSTROM, 0)) {
        if (do_maelstrom(next_point, u)==NULL) break;
      }

    } /* !flying_ship */

    /* Falls Blockade, endet die Seglerei hier */
    if (move_blocked(u, current_point, next_point)) {
      ADDMSG(&u->faction->msgs, msg_message("sailfail", "ship region", sh, current_point));
      break;
    }

    /* Falls kein Problem, eines weiter ziehen */
    fset(sh, SF_MOVED);
    if (iroute) {
      add_regionlist(iroute, next_point);
      iroute = &(*iroute)->next;
    }
    step++;

    last_point = current_point;
    current_point = next_point;

    if (rterrain(current_point) != T_OCEAN && !is_cursed(sh->attribs, C_SHIP_FLYING, 0)) break;
    token = getstrtoken();
    error = movewhere(u, token, current_point, &next_point);
    if (error || next_point==NULL) {
      message * msg = movement_error(u, token, ord, error);
      if (msg!=NULL) {
        add_message(&u->faction->msgs, msg);
        msg_release(msg);
      }
      next_point = current_point;
      break;
    }
  }

  if (sh->damage>=sh->size * DAMAGE_SCALE) {
    ADDMSG(&f->msgs, msg_message("shipsink", "ship", sh));
    destroy_ship(sh);
    sh = NULL;
  }

  /* Nun enthält current_point die Region, in der das Schiff seine Runde
  * beendet hat. Wir generieren hier ein Ereignis für den Spieler, das
  * ihm sagt, bis wohin er gesegelt ist, falls er überhaupt vom Fleck
  * gekommen ist. Das ist nicht der Fall, wenn er von der Küste ins
  * Inland zu segeln versuchte */

  if (sh!=NULL && fval(sh, SF_MOVED)) {
    unit * hafenmeister;
    /* nachdem alle Richtungen abgearbeitet wurden, und alle Einheiten
    * transferiert wurden, kann der aktuelle Befehl gelöscht werden. */
    cycle_route(ord, u, step);
    set_order(&u->thisorder, NULL);
    set_coast(sh, last_point, current_point);

    sprintf(buf, "Die %s ", shipname(sh));
    if( is_cursed(sh->attribs, C_SHIP_FLYING, 0) )
      scat("fliegt");
    else
      scat("segelt");
    scat(" von ");
    scat(regionname(starting_point, u->faction));
    scat(" nach ");
    scat(regionname(current_point, u->faction));
    scat(".");
    addmessage(0, u->faction, buf, MSG_MOVE, ML_INFO);

    /* Das Schiff und alle Einheiten darin werden nun von
    * starting_point nach current_point verschoben */

    /* Verfolgungen melden */
    if (fval(u, UFL_FOLLOWING)) caught_target(current_point, u);

    sh = move_ship(sh, starting_point, current_point, *routep);

    /* Hafengebühren ? */

    hafenmeister = owner_buildingtyp(current_point, bt_find("harbour"));
    if (sh && hafenmeister != NULL) {
      item * itm;
      unit * u2;
      boolean first = true;
      item * trans = NULL;

      for (u2 = current_point->units; u2; u2 = u2->next) {
        if (u2->ship == sh &&
          !alliedunit(hafenmeister, u->faction, HELP_GUARD)) {


            if (effskill(hafenmeister, SK_OBSERVATION) > effskill(u2, SK_STEALTH)) {
              for (itm=u2->items; itm; itm=itm->next) {
                const luxury_type * ltype = resource2luxury(itm->type->rtype);
                if (ltype!=NULL && itm->number>0) {
                  int st = itm->number * effskill(hafenmeister, SK_TRADE) / 50;
                  st = min(itm->number, st);

                  if (st > 0) {
                    i_change(&u2->items, itm->type, -st);
                    i_change(&hafenmeister->items, itm->type, st);
                    i_add(&trans, i_new(itm->type, st));
                  }
                }
              }
            }
          }
      }
      if (trans) {
        sprintf(buf, "%s erhielt ", hafenmeister->name);
        for (itm = trans; itm; itm=itm->next) {
          if (!first) {
            if (itm->next!=NULL && itm->next->next==NULL) {
              scat(" und ");
            } else {
              scat(", ");
            }
          }
          first = false;
          icat(trans->number);
          scat(" ");
          if (itm->number == 1) {
            scat(locale_string(default_locale, resourcename(itm->type->rtype, 0)));
          } else {
            scat(locale_string(default_locale, resourcename(itm->type->rtype, NMF_PLURAL)));
          }
        }
        scat(" von der ");
        scat(shipname(u->ship));
        scat(".");
        addmessage(0, u->faction, buf, MSG_COMMERCE, ML_INFO);
        addmessage(0, hafenmeister->faction, buf, MSG_INCOME, ML_INFO);
        while (trans) i_remove(&trans, trans);
      }
    }
  }
}

unit *
get_captain(const ship * sh)
{
  const region * r = sh->region;
  unit *u;

  for (u = r->units; u; u = u->next) {
    if (u->ship == sh && eff_skill(u, SK_SAILING, r) >= sh->type->cptskill)
      return u;
  }

  return NULL;
}

/* Segeln, Wandern, Reiten 
* when this routine returns a non-zero value, movement for the region needs 
* to be done again because of followers that got new MOVE orders. 
* Setting FL_LONGACTION will prevent a unit from being handled more than once
* by this routine 
*
* the token parser needs to be initialized before calling this function!
*/

/** fleeing units use this function 
*/
void 
run_to(unit * u, region * to)
{
  region_list * route = NULL;
  add_regionlist(&route, to);
  travel_route(u, route, NULL, NULL, TRAVEL_RUNNING);
  free_regionlist(route);
  /* weder transport noch follow */
}

static region_list *
travel_i(unit * u, region_list * route_begin, region_list * route_end, order * ord, int mode, follower ** followers)
{
  region * r = u->region;

  switch (canwalk(u)) {
    case E_CANWALK_TOOHEAVY:
      cmistake(u, ord, 57, MSG_MOVE);
      return route_begin;
    case E_CANWALK_TOOMANYHORSES:
      cmistake(u, ord, 56, MSG_MOVE);
      return route_begin;
    case E_CANWALK_TOOMANYCARTS:
      cmistake(u, ord, 42, MSG_MOVE);
      return route_begin;
  }
  route_end = cap_route(r, route_begin, route_end, movement_speed(u));

  route_end = travel_route(u, route_begin, route_end, ord, mode);
  get_followers(u, r, route_end, followers);

  /* transportation */
  for (ord = u->orders; ord; ord = ord->next) {
    unit * ut;

    if (get_keyword(ord) != K_TRANSPORT) continue;
    
    init_tokens(ord);
    skip_token();
    ut = getunit(r, u->faction);
    if (ut!=NULL) {
      boolean found = false;
      if (get_keyword(ut->thisorder) == K_DRIVE && can_move(ut)) {
        if (!fval(ut, UFL_LONGACTION) && !LongHunger(ut)) {
          init_tokens(ut->thisorder);
          skip_token();
          if (getunit(u->region, ut->faction) == u) {
            region_list * route_to = travel_route(ut, route_begin, route_end, ord, TRAVEL_TRANSPORTED);

            if (route_to!=route_begin) {
              get_followers(ut, r, route_to, followers);
            }
            ADDMSG(&ut->faction->msgs, msg_message("transport", 
              "unit target start end", u, ut, r, ut->region));
            found = true;
          }
        }
        if (!found) {
          if (cansee(u->faction, u->region, ut, 0)) {
            cmistake(u, ord, 90, MSG_MOVE);
          } else {
            cmistake(u, ord, 63, MSG_MOVE);
          }
        }
      } else {
        cmistake(u, ord, 99, MSG_MOVE);
      }
    } else {
      cmistake(u, ord, 63, MSG_MOVE);
    }
  }
  return route_end;
}

/** traveling without ships
 * walking, flying or riding units use this function
 */
static void
travel(unit * u, region_list ** routep)
{
  region * r = u->region;
  region_list * route_end;
  region_list * route_begin = NULL;
  follower * followers = NULL;

  if (routep) *routep = NULL;

  /* a few pre-checks that need not be done for each step: */
  if (rterrain(r)!=T_OCEAN) {
    ship * sh = u->ship;
    /* An Land kein NACH wenn in dieser Runde Schiff VERLASSEN! */
    if (sh==NULL) {
      sh = leftship(u);
      if (sh && sh->region!=u->region) sh = NULL;
    }
    if (sh && is_guarded(r, u, GUARD_LANDING)) {
      cmistake(u, u->thisorder, 70, MSG_MOVE);
      return;
    }
    if (u->ship && u->race->flags & RCF_SWIM) {
      cmistake(u, u->thisorder, 143, MSG_MOVE);
      return;
    }
  }
  else if (u->ship && fval(u->ship, SF_MOVED)) {
    /* die Einheit ist auf einem Schiff, das sich bereits bewegt hat */
    cmistake(u, u->thisorder, 13, MSG_MOVE);
    return;
  }

  make_route(u, u->thisorder, routep);
  route_begin = *routep;

  /* und ab die post: */
  route_end = travel_i(u, route_begin, NULL, u->thisorder, TRAVEL_NORMAL, &followers);

  /* followers */
  while (followers!=NULL) {
    follower * fnext = followers->next;
    unit * uf = followers->uf;
    unit * ut = followers->ut;
    region_list * route_end = followers->route_end;
    
    free(followers);
    followers = fnext;

    if (uf->region==r) {
      order * follow_order;
      char str[32];

      /* construct an order */
      sprintf(str, "%s %s %s", 
        locale_string(uf->faction->locale, keywords[K_FOLLOW]), 
        locale_string(uf->faction->locale, parameters[P_UNIT]), 
        itoa36(ut->no));
      follow_order = parse_order(str, uf->faction->locale);

      route_end = travel_i(uf, route_begin, route_end, follow_order, TRAVEL_FOLLOWING, &followers);
      caught_target(uf->region, uf);
      free_order(follow_order);
    }
  }

}

static void
move(unit * u, boolean move_on_land)
{
  region_list * route = NULL;

  if (u->ship && fval(u, UFL_OWNER)) {
    sail(u, u->thisorder, move_on_land, &route);
  } else {
    travel(u, &route);
  }

  fset(u, UFL_LONGACTION);
  set_order(&u->thisorder, NULL);

  if (route!=NULL) free_regionlist(route);
}

typedef struct piracy_data {
  const struct faction * follow;
  direction_t dir;
} piracy_data;

static void 
piracy_init(struct attrib * a)
{
  a->data.v = calloc(1, sizeof(piracy_data));
}

static void 
piracy_done(struct attrib * a)
{
  free(a->data.v);
}

static attrib_type at_piracy_direction = {
	"piracy_direction",
	piracy_init,
	piracy_done,
	DEFAULT_AGE,
	NO_WRITE,
	NO_READ
};

static attrib * 
mk_piracy(const faction * f, direction_t target_dir)
{
  attrib * a = a_new(&at_piracy_direction);
  piracy_data * data = a->data.v;
  data->follow = f;
  data->dir = target_dir;
  return a;
}

static void
piracy_cmd(unit *u, struct order * ord)
{
	region *r = u->region;
	ship *sh = u->ship, *sh2;
	direction_t dir, target_dir = NODIRECTION;
	int         aff[MAXDIRECTIONS];
	int         saff = 0;
	int					*il;
	const char *s;
	boolean     all = true;
	attrib      *a;

	if (!sh) {
		cmistake(u, ord, 144, MSG_MOVE);
		return;
	}

	if (!fval(u, UFL_OWNER)) {
		cmistake(u, ord, 46, MSG_MOVE);
		return;
	}

	/* Feststellen, ob schon ein anderer alliierter Pirat ein
	 * Ziel gefunden hat. */

	il = intlist_init();

  init_tokens(ord);
  skip_token();
	s = getstrtoken();
	while(s && *s) {
		il = intlist_add(il, atoi(s));
		all = false;
		s = getstrtoken();
	}

	for (a = a_find(r->attribs, &at_piracy_direction); a; a=a->nexttype) {
    piracy_data * data = a->data.v;
		const faction *f = data->follow;

		if (alliedunit(u, f, HELP_FIGHT) && intlist_find(il, a->data.sa[1])) {
			target_dir = data->dir;
			break;
		}
	}

	/* Wenn nicht, sehen wir, ob wir ein Ziel finden. */

	if (target_dir == NODIRECTION) {

		/* Einheit ist also Kapitän. Jetzt gucken, in wievielen
		 * Nachbarregionen potentielle Opfer sind. */

		for(dir = 0; dir < MAXDIRECTIONS; dir++) {
			region *rc = rconnect(r, dir);
			aff[dir] = 0;
			if (rc && (terrain[rterrain(rc)].flags & SWIM_INTO)
					&& check_takeoff(sh, r, rc) == true) {

				for (sh2 = rc->ships; sh2; sh2 = sh2->next) {
					unit * cap = shipowner(sh2);
          if (cap) {
            faction * f = visible_faction(cap->faction, cap);
            if (alliedunit(u, f, HELP_GUARD)) continue;
            if (all || intlist_find(il, cap->faction->no)) {
              aff[dir]++;
            }
          }
				}

				/* Und aufaddieren. */
				saff += aff[dir];
			}
		}

		if(saff != 0) {
			saff = rand()%saff;
			for(dir = 0; dir < MAXDIRECTIONS; dir++) {
				if(saff < aff[dir]) break;
				saff -= aff[dir];
			}
			target_dir = dir;
      a = a_add(&r->attribs, mk_piracy(u->faction, target_dir));
		}
	}

	free(il);

	/* Wenn kein Ziel gefunden, entsprechende Meldung generieren */
	if (target_dir == NODIRECTION) {
		ADDMSG(&u->faction->msgs, msg_message("piratenovictim", 
      "ship region", sh, r));
		return;
	}

	/* Meldung generieren */
	ADDMSG(&u->faction->msgs, msg_message("piratesawvictim",
    "ship region dir", sh, r, target_dir));

	/* Befehl konstruieren */
	sprintf(buf, "%s %s", locale_string(u->faction->locale, keywords[K_MOVE]),
		locale_string(u->faction->locale, directions[target_dir]));
	set_order(&u->thisorder, parse_order(buf, u->faction->locale));

	/* Bewegung ausführen */
  init_tokens(u->thisorder);
  skip_token();
	move(u, true);
}

static void
age_traveldir(region *r)
{
	attrib *a = a_find(r->attribs, &at_traveldir);

	while(a) {
		a->data.ca[3]--;
		if(a->data.ca[3] <= 0) {
			attrib *an = a->nexttype;
			a_remove(&r->attribs, a);
			a = an;
		} else {
			a = a->nexttype;
		}
	}
}

static direction_t
hunted_dir(attrib *at, int id)
{
  attrib *a = a_find(at, &at_shiptrail);

  while (a!=NULL) {
    traveldir *t = (traveldir *)(a->data.v);
    if (t->no == id) return t->dir;
    a = a->nexttype;
  }

  return NODIRECTION;
}

static int
hunt(unit *u)
{
  region *rc = u->region;
  int moves, id;
  char command[256];
  direction_t dir;

  if (fval(u, UFL_LONGACTION)) {
    return 0;
  } else if (u->ship==NULL) {
    cmistake(u, u->thisorder, 144, MSG_MOVE);
    fset(u, UFL_LONGACTION); /* FOLGE SCHIFF ist immer lang */
    return 0;
  } else if(!fval(u, UFL_OWNER)) {
    cmistake(u, u->thisorder, 146, MSG_MOVE);
    fset(u, UFL_LONGACTION); /* FOLGE SCHIFF ist immer lang */
    return 0;
  } else if(attacked(u)) {
    cmistake(u, u->thisorder, 52, MSG_MOVE);
    fset(u, UFL_LONGACTION); /* FOLGE SCHIFF ist immer lang */
    return 0;
  } else if (!can_move(u)) {
    cmistake(u, u->thisorder, 55, MSG_MOVE);
    fset(u, UFL_LONGACTION); /* FOLGE SCHIFF ist immer lang */
    return 0;
  }

  id = getshipid();

  if (id <= 0) {
    cmistake(u,  u->thisorder, 20, MSG_MOVE);
    fset(u, UFL_LONGACTION); /* FOLGE SCHIFF ist immer lang */
    return 0;
  }

  dir = hunted_dir(rc->attribs, id);

  if (dir == NODIRECTION) {
    ship * sh = findship(id);
    if (sh==NULL || sh->region!=rc) {
      cmistake(u, u->thisorder, 20, MSG_MOVE);
    }
    fset(u, UFL_LONGACTION); /* FOLGE SCHIFF ist immer lang */
    return 0;
  }

  sprintf(command, "%s %s", locale_string(u->faction->locale, keywords[K_MOVE]),
    locale_string(u->faction->locale, directions[dir]));
  moves = 1;

  rc = rconnect(rc, dir);
  while (moves < shipspeed(u->ship, u) && (dir = hunted_dir(rc->attribs, id)) != NODIRECTION) 
  {
    strcat(command, " ");
    strcat(command, locale_string(u->faction->locale, directions[dir]));
    moves++;
    rc = rconnect(rc, dir);
  }

  /* In command steht jetzt das NACH-Kommando. */

  /* NACH ignorieren und Parsing initialisieren. */
  igetstrtoken(command);
  /* NACH ausführen */
  move(u, false);
  return 1; /* true -> Einheitenliste von vorne durchgehen */
}

void
destroy_damaged_ships(void)
{
	region *r;
	ship   *sh,*shn;

	for(r=regions;r;r=r->next) {
		for(sh=r->ships;sh;) {
			shn = sh->next;
			if (sh->damage>=sh->size * DAMAGE_SCALE) {
				destroy_ship(sh);
			}
			sh = shn;
		}
	}
}

#ifdef TODO /* Wenn Feature ausgearbeitet */

static boolean
is_disorientated(unit *u)
{
  static boolean init = false;
  static const curse_type * shipconf_ct, * regconf_ct;
  if (!init) { 
    init = true; 
    regconf_ct = ct_find("disorientationzone"); 
    shipconf_ct = ct_find("shipdisorientation"); 
  }
  if (u->ship && curse_active(get_curse(u->ship->attribs, shipconf_ct)))
    return true;

  if (curse_active(get_curse(u->region->attribs, regconf_ct)))
    return true;

  return false;
}

void
regain_orientation(region * r)
{
	ship *sh;
	curse *c;
	unit *u, *cap;
  static int thismonth = get_gamedate(turn, 0)->month;

	for (sh = r->ships; sh; sh = sh->next) {
		c = get_curse(sh->attribs, C_DISORIENTATION, 0);
		if(!c) continue;

		/* Das Schiff bekommt seine Orientierung zurück, wenn es:
		 * a) An Land treibt.
		 * b) Glück hat.
		 * c) In einer Region mit einem nicht verwirrten alliierten
		 * 		Schiff steht.
		 */

		cap = shipowner(r, sh);

		if (r->terrain != T_OCEAN || rand() % 10 >= storms[thismonth]) {
			remove_curse(&sh->attribs, C_DISORIENTATION, 0);
				ADDMSG(&cap->faction->msgs, msg_message("shipnoconf", "ship", sh));
        continue;
		}

		for(u=r->units;u;u=u->next) {
			if(u != cap
					&& allied(cap, u->faction, HELP_GUARD)
					&& is_disorientated(u) == false) {
				remove_curse(&sh->attribs, C_DISORIENTATION, 0);
				ADDMSG(&cap->faction->msgs, msg_message("shipnoconf", "ship", sh));
				break;
			}
		}
	}
}
#endif

/* Bewegung, Verfolgung, Piraterie */

/** ships that folow other ships
 * Dann generieren die jagenden Einheiten ihre Befehle und
 * bewegen sich. 
 * Following the trails of other ships.
 */
static void
move_hunters(void)
{
  region *r;

  for (r = regions; r; r = r->next) {
    unit ** up = &r->units, * u;
    for (u=r->units; u; u=u->next) freset(u, FL_DH);

    while (*up!=NULL) {
      unit * u = *up;

      if (!fval(u, UFL_MOVED)) {
        order * ord;

        for (ord=u->orders;ord;ord=ord->next) {
          if (get_keyword(ord) == K_FOLLOW) {
            param_t p;

            init_tokens(ord);
            skip_token();
            p = getparam(u->faction->locale);
            if (p != P_SHIP) {
              if (p != P_UNIT) {
                cmistake(u, ord, 240, MSG_MOVE);
              }
              break;
            }

            /* wir folgen definitiv einem Schiff. */

            if (fval(u, UFL_LONGACTION)) {
              cmistake(u, ord, 52, MSG_MOVE);
              break;
            } else if (!can_move(u)) {
              cmistake(u, ord, 55, MSG_MOVE);
              break;
            }

            if (!fval(u, UFL_LONGACTION) && !LongHunger(u) && hunt(u)) {
              up = &r->units;
              break;
            }
          }
        }
      }
      if (*up==u) up=&u->next;
    }
  }
}

/** Piraten and Drift 
 * 
 */
static void
move_pirates(void)
{
  region * r;

  for (r = regions; r; r = r->next) {
    unit ** up = &r->units;

    while (*up) {
      unit *u = *up;

      if (!fval(u, UFL_LONGACTION) && get_keyword(u->thisorder) == K_PIRACY) {
        piracy_cmd(u, u->thisorder);
        fset(u, UFL_LONGACTION);
      }

      if (*up==u) {
        /* not moved, use next unit */
        up = &u->next;
      } else if (*up && (*up)->region!=r) {
        /* moved the previous unit along with u (units on same ship)
        * must start from the beginning again */
        up = &r->units;
      }
      /* else *up is already the next unit */
    }

    a_removeall(&r->attribs, &at_piracy_direction);
    age_traveldir(r);
  }
}

void
movement(void)
{
  int ships;

  /* Initialize the additional encumbrance by transported units */
  init_transportation();

  /* Move ships in last phase, others first 
  * This is to make sure you can't land someplace and then get off the ship
  * in the same turn.
  */
  for (ships=0;ships<=1;++ships) {
    region * r = regions;
    while (r!=NULL) {
      unit ** up = &r->units;
      boolean repeat = false;

      while (*up) {
        unit *u = *up;
        keyword_t kword = get_keyword(u->thisorder);

        switch (kword) {
        case K_ROUTE:
        case K_MOVE:
          if (attacked(u)) {
            cmistake(u, u->thisorder, 52, MSG_MOVE);
            set_order(&u->thisorder, NULL);
          } else if (!can_move(u)) {
            cmistake(u, u->thisorder, 55, MSG_MOVE);
            set_order(&u->thisorder, NULL);
          } else {
            if (ships) {
              if (u->ship && fval(u, UFL_OWNER)) {
                init_tokens(u->thisorder);
                skip_token();
                move(u, false);
              }
            } else {
              if (u->ship==NULL || !fval(u, UFL_OWNER)) {
                init_tokens(u->thisorder);
                skip_token();
                move(u, false);
              }
            }
          }
          break;
        }
        if (u->region==r) {
          /* not moved, use next unit */
          up = &u->next;
        } else {
          if (*up && (*up)->region!=r) {
            /* moved the upcoming unit along with u (units on ships or followers,
            * for example). must start from the beginning again immediately */
            up = &r->units;
            repeat = false;
          } else {
            repeat = true;
          }
        }
        /* else *up is already the next unit */
      }
      if (repeat) continue;
      if (ships==0) {
        /* Abtreiben von beschädigten, unterbemannten, überladenen Schiffen */
        drifting_ships(r);
      }
      r = r->next;
    }
  }

  move_hunters();
  move_pirates();
}

/** Overrides long orders with a FOLLOW order if the target is moving.
 * FOLLOW SHIP is a long order, and doesn't need to be treated in here.
 */
void
follow_unit(void)
{
  region * r;

  for (r=regions;r;r=r->next) {
    unit * u;

    for (u=r->units;u;u=u->next) {
      attrib * a;
      order * ord;

      if (fval(u, UFL_LONGACTION) || LongHunger(u)) continue;
      a = a_find(u->attribs, &at_follow);
      for (ord=u->orders;ord;ord=ord->next) {
        const struct locale * lang = u->faction->locale;

        if (get_keyword(ord) == K_FOLLOW) {
          init_tokens(ord);
          skip_token();
          if (getparam(lang) == P_UNIT) {
            int id = read_unitid(u->faction, r);

            if (id>0) {
              unit * uf = findunit(id);
              if (!a) {
                a = a_add(&u->attribs, make_follow(uf));
              } else {
                a->data.v = uf;
              }
            } else if (a) {
              a_remove(&u->attribs, a);
              a = NULL;
            }
          }
        }
      }

      if (a && !fval(u, UFL_MOVED)) {
        unit * u2 = a->data.v;

        if (!u2 || u2->region!=r || !cansee(u->faction, r, u2, 0))
          continue;
        for (ord=u2->orders;ord;ord=ord->next) {
          switch (get_keyword(ord)) {
            case K_MOVE:
            case K_ROUTE:
            case K_DRIVE:
            case K_FOLLOW:
            case K_PIRACY:
              fset(u, UFL_FOLLOWING);
              fset(u2, UFL_FOLLOWED);
              set_order(&u->thisorder, NULL);
              break;
            default:
              continue;
          }
          break;
        }
      }
    }
  }
}
