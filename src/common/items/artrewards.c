/* vi: set ts=2:
*
* Eressea PB(E)M host Copyright (C) 1998-2003
*      Christian Schlittchen (corwin@amber.kn-bremen.de)
*      Katja Zedel (katze@felidae.kn-bremen.de)
*      Henning Peters (faroul@beyond.kn-bremen.de)
*      Enno Rehling (enno@eressea-pbem.de)
*      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
*
* This program may not be used, modified or distributed without
* prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "artrewards.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/skill.h>
#include <kernel/curse.h>
#include <kernel/message.h>
#include <kernel/magic.h>
#include <kernel/ship.h>
#include <kernel/building.h>

/* gamecode includes */
#include <gamecode/economy.h>

/* util includes */
#include <util/functions.h>
#include <util/rand.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define HORNRANGE 10
#define HORNDURATION 3
#define HORNIMMUNITY 30

static int
age_peaceimmune(attrib * a)
{
  return --a->data.i;
}

static attrib_type at_peaceimmune = {
  "peaceimmune",
    NULL, NULL,
    age_peaceimmune,
    a_writedefault,
    a_readdefault
};

static int
use_hornofdancing(struct unit * u, const struct item_type * itype,
                  int amount, const char *cm)
{
  region *r;
  int    regionsPacified = 0;

  for(r=regions; r; r=r->next) {
    if(distance(u->region, r) < HORNRANGE) {
      if(a_find(r->attribs, &at_peaceimmune) == NULL) {
        attrib *a;

        create_curse(u, &r->attribs, ct_find("peacezone"),
          20, HORNDURATION, 1, 0);

        a = a_add(&r->attribs, a_new(&at_peaceimmune));
        a->data.i = HORNIMMUNITY;

        ADDMSG(&r->msgs, msg_message("hornofpeace_r_success",
          "unit region", u, u->region));

        regionsPacified++;
      } else {
        ADDMSG(&r->msgs, msg_message("hornofpeace_r_nosuccess",
          "unit region", u, u->region));
      }
    }
  }

  if(regionsPacified > 0) {
    ADDMSG(&u->faction->msgs, msg_message("hornofpeace_u_success",
      "unit region command pacified", u, u->region, cm, regionsPacified));
  } else {
    ADDMSG(&u->faction->msgs, msg_message("hornofpeace_u_nosuccess",
      "unit region command", u, u->region, cm));
  }

  return 0;
}

static resource_type rt_hornofdancing = {
  { "hornofdancing", "hornofdancing_p" },
  { "hornofdancing", "hornofdancing_p" },
  RTF_ITEM,
  &res_changeitem
};

item_type it_hornofdancing = {
  &rt_hornofdancing,        /* resourcetype */
    0, 1, 0,                  /* flags, weight, capacity */
    NULL,                     /* construction */
    &use_hornofdancing,
    NULL,
    NULL
};


#define SPEEDUP 2

static int
use_trappedairelemental(struct unit * u, const struct item_type * itype,
                        int amount, const char *cm)
{
  curse  *c;
  int    shipId;
  ship   *sh;

  shipId = getshipid();
  if(shipId <= 0) {
    cmistake(u, cm, 20, MSG_MOVE);
    return 0;
  }

  sh = findshipr(u->region, shipId);
  if(!sh) {
    cmistake(u, cm, 20, MSG_MOVE);
    return 0;
  }

  c = create_curse(u, &sh->attribs, ct_find("shipspeedup"),
    20, 999999, SPEEDUP, 0);
  curse_setflag(c, CURSE_NOAGE);

  ADDMSG(&u->faction->msgs, msg_message("trappedairelemental_success",
    "unit region command ship", u, u->region, cm, sh));
  
  itype->rtype->uchange(u, itype->rtype, -1);

  return 1;
}

static resource_type rt_trappedairelemental = {
  { "trappedairelemental", "trappedairelemental_p" },
  { "trappedairelemental", "trappedairelemental_p" },
  RTF_ITEM,
  &res_changeitem
};

item_type it_trappedairelemental = {
  &rt_trappedairelemental,        /* resourcetype */
    0, 1, 0,                        /* flags, weight, capacity */
    NULL,                           /* construction */
    &use_trappedairelemental,
    NULL,
    NULL
};


static int
use_aurapotion50(struct unit * u, const struct item_type * itype,
                 int amount, const char *cm)
{
  if(!is_mage(u)) {
    cmistake(u, cm, 214, MSG_MAGIC);
    return 0;
  }

  change_spellpoints(u, 50);

  ADDMSG(&u->faction->msgs, msg_message("aurapotion50",
    "unit region command", u, u->region, cm));
  
  itype->rtype->uchange(u, itype->rtype, -1);

  return 1;
}

static resource_type rt_aurapotion50 = {
  { "aurapotion50", "aurapotion50_p" },
  { "aurapotion50", "aurapotion50_p" },
  RTF_ITEM,
  &res_changeitem
};

