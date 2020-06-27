#include <platform.h>
#include <kernel/config.h>

#include "travelthru.h"
#include "laws.h"
#include "report.h"

#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/attrib.h>
#include <util/log.h>
#include <util/language.h>

#include <stream.h>
#include <selist.h>

#include <assert.h>
#include <string.h>

static void travel_done(variant *var) {
    selist_free((selist *)var->v);
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
    selist *ql;

    assert(r);
    assert(u);

    a = a_find(r->attribs, &at_travelunit);
    if (!a) {
        a = a_add(&r->attribs, a_new(&at_travelunit));
    }
    ql = (selist *)a->data.v;

    fset(r, RF_TRAVELUNIT);
    selist_push(&ql, u);
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

struct cb_data {
    void(*call)(region *, struct unit *, void *);
    void *data;
    struct region *r;
};

static bool cb_map(void *data, void *ex) {
    struct cb_data *cb = (struct cb_data *)ex;
    struct unit *u = (struct unit *)data;
    cb->call(cb->r, u, cb->data);
    return true;
}

void travelthru_map(region * r, void(*cb)(region *, struct unit *, void *), void *data)
{
    attrib *a;
    struct cb_data cbdata;
    
    assert(r);
    cbdata.call = cb;
    cbdata.data = data;
    cbdata.r = r;
    a = a_find(r->attribs, &at_travelunit);
    if (a) {
        selist *ql = (selist *)a->data.v;
        selist_foreach_ex(ql, cb_map, &cbdata);
    }
}
