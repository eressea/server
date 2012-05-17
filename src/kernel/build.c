/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "build.h"

/* kernel includes */
#include "alchemy.h"
#include "alliance.h"
#include "connection.h"
#include "building.h"
#include "curse.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "magic.h"
#include "message.h"
#include "move.h"
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
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/resolve.h>
#include <util/xml.h>

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

static boolean CheckOverload(void)
{
  static int value = -1;
  if (value < 0) {
    value = get_param_int(global.parameters, "rules.check_overload", 0);
  }
  return value;
}

/* test if the unit can slip through a siege undetected.
 * returns 0 if siege is successful, or 1 if the building is either
 * not besieged or the unit can slip through the siege due to better stealth.
 */
static int slipthru(const region * r, const unit * u, const building * b)
{
  unit *u2;
  int n, o;

  /* b ist die burg, in die man hinein oder aus der man heraus will. */
  if (b == NULL || b->besieged < b->size * SIEGEFACTOR) {
    return 1;
  }

  /* u wird am hinein- oder herausschluepfen gehindert, wenn STEALTH <=
   * OBSERVATION +2 der belagerer u2 ist */
  n = eff_skill(u, SK_STEALTH, r);

  for (u2 = r->units; u2; u2 = u2->next) {
    if (usiege(u2) == b) {

      if (invisible(u, u2) >= u->number)
        continue;

      o = eff_skill(u2, SK_PERCEPTION, r);

      if (o + 2 >= n) {
        return 0;               /* entdeckt! */
      }
    }
  }
  return 1;
}

int can_contact(const region * r, const unit * u, const unit * u2)
{

  /* hier geht es nur um die belagerung von burgen */

  if (u->building == u2->building) {
    return 1;
  }

  /* unit u is trying to contact u2 - unasked for contact. wenn u oder u2
   * nicht in einer burg ist, oder die burg nicht belagert ist, ist
   * slipthru () == 1. ansonsten ist es nur 1, wenn man die belagerer */

  if (slipthru(u->region, u, u->building) && slipthru(u->region, u2, u2->building)) {
    return 1;
  }

  return (alliedunit(u, u2->faction, HELP_GIVE));
}

static void contact_cmd(unit * u, order * ord, int final)
{
  /* unit u kontaktiert unit u2. Dies setzt den contact einfach auf 1 -
   * ein richtiger toggle ist (noch?) nicht noetig. die region als
   * parameter ist nur deswegen wichtig, weil er an getunit ()
   * weitergegeben wird. dies wird fuer das auffinden von tempunits in
   * getnewunit () verwendet! */
  unit *u2;
  region *r = u->region;

  init_tokens(ord);
  skip_token();
  u2 = getunitg(r, u->faction);

  if (u2 != NULL) {
    if (!can_contact(r, u, u2)) {
      if (final)
        cmistake(u, u->thisorder, 23, MSG_EVENT);
      return;
    }
    usetcontact(u, u2);
  }
}

/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */

struct building *getbuilding(const struct region *r)
{
  building *b = findbuilding(getid());
  if (b == NULL || r != b->region)
    return NULL;
  return b;
}

ship *getship(const struct region * r)
{
  ship *sh, *sx = findship(getshipid());
  for (sh = r->ships; sh; sh = sh->next) {
    if (sh == sx)
      return sh;
  }
  return NULL;
}

/* ------------------------------------------------------------- */

static void siege_cmd(unit * u, order * ord)
{
  region *r = u->region;
  building *b;
  int d, pooled;
  int bewaffnete, katapultiere = 0;
  static boolean init = false;
  static const curse_type *magicwalls_ct;
  static item_type *it_catapultammo = NULL;
  static item_type *it_catapult = NULL;
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
  d = MIN(u->number, d);
  pooled = get_pooled(u, it_catapultammo->rtype, GET_DEFAULT, d);
  d = MIN(pooled, d);
  if (eff_skill(u, SK_CATAPULT, r) >= 1) {
    katapultiere = d;
    d *= eff_skill(u, SK_CATAPULT, r);
  } else {
    d = 0;
  }

  bewaffnete = armedmen(u, true);
  if (d == 0 && bewaffnete == 0) {
    /* abbruch, falls unbewaffnet oder unfaehig, katapulte zu benutzen */
    cmistake(u, ord, 80, MSG_EVENT);
    return;
  }

  if (!is_guard(u, GUARD_TRAVELTHRU)) {
    /* abbruch, wenn die einheit nicht vorher die region bewacht - als
     * warnung fuer alle anderen! */
    cmistake(u, ord, 81, MSG_EVENT);
    return;
  }
  /* einheit und burg markieren - spart zeit beim behandeln der einheiten
   * in der burg, falls die burg auch markiert ist und nicht alle
   * einheiten wieder abgesucht werden muessen! */

  usetsiege(u, b);
  b->besieged += MAX(bewaffnete, katapultiere);

  /* definitiver schaden eingeschraenkt */

  d = MIN(d, b->size - 1);

  /* meldung, schaden anrichten */
  if (d && !curse_active(get_curse(b->attribs, magicwalls_ct))) {
    b->size -= d;
    use_pooled(u, it_catapultammo->rtype,
      GET_SLACK | GET_RESERVE | GET_POOLED_SLACK, d);
    /* send message to the entire region */
    ADDMSG(&r->msgs, msg_message("siege_catapults",
        "unit building destruction", u, b, d));
  } else {
    /* send message to the entire region */
    ADDMSG(&r->msgs, msg_message("siege", "unit building", u, b));
  }
}

