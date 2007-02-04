/* vi: set ts=2:
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
#include "eressea.h"
#include "build.h"

/* kernel includes */
#include "alchemy.h"
#include "alliance.h"
#include "border.h"
#include "building.h"
#include "curse.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "magic.h"
#include "message.h"
#include "movement.h"
#include "order.h"
#include "pool.h"
#include "race.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "terrainid.h"
#include "unit.h"

/* from libutil */
#include <attrib.h>
#include <base36.h>
#include <event.h>
#include <goodies.h>
#include <resolve.h>
#include <xml.h>

/* from libc */
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* attributes inclues */
#include <attributes/matmod.h>
#include <attributes/alliance.h>

#define STONERECYCLE 50
/* Name, MaxGroesse, MinBauTalent, Kapazitaet, {Eisen, Holz, Stein, BauSilber,
 * Laen, Mallorn}, UnterSilber, UnterSpezialTyp, UnterSpezial */


static boolean
CheckOverload(void)
{
  static int value = -1;
  if (value<0) {
    const char * str = get_param(global.parameters, "rules.check_overload");
    value = str?atoi(str):0;
  }
  return value;
}

static int
slipthru(const region * r, const unit * u, const building * b)
{
  unit *u2;
  int n, o;

  /* b ist die burg, in die man hinein oder aus der man heraus will. */

  if (!b) {
    return 1;
  }
  if (b->besieged < b->size * SIEGEFACTOR) {
    return 1;
  }
  /* u wird am hinein- oder herausschluepfen gehindert, wenn STEALTH <=
   * OBSERVATION +2 der belagerer u2 ist */

  n = eff_skill(u, SK_STEALTH, r);

  for (u2 = r->units; u2; u2 = u2->next)
    if (usiege(u2) == b) {

      if (invisible(u, u2) >= u->number) continue;

      o = eff_skill(u2, SK_OBSERVATION, r);

      if (o + 2 >= n)
        return 0;   /* entdeckt! */
    }
  return 1;
}

boolean
can_contact(const region * r, const unit * u, const unit * u2)
{

  /* hier geht es nur um die belagerung von burgen */

  if (u->building == u2->building)
    return true;

  /* unit u is trying to contact u2 - unasked for contact. wenn u oder u2
   * nicht in einer burg ist, oder die burg nicht belagert ist, ist
   * slipthru () == 1. ansonsten ist es nur 1, wenn man die belagerer */

  if (slipthru(u->region, u, u->building)
      && slipthru(u->region, u2, u2->building))
    return true;

  if (alliedunit(u, u2->faction, HELP_GIVE))
    return true;

  return false;
}


static void
contact_cmd(unit * u, order * ord, boolean tries)
{
  /* unit u kontaktiert unit u2. Dies setzt den contact einfach auf 1 -
   * ein richtiger toggle ist (noch?) nicht noetig. die region als
   * parameter ist nur deswegen wichtig, weil er an getunit ()
   * weitergegeben wird. dies wird fuer das auffinden von tempunits in
   * getnewunit () verwendet! */
  unit *u2;
  region * r = u->region;

  init_tokens(ord);
  skip_token();
  u2 = getunitg(r, u->faction);

  if (u2!=NULL) {
    if (!can_contact(r, u, u2)) {
      if (tries) cmistake(u, u->thisorder, 23, MSG_EVENT);
      return;
    }
    usetcontact(u, u2);
  }
}
/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */


/* ------------------------------------------------------------- */

struct building *
getbuilding(const struct region * r)
{
  building * b = findbuilding(getid());
  if (b==NULL || r!=b->region) return NULL;
  return b;
}

ship *
getship(const struct region * r)
{
  ship *sh, *sx = findship(getshipid());
  for (sh = r->ships; sh; sh = sh->next) {
    if (sh == sx) return sh;
  }
  return NULL;
}

/* ------------------------------------------------------------- */

