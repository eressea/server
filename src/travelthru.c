/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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

#include "travelthru.h"
#include "laws.h"
#include "report.h"

#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/log.h>
#include <util/language.h>

#include <stream.h>
#include <quicklist.h>

#include <assert.h>
#include <string.h>

static void travel_done(attrib *a) {
    quicklist *ql = (quicklist *)a->data.v;
    ql_free(ql);
}

/*********************/
/*   at_travelunit   */
/*********************/
attrib_type at_travelunit = {
    "travelunit",
    DEFAULT_INIT,
    travel_done,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

/** sets a marker in the region telling that the unit has travelled through it
* this is used for two distinctly different purposes:
* - to report that a unit has travelled through. the report function
*   makes sure to only report the ships of travellers, not the travellers
*   themselves
* - to report the region to the traveller
*/
void travelthru_add(region * r, unit * u)
{
    region *next[MAXDIRECTIONS];
    int d;
    attrib *a;
    quicklist *ql;

    assert(r);
    assert(u);

    a = a_find(r->attribs, &at_travelunit);
    if (!a) {
        a = a_add(&r->attribs, a_new(&at_travelunit));
    }
    ql = (quicklist *)a->data.v;

    fset(r, RF_TRAVELUNIT);
    ql_push(&ql, u);
    a->data.v = ql;

    /* the first and last region of the faction gets reset, because travelthrough
    * could be in regions that are located before the [first, last] interval,
    * and recalculation is needed */
    update_interval(u->faction, r);
    get_neighbours(r, next);
    for (d=0;d!=MAXDIRECTIONS;++d) {
        update_interval(u->faction, next[d]);
    }
}

bool travelthru_cansee(const struct region *r, const struct faction *f, const struct unit *u) {
    if (r != u->region && (!u->ship || ship_owner(u->ship) == u)) {
        return cansee_durchgezogen(f, r, u, 0);
    }
    return false;
}

void travelthru_map(region * r, void(*cb)(region *, struct unit *, void *), void *cbdata)
{
    attrib *a;
    assert(r);
    a = a_find(r->attribs, &at_travelunit);
    if (a) {
        quicklist *ql;
        ql_iter qi;
        ql = (quicklist *)a->data.v;
        for (qi = qli_init(&ql); qli_more(qi);) {
            unit *u = (unit *)qli_next(&qi);
            cb(r, u, cbdata);
        }
    }
}
