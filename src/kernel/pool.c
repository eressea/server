#ifdef _MSC_VER
#include <platform.h>
#endif

#include "pool.h"

#include "ally.h"
#include "config.h"
#include "faction.h"
#include "item.h"
#include "order.h"
#include "region.h"
#include "race.h"
#include "unit.h"

#include <util/parser.h>
#include <util/log.h>

#include <assert.h>
#include <stdlib.h>

#define TODO_POOL
#undef TODO_RESOURCES

int get_resource(const unit * u, const resource_type * rtype)
{
    assert(rtype);
    if (rtype == get_resourcetype(R_PEASANT)) {
        return u->region->land ? u->region->land->peasants : 0;
    }
    else if (rtype == rt_find("hp")) {
        return u->hp;
    }
    else if (rtype->uchange) {
        /* this resource is probably special */
        int i = rtype->uchange((unit *)u, rtype, 0);
        if (i >= 0)
            return i;
    }
    if (rtype->itype) {
        return i_get(u->items, rtype->itype);
    }
    if (rtype == get_resourcetype(R_AURA)) {
        return get_spellpoints(u);
    }
    if (rtype == get_resourcetype(R_PERMAURA)) {
        return max_spellpoints_depr(u->region, u);
    }
    log_error("trying to get unknown resource '%s'.\n", rtype->_name);
    return 0;
}

int change_resource(unit * u, const resource_type * rtype, int change)
{
    int i = 0;

    assert(rtype);

    if (rtype->uchange)
        i = rtype->uchange(u, rtype, change);
    else if (rtype == get_resourcetype(R_AURA))
        i = change_spellpoints(u, change);
    else if (rtype == get_resourcetype(R_PERMAURA))
        i = change_maxspellpoints(u, change);
    else
        assert(!"undefined resource detected. rtype->uchange not initialized.");
    assert(i == get_resource(u, rtype));
    assert(i >= 0);
    return i;
}

int get_reservation(const unit * u, const item_type * itype)
{
    reservation *res = u->reservations;

    if (itype->rtype == get_resourcetype(R_STONE) && (u_race(u)->ec_flags & ECF_STONEGOLEM))
        return (u->number * GOLEM_STONE);
    if (itype->rtype == get_resourcetype(R_IRON) && (u_race(u)->ec_flags & ECF_IRONGOLEM))
        return (u->number * GOLEM_IRON);
    while (res && res->type != itype)
        res = res->next;
    if (res)
        return res->value;
    return 0;
}

int change_reservation(unit * u, const item_type * itype, int value)
{
    reservation *res, **rp = &u->reservations;

    if (!value)
        return 0;

    while (*rp && (*rp)->type != itype)
        rp = &(*rp)->next;
    res = *rp;
    if (!res) {
        *rp = res = calloc(1, sizeof(reservation));
        if (!res) abort();
        res->type = itype;
        res->value = value;
    }
    else if (res->value + value <= 0) {
        *rp = res->next;
        free(res);
        return 0;
    }
    else {
        res->value += value;
    }
    return res->value;
}

int set_resvalue(unit * u, const item_type * itype, int value)
{
    reservation *res, **rp = &u->reservations;

    while (*rp && (*rp)->type != itype)
        rp = &(*rp)->next;
    res = *rp;
    if (!res) {
        if (!value)
            return 0;
        *rp = res = calloc(1, sizeof(reservation));
        if (!res) abort();
        res->type = itype;
        res->value = value;
    }
    else if (value <= 0) {
        *rp = res->next;
        free(res);
        return 0;
    }
    else {
        res->value = value;
    }
    return res->value;
}

int
get_pooled(const unit * u, const resource_type * rtype, int mode,
int count)
{
    const faction *f = u->faction;
    unit *v;
    int use = 0;
    region *r = u->region;
    int have = get_resource(u, rtype);

    if ((u_race(u)->ec_flags & ECF_GETITEM) == 0) {
        mode &= (GET_SLACK | GET_RESERVE);
    }

    if ((mode & GET_SLACK) && (mode & GET_RESERVE))
        use = have;
    else if (rtype->itype && mode & (GET_SLACK | GET_RESERVE)) {
        int reserve = get_reservation(u, rtype->itype);
        int slack = have - reserve;
        if (slack < 0) slack = 0;
        if (mode & GET_RESERVE)
            use = have - slack;
        else if (mode & GET_SLACK)
            use = slack;
    }
    if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK | GET_RESERVE)) {
        for (v = r->units; v && use < count; v = v->next)
            if (u != v) {
                int mask;

                if (v->faction == f) {
                    mask = (mode >> 3) & (GET_SLACK | GET_RESERVE);
                }
                else if (alliedunit(v, f, HELP_MONEY))
                    mask = (mode >> 6) & (GET_SLACK | GET_RESERVE);
                else
                    continue;
                use += get_pooled(v, rtype, mask, count - use);
            }
    }
    return use;
}

int
use_pooled(unit * u, const resource_type * rtype, int mode, int count)
{
    const faction *f = u->faction;
    unit *v;
    int use = count;
    region *r = u->region;
    int n = 0, have = get_resource(u, rtype);

    if ((u_race(u)->ec_flags & ECF_GETITEM) == 0) {
        mode &= (GET_SLACK | GET_RESERVE);
    }

    if ((mode & GET_SLACK) && (mode & GET_RESERVE)) {
        n = (use < have) ? use : have;
    }
    else if (rtype->itype) {
        int reserve = get_reservation(u, rtype->itype);
        int slack = have - reserve;
        if (slack < 0) slack = 0;
        if (mode & GET_RESERVE) {
            n = have - slack;
            if (n > use) n = use;
            change_reservation(u, rtype->itype, -n);
        }
        else if (mode & GET_SLACK) {
            n = slack;
            if (n > use) n = use;
        }
    }
    if (n > 0) {
        change_resource(u, rtype, -n);
        use -= n;
    }

    if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK | GET_RESERVE)) {
        for (v = r->units; use > 0 && v != NULL; v = v->next) {
            if (u != v) {
                int mask;
                if (v->faction == f) {
                    mask = (mode >> 3) & (GET_SLACK | GET_RESERVE);
                }
                else if (alliedunit(v, f, HELP_MONEY))
                    mask = (mode >> 6) & (GET_SLACK | GET_RESERVE);
                else
                    continue;
                use -= use_pooled(v, rtype, mask, use);
            }
        }
    }
    return count - use;
}