void do_siege(region * r)
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

static void destroy_road(unit * u, int nmax, struct order *ord)
{
  direction_t d = getdirection(u->faction->locale);
  unit *u2;
  region *r = u->region;
  short n = (short)nmax;

  if (nmax > SHRT_MAX)
    n = SHRT_MAX;
  else if (nmax < 0)
    n = 0;

  for (u2 = r->units; u2; u2 = u2->next) {
    if (u2->faction != u->faction && is_guard(u2, GUARD_TAX)
      && cansee(u2->faction, u->region, u, 0)
      && !alliedunit(u, u2->faction, HELP_GUARD)) {
      cmistake(u, ord, 70, MSG_EVENT);
      return;
    }
  }

  if (d == NODIRECTION) {
    /* Die Richtung wurde nicht erkannt */
    cmistake(u, ord, 71, MSG_PRODUCE);
  } else {
    short road = rroad(r, d);
    n = MIN(n, road);
    if (n != 0) {
      region *r2 = rconnect(r, d);
      int willdo = eff_skill(u, SK_ROAD_BUILDING, r) * u->number;
      willdo = MIN(willdo, n);
      if (willdo == 0) {
        /* TODO: error message */
      }
      if (willdo > SHRT_MAX)
        road = 0;
      else
        road = road - (short)willdo;
      rsetroad(r, d, road);
      ADDMSG(&u->faction->msgs, msg_message("destroy_road",
          "unit from to", u, r, r2));
    }
  }
}

int destroy_cmd(unit * u, struct order *ord)
{
  ship *sh;
  unit *u2;
  region *r = u->region;
  const construction *con = NULL;
  int size = 0;
  const char *s;
  int n = INT_MAX;

  if (u->number < 1)
    return 0;

  init_tokens(ord);
  skip_token();
  s = getstrtoken();

  if (findparam(s, u->faction->locale) == P_ROAD) {
    destroy_road(u, INT_MAX, ord);
    return 0;
  }

  if (s && *s) {
    n = atoi((const char *)s);
    if (n <= 0) {
      cmistake(u, ord, 288, MSG_PRODUCE);
      return 0;
    }
  }

  if (getparam(u->faction->locale) == P_ROAD) {
    destroy_road(u, n, ord);
    return 0;
  }

  if (u->building) {
    building *b = u->building;

    if (u!=building_owner(b)) {
      cmistake(u, ord, 138, MSG_PRODUCE);
      return 0;
    }
    if (fval(b->type, BTF_INDESTRUCTIBLE)) {
      cmistake(u, ord, 138, MSG_PRODUCE);
      return 0;
    }
    if (n >= b->size) {
      /* destroy completly */
      /* all units leave the building */
      for (u2 = r->units; u2; u2 = u2->next) {
        if (u2->building == b) {
          freset(u2, UFL_OWNER);
          leave_building(u2);
        }
      }
      ADDMSG(&u->faction->msgs, msg_message("destroy", "building unit", b, u));
      con = b->type->construction;
      remove_building(&r->buildings, b);
    } else {
      /* partial destroy */
      b->size -= n;
      ADDMSG(&u->faction->msgs, msg_message("destroy_partial",
          "building unit", b, u));
    }
  } else if (u->ship) {
    sh = u->ship;

    if (u!=ship_owner(sh)) {
      cmistake(u, ord, 138, MSG_PRODUCE);
      return 0;
    }
    if (fval(r->terrain, SEA_REGION)) {
      cmistake(u, ord, 14, MSG_EVENT);
      return 0;
    }

    if (n >= (sh->size * 100) / sh->type->construction->maxsize) {
      /* destroy completly */
      /* all units leave the ship */
      for (u2 = r->units; u2; u2 = u2->next) {
        if (u2->ship == sh) {
          leave_ship(u2);
        }
      }
      ADDMSG(&u->faction->msgs, msg_message("shipdestroy",
          "unit region ship", u, r, sh));
      con = sh->type->construction;
      remove_ship(&sh->region->ships, sh);
    } else {
      /* partial destroy */
      sh->size -= (sh->type->construction->maxsize * n) / 100;
      ADDMSG(&u->faction->msgs, msg_message("shipdestroy_partial",
          "unit region ship", u, r, sh));
    }
  } else {
    log_error("Die Einheit %s von %s war owner eines objects, war aber weder in einer Burg noch in einem Schiff.\n", unitname(u), u->faction->name, u->faction->email);
  }

  if (con) {
    /* TODO: Nicht an ZERSTÖRE mit Punktangabe angepaßt! */
    int c;
    for (c = 0; con->materials[c].number; ++c) {
      const requirement *rq = con->materials + c;
      int recycle = (int)(rq->recycle * rq->number * size / con->reqsize);
      if (recycle) {
        change_resource(u, rq->rtype, recycle);
      }
    }
  }
  return 0;
}