static void
siege_cmd(unit * u, order * ord)
{
  region * r = u->region;
  unit *u2;
  building *b;
  int d, pooled;
  int bewaffnete, katapultiere = 0;
  static boolean init = false;
  static const curse_type * magicwalls_ct;
  static item_type * it_catapultammo = NULL;
  static item_type * it_catapult = NULL;
  if (!init) {
    init = true;
    magicwalls_ct = ct_find("magicwalls");
    it_catapultammo = it_find("catapultammo");
    it_catapult = it_find("catapult");
  }
  /* gibt es ueberhaupt Burgen? */

  init_tokens(ord);
  skip_token();
  b = getbuilding(r);

  if (!b) {
    cmistake(u, ord, 31, MSG_BATTLE);
    return;
  }

  if (!playerrace(u->race)) {
    /* keine Drachen, Illusionen, Untote etc */
    cmistake(u, ord, 166, MSG_BATTLE);
    return;
  }
  /* schaden durch katapulte */

  d = i_get(u->items, it_catapult);
  d = min(u->number, d);
  pooled = get_pooled(u, it_catapultammo->rtype, GET_DEFAULT, d);
  d = min(pooled, d);
  if (eff_skill(u, SK_CATAPULT, r) >= 1) {
    katapultiere = d;
    d *= eff_skill(u, SK_CATAPULT, r);
  } else {
    d = 0;
  }

  if ((bewaffnete = armedmen(u)) == 0 && d == 0) {
    /* abbruch, falls unbewaffnet oder unfaehig, katapulte zu benutzen */
    cmistake(u, ord, 80, MSG_EVENT);
    return;
  }

  if (!(getguard(u) & GUARD_TRAVELTHRU)) {
    /* abbruch, wenn die einheit nicht vorher die region bewacht - als
    * warnung fuer alle anderen! */
    cmistake(u, ord, 81, MSG_EVENT);
    return;
  }
  /* einheit und burg markieren - spart zeit beim behandeln der einheiten
  * in der burg, falls die burg auch markiert ist und nicht alle
  * einheiten wieder abgesucht werden muessen! */

  usetsiege(u, b);
  b->besieged += max(bewaffnete, katapultiere);

  /* definitiver schaden eingeschraenkt */

  d = min(d, b->size - 1);

  /* meldung, schaden anrichten */
  if (d && !curse_active(get_curse(b->attribs, magicwalls_ct))) {
    b->size -= d;
    use_pooled(u, it_catapultammo->rtype, GET_SLACK|GET_RESERVE|GET_POOLED_SLACK, d);
    d = 100 * d / b->size;
  } else d = 0;

  /* meldung fuer belagerer */
  ADDMSG(&u->faction->msgs, msg_message("siege",
    "unit building destruction", u, b, d));

  for (u2 = r->units; u2; u2 = u2->next) freset(u2->faction, FL_DH);
  fset(u->faction, FL_DH);

  /* Meldung fuer Burginsassen */
  for (u2 = r->units; u2; u2 = u2->next) {
    if (u2->building == b && !fval(u2->faction, FL_DH)) {
      fset(u2->faction, FL_DH);
      ADDMSG(&u2->faction->msgs, msg_message("siege",
        "unit building destruction", u, b, d));
    }
  }
}

void
do_siege(region *r)
{
  if (fval(r->terrain, LAND_REGION)) {
    unit *u;

    for (u = r->units; u; u = u->next) {
      if (get_keyword(u->thisorder) == K_BESIEGE) {
        siege_cmd(u, u->thisorder);
      }
    }
  }
}
/* ------------------------------------------------------------- */

static void
destroy_road(unit *u, int nmax, struct order * ord)
{
  direction_t d = getdirection(u->faction->locale);
  unit *u2;
  region *r = u->region;
  short n = (short)nmax;

  if (nmax>SHRT_MAX) n = SHRT_MAX;
  else if (nmax<0) n = 0;

  for (u2=r->units;u2;u2=u2->next) {
    if (u2->faction!=u->faction && getguard(u2)&GUARD_TAX
      && cansee(u2->faction, u->region, u, 0)
      && !alliedunit(u, u2->faction, HELP_GUARD)) {
      cmistake(u, ord, 70, MSG_EVENT);
      return;
    }
  }

  if (d==NODIRECTION) {
    cmistake(u, ord, 71, MSG_PRODUCE);
  } else {
    short road = rroad(r, d);
    n = min(n, road);
    if (n!=0) {
      region * r2 = rconnect(r,d);
      int willdo = eff_skill(u, SK_ROAD_BUILDING, r)*u->number;
      willdo = min(willdo, n);
      if (willdo==0) {
        /* TODO: error message */
      }
      if (willdo>SHRT_MAX) road = 0;
      else road = road - (short)willdo;
      rsetroad(r, d, road);
      ADDMSG(&u->faction->msgs, msg_message("destroy_road",
        "unit from to", u, r, r2));
    }
  }
}