item_type it_aurapotion50 = {
  &rt_aurapotion50,               /* resourcetype */
    0, 1, 0,                        /* flags, weight, capacity */
    NULL,                           /* construction */
    &use_aurapotion50,
    NULL,
    NULL
};

#define BAGPIPEFRACTION dice_rand("2d4+2")
#define BAGPIPEDURATION dice_rand("2d10+4")

static int
use_bagpipeoffear(struct unit * u, const struct item_type * itype,
                  int amount, const char *cm)
{
  int money;

  if(get_curse(u->region->attribs, ct_find("depression"))) {
    cmistake(u, cm, 58, MSG_MAGIC);
    return 0;
  }

  money = entertainmoney(u->region)/BAGPIPEFRACTION;
  change_money(u, money);
  rsetmoney(u->region, rmoney(u->region) - money);

  create_curse(u, &u->region->attribs, ct_find("depression"),
    20, BAGPIPEDURATION, 0, 0);

  ADDMSG(&u->faction->msgs, msg_message("bagpipeoffear_faction",
    "unit region command money", u, u->region, cm, money));

  ADDMSG(&u->region->msgs, msg_message("bagpipeoffear_region",
    "unit money", u, money));

  return 0;
}

static resource_type rt_bagpipeoffear = {
  { "bagpipeoffear", "bagpipeoffear_p" },
  { "bagpipeoffear", "bagpipeoffear_p" },
  RTF_ITEM,
  &res_changeitem
};

item_type it_bagpipeoffear = {
  &rt_bagpipeoffear,              /* resourcetype */
  0, 1, 0,                        /* flags, weight, capacity */
  NULL,                           /* construction */
  &use_bagpipeoffear,
  NULL,
  NULL
};

static int
use_instantartacademy(struct unit * u, const struct item_type * itype,
                    int amount, const char *cm)
{
  building *b;

  if(u->region->land == NULL) {
    cmistake(u, cm, 242, MSG_MAGIC);
    return 0;
  }

  b = new_building(bt_find("artacademy"), u->region, u->faction->locale);
  b->size = 100;
  sprintf(buf, "%s", LOC(u->faction->locale, "artacademy"));
  set_string(&b->name, buf);

  ADDMSG(&u->region->msgs, msg_message(
    "artacademy_create", "unit region", u, u->region));
  
  itype->rtype->uchange(u, itype->rtype, -1);

  return 1;
}

static resource_type rt_instantartacademy = {
  { "instantartacademy", "instantartacademy_p" },
  { "instantartacademy", "instantartacademy_p" },
  RTF_ITEM,
  &res_changeitem
};

item_type it_instantartacademy = {
  &rt_instantartacademy,              /* resourcetype */
  0, 1, 0,                        /* flags, weight, capacity */
  NULL,                           /* construction */
  &use_instantartacademy,
  NULL,
  NULL
};

static int
use_instantartsculpture(struct unit * u, const struct item_type * itype,
                    int amount, const char *cm)
{
  building *b;

  if(u->region->land == NULL) {
    cmistake(u, cm, 242, MSG_MAGIC);
    return 0;
  }

  b = new_building(bt_find("artsculpture"), u->region, u->faction->locale);
  b->size = 100;
  sprintf(buf, "%s", LOC(u->faction->locale, "artsculpture"));
  set_string(&b->name, buf);

  ADDMSG(&u->region->msgs, msg_message(
    "artsculpture_create", "unit region command", u, u->region, cm));

  itype->rtype->uchange(u, itype->rtype, -1);

  return 1;
}

static resource_type rt_instantartsculpture = {
  { "instantartsculpture", "instantartsculpture_p" },
  { "instantartsculpture", "instantartsculpture_p" },
  RTF_ITEM,
  &res_changeitem
};

item_type it_instantartsculpture = {
  &rt_instantartsculpture,              /* resourcetype */
  0, 1, 0,                          /* flags, weight, capacity */
  NULL,                                 /* construction */
  &use_instantartsculpture,
  NULL,
  NULL
};


void
register_artrewards(void)
{
  at_register(&at_peaceimmune);
  it_register(&it_hornofdancing);
  register_function((pf_generic)use_hornofdancing, "usehornofdancing");
  it_register(&it_trappedairelemental);
  register_function((pf_generic)use_trappedairelemental, "trappedairelemental");
  it_register(&it_aurapotion50);
  register_function((pf_generic)use_aurapotion50, "aurapotion50");
  it_register(&it_bagpipeoffear);
  register_function((pf_generic)use_bagpipeoffear, "bagpipeoffear");
  it_register(&it_instantartacademy);
  register_function((pf_generic)use_instantartacademy, "instantartacademy");
  it_register(&it_instantartsculpture);
  register_function((pf_generic)use_instantartsculpture, "instantartsculpture");
}