/* ------------------------------------------------------------- */

void build_road(region * r, unit * u, int size, direction_t d)
{
  int n, left;
  region *rn = rconnect(r, d);

  assert(u->number);
  if (!eff_skill(u, SK_ROAD_BUILDING, r)) {
    cmistake(u, u->thisorder, 103, MSG_PRODUCE);
    return;
  }
  if (besieged(u)) {
    cmistake(u, u->thisorder, 60, MSG_PRODUCE);
    return;
  }

  if (rn == NULL || rn->terrain->max_road < 0) {
    cmistake(u, u->thisorder, 94, MSG_PRODUCE);
    return;
  }

  if (r->terrain->max_road < 0) {
    cmistake(u, u->thisorder, 94, MSG_PRODUCE);
    return;
  }

  if (r->terrain == newterrain(T_SWAMP)) {
    /* wenn kein Damm existiert */
    static const struct building_type *bt_dam;
    if (!bt_dam)
      bt_dam = bt_find("dam");
    assert(bt_dam);
    if (!buildingtype_exists(r, bt_dam, true)) {
      cmistake(u, u->thisorder, 132, MSG_PRODUCE);
      return;
    }
  } else if (r->terrain == newterrain(T_DESERT)) {
    static const struct building_type *bt_caravan;
    if (!bt_caravan)
      bt_caravan = bt_find("caravan");
    assert(bt_caravan);
    /* wenn keine Karawanserei existiert */
    if (!buildingtype_exists(r, bt_caravan, true)) {
      cmistake(u, u->thisorder, 133, MSG_PRODUCE);
      return;
    }
  } else if (r->terrain == newterrain(T_GLACIER)) {
    static const struct building_type *bt_tunnel;
    if (!bt_tunnel)
      bt_tunnel = bt_find("tunnel");
    assert(bt_tunnel);
    /* wenn kein Tunnel existiert */
    if (!buildingtype_exists(r, bt_tunnel, true)) {
      cmistake(u, u->thisorder, 131, MSG_PRODUCE);
      return;
    }
  }

  /* left kann man noch bauen */
  left = r->terrain->max_road - rroad(r, d);

  /* hoffentlich ist r->road <= r->terrain->max_road, n also >= 0 */
  if (left <= 0) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
        "error_roads_finished", ""));
    return;
  }

  if (size > 0)
    left = MIN(size, left);
  /* baumaximum anhand der rohstoffe */
  if (u->race == new_race[RC_STONEGOLEM]) {
    n = u->number * GOLEM_STONE;
  } else {
    n = get_pooled(u, oldresourcetype[R_STONE], GET_DEFAULT, left);
    if (n == 0) {
      cmistake(u, u->thisorder, 151, MSG_PRODUCE);
      return;
    }
  }
  left = MIN(n, left);

  /* n = maximum by skill. try to maximize it */
  n = u->number * eff_skill(u, SK_ROAD_BUILDING, r);
  if (n < left) {
    item *itm = *i_find(&u->items, olditemtype[I_RING_OF_NIMBLEFINGER]);
    if (itm != NULL && itm->number > 0) {
      int rings = MIN(u->number, itm->number);
      n = n * ((roqf_factor() - 1) * rings + u->number) / u->number;
    }
  }
  if (n < left) {
    int dm = get_effect(u, oldpotiontype[P_DOMORE]);
    if (dm != 0) {
      int sk = eff_skill(u, SK_ROAD_BUILDING, r);
      int todo = (left - n + sk - 1) / sk;
      todo = MIN(todo, u->number);
      dm = MIN(dm, todo);
      change_effect(u, oldpotiontype[P_DOMORE], -dm);
      n += dm * sk;
    }                           /* Auswirkung Schaffenstrunk */
  }

  /* make minimum of possible and available: */
  n = MIN(left, n);

  /* n is now modified by several special effects, so we have to
   * minimize it again to make sure the road will not grow beyond
   * maximum. */
  rsetroad(r, d, rroad(r, d) + (short)n);

  if (u->race == new_race[RC_STONEGOLEM]) {
    int golemsused = n / GOLEM_STONE;
    if (n % GOLEM_STONE != 0) {
      ++golemsused;
    }
    scale_number(u, u->number - golemsused);
  } else {
    use_pooled(u, oldresourcetype[R_STONE], GET_DEFAULT, n);
    /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
    produceexp(u, SK_ROAD_BUILDING, MIN(n, u->number));
  }
  ADDMSG(&u->faction->msgs, msg_message("buildroad",
      "region unit size", r, u, n));
}