int
destroy_cmd(unit * u, struct order * ord)
{
  ship *sh;
  unit *u2;
  region * r = u->region;
#if 0
  const construction * con = NULL;
  int size = 0;
#endif
  const char *s;
  int n = INT_MAX;

  if (u->number < 1)
    return 0;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (findparam(s, u->faction->locale)==P_ROAD) {
    destroy_road(u, INT_MAX, ord);
    return 0;
  }

  if (s && *s) {
    n = atoi(s);
    if(n <= 0) {
      cmistake(u, ord, 288, MSG_PRODUCE);
      return 0;
    }
  }

  if (getparam(u->faction->locale) == P_ROAD) {
    destroy_road(u, n, ord);
    return 0;
  }

  if (!fval(u, UFL_OWNER)) {
    cmistake(u, ord, 138, MSG_PRODUCE);
    return 0;
  }

  if (u->building) {
    building *b = u->building;

    if(n >= b->size) {
      /* destroy completly */
      /* all units leave the building */
      for (u2 = r->units; u2; u2 = u2->next)
        if (u2->building == b) {
          u2->building = 0;
          freset(u2, UFL_OWNER);
        }
        ADDMSG(&u->faction->msgs, msg_message("destroy",
          "building unit", b, u));
        destroy_building(b);
    } else {
      /* partial destroy */
      b->size -= n;
      ADDMSG(&u->faction->msgs, msg_message("destroy_partial",
        "building unit", b, u));
    }
  } else if (u->ship) {
    sh = u->ship;

    if (fval(r->terrain, SEA_REGION)) {
      cmistake(u, ord, 14, MSG_EVENT);
      return 0;
    }

    if(n >= (sh->size*100)/sh->type->construction->maxsize) {
      /* destroy completly */
      /* all units leave the ship */
      for (u2 = r->units; u2; u2 = u2->next)
        if (u2->ship == sh) {
          u2->ship = 0;
          freset(u2, UFL_OWNER);
        }
        ADDMSG(&u->faction->msgs, msg_message("shipdestroy",
          "unit region ship", u, r, sh));
        destroy_ship(sh);
    } else {
      /* partial destroy */
      sh->size -= (sh->type->construction->maxsize * n)/100;
      ADDMSG(&u->faction->msgs, msg_message("shipdestroy_partial",
        "unit region ship", u, r, sh));
    }
  } else {
    log_error(("Die Einheit %s von %s war owner eines objects, war aber weder in einer Burg noch in einem Schiff.\n",
      unitname(u), u->faction->name, u->faction->email));
  }

#if 0
  /* Achtung: Nicht an ZERSTÖRE mit Punktangabe angepaßt! */
  if (con) {
    /* TODO: ZERSTÖRE - Man sollte alle Materialien zurückkriegen können: */
    int c;
    for (c=0;con->materials[c].number;++c) {
      const requirement * rq = con->materials+c;
      int recycle = (int)(rq->recycle * rq->number * size/con->reqsize);
      if (recycle)
        change_resource(u, rq->rtype, recycle);
    }
  }
#endif
  return 0;
}
/* ------------------------------------------------------------- */

void
build_road(region * r, unit * u, int size, direction_t d)
{
  int n, left;
  region * rn = rconnect(r,d);

  assert(u->number);
  if (!eff_skill(u, SK_ROAD_BUILDING, r)) {
    cmistake(u, u->thisorder, 103, MSG_PRODUCE);
    return;
  }
  if (besieged(u)) {
    cmistake(u, u->thisorder, 60, MSG_PRODUCE);
    return;
  }

  if (rn==NULL || rn->terrain->max_road < 0) {
    cmistake(u, u->thisorder, 94, MSG_PRODUCE);
    return;
  }

  if (r->terrain->max_road < 0) {
    cmistake(u, u->thisorder, 94, MSG_PRODUCE);
    return;
  }

  if (r->terrain == newterrain(T_SWAMP)) {
    /* wenn kein Damm existiert */
    static const struct building_type * bt_dam;
    if (!bt_dam) bt_dam = bt_find("dam");
    assert(bt_dam);
    if (!buildingtype_exists(r, bt_dam)) {
      cmistake(u, u->thisorder, 132, MSG_PRODUCE);
      return;
    }
  } else if (r->terrain == newterrain(T_DESERT)) {
    static const struct building_type * bt_caravan;
    if (!bt_caravan) bt_caravan = bt_find("caravan");
    assert(bt_caravan);
    /* wenn keine Karawanserei existiert */
    if (!buildingtype_exists(r, bt_caravan)) {
      cmistake(u, u->thisorder, 133, MSG_PRODUCE);
      return;
    }
  } else if (r->terrain == newterrain(T_GLACIER)) {
    static const struct building_type * bt_tunnel;
    if (!bt_tunnel) bt_tunnel = bt_find("tunnel");
    assert(bt_tunnel);
    /* wenn kein Tunnel existiert */
    if (!buildingtype_exists(r, bt_tunnel)) {
      cmistake(u, u->thisorder, 131, MSG_PRODUCE);
      return;
    }
  }

  /* left kann man noch bauen */
  left = r->terrain->max_road - rroad(r, d);

  /* hoffentlich ist r->road <= r->terrain->max_road, n also >= 0 */
  if (left <= 0) {
    sprintf(buf, "In %s gibt es keine Brücken und Straßen "
      "mehr zu bauen", regionname(r, u->faction));
    mistake(u, u->thisorder, buf, MSG_PRODUCE);
    return;
  }

  if (size>0) left = min(size, left);
  /* baumaximum anhand der rohstoffe */
  if (u->race == new_race[RC_STONEGOLEM]){
    n = u->number * GOLEM_STONE;
  } else {
    n = get_pooled(u, oldresourcetype[R_STONE], GET_DEFAULT, left);
    if (n==0) {
      cmistake(u, u->thisorder, 151, MSG_PRODUCE);
      return;
    }
  }
  left = min(n, left);

  /* n = maximum by skill. try to maximize it */
  n = u->number * eff_skill(u, SK_ROAD_BUILDING, r);
  if (n < left) {
    item * itm = *i_find(&u->items, olditemtype[I_RING_OF_NIMBLEFINGER]);
    if (itm!=NULL && itm->number>0) {
      int rings = min(u->number, itm->number);
      n = n * (9*rings+u->number) / u->number;
    }
  }
  if (n < left) {
    int dm = get_effect(u, oldpotiontype[P_DOMORE]);
    if (dm != 0) {
      int sk = eff_skill(u, SK_ROAD_BUILDING, r);
      int todo = (left - n + sk - 1) / sk;
      todo = min(todo, u->number);
      dm = min(dm, todo);
      change_effect(u, oldpotiontype[P_DOMORE], -dm);
      n += dm * sk;
    }             /* Auswirkung Schaffenstrunk */
  }

  /* make minimum of possible and available: */
  n = min(left, n);

  /* n is now modified by several special effects, so we have to
   * minimize it again to make sure the road will not grow beyond
   * maximum. */
  rsetroad(r, d, rroad(r, d) + (short)n);

  if (u->race == new_race[RC_STONEGOLEM]) {
    int golemsused = n / GOLEM_STONE;
    if (n%GOLEM_STONE != 0){
      ++golemsused;
    }
    scale_number(u, u->number - golemsused);
  } else {
    use_pooled(u, oldresourcetype[R_STONE], GET_DEFAULT, n);
    /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
    produceexp(u, SK_ROAD_BUILDING, min(n, u->number));
  }
  ADDMSG(&u->faction->msgs, msg_message("buildroad",
    "region unit size", r, u, n));
}
/* ------------------------------------------------------------- */

