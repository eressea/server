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
#include <item.h>
#include <region.h>
#include <faction.h>
#include <unit.h>
#include <skill.h>
#include <curse.h>
#include <message.h>
#include <magic.h>
#include <ship.h>

/* util includes */
#include <functions.h>

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
	0, 0, 0,		  /* flags, weight, capacity */
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
	0, 0, 0,		                    /* flags, weight, capacity */
	NULL,                           /* construction */
	&use_trappedairelemental,
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
}

