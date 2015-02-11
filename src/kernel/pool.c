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
#include "pool.h"

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
    if (rtype->uget) {
        /* this resource is probably special */
        int i = rtype->uget(u, rtype);
        if (i >= 0)
            return i;
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
        return max_spellpoints(u->region, u);
    }
    log_error("trying to get unknown resource '%s'.\n", rtype->_name);
    return 0;
}

int change_resource(unit * u, const resource_type * rtype, int change)
{
    int i = 0;

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
    if (i >= 100000000) {
        log_warning("%s has %d %s\n", unitname(u), i, rtype->_name);
    }
    return i;
}

int get_reservation(const unit * u, const item_type * itype)
{
    reservation *res = u->reservations;

    if (itype->rtype == get_resourcetype(R_STONE) && (u_race(u)->flags & RCF_STONEGOLEM))
        return (u->number * GOLEM_STONE);
    if (itype->rtype == get_resourcetype(R_IRON) && (u_race(u)->flags & RCF_IRONGOLEM))
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
        *rp = res = calloc(sizeof(reservation), 1);
        res->type = itype;
        res->value = value;
    }
    else if (res && res->value + value <= 0) {
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
        *rp = res = calloc(sizeof(reservation), 1);
        res->type = itype;
        res->value = value;
    }
    else if (res && value <= 0) {
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
get_pooled(const unit * u, const resource_type * rtype, unsigned int mode,
int count)
{
    const faction *f = u->faction;
    unit *v;
    int use = 0;
    region *r = u->region;
    int have = get_resource(u, rtype);

    if ((u_race(u)->ec_flags & GETITEM) == 0) {
        mode &= (GET_SLACK | GET_RESERVE);
    }

    if ((mode & GET_SLACK) && (mode & GET_RESERVE))
        use = have;
    else if (rtype->itype && mode & (GET_SLACK | GET_RESERVE)) {
        int reserve = get_reservation(u, rtype->itype);
        int slack = _max(0, have - reserve);
        if (mode & GET_RESERVE)
            use = have - slack;
        else if (mode & GET_SLACK)
            use = slack;
    }
    if (rtype->flags & RTF_POOLED && mode & ~(GET_SLACK | GET_RESERVE)) {
        for (v = r->units; v && use < count; v = v->next)
            if (u != v && (v->items || rtype->uget)) {
                int mask;

                if ((urace(v)->ec_flags & GIVEITEM) == 0)
                    continue;

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
use_pooled(unit * u, const resource_type * rtype, unsigned int mode, int count)
{
    const faction *f = u->faction;
    unit *v;
    int use = count;
    region *r = u->region;
    int n = 0, have = get_resource(u, rtype);

    if ((u_race(u)->ec_flags & GETITEM) == 0) {
        mode &= (GET_SLACK | GET_RESERVE);
    }

    if ((mode & GET_SLACK) && (mode & GET_RESERVE)) {
        n = _min(use, have);
    }
    else if (rtype->itype) {
        int reserve = get_reservation(u, rtype->itype);
        int slack = _max(0, have - reserve);
        if (mode & GET_RESERVE) {
            n = have - slack;
            n = _min(use, n);
            change_reservation(u, rtype->itype, -n);
        }
        else if (mode & GET_SLACK) {
            n = _min(use, slack);
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
                if ((urace(v)->ec_flags & GIVEITEM) == 0)
                    continue;
                if (v->items == NULL && rtype->uget == NULL)
                    continue;

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