/* ** ** ** ** ** ** *
 *  new build rules  *
 * ** ** ** ** ** ** */

static int
required(int size, int msize, int maxneed)
  /* um size von msize Punkten zu bauen,
   * braucht man required von maxneed resourcen */
{
  int used;

  used = size * maxneed / msize;
  if (size * maxneed % msize)
    ++used;
  return used;
}

static int
matmod(const attrib * a, const unit * u, const resource_type * material, int value)
{
  for (a=a_find((attrib*)a, &at_matmod);a && a->type==&at_matmod;a=a->next) {
    mm_fun fun = (mm_fun)a->data.f;
    value = fun(u, material, value);
    if (value<0) return value; /* pass errors to caller */
  }
  return value;
}

/** Use up resources for building an object.
* Build up to 'size' points of 'type', where 'completed'
* of the first object have already been finished. return the
* actual size that could be built.
*/
int
build(unit * u, const construction * ctype, int completed, int want)
{
  const construction * type = ctype;
  int skills; /* number of skill points remainig */
  int dm = get_effect(u, oldpotiontype[P_DOMORE]);
  int made = 0;
  int basesk, effsk;

  assert(u->number);
  if (want<=0) return 0;
  if (type==NULL) return 0;
  if (type->improvement==NULL && completed==type->maxsize)
    return ECOMPLETE;

  basesk = effskill(u, type->skill);
  if (basesk==0) return ENEEDSKILL;

  effsk = basesk;
  if (inside_building(u)) {
    effsk = skillmod(u->building->type->attribs, u, u->region, type->skill,
        effsk, SMF_PRODUCTION);
  }
  effsk = skillmod(type->attribs, u, u->region, type->skill,
      effsk, SMF_PRODUCTION);
  if (effsk<0) return effsk; /* pass errors to caller */
  if (effsk==0) return ENEEDSKILL;

  skills = effsk * u->number;

  /* technically, nimblefinge and domore should be in a global set of
   * "game"-attributes, (as at_skillmod) but for a while, we're leaving
   * them in here. */

  if (dm != 0) {
    /* Auswirkung Schaffenstrunk */
    dm = min(dm, u->number);
    change_effect(u, oldpotiontype[P_DOMORE], -dm);
    skills += dm * effsk;
  }
  for (;want>0 && skills>0;) {
    int c, n;

    /* skip over everything that's already been done:
     * type->improvement==NULL means no more improvements, but no size limits
     * type->improvement==type means build another object of the same time
     * while material lasts type->improvement==x means build x when type
     * is finished */
    while (type->improvement!=NULL &&
         type->improvement!=type &&
         type->maxsize>0 &&
         type->maxsize<=completed)
    {
      completed -= type->maxsize;
      type = type->improvement;
    }
    if (type==NULL) {
      if (made==0) return ECOMPLETE;
      break; /* completed */
    }

    /*  Hier ist entweder maxsize == -1, oder completed < maxsize.
     *  Andernfalls ist das Datenfile oder sonstwas kaputt...
     *  (enno): Nein, das ist für Dinge, bei denen die nächste Ausbaustufe
     *  die gleiche wie die vorherige ist. z.b. gegenstände.
     */
    if (type->maxsize>1) {
      completed = completed % type->maxsize;
    }
    else {
      completed = 0; assert(type->reqsize>=1);
    }

    if (basesk < type->minskill) {
      if (made==0) return ELOWSKILL; /* not good enough to go on */
    }

    /* n = maximum buildable size */
    if (type->minskill > 1) {
      n = skills / type->minskill;
    } else {
      n = skills;
    }
    /* Flinkfingerring wirkt nicht auf Mengenbegrenzte (magische)
     * Talente */
    if (max_skill(u->faction, type->skill)==INT_MAX) {
      int i = 0;
      item * itm = *i_find(&u->items, olditemtype[I_RING_OF_NIMBLEFINGER]);
      if (itm!=NULL) i = itm->number;
      if (i>0) {
        int rings = min(u->number, i);
        n = n * (9*rings+u->number) / u->number;
      }
    }

    if (want>0) {
      n = min(want, n);
    }

    if (type->maxsize>0) {
      n = min(type->maxsize-completed, n);
      if (type->improvement==NULL) {
        want = n;
      }
    }

    if (type->materials) for (c=0;n>0 && type->materials[c].number;c++) {
      const struct resource_type * rtype = type->materials[c].rtype;
      int need, prebuilt;
      int canuse = get_pooled(u, rtype, GET_DEFAULT, INT_MAX);

      if (inside_building(u)) {
        canuse = matmod(u->building->type->attribs, u, rtype, canuse);
      }

      if (canuse<0) return canuse; /* pass errors to caller */
      canuse = matmod(type->attribs, u, rtype, canuse);
      if (type->reqsize>1) {
        prebuilt = required(completed, type->reqsize, type->materials[c].number);
        for (;n;) {
          need = required(completed + n, type->reqsize, type->materials[c].number);
          if (need-prebuilt<=canuse) break;
          --n; /* TODO: optimieren? */
        }
      } else {
        int maxn = canuse / type->materials[c].number;
        if (maxn < n) n = maxn;
      }
    }
    if (n<=0) {
      if (made==0) return ENOMATERIALS;
      else break;
    }
    if (type->materials) for (c=0;type->materials[c].number;c++) {
      const struct resource_type * rtype = type->materials[c].rtype;
      int prebuilt = required(completed, type->reqsize, type->materials[c].number);
      int need = required(completed + n, type->reqsize, type->materials[c].number);
      int multi = 1;
      int canuse = 100; /* normalization */
      if (inside_building(u)) canuse = matmod(u->building->type->attribs, u, rtype, canuse);
      if (canuse<0) return canuse; /* pass errors to caller */
      canuse = matmod(type->attribs, u, rtype, canuse);

      assert(canuse % 100 == 0 || !"only constant multipliers are implemented in build()");
      multi = canuse/100;
      if (canuse<0) return canuse; /* pass errors to caller */

      use_pooled(u, rtype, GET_DEFAULT, (need-prebuilt+multi-1)/multi);
    }
    made += n;
    skills -= n * type->minskill;
    want -= n;
    completed = completed + n;
  }
  /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
  produceexp(u, ctype->skill, min(made, u->number));

  return made;
}