/* ------------------------------------------------------------- */

/* ** ** ** ** ** ** *
 *  new build rules  *
 * ** ** ** ** ** ** */

static int required(int size, int msize, int maxneed)
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
matmod(const attrib * a, const unit * u, const resource_type * material,
  int value)
{
  for (a = a_find((attrib *) a, &at_matmod); a && a->type == &at_matmod;
    a = a->next) {
    mm_fun fun = (mm_fun) a->data.f;
    value = fun(u, material, value);
    if (value < 0)
      return value;             /* pass errors to caller */
  }
  return value;
}

int roqf_factor(void)
{
  int value = -1;
  if (value < 0) {
    value = get_param_int(global.parameters, "rules.economy.roqf", 10);
  }
  return value;
}

/** Use up resources for building an object.
* Build up to 'size' points of 'type', where 'completed'
* of the first object have already been finished. return the
* actual size that could be built.
*/
int build(unit * u, const construction * ctype, int completed, int want)
{
  const construction *type = ctype;
  int skills = INT_MAX;         /* number of skill points remainig */
  int basesk = 0;
  int made = 0;

  if (want <= 0)
    return 0;
  if (type == NULL)
    return 0;
  if (type->improvement == NULL && completed == type->maxsize)
    return ECOMPLETE;
  if (type->btype != NULL) {
    building *b;
    if (!u->building || u->building->type != type->btype) {
      return EBUILDINGREQ;
    }
    b = inside_building(u);
    if (b == NULL)
      return EBUILDINGREQ;
  }

  if (type->skill != NOSKILL) {
    int effsk;
    int dm = get_effect(u, oldpotiontype[P_DOMORE]);

    assert(u->number);
    basesk = effskill(u, type->skill);
    if (basesk == 0)
      return ENEEDSKILL;

    effsk = basesk;
    if (inside_building(u)) {
      effsk = skillmod(u->building->type->attribs, u, u->region, type->skill,
        effsk, SMF_PRODUCTION);
    }
    effsk = skillmod(type->attribs, u, u->region, type->skill,
      effsk, SMF_PRODUCTION);
    if (effsk < 0)
      return effsk;             /* pass errors to caller */
    if (effsk == 0)
      return ENEEDSKILL;

    skills = effsk * u->number;

    /* technically, nimblefinge and domore should be in a global set of
     * "game"-attributes, (as at_skillmod) but for a while, we're leaving
     * them in here. */

    if (dm != 0) {
      /* Auswirkung Schaffenstrunk */
      dm = MIN(dm, u->number);
      change_effect(u, oldpotiontype[P_DOMORE], -dm);
      skills += dm * effsk;
    }
  }
  for (; want > 0 && skills > 0;) {
    int c, n;

    /* skip over everything that's already been done:
     * type->improvement==NULL means no more improvements, but no size limits
     * type->improvement==type means build another object of the same time
     * while material lasts type->improvement==x means build x when type
     * is finished */
    while (type->improvement != NULL &&
      type->improvement != type &&
      type->maxsize > 0 && type->maxsize <= completed) {
      completed -= type->maxsize;
      type = type->improvement;
    }
    if (type == NULL) {
      if (made == 0)
        return ECOMPLETE;
      break;                    /* completed */
    }

    /*  Hier ist entweder maxsize == -1, oder completed < maxsize.
     *  Andernfalls ist das Datenfile oder sonstwas kaputt...
     *  (enno): Nein, das ist für Dinge, bei denen die nächste Ausbaustufe
     *  die gleiche wie die vorherige ist. z.b. gegenstände.
     */
    if (type->maxsize > 1) {
      completed = completed % type->maxsize;
    } else {
      completed = 0;
      assert(type->reqsize >= 1);
    }

    if (basesk < type->minskill) {
      if (made == 0)
        return ELOWSKILL;       /* not good enough to go on */
    }

    /* n = maximum buildable size */
    if (type->minskill > 1) {
      n = skills / type->minskill;
    } else {
      n = skills;
    }
    /* Flinkfingerring wirkt nicht auf Mengenbegrenzte (magische)
     * Talente */
    if (skill_limit(u->faction, type->skill) == INT_MAX) {
      int i = 0;
      item *itm = *i_find(&u->items, olditemtype[I_RING_OF_NIMBLEFINGER]);
      if (itm != NULL)
        i = itm->number;
      if (i > 0) {
        int rings = MIN(u->number, i);
        n = n * ((roqf_factor() - 1) * rings + u->number) / u->number;
      }
    }

    if (want > 0) {
      n = MIN(want, n);
    }

    if (type->maxsize > 0) {
      n = MIN(type->maxsize - completed, n);
      if (type->improvement == NULL) {
        want = n;
      }
    }

    if (type->materials)
      for (c = 0; n > 0 && type->materials[c].number; c++) {
        const struct resource_type *rtype = type->materials[c].rtype;
        int need, prebuilt;
        int canuse = get_pooled(u, rtype, GET_DEFAULT, INT_MAX);

        if (inside_building(u)) {
          canuse = matmod(u->building->type->attribs, u, rtype, canuse);
        }

        if (canuse < 0)
          return canuse;        /* pass errors to caller */
        canuse = matmod(type->attribs, u, rtype, canuse);
        if (type->reqsize > 1) {
          prebuilt =
            required(completed, type->reqsize, type->materials[c].number);
          for (; n;) {
            need =
              required(completed + n, type->reqsize, type->materials[c].number);
            if (need - prebuilt <= canuse)
              break;
            --n;                /* TODO: optimieren? */
          }
        } else {
          int maxn = canuse / type->materials[c].number;
          if (maxn < n)
            n = maxn;
        }
      }
    if (n <= 0) {
      if (made == 0)
        return ENOMATERIALS;
      else
        break;
    }
    if (type->materials)
      for (c = 0; type->materials[c].number; c++) {
        const struct resource_type *rtype = type->materials[c].rtype;
        int prebuilt =
          required(completed, type->reqsize, type->materials[c].number);
        int need =
          required(completed + n, type->reqsize, type->materials[c].number);
        int multi = 1;
        int canuse = 100;       /* normalization */
        if (inside_building(u))
          canuse = matmod(u->building->type->attribs, u, rtype, canuse);
        if (canuse < 0)
          return canuse;        /* pass errors to caller */
        canuse = matmod(type->attribs, u, rtype, canuse);

        assert(canuse % 100 == 0
          || !"only constant multipliers are implemented in build()");
        multi = canuse / 100;
        if (canuse < 0)
          return canuse;        /* pass errors to caller */

        use_pooled(u, rtype, GET_DEFAULT,
          (need - prebuilt + multi - 1) / multi);
      }
    made += n;
    skills -= n * type->minskill;
    want -= n;
    completed = completed + n;
  }
  /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
  produceexp(u, ctype->skill, MIN(made, u->number));

  return made;
}

