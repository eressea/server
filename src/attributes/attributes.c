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
#include "attributes.h"

#include "laws.h"
#include "move.h"
#include "magic.h"

/* attributes includes */
#include "follow.h"
#include "hate.h"
#include "iceberg.h"
#include "key.h"
#include "stealth.h"
#include "magic.h"
#include "movement.h"
#include "dict.h"
#include "otherfaction.h"
#include "overrideroads.h"
#include "racename.h"
#include "raceprefix.h"
#include "reduceproduction.h"
#include "seenspell.h"
#include "targetregion.h"

/* kernel includes */
#include <kernel/ally.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/building.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/macros.h>
#include <util/resolve.h>

#include <storage.h>
#include <stdlib.h>

typedef struct obs_data {
    faction *f;
    int skill;
    int timer;
} obs_data;

static void obs_init(variant *var)
{
    var->v = malloc(sizeof(obs_data));
}

static int obs_age(struct attrib *a, void *owner)
{
    obs_data *od = (obs_data *)a->data.v;

    UNUSED_ARG(owner);
    update_interval(od->f, (region *)owner);
    return --od->timer;
}

static void obs_write(const variant *var, const void *owner,
    struct storage *store)
{
    obs_data *od = (obs_data *)var->v;

    UNUSED_ARG(owner);
    write_faction_reference(od->f, store);
    WRITE_INT(store, od->skill);
    WRITE_INT(store, od->timer);
}

static int obs_read(variant *var, void *owner, struct gamedata *data)
{
    obs_data *od = (obs_data *)var->v;

    UNUSED_ARG(owner);
    read_faction_reference(data, &od->f, NULL);
    READ_INT(data->store, &od->skill);
    READ_INT(data->store, &od->timer);
    return AT_READ_OK;
}

attrib_type at_observer = {
    "observer", obs_init, a_free_voidptr, obs_age, obs_write, obs_read
};

static attrib *make_observer(faction *f, int perception)
{
    attrib * a = a_new(&at_observer);
    obs_data *od = (obs_data *)a->data.v;
    od->f = f;
    od->skill = perception;
    od->timer = 2;
    return a;
}

int get_observer(const region *r, const faction *f) {
    if (fval(r, RF_OBSERVER)) {
        attrib *a = a_find(r->attribs, &at_observer);
        while (a && a->type == &at_observer) {
            obs_data *od = (obs_data *)a->data.v;
            if (od->f == f) {
                return od->skill;
            }
            a = a->next;
        }
    }
    return -1;
}

void set_observer(region *r, faction *f, int skill, int turns)
{
    update_interval(f, r);
    if (fval(r, RF_OBSERVER)) {
        attrib *a = a_find(r->attribs, &at_observer);
        while (a && a->type == &at_observer) {
            obs_data *od = (obs_data *)a->data.v;
            if (od->f == f && od->skill < skill) {
                od->skill = skill;
                od->timer = turns;
                return;
            }
            a = a->nexttype;
        }
    }
    else {
        fset(r, RF_OBSERVER);
    }
    a_add(&r->attribs, make_observer(f, skill));
}

attrib_type at_unitdissolve = {
    "unitdissolve", NULL, NULL, NULL, a_writechars, a_readchars
};

static int read_ext(variant *var, void *owner, gamedata *data)
{
    int len;

    UNUSED_ARG(var);
    READ_INT(data->store, &len);
    data->store->api->r_bin(data->store->handle, NULL, (size_t)len);
    return AT_READ_OK;
}

void register_attributes(void)
{
    /* Alle speicherbaren Attribute müssen hier registriert werden */
    at_register(&at_shiptrail);
    at_register(&at_familiar);
    at_register(&at_familiarmage);
    at_register(&at_clone);
    at_register(&at_clonemage);
    at_register(&at_eventhandler);
    at_register(&at_mage);
    at_register(&at_countdown);
    at_register(&at_curse);
    at_register(&at_seenspell);
    at_register(&at_seenspells);

    /* REGION Attribute */
    at_register(&at_moveblock);
    at_register(&at_deathcount);
    at_register(&at_woodcount);
    at_register(&at_germs);

    /* UNIT Attribute */
    at_register(&at_effect);
    at_register(&at_private);

    at_register(&at_icastle);
    at_register(&at_group);

    at_register(&at_building_generic_type);
    at_register(&at_npcfaction);

    /* connection-typen */
    register_bordertype(&bt_noway);
    register_bordertype(&bt_fogwall);
    register_bordertype(&bt_wall);
    register_bordertype(&bt_illusionwall);
    register_bordertype(&bt_road);

    at_deprecate("siege", a_readint);
    at_deprecate("maxmagicians", a_readint); /* factions with differnt magician limits, probably unused */
    at_deprecate("hurting", a_readint); /* an old arena attribute */
    at_deprecate("chaoscount", a_readint); /* used to increase the chance of monster spawns */
    at_deprecate("xontormiaexpress", a_readint);    /* required for old datafiles */
    at_deprecate("orcification", a_readint);    /* required for old datafiles */
    at_deprecate("lua", read_ext);    /* required for old datafiles */
    at_deprecate("moved", a_readint);
    at_deprecate("gm", a_readint);
    at_deprecate("guard", a_readint); /* used to contain guard-flags (v3.10.0-259-g8597e8b) */
    at_register(&at_stealth);
    at_register(&at_dict);
    at_register(&at_unitdissolve);
    at_register(&at_observer);
    at_register(&at_overrideroads);
    at_register(&at_raceprefix);
    at_register(&at_iceberg);
    at_register(&at_key);
    at_register(&at_keys);
    at_register(&at_follow);
    at_register(&at_targetregion);
    at_register(&at_hate);
    at_register(&at_reduceproduction);
    at_register(&at_otherfaction);
    at_register(&at_racename);
    at_register(&at_speedup);
    at_register(&at_movement);
}
