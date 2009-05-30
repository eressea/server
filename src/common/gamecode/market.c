/* vi: set ts=2:
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
| (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
|                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
+-------------------+  Stefan Reich <reich@halbling.de>

This program may not be used, modified or distributed 
without prior permission by the authors of Eressea.

*/
#include <config.h>
#include <kernel/eressea.h>
#include "market.h"

#include <assert.h>

#include <util/attrib.h>
#include <util/rng.h>

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/message.h>
#include <kernel/region.h>
#include <kernel/unit.h>

static unsigned int
get_markets(region * r, unit ** results, size_t size)
{
  unsigned int n = 0;
  building * b;
  static building_type * btype;
  if (!btype) btype = bt_find("market");
  if (!btype) return 0;
  for (b=r->buildings;n<size && b;b=b->next) {
    if (b->type==btype && (b->flags&BLD_WORKING)) {
      unit * u = buildingowner(r, b);
      unsigned int i;
      for (i=0;u && i!=n;++i) {
        /* only one market per faction */
        if (results[i]->faction==u->faction) u = NULL;
      }
      if (u) {
        results[n++] = u;
      }
    }
  }
  return n;
}


static void
free_market(attrib * a)
{
  item * items = (item *)a->data.v;
  i_freeall(&items);
  a->data.v = 0;
}

const item_type * r_luxury(region * r)
{
  const luxury_type * ltype;
  for (ltype = luxurytypes;ltype;ltype=ltype->next) {
    if (r_demand(r, ltype)<0) return ltype->itype;
  }
  return NULL;
}

attrib_type at_market = {
  "script",
  NULL, free_market, NULL,
  NULL, NULL, ATF_UNIQUE
};

#define MAX_MARKETS 128
void do_markets(void)
{
  unit_list * traders = 0;
  unit * markets[MAX_MARKETS];
  region * r;
  for (r=regions;r;r=r->next) {
    if (r->land) {
      int p = rpeasants(r);
      int numlux = p/1000;
      int numherbs = p/500;
      if (numlux>0 || numherbs>0) {
        int d, nmarkets = 0;
        const item_type * lux = r_luxury(r);
        const item_type * herb = r->land->herbtype;

        nmarkets += get_markets(r, markets+nmarkets, MAX_MARKETS-nmarkets);
        for (d=0;d!=MAXDIRECTIONS;++d) {
          region * r2 = rconnect(r, d);
          if (r2 && r2->buildings) {
            nmarkets += get_markets(r, markets+nmarkets, MAX_MARKETS-nmarkets);
          }
        }
        while (lux && numlux--) {
          int n = rng_int() % nmarkets;
          unit * u = markets[n];
          item * items;
          attrib * a = a_find(u->attribs, &at_market);
          if (a==NULL) {
            unit_list * ulist = malloc(sizeof(unit_list));
            a = a_add(&u->attribs, a_new(&at_market));
            ulist->next = traders;
            ulist->data = u;
            traders = ulist;
          }
          items = (item *)a->data.v;
          i_change(&items, lux, 1);
          a->data.v = items;
          /* give 1 luxury */
        }
        while (herb && numherbs--) {
          int n = rng_int() % nmarkets;
          unit * u = markets[n];
          item * items;
          attrib * a = a_find(u->attribs, &at_market);
          if (a==NULL) {
            unit_list * ulist = malloc(sizeof(unit_list));
            a = a_add(&u->attribs, a_new(&at_market));
            ulist->next = traders;
            ulist->data = u;
            traders = ulist;
          }
          items = (item *)a->data.v;
          i_change(&items, herb, 1);
          a->data.v = items;
          /* give 1 luxury */
        }
      }
    }
  }

  while (traders) {
    unit_list * trade = traders;
    unit * u = trade->data;
    attrib * a = a_find(u->attribs, &at_market);
    item * items = a->data.v;

    while (items) {
      item * itm = items;
      items = itm->next;

      if (itm->number) {
        ADDMSG(&u->faction->msgs, msg_message("buyamount",
          "unit amount resource", u, itm->number, itm->type->rtype));
        i_add(&u->items, itm);
      }
    }

    traders = trade->next;

    a_remove(&u->attribs, a);
    free(trade);
  }
}