message *msg_materials_required(unit * u, order * ord,
  const construction * ctype, int multi)
{
  int c;
  /* something missing from the list of materials */
  resource *reslist = NULL;

  if (multi <= 0 || multi == INT_MAX)
    multi = 1;
  for (c = 0; ctype->materials[c].number; ++c) {
    resource *res = malloc(sizeof(resource));
    res->number = multi * ctype->materials[c].number / ctype->reqsize;
    res->type = ctype->materials[c].rtype;
    res->next = reslist;
    reslist = res;
  }
  return msg_feedback(u, ord, "build_required", "required", reslist);
}

int maxbuild(const unit * u, const construction * cons)
  /* calculate maximum size that can be built from available material */
  /* !! ignores maximum objectsize and improvements... */
{
  int c;
  int maximum = INT_MAX;
  for (c = 0; cons->materials[c].number; c++) {
    const resource_type *rtype = cons->materials[c].rtype;
    int have = get_pooled(u, rtype, GET_DEFAULT, INT_MAX);
    int need = required(1, cons->reqsize, cons->materials[c].number);
    if (have < need) {
      return 0;
    } else
      maximum = MIN(maximum, have / need);
  }
  return maximum;
}

/** old build routines */

void
build_building(unit * u, const building_type * btype, int want, order * ord)
{
  region *r = u->region;
  boolean newbuilding = false;
  int n = want, built = 0, id;
  building *b = NULL;
  /* einmalige Korrektur */
  const char *btname;
  order *new_order = NULL;
  const struct locale *lang = u->faction->locale;
  static int rule_other = -1;

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
  id = getid();
  if (id > 0) {                 /* eine Nummer angegeben, keine neue Burg bauen */
    b = findbuilding(id);
    if (!b || b->region != u->region) { /* eine Burg mit dieser Nummer gibt es hier nicht */
      /* vieleicht Tippfehler und die eigene Burg ist gemeint? */
      if (u->building && u->building->type == btype) {
        b = u->building;
      } else {
        /* keine neue Burg anfangen wenn eine Nummer angegeben war */
        cmistake(u, ord, 6, MSG_PRODUCE);
        return;
      }
    }
  } else if (u->building && u->building->type == btype) {
    b = u->building;
  }

  if (b)
    btype = b->type;

  if (fval(btype, BTF_UNIQUE) && buildingtype_exists(r, btype, false)) {
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
  if (r->terrain->max_road <= 0) {
    /* special terrain, cannot build */
    cmistake(u, ord, 221, MSG_PRODUCE);
    return;
  }
  if (btype->flags & BTF_ONEPERTURN) {
    if (b && fval(b, BLD_EXPANDED)) {
      cmistake(u, ord, 318, MSG_PRODUCE);
      return;
    }
    n = 1;
  }
  if (b) {
    if (rule_other < 0) {
      rule_other =
        get_param_int(global.parameters, "rules.build.other_buildings", 1);
    }
    if (!rule_other) {
      unit *owner = building_owner(b);
      if (!owner || owner->faction != u->faction) {
        cmistake(u, ord, 1222, MSG_PRODUCE);
        return;
      }
    }
  }

  if (b)
    built = b->size;
  if (n <= 0 || n == INT_MAX) {
    if (b == NULL) {
      if (btype->maxsize > 0) {
        n = btype->maxsize - built;
      } else {
        n = INT_MAX;
      }
    } else {
      if (b->type->maxsize > 0) {
        n = b->type->maxsize - built;
      } else {
        n = INT_MAX;
      }
    }
  }
  built = build(u, btype->construction, built, n);

  switch (built) {
  case ECOMPLETE:
    /* the building is already complete */
    cmistake(u, ord, 4, MSG_PRODUCE);
    return;
  case ENOMATERIALS:
    ADDMSG(&u->faction->msgs, msg_materials_required(u, ord,
        btype->construction, want));
    return;
  case ELOWSKILL:
  case ENEEDSKILL:
    /* no skill, or not enough skill points to build */
    cmistake(u, ord, 50, MSG_PRODUCE);
    return;
  }

  /* at this point, the building size is increased. */
  if (b == NULL) {
    /* build a new building */
    b = new_building(btype, r, lang);
    b->type = btype;
    fset(b, BLD_MAINTAINED);

    /* Die Einheit befindet sich automatisch im Inneren der neuen Burg. */
    if (leave(u, false)) {
      u_set_building(u, b);
      assert(building_owner(b)==u);
    }
#ifdef WDW_PYRAMID
    if (b->type == bt_find("pyramid") && f_get_alliance(u->faction) != NULL) {
      attrib *a = a_add(&b->attribs, a_new(&at_alliance));
      a->data.i = u->faction->alliance->id;
    }
#endif

    newbuilding = true;
  }

  btname = LOC(lang, btype->_name);

  if (want - built <= 0) {
    /* gebäude fertig */
    new_order = default_order(lang);
  } else if (want != INT_MAX) {
    /* reduzierte restgröße */
    const char *hasspace = strchr(btname, ' ');
    if (hasspace) {
      new_order =
        create_order(K_MAKE, lang, "%d \"%s\" %i", n - built, btname, b->no);
    } else {
      new_order =
        create_order(K_MAKE, lang, "%d %s %i", n - built, btname, b->no);
    }
  } else if (btname) {
    /* Neues Haus, Befehl mit Gebäudename */
    const char *hasspace = strchr(btname, ' ');
    if (hasspace) {
      new_order = create_order(K_MAKE, lang, "\"%s\" %i", btname, b->no);
    } else {
      new_order = create_order(K_MAKE, lang, "%s %i", btname, b->no);
    }
  }

  if (new_order) {
    replace_order(&u->orders, ord, new_order);
    free_order(new_order);
  }

  b->size += built;
  fset(b, BLD_EXPANDED);

  update_lighthouse(b);

  ADDMSG(&u->faction->msgs, msg_message("buildbuilding",
      "building unit size", b, u, built));
}

static void build_ship(unit * u, ship * sh, int want)
{
  const construction *construction = sh->type->construction;
  int size = (sh->size * DAMAGE_SCALE - sh->damage) / DAMAGE_SCALE;
  int n;
  int can = build(u, construction, size, want);

  if ((n = construction->maxsize - sh->size) > 0 && can > 0) {
    if (can >= n) {
      sh->size += n;
      can -= n;
    } else {
      sh->size += can;
      n = can;
      can = 0;
    }
  }

  if (sh->damage && can) {
    int repair = MIN(sh->damage, can * DAMAGE_SCALE);
    n += repair / DAMAGE_SCALE;
    if (repair % DAMAGE_SCALE)
      ++n;
    sh->damage = sh->damage - repair;
  }

  if (n)
    ADDMSG(&u->faction->msgs,
      msg_message("buildship", "ship unit size", sh, u, n));
}

void
create_ship(region * r, unit * u, const struct ship_type *newtype, int want,
  order * ord)
{
  ship *sh;
  int msize;
  const construction *cons = newtype->construction;
  order *new_order;

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
    ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
        "error_build_skill_low", "value name", cons->minskill,
        newtype->name[1]));
    return;
  }

  msize = maxbuild(u, cons);
  if (msize == 0) {
    cmistake(u, ord, 88, MSG_PRODUCE);
    return;
  }
  if (want > 0)
    want = MIN(want, msize);
  else
    want = msize;

  sh = new_ship(newtype, r, u->faction->locale);

  if (leave(u, false)) {
    if (fval(u->race, RCF_CANSAIL)) {
      u_set_ship(u, sh);
    }
  }
  new_order =
    create_order(K_MAKE, u->faction->locale, "%s %i", LOC(u->faction->locale,
      parameters[P_SHIP]), sh->no);
  replace_order(&u->orders, ord, new_order);
  free_order(new_order);

  build_ship(u, sh, want);
}