int
maxbuild(const unit * u, const construction * cons)
  /* calculate maximum size that can be built from available material */
  /* !! ignores maximum objectsize and improvements...*/
{
  int c;
  int maximum = INT_MAX;
  for (c=0;cons->materials[c].number;c++) {
    const resource_type * rtype = cons->materials[c].rtype;
    int have = get_pooled(u, rtype, GET_DEFAULT, INT_MAX);
    int need = required(1, cons->reqsize, cons->materials[c].number);
    if (have<need) {
      cmistake(u, u->thisorder, 88, MSG_PRODUCE);
      return 0;
    }
    else maximum = min(maximum, have/need);
  }
  return maximum;
}

/** old build routines */

void
build_building(unit * u, const building_type * btype, int want, order * ord)
{
  region * r = u->region;
  boolean newbuilding = false;
  int c, built = 0, id;
  building * b = NULL;
  /* einmalige Korrektur */
  static char buffer[8 + IDSIZE + 1 + NAMESIZE + 1];
  const char * btname;
  order * new_order = NULL;
  const struct locale * lang = u->faction->locale;

  assert(u->number);
  if (eff_skill(u, SK_BUILDING, r) == 0) {
    cmistake(u, ord, 101, MSG_PRODUCE);
    return;
  }

  /* Falls eine Nummer angegeben worden ist, und ein Gebaeude mit der
   * betreffenden Nummer existiert, ist b nun gueltig. Wenn keine Burg
   * gefunden wurde, dann wird nicht einfach eine neue erbaut. Ansonsten
   * baut man an der eigenen burg weiter. */

  /* Wenn die angegebene Nummer falsch ist, KEINE Burg bauen! */
  id = atoi36(getstrtoken());
  if (id!=0){ /* eine Nummer angegeben, keine neue Burg bauen */
    b = findbuilding(id);
    if (!b || b->region != u->region){ /* eine Burg mit dieser Nummer gibt es hier nicht */
      /* vieleicht Tippfehler und die eigene Burg ist gemeint? */
      if (u->building && u->building->type==btype) {
        b = u->building;
      } else {
        /* keine neue Burg anfangen wenn eine Nummer angegeben war */
        cmistake(u, ord, 6, MSG_PRODUCE);
        return;
      }
    }
  }

  if (b) btype = b->type;

  if (b && fval(btype, BTF_UNIQUE) && buildingtype_exists(r, btype)) {
    /* only one of these per region */
    cmistake(u, ord, 93, MSG_PRODUCE);
    return;
  }
  if (besieged(u)) {
    /* units under siege can not build */
    cmistake(u, ord, 60, MSG_PRODUCE);
    return;
  }
  if (btype->flags & BTF_NOBUILD) {
    /* special building, cannot be built */
    cmistake(u, ord, 221, MSG_PRODUCE);
    return;
  }
  if (btype->flags & BTF_ONEPERTURN) {
    if(b && fval(b, BLD_EXPANDED)) {
      cmistake(u, ord, 318, MSG_PRODUCE);
      return;
    }
    want = 1;
  }

  if (b) built = b->size;
  if (want<=0 || want == INT_MAX) {
    if(b == NULL) {
      if(btype->maxsize > 0) {
        want = btype->maxsize - built;
      } else {
        want = INT_MAX;
      }
    } else {
      if(b->type->maxsize > 0) {
        want = b->type->maxsize - built;
      } else {
        want = INT_MAX;
      }
    }
  }
  built = build(u, btype->construction, built, want);

  switch (built) {
  case ECOMPLETE:
    /* the building is already complete */
    cmistake(u, ord, 4, MSG_PRODUCE);
    return;
  case ENOMATERIALS: {
    /* something missing from the list of materials */
    const construction * cons = btype->construction;
    resource * reslist = NULL;
    assert(cons);
    for (c=0;cons->materials[c].number; ++c) {
      resource * res = malloc(sizeof(resource));
      res->number = cons->materials[c].number / cons->reqsize;
      res->type = cons->materials[c].rtype;
      res->next = reslist;
      reslist = res;
    }
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "build_required",
      "required", reslist));
    return;
  }
  case ELOWSKILL:
  case ENEEDSKILL:
    /* no skill, or not enough skill points to build */
    cmistake(u, ord, 50, MSG_PRODUCE);
    return;
  }


  /* at this point, the building size is increased. */
  if (b==NULL) {
    /* build a new building */
    b = new_building(btype, r, lang);
    b->type = btype;
    fset(b, BLD_MAINTAINED);

    /* Die Einheit befindet sich automatisch im Inneren der neuen Burg. */
    leave(r, u);
    u->building = b;
    fset(u, UFL_OWNER);

#ifdef WDW_PYRAMID
    if(b->type == bt_find("pyramid") && u->faction->alliance != NULL) {
      attrib * a = a_add(&b->attribs, a_new(&at_alliance));
      a->data.i = u->faction->alliance->id;
    }
#endif

    newbuilding = true;
  }

  btname = LOC(lang, btype->_name);

  if (want-built <= 0) {
    /* gebäude fertig */
    strcpy(buffer, LOC(lang, "defaultorder"));
    new_order = parse_order(buffer, lang);
  } else if (want!=INT_MAX) {
    /* reduzierte restgröße */
    sprintf(buffer, "%s %d %s %s", LOC(lang, keywords[K_MAKE]), want-built, btname, buildingid(b));
    new_order = parse_order(buffer, lang);
  } else if (btname) {
    /* Neues Haus, Befehl mit Gebäudename */
    sprintf(buffer, "%s %s %s", LOC(lang, keywords[K_MAKE]), btname, buildingid(b));
    new_order = parse_order(buffer, u->faction->locale);
  }

  if (new_order) {
#ifdef LASTORDER
    set_order(&u->lastorder, new_order);
#else
    replace_order(&u->orders, ord, new_order);
    free_order(new_order);
#endif
  }

  b->size += built;
  fset(b, BLD_EXPANDED);

  update_lighthouse(b);


  ADDMSG(&u->faction->msgs, msg_message("buildbuilding",
    "building unit size", b, u, built));
}

