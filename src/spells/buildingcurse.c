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
#include "buildingcurse.h"

/* kernel includes */
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/curse.h>

/* util includes */
#include <util/nrmessage.h>
#include <util/base36.h>
#include <util/functions.h>
#include <util/language.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

message *cinfo_building(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    assert(typ == TYP_BUILDING);

    if (self != 0) {              /* owner or inside */
        building *b = (building *)obj;
        return msg_message(mkname("curseinfo", c->type->cname), "id building", c->no, b);
    }
    return msg_message(mkname("curseinfo", "buildingunknown"), "id", c->no);
}

/* CurseInfo mit Spezialabfragen */

/* C_MAGICWALLS*/
static message *cinfo_magicrunes(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    message *msg = NULL;
    if (typ == TYP_BUILDING) {
        building *b;
        b = (building *)obj;
        if (self != 0) {
            msg =
                msg_message("curseinfo::magicrunes_building", "building id", b, c->no);
        }
    }
    else if (typ == TYP_SHIP) {
        ship *sh;
        sh = (ship *)obj;
        if (self != 0) {
            msg = msg_message("curseinfo::magicrunes_ship", "ship id", sh, c->no);
        }
    }

    return msg;
}

static struct curse_type ct_magicrunes = { "magicrunes",
CURSETYP_NORM, 0, M_SUMEFFECT, cinfo_magicrunes
};

/* Heimstein-Zauber */
static struct curse_type ct_magicwalls = { "magicwalls",
CURSETYP_NORM, 0, NO_MERGE, cinfo_building
};

/* Feste Mauer - Präkampfzauber, wirkt nur 1 Runde */
static struct curse_type ct_strongwall = { "strongwall",
CURSETYP_NORM, 0, NO_MERGE, NULL
};

/* Ewige Mauern-Zauber */
static struct curse_type ct_nocostbuilding = { "nocostbuilding",
CURSETYP_NORM, CURSE_NOAGE | CURSE_ONLYONE, NO_MERGE, cinfo_building
};

void register_buildingcurse(void)
{
    ct_register(&ct_magicwalls);
    ct_register(&ct_strongwall);
    ct_register(&ct_magicrunes);
    ct_register(&ct_nocostbuilding);
}