void continue_ship(region * r, unit * u, int want)
{
  const construction *cons;
  ship *sh;
  int msize;

  if (!eff_skill(u, SK_SHIPBUILDING, r)) {
    cmistake(u, u->thisorder, 100, MSG_PRODUCE);
    return;
  }

  /* Die Schiffsnummer bzw der Schiffstyp wird eingelesen */
  sh = getship(r);

  if (!sh)
    sh = u->ship;

  if (!sh) {
    cmistake(u, u->thisorder, 20, MSG_PRODUCE);
    return;
  }
  cons = sh->type->construction;
  assert(cons->improvement == NULL);    /* sonst ist construction::size nicht ship_type::maxsize */
  if (sh->size == cons->maxsize && !sh->damage) {
    cmistake(u, u->thisorder, 16, MSG_PRODUCE);
    return;
  }
  if (eff_skill(u, cons->skill, r) < cons->minskill) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
        "error_build_skill_low", "value name", cons->minskill,
        sh->type->name[1]));
    return;
  }
  msize = maxbuild(u, cons);
  if (msize == 0) {
    cmistake(u, u->thisorder, 88, MSG_PRODUCE);
    return;
  }
  if (want > 0)
    want = MIN(want, msize);
  else
    want = msize;

  build_ship(u, sh, want);
}

