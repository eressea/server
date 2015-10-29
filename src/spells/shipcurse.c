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
#include <kernel/config.h>
#include <kernel/version.h>
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

    unused_arg(typ);
    unused_arg(obj);
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

    unused_arg(typ);
    assert(typ == TYP_SHIP);

    if (self != 0) {
        return msg_message("curseinfo::shipnodrift_1", "ship duration id", sh,
            c->duration, c->no);
    }
    return msg_message("curseinfo::shipnodrift_0", "ship id", sh, c->no);
}

static struct curse_type ct_stormwind = { "stormwind",
CURSETYP_NORM, 0, NO_MERGE, cinfo_ship
};

static int flyingship_read(storage * store, curse * c, void *target)
{
    ship *sh = (ship *)target;
    c->data.v = sh;
    if (global.data_version < FOSS_VERSION) {
        sh->flags |= SF_FLYING;
        return 0;
    }
    assert(sh->flags & SF_FLYING);
    return 0;
}

static int flyingship_write(storage * store, const curse * c,
    const void *target)
{
    const ship *sh = (const ship *)target;
    assert(sh->flags & SF_FLYING);
    return 0;
}

static int flyingship_age(curse * c)
{
    ship *sh = (ship *)c->data.v;
    if (sh && c->duration == 1) {
        freset(sh, SF_FLYING);
        return 1;
    }
    return 0;
}

static struct curse_type ct_flyingship = { "flyingship",
CURSETYP_NORM, 0, NO_MERGE, cinfo_ship, NULL, flyingship_read,
flyingship_write, NULL, flyingship_age
};

static struct curse_type ct_nodrift = { "nodrift",
CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR), cinfo_shipnodrift
};

static struct curse_type ct_shipspeedup = { "shipspeedup",
CURSETYP_NORM, 0, 0, cinfo_ship
};

curse *shipcurse_flyingship(ship * sh, unit * mage, double power, int duration)
{
    static const curse_type *ct_flyingship = NULL;
    if (!ct_flyingship) {
        ct_flyingship = ct_find("flyingship");
        assert(ct_flyingship);
    }
    if (curse_active(get_curse(sh->attribs, ct_flyingship))) {
        return NULL;
    }
    else if (is_cursed(sh->attribs, C_SHIP_SPEEDUP, 0)) {
        return NULL;
    }
    else {
        /* mit C_SHIP_NODRIFT haben wir kein Problem */
        curse *c =
            create_curse(mage, &sh->attribs, ct_flyingship, power, duration, 0.0, 0);
        if (c) {
            c->data.v = sh;
            if (c->duration > 0) {
                sh->flags |= SF_FLYING;
            }
        }
        return c;
    }
}

int levitate_ship(ship * sh, unit * mage, double power, int duration)
{
    curse *c = shipcurse_flyingship(sh, mage, power, duration);
    if (c) {
        return c->no;
    }
    return 0;
}

void register_shipcurse(void)
{
    ct_register(&ct_stormwind);
    ct_register(&ct_flyingship);
    ct_register(&ct_nodrift);
    ct_register(&ct_shipspeedup);
}
