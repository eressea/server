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

#if ARENA_MODULE
#include "arena.h"

/* modules include */
#include "score.h"

/* items include */
#include <items/demonseye.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <move.h>

/* util include */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/functions.h>
#include <util/strings.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/rng.h>

#include <storage.h>

/* libc include */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* exports: */
plane *arena = NULL;

/* local vars */
#define CENTRAL_VOLCANO 1

static region *tower_region[6];
static region *start_region[6];

static region *arena_region(int school)
{
    return tower_region[school];
}

static building *arena_tower(int school)
{
    return arena_region(school)->buildings;
}

static int leave_fail(unit * u)
{
    ADDMSG(&u->faction->msgs, msg_message("arena_leave_fail", "unit", u));
    return 1;
}

static int
leave_arena(struct unit *u, const struct item_type *itype, int amount,
order * ord)
{
    if (!u->building && leave_fail(u)) {
        return -1;
    }
    if (u->building != arena_tower(u->faction->magiegebiet) && leave_fail(u)) {
        return -1;
    }
    unused_arg(amount);
    unused_arg(ord);
    unused_arg(itype);
    assert(!"not implemented");
    return 0;
}

static int enter_fail(unit * u)
{
    ADDMSG(&u->faction->msgs, msg_message("arena_enter_fail", "region unit",
        u->region, u));
    return 1;
}

static int
enter_arena(unit * u, const item_type * itype, int amount, order * ord)
{
    skill_t sk;
    region *r = u->region;
    unit *u2;
    int fee = 2000;
    unused_arg(ord);
    unused_arg(amount);
    unused_arg(itype);
    if (u->faction->score > fee * 5) {
        score_t score = u->faction->score / 5;
        if (score < INT_MAX) {
            fee = (int)score;
        } 
        else {
            fee = INT_MAX;
        }
    }
    if (getplane(r) == arena)
        return -1;
    if (u->number != 1 && enter_fail(u))
        return -1;
    if (get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, fee) < fee
        && enter_fail(u))
        return -1;
    for (sk = 0; sk != MAXSKILLS; ++sk) {
        if (get_level(u, sk) > 1 && enter_fail(u))
            return -1;
    }
    for (u2 = r->units; u2; u2 = u2->next)
        if (u2->faction == u->faction)
            break;

    assert(!"not implemented");
    /*
            for (res=0;res!=MAXRESOURCES;++res) if (res!=R_SILVER && res!=R_ARENA_GATE && (is_item(res) || is_herb(res) || is_potion(res))) {
            int x = get_resource(u, res);
            if (x) {
            if (u2) {
            change_resource(u2, res, x);
            change_resource(u, res, -x);
            }
            else if (enter_fail(u)) return -1;
            }
            }
            */
    if (get_money(u) > fee) {
        if (u2)
            change_money(u2, get_money(u) - fee);
        else if (enter_fail(u))
            return -1;
    }
    ADDMSG(&u->faction->msgs, msg_message("arena_enter_fail", "region unit",
        u->region, u));
    use_pooled(u, itype->rtype, GET_SLACK | GET_RESERVE, 1);
    use_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, fee);
    set_money(u, 109);
    fset(u, UFL_ANON_FACTION);
    move_unit(u, start_region[rng_int() % 6], NULL);
    return 0;
}

/**
 * Tempel der Schreie, Demo-Gebäude **/

static int age_hurting(attrib * a, void *owner)
{
    building *b = (building *)a->data.v;
    unit *u;
    int active = 0;
    assert(owner == b);
    if (b == NULL)
        return AT_AGE_REMOVE;
    for (u = b->region->units; u; u = u->next) {
        if (u->building == b) {
            if (u->faction->magiegebiet == M_DRAIG) {
                active++;
                ADDMSG(&b->region->msgs, msg_message("praytoigjarjuk", "unit", u));
            }
        }
    }
    if (active)
        for (u = b->region->units; u; u = u->next)
            if (playerrace(u->faction->race)) {
                int i;
                if (u->faction->magiegebiet != M_DRAIG) {
                    for (i = 0; i != active; ++i)
                        u->hp = (u->hp + 1) / 2;    /* make them suffer, but not die */
                    ADDMSG(&b->region->msgs, msg_message("cryinpain", "unit", u));
                }
            }
    return AT_AGE_KEEP;
}

static void
write_hurting(const attrib * a, const void *owner, struct storage *store)
{
    building *b = a->data.v;
    WRITE_INT(store, b->no);
}

static int read_hurting(attrib * a, void *owner, struct gamedata *data)
{
    int i;
    READ_INT(data->store, &i);
    a->data.v = (void *)findbuilding(i);
    if (a->data.v == NULL) {
        log_error("temple of pain is broken\n");
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

static attrib_type at_hurting = {
    "hurting", NULL, NULL, age_hurting, write_hurting, read_hurting
};

#ifdef CENTRAL_VOLCANO

static int caldera_handle(trigger * t, void *data)
{
    /* call an event handler on caldera.
     * data.v -> ( variant event, int timer )
     */
    building *b = (building *)t->data.v;
    if (b != NULL) {
        unit **up = &b->region->units;
        while (*up) {
            unit *u = *up;
            if (u->building == b) {
                message *msg;
                if (u->items) {
                    item **ip = &u->items;
                    msg = msg_message("caldera_handle_1", "unit items", u, u->items);
                    while (*ip) {
                        item *i = *ip;
                        i_remove(ip, i);
                        if (*ip == i)
                            ip = &i->next;
                    }
                }
                else {
                    msg = msg_message("caldera_handle_0", "unit", u);
                }
                add_message(&u->region->msgs, msg);
                set_number(u, 0);
            }
            if (*up == u)
                up = &u->next;
        }
    }
    else {
        log_error("could not perform caldera::handle()\n");
    }
    unused_arg(data);
    return 0;
}

static void caldera_write(const trigger * t, struct storage *store)
{
    building *b = (building *)t->data.v;
    write_building_reference(b, store);
}

static int caldera_read(trigger * t, struct gamedata *data)
{
    int rb =
        read_reference(&t->data.v, data, read_building_reference,
        resolve_building);
    if (rb == 0 && !t->data.v) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

struct trigger_type tt_caldera = {
    "caldera",
    NULL,
    NULL,
    caldera_handle,
    caldera_write,
    caldera_read
};

#endif

void register_arena(void)
{
    at_register(&at_hurting);
    register_function((pf_generic)enter_arena, "enter_arena");
    register_function((pf_generic)leave_arena, "leave_arena");
    tt_register(&tt_caldera);
}

#endif /* def ARENA_MODULE */