/* ------------------------------------------------------------- */

static boolean mayenter(region * r, unit * u, building * b)
{
  unit *u2;
  if (fval(b, BLD_UNGUARDED))
    return true;
  u2 = building_owner(b);

  if (u2 == NULL || ucontact(u2, u)
    || alliedunit(u2, u->faction, HELP_GUARD))
    return true;

  return false;
}

static int mayboard(const unit * u, ship * sh)
{
  unit *u2 = ship_owner(sh);

  return (!u2 || ucontact(u2, u) || alliedunit(u2, u->faction, HELP_GUARD));
}

int leave_cmd(unit * u, struct order *ord)
{
  region *r = u->region;

  if (fval(u, UFL_ENTER)) {
    /* if we just entered this round, then we don't leave again */
    return 0;
  }

  if (fval(r->terrain, SEA_REGION) && u->ship) {
    if (!fval(u->race, RCF_SWIM)) {
      cmistake(u, ord, 11, MSG_MOVE);
      return 0;
    }
    if (has_horses(u)) {
      cmistake(u, ord, 231, MSG_MOVE);
      return 0;
    }
  }
  if (!slipthru(r, u, u->building)) {
    ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "entrance_besieged",
        "building", u->building));
  } else {
    leave(u, true);
  }
  return 0;
}

