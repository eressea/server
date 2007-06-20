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
#include <kernel/save.h>
#include <kernel/skill.h>
#include <kernel/curse.h>
#include <kernel/message.h>
#include <kernel/magic.h>
#include <kernel/ship.h>

/* util includes */
#include <util/attrib.h>
#include <util/functions.h>
#include <util/rand.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
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
    a_writeint,
    a_readint
};

static int
use_hornofdancing(struct unit * u, const struct item_type * itype,
                  int amount, struct order * ord)
{
  region *r;
  int    regionsPacified = 0;

  for(r=regions; r; r=r->next) {
    if(distance(u->region, r) < HORNRANGE) {
      if(a_find(r->attribs, &at_peaceimmune) == NULL) {
        attrib *a;
        variant effect;

        effect.i = 1;
        create_curse(u, &r->attribs, ct_find("peacezone"),
          20, HORNDURATION, effect, 0);

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
      "unit region command pacified", u, u->region, ord, regionsPacified));
  } else {
    ADDMSG(&u->faction->msgs, msg_message("hornofpeace_u_nosuccess",
      "unit region command", u, u->region, ord));
  }

  return 0;
}

#define SPEEDUP 2


static int
useonother_trappedairelemental(struct unit * u, int shipId,
                        const struct item_type * itype,
                        int amount, struct order * ord)
{
  curse  *c;
  ship   *sh;
  variant effect;

  if(shipId <= 0) {
    cmistake(u, ord, 20, MSG_MOVE);
    return -1;
  }

  sh = findshipr(u->region, shipId);
  if(!sh) {
    cmistake(u, ord, 20, MSG_MOVE);
    return -1;
  }

  effect.i = SPEEDUP;
  c = create_curse(u, &sh->attribs, ct_find("shipspeedup"), 20, INT_MAX, effect, 0);
  c_setflag(c, CURSE_NOAGE);

  ADDMSG(&u->faction->msgs, msg_message("trappedairelemental_success",
    "unit region command ship", u, u->region, ord, sh));

  itype->rtype->uchange(u, itype->rtype, -1);

  return 0;
}

static int
use_trappedairelemental(struct unit * u,
    const struct item_type * itype,
    int amount, struct order * ord)
{
  ship *sh = u->ship;

  if(sh == NULL) {
    cmistake(u, ord, 20, MSG_MOVE);
    return -1;
  }
  return useonother_trappedairelemental(u, sh->no, itype, amount,ord);
}

void
register_artrewards(void)
{
  at_register(&at_peaceimmune);
  register_function((pf_generic)use_hornofdancing, "use_hornofdancing");
  register_function((pf_generic)use_trappedairelemental, "use_trappedairelemental");
  register_function((pf_generic)useonother_trappedairelemental, "useonother_trappedairelemental");
}
