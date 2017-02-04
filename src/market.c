/*
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
| (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
|                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
+-------------------+  Stefan Reich <reich@halbling.de>

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.

*/
#include <platform.h>
#include <kernel/config.h>
#include "market.h"

#include <assert.h>

#include <util/attrib.h>
#include <selist.h>
#include <util/rng.h>

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

static unsigned int get_markets(region * r, unit ** results, size_t size)
{
    unsigned int n = 0;
    building *b;
    const building_type *btype = bt_find("market");
    if (!btype)
        return 0;
    for (b = r->buildings; n < size && b; b = b->next) {
        if (b->type == btype && building_is_active(b)) {
            unit *u = building_owner(b);
            /* I decided to omit check for inside_building(u) */
            unsigned int i;
            for (i = 0; u && i != n; ++i) {
                /* only one market per faction */
                if (results[i]->faction == u->faction)
                    u = NULL;
            }
            if (u) {
                results[n++] = u;
            }
        }
    }
    return n;
}

static void free_market(attrib * a)
{
    item *items = (item *)a->data.v;
    i_freeall(&items);
    a->data.v = 0;
}

attrib_type at_market = {
    "script",
    NULL, free_market, NULL,
    NULL, NULL, NULL, ATF_UNIQUE
};

int rc_luxury_trade(const struct race *rc)
{
    if (rc) {
        return get_param_int(rc->parameters, "luxury_trade", 1000);
    }
    return 1000;
}

int rc_herb_trade(const struct race *rc)
{
    if (rc) {
        return get_param_int(rc->parameters, "herb_trade", 500);
    }
    return 500;
}

#define MAX_MARKETS 128
#define MIN_PEASANTS 50         /* if there are at least this many peasants, you will get 1 good */

bool markets_module(void)
{
    return (bool)config_get_int("modules.markets", 0);
}

void do_markets(void)
{
    selist *traders = 0;
    unit *markets[MAX_MARKETS];
    region *r;
    for (r = regions; r; r = r->next) {
        if (r->land) {
            faction *f = region_get_owner(r);
            const struct race *rc = f ? f->race : NULL;
            int p = rpeasants(r);
            int numlux = rc_luxury_trade(rc), numherbs = rc_herb_trade(rc);
            if (numlux>0) numlux = (p + numlux - MIN_PEASANTS) / numlux;
            if (numherbs>0) numherbs = (p + numherbs - MIN_PEASANTS) / numherbs;
            if (numlux > 0 || numherbs > 0) {
                int d, nmarkets = 0;
                const item_type *lux = r_luxury(r);
                const item_type *herb = r->land->herbtype;

                nmarkets += get_markets(r, markets + nmarkets, MAX_MARKETS - nmarkets);
                for (d = 0; d != MAXDIRECTIONS; ++d) {
                    region *r2 = rconnect(r, d);
                    if (r2 && r2->buildings) {
                        nmarkets +=
                            get_markets(r2, markets + nmarkets, MAX_MARKETS - nmarkets);
                    }
                }
                if (nmarkets) {
                    while (lux && numlux--) {
                        int n = rng_int() % nmarkets;
                        unit *u = markets[n];
                        item *items;
                        attrib *a = a_find(u->attribs, &at_market);
                        if (a == NULL) {
                            a = a_add(&u->attribs, a_new(&at_market));
                            selist_push(&traders, u);
                        }
                        items = (item *)a->data.v;
                        i_change(&items, lux, 1);
                        a->data.v = items;
                        /* give 1 luxury */
                    }
                    while (herb && numherbs--) {
                        int n = rng_int() % nmarkets;
                        unit *u = markets[n];
                        item *items;
                        attrib *a = a_find(u->attribs, &at_market);
                        if (a == NULL) {
                            a = a_add(&u->attribs, a_new(&at_market));
                            selist_push(&traders, u);
                        }
                        items = (item *)a->data.v;
                        i_change(&items, herb, 1);
                        a->data.v = items;
                        /* give 1 herb */
                    }
                }
            }
        }
    }

    if (traders) {
        selist *qliter = traders;
        int qli = 0;
        for (qli = 0; qliter; selist_advance(&qliter, &qli, 1)) {
            unit *u = (unit *)selist_get(qliter, qli);
            attrib *a = a_find(u->attribs, &at_market);
            item *items = (item *)a->data.v;

            a->data.v = NULL;
            while (items) {
                item *itm = items;
                items = itm->next;

                if (itm->number) {
                    ADDMSG(&u->faction->msgs, msg_message("buyamount",
                        "unit amount resource", u, itm->number, itm->type->rtype));
                    itm->next = NULL;
                    i_add(&u->items, itm);
                }
                else {
                    i_free(itm);
                }
            }

            a_remove(&u->attribs, a);
        }
        selist_free(traders);
    }
}
