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
#include "guard.h"

/* attributes includes */
#include "follow.h"
#include "hate.h"
#include "iceberg.h"
#include "key.h"
#include "stealth.h"
#include "magic.h"
#include "moved.h"
#include "movement.h"
#include "dict.h"
#include "orcification.h"
#include "otherfaction.h"
#include "overrideroads.h"
#include "racename.h"
#include "raceprefix.h"
#include "reduceproduction.h"
#include "targetregion.h"

/* kernel includes */
#include <kernel/ally.h>
#include <kernel/connection.h>
#include <kernel/curse.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/building.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>

#include <storage.h>

attrib_type at_unitdissolve = {
    "unitdissolve", NULL, NULL, NULL, a_writechars, a_readchars
};

static int read_ext(attrib * a, void *owner, struct storage *store)
{
    int len;

    READ_INT(store, &len);
    store->api->r_bin(store->handle, NULL, (size_t)len);
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

    /* neue REGION-Attribute */
    at_register(&at_moveblock);
    at_register(&at_deathcount);
    at_register(&at_woodcount);

    /* neue UNIT-Attribute */
    at_register(&at_siege);
    at_register(&at_effect);
    at_register(&at_private);

    at_register(&at_icastle);
    at_register(&at_guard);
    at_register(&at_group);

    at_register(&at_building_generic_type);
    at_register(&at_maxmagicians);
    at_register(&at_npcfaction);

    /* connection-typen */
    register_bordertype(&bt_noway);
    register_bordertype(&bt_fogwall);
    register_bordertype(&bt_wall);
    register_bordertype(&bt_illusionwall);
    register_bordertype(&bt_road);

    at_register(&at_germs);

    at_deprecate("xontormiaexpress", a_readint);    /* required for old datafiles */
    at_deprecate("lua", read_ext);    /* required for old datafiles */
    at_deprecate("gm", a_readint);
    at_register(&at_stealth);
    at_register(&at_dict);
    at_register(&at_unitdissolve);
    at_register(&at_overrideroads);
    at_register(&at_raceprefix);
    at_register(&at_iceberg);
    at_register(&at_key);
    at_register(&at_follow);
    at_register(&at_targetregion);
    at_register(&at_orcification);
    at_register(&at_hate);
    at_register(&at_reduceproduction);
    at_register(&at_otherfaction);
    at_register(&at_racename);
    at_register(&at_speedup);
    at_register(&at_movement);
    at_register(&at_moved);
}
