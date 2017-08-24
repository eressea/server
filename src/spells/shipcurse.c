/*
 *
 * Eressea PB(E)M host Copyright (C) 1998-2015
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <platform.h>
#include "shipcurse.h"

/* kernel includes */
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/curse.h>

/* util includes */
#include <util/base36.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>

#include <storage.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

message *cinfo_ship(const void *obj, objtype_t typ, const curse * c, int self)
{
    message *msg;

    UNUSED_ARG(typ);
    UNUSED_ARG(obj);
    assert(typ == TYP_SHIP);

    if (self != 0) {              /* owner or inside */
        msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
    }
    else {
        msg = msg_message("curseinfo::ship_unknown", "id", c->no);
    }
    if (msg == NULL) {
        log_error("There is no curseinfo for %s.\n", c->type->cname);
    }
    return msg;
}

/* CurseInfo mit Spezialabfragen */

/* C_SHIP_NODRIFT */
static message *cinfo_shipnodrift(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    ship *sh = (ship *)obj;

    UNUSED_ARG(typ);
    assert(typ == TYP_SHIP);

    if (self != 0) {
        return msg_message("curseinfo::shipnodrift_1", "ship duration id", sh,
            c->duration, c->no);
    }
    return msg_message("curseinfo::shipnodrift_0", "ship id", sh, c->no);
}

const struct curse_type ct_stormwind = { "stormwind",
CURSETYP_NORM, 0, NO_MERGE, cinfo_ship
};

const struct curse_type ct_nodrift = { "nodrift",
CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR), cinfo_shipnodrift
};

const struct curse_type ct_shipspeedup = { "shipspeedup",
CURSETYP_NORM, 0, 0, cinfo_ship
};

void register_shipcurse(void)
{
    ct_register(&ct_stormwind);
    ct_register(&ct_nodrift);
    ct_register(&ct_shipspeedup);
}