static void
build_ship(unit * u, ship * sh, int want)
{
  const construction * construction = sh->type->construction;
  int size = (sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE;
  int n;
  int can = build(u, construction, size, want);

  if ((n=construction->maxsize - sh->size)>0 && can>0) {
    if (can>=n) {
     sh->size += n;
     can -= n;
    }
    else {
     sh->size += can;
     n=can;
     can = 0;
    }
  }

  if (sh->damage && can) {
    int repair = min(sh->damage, can * DAMAGE_SCALE);
    n += repair / DAMAGE_SCALE;
    if (repair % DAMAGE_SCALE) ++n;
    sh->damage = sh->damage - repair;
  }

  if (n) ADDMSG(&u->faction->msgs,
    msg_message("buildship", "ship unit size", sh, u, n));
}

void
create_ship(region * r, unit * u, const struct ship_type * newtype, int want, order * ord)
{
  static char buffer[IDSIZE + 2 * KEYWORDSIZE + 3];
  ship *sh;
  int msize;
  const construction * cons = newtype->construction;
  order * new_order;

  if (!eff_skill(u, SK_SHIPBUILDING, r)) {
    cmistake(u, ord, 100, MSG_PRODUCE);
    return;
  }
  if (besieged(u)) {
    cmistake(u, ord, 60, MSG_PRODUCE);
    return;
  }

  /* check if skill and material for 1 size is available */
  if (eff_skill(u, cons->skill, r) < cons->minskill) {
    sprintf(buf, "Um %s zu bauen, braucht man ein Talent von "
      "mindestens %d.", newtype->name[1], cons->minskill);
    mistake(u, ord, buf, MSG_PRODUCE);
    return;
  }

  msize = maxbuild(u, cons);
  if (msize==0) {
    cmistake(u, ord, 88, MSG_PRODUCE);
    return;
  }
  if (want>0) want = min(want, msize);
  else want = msize;

  sh = new_ship(newtype, u->faction->locale, r);

  leave(r, u);
  u->ship = sh;
  fset(u, UFL_OWNER);
  sprintf(buffer, "%s %s %s",
    locale_string(u->faction->locale, keywords[K_MAKE]), locale_string(u->faction->locale, parameters[P_SHIP]), shipid(sh));

  new_order = parse_order(buffer, u->faction->locale);
#ifdef LASTORDER
  set_order(&u->lastorder, new_order);
#else
  replace_order(&u->orders, ord, new_order);
  free_order(new_order);
#endif

  build_ship(u, sh, want);
}

void
continue_ship(region * r, unit * u, int want)
{
  const construction * cons;
  ship *sh;
  int msize;

  if (!eff_skill(u, SK_SHIPBUILDING, r)) {
    cmistake(u, u->thisorder, 100, MSG_PRODUCE);
    return;
  }

  /* Die Schiffsnummer bzw der Schiffstyp wird eingelesen */
  sh = getship(r);

  if (!sh) sh = u->ship;

  if (!sh) {
    cmistake(u, u->thisorder, 20, MSG_PRODUCE);
    return;
  }
  cons = sh->type->construction;
  assert(cons->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
  if (sh->size==cons->maxsize && !sh->damage) {
    cmistake(u, u->thisorder, 16, MSG_PRODUCE);
    return;
  }
  if (eff_skill(u, cons->skill, r) < cons->minskill) {
    sprintf(buf, "Um %s zu bauen, braucht man ein Talent von "
        "mindestens %d.", sh->type->name[1], cons->minskill);
    mistake(u, u->thisorder, buf, MSG_PRODUCE);
    return;
  }
  msize = maxbuild(u, cons);
  if (msize==0) {
    cmistake(u, u->thisorder, 88, MSG_PRODUCE);
    return;
  }
  if (want > 0) want = min(want, msize);
  else want = msize;

  build_ship(u, sh, want);
}
/* ------------------------------------------------------------- */

static boolean
mayenter(region * r, unit * u, building * b)
{
  unit *u2;
  if (fval(b, BLD_UNGUARDED)) return true;
  u2 = buildingowner(r, b);

  if (u2==NULL || ucontact(u2, u)
    || alliedunit(u2, u->faction, HELP_GUARD)) return true;

  return false;
}

static int
mayboard(const unit * u, const ship * sh)
{
  unit *u2 = shipowner(sh);

  return (!u2 || ucontact(u2, u) || alliedunit(u2, u->faction, HELP_GUARD));
}

int
leave_cmd(unit * u, struct order * ord)
{
  region * r = u->region;

  if (fval(u, UFL_ENTER)) {
    /* if we just entered this round, then we don't leave again */
    return 0;
  }

  if (fval(r->terrain, SEA_REGION) && u->ship) {
    if(!fval(u->race, RCF_SWIM)) {
      cmistake(u, ord, 11, MSG_MOVE);
      return 0;
    }
    if(get_item(u, I_HORSE)) {
      cmistake(u, ord, 231, MSG_MOVE);
      return 0;
    }
  }
  if (!slipthru(r, u, u->building)) {
    sprintf(buf, "%s wird belagert.", buildingname(u->building));
    mistake(u, ord, buf, MSG_MOVE);
  } else {
    leave(r, u);
  }
  return 0;
}

static boolean
enter_ship(unit * u, struct order * ord, int id, boolean report)
{
  region * r = u->region;
  ship * sh;

  /* Muß abgefangen werden, sonst könnten Schwimmer an
   * Bord von Schiffen an Land gelangen. */
  if (!fval(u->race, RCF_WALK) && !fval(u->race, RCF_FLY)) {
    cmistake(u, ord, 233, MSG_MOVE);
    return false;
  }

  sh = findship(id);
  if (sh == NULL || sh->region!=r) {
    if (report) cmistake(u, ord, 20, MSG_MOVE);
    return false;
  }
  if (sh==u->ship) return true;
  if (!mayboard(u, sh)) {
    if (report) cmistake(u, ord, 34, MSG_MOVE);
    return false;
  }
  if (CheckOverload()) {
    int sweight, scabins;
    int mweight = shipcapacity(sh);
    int mcabins = sh->type->cabins;
    if (mweight>0 && mcabins>0) {
      getshipweight(sh, &sweight, &scabins);
      sweight += weight(u);
      scabins += u->number;

      if (sweight > mweight || scabins > mcabins) {
        if (report) cmistake(u, ord, 34, MSG_MOVE);
        return false;
      }
    }
  }

  leave(u->region, u);
  u->ship = sh;

  if (shipowner(sh) == NULL) {
    fset(u, UFL_OWNER);
  }
  fset(u, UFL_ENTER);
  return true;
}

static boolean
enter_building(unit * u, order * ord, int id, boolean report)
{
  region * r = u->region;
  building * b;

  /* Schwimmer können keine Gebäude betreten, außer diese sind
  * auf dem Ozean */
  if (!fval(u->race, RCF_WALK) && !fval(u->race, RCF_FLY)) {
    if (!fval(r->terrain, SEA_REGION)) {
      if (report) {
        cmistake(u, ord, 232, MSG_MOVE);
      }
      return false;
    }
  }

  b = findbuilding(id);
  if (b==NULL || b->region!=r) {
    if (report) {
      cmistake(u, ord, 6, MSG_MOVE);
    }
    return false;
  }
  if (!mayenter(r, u, b)) {
    if (report) {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "entrance_denied", "building", b));
    }
    return false;
  }
  if (!slipthru(r, u, b)) {
    if (report) {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "entrance_besieged", "building", b));
    }
    return false;
  }

  leave(r, u);
  u->building = b;
  if (buildingowner(r, b) == 0) {
    fset(u, UFL_OWNER);
  }
  fset(u, UFL_ENTER);
  return true;
}