static boolean enter_ship(unit * u, struct order *ord, int id, boolean report)
{
  region *r = u->region;
  ship *sh;

  /* Muß abgefangen werden, sonst könnten Schwimmer an
   * Bord von Schiffen an Land gelangen. */
  if (!fval(u->race, RCF_CANSAIL) || (!fval(u->race, RCF_WALK)
      && !fval(u->race, RCF_FLY))) {
    cmistake(u, ord, 233, MSG_MOVE);
    return false;
  }

  sh = findship(id);
  if (sh == NULL || sh->region != r) {
    if (report)
      cmistake(u, ord, 20, MSG_MOVE);
    return false;
  }
  if (sh == u->ship)
    return true;
  if (!mayboard(u, sh)) {
    if (report)
      cmistake(u, ord, 34, MSG_MOVE);
    return false;
  }
  if (CheckOverload()) {
    int sweight, scabins;
    int mweight = shipcapacity(sh);
    int mcabins = sh->type->cabins;

    if (mweight > 0) {
      getshipweight(sh, &sweight, &scabins);
      sweight += weight(u);
      if (mcabins) {
        int pweight = u->number * u->race->weight;
        /* weight goes into number of cabins, not cargo */
        scabins += pweight;
        sweight -= pweight;
      }

      if (sweight > mweight || (mcabins && (scabins > mcabins))) {
        if (report)
          cmistake(u, ord, 34, MSG_MOVE);
        return false;
      }
    }
  }

  if (leave(u, false)) {
    u_set_ship(u, sh);
    fset(u, UFL_ENTER);
  }
  return true;
}

static boolean enter_building(unit * u, order * ord, int id, boolean report)
{
  region *r = u->region;
  building *b;

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
  if (b == NULL || b->region != r) {
    if (report) {
      cmistake(u, ord, 6, MSG_MOVE);
    }
    return false;
  }
  if (!mayenter(r, u, b)) {
    if (report) {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "entrance_denied",
          "building", b));
    }
    return false;
  }
  if (!slipthru(r, u, b)) {
    if (report) {
      ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "entrance_besieged",
          "building", b));
    }
    return false;
  }

  if (leave(u, false)) {
    fset(u, UFL_ENTER);
    u_set_building(u, b);
    return true;
  }
  return false;
}

void do_misc(region * r, int is_final_attempt)
{
  unit **uptr, *uc;

  for (uc = r->units; uc; uc = uc->next) {
    order *ord;
    for (ord = uc->orders; ord; ord = ord->next) {
      keyword_t kwd = get_keyword(ord);
      if (kwd == K_CONTACT) {
        contact_cmd(uc, ord, is_final_attempt);
      }
    }
  }

  for (uptr = &r->units; *uptr;) {
    unit *u = *uptr;
    order **ordp = &u->orders;

    while (*ordp) {
      order *ord = *ordp;
      if (get_keyword(ord) == K_ENTER) {
        param_t p;
        int id;
        unit *ulast = NULL;
        const char * s;

        init_tokens(ord);
        skip_token();
        s = getstrtoken();
        p = findparam_ex(s, u->faction->locale);
        id = getid();

        switch (p) {
        case P_BUILDING:
        case P_GEBAEUDE:
          if (u->building && u->building->no == id)
            break;
          if (enter_building(u, ord, id, is_final_attempt)) {
            unit *ub;
            for (ub = u; ub; ub = ub->next) {
              if (ub->building == u->building) {
                ulast = ub;
              }
            }
          }
          break;

        case P_SHIP:
          if (u->ship && u->ship->no == id)
            break;
          if (enter_ship(u, ord, id, is_final_attempt)) {
            unit *ub;
            ulast = u;
            for (ub = u; ub; ub = ub->next) {
              if (ub->ship == u->ship) {
                ulast = ub;
              }
            }
          }
          break;

        default:
          if (is_final_attempt) {
            cmistake(u, ord, 79, MSG_MOVE);
          }
        }
        if (ulast != NULL) {
          /* Wenn wir hier angekommen sind, war der Befehl
           * erfolgreich und wir löschen ihn, damit er im
           * zweiten Versuch nicht nochmal ausgeführt wird. */
          *ordp = ord->next;
          ord->next = NULL;
          free_order(ord);

          if (ulast != u) {
            /* put u behind ulast so it's the last unit in the building */
            *uptr = u->next;
            u->next = ulast->next;
            ulast->next = u;
          }
          break;
        }
      }
      if (*ordp == ord)
        ordp = &ord->next;
    }
    if (*uptr == u)
      uptr = &u->next;
  }
}