void
do_misc(region * r, boolean lasttry)
{
  unit **uptr, *uc;

  for (uc = r->units; uc; uc = uc->next) {
    order * ord;
    for (ord = uc->orders; ord; ord = ord->next) {
      switch (get_keyword(ord)) {
      case K_CONTACT:
        contact_cmd(uc, ord, lasttry);
        break;
      }
    }
  }

  for (uptr = &r->units; *uptr;) {
    unit * u = *uptr;
    order ** ordp = &u->orders;

    while (*ordp) {
      order * ord = *ordp;
      if (get_keyword(ord) == K_ENTER) {
        param_t p;
        int id;
        unit * ulast = NULL;

        init_tokens(ord);
        skip_token();
        p = getparam(u->faction->locale);
        id = getid();

        switch (p) {
        case P_BUILDING:
        case P_GEBAEUDE:
          if (u->building && u->building->no==id) break;
          if (enter_building(u, ord, id, lasttry)) {
            unit *ub;
            for (ub=u;ub;ub=ub->next) {
              if (ub->building==u->building) {
                ulast = ub;
              }
            }
          }
          break;

        case P_SHIP:
          if (u->ship && u->ship->no==id) break;
          if (enter_ship(u, ord, id, lasttry)) {
            unit *ub;
            ulast = u;
            for (ub=u;ub;ub=ub->next) {
              if (ub->ship==u->ship) {
                ulast = ub;
              }
            }
          }
          break;

        default:
          if (lasttry) cmistake(u, ord, 79, MSG_MOVE);

        }
        if (ulast!=NULL) {
          /* Wenn wir hier angekommen sind, war der Befehl
            * erfolgreich und wir löschen ihn, damit er im
            * zweiten Versuch nicht nochmal ausgeführt wird. */
          *ordp = ord->next;
          ord->next = NULL;
          free_order(ord);

          if (ulast!=u) {
            /* put u behind ulast so it's the last unit in the building */
            *uptr = u->next;
            u->next = ulast->next;
            ulast->next = u;
          }
          break;
        }
      }
      if (*ordp==ord) ordp = &ord->next;
    }
    if (*uptr==u) uptr = &u->next;
  }
}
