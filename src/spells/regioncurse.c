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
#include "regioncurse.h"
#include "magic.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <kernel/objtypes.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/nrmessage.h>
#include <util/message.h>
#include <util/functions.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* --------------------------------------------------------------------- */
/* CurseInfo mit Spezialabfragen
 */

/*
 * godcursezone
 */
static message *cinfo_cursed_by_the_gods(const void *obj, objtype_t typ,
    const curse * c, int self)
{
    region *r = (region *)obj;

    unused_arg(typ);
    unused_arg(self);
    assert(typ == TYP_REGION);

    if (r->terrain->flags & SEA_REGION) {
        return msg_message("curseinfo::godcurseocean", "id", c->no);
    }
    return msg_message("curseinfo::godcurse", "id", c->no);
}

static struct curse_type ct_godcursezone = {
    "godcursezone",
    CURSETYP_NORM, CURSE_IMMUNE | CURSE_ISNEW, (NO_MERGE),
    cinfo_cursed_by_the_gods,
};

/* --------------------------------------------------------------------- */
/*
 * C_GBDREAM
 */
static message *cinfo_dreamcurse(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(self);
    unused_arg(typ);
    unused_arg(obj);
    assert(typ == TYP_REGION);

    if (c->effect > 0) {
        return msg_message("curseinfo::gooddream", "id", c->no);
    }
    else {
        return msg_message("curseinfo::baddream", "id", c->no);
    }
}

static struct curse_type ct_gbdream = {
    "gbdream",
    CURSETYP_NORM, CURSE_ISNEW, (NO_MERGE), cinfo_dreamcurse
};

/* --------------------------------------------------------------------- */
/*
 * C_MAGICSTREET
 *  erzeugt Straßennetz
 */
static message *cinfo_magicstreet(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    unused_arg(self);
    unused_arg(obj);
    assert(typ == TYP_REGION);

    /* Warnung vor Auflösung */
    if (c->duration >= 2) {
        return msg_message("curseinfo::magicstreet", "id", c->no);
    }
    return msg_message("curseinfo::magicstreetwarn", "id", c->no);
}

static struct curse_type ct_magicstreet = {
    "magicstreet",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
    cinfo_magicstreet
};

/* --------------------------------------------------------------------- */

static message *cinfo_antimagiczone(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    unused_arg(self);
    unused_arg(obj);
    assert(typ == TYP_REGION);

    /* Magier spüren eine Antimagiezone */
    if (self != 0) {
        return msg_message("curseinfo::antimagiczone", "id", c->no);
    }

    return NULL;
}

/* alle Magier können eine Antimagiezone wahrnehmen */
static int
cansee_antimagiczone(const struct faction *viewer, const void *obj, objtype_t typ,
const curse * c, int self)
{
    region *r;
    unit *u = NULL;
    unit *mage = c->magician;

    unused_arg(typ);

    assert(typ == TYP_REGION);
    r = (region *)obj;
    for (u = r->units; u; u = u->next) {
        if (u->faction == viewer) {
            if (u == mage) {
                self = 2;
                break;
            }
            if (is_mage(u)) {
                self = 1;
            }
        }
    }
    return self;
}

static struct curse_type ct_antimagiczone = {
    "antimagiczone",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
    cinfo_antimagiczone, NULL, NULL, NULL, cansee_antimagiczone
};

/* --------------------------------------------------------------------- */
static message *cinfo_farvision(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    unused_arg(obj);

    assert(typ == TYP_REGION);

    /* Magier spüren eine farvision */
    if (self != 0) {
        return msg_message("curseinfo::farvision", "id", c->no);
    }

    return 0;
}

static struct curse_type ct_farvision = {
    "farvision",
    CURSETYP_NORM, 0, (NO_MERGE),
    cinfo_farvision
};

/* --------------------------------------------------------------------- */

static struct curse_type ct_fogtrap = {
    "fogtrap",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
    cinfo_simple
};

static struct curse_type ct_maelstrom = {
    "maelstrom",
    CURSETYP_NORM, CURSE_ISNEW, (M_DURATION | M_VIGOUR),
    cinfo_simple
};

static struct curse_type ct_blessedharvest = {
    "blessedharvest",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
    cinfo_simple
};

static struct curse_type ct_drought = {
    "drought",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
    cinfo_simple
};

static struct curse_type ct_badlearn = {
    "badlearn",
    CURSETYP_NORM, CURSE_ISNEW, (M_DURATION | M_VIGOUR),
    cinfo_simple
};

/*  Trübsal-Zauber */
static struct curse_type ct_depression = {
    "depression",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
    cinfo_simple
};

/* Astralblock, auf Astralregion */
static struct curse_type ct_astralblock = {
    "astralblock",
    CURSETYP_NORM, 0, NO_MERGE,
    cinfo_simple
};

/* Unterhaltungsanteil vermehren */
static struct curse_type ct_generous = {
    "generous",
    CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR | M_MAXEFFECT),
    cinfo_simple
};

/* verhindert Attackiere regional */
static struct curse_type ct_peacezone = {
    "peacezone",
    CURSETYP_NORM, 0, NO_MERGE,
    cinfo_simple
};

/*  erniedigt Magieresistenz von nicht-aliierten Einheiten, wirkt nur 1x
*  pro Einheit */
static struct curse_type ct_badmagicresistancezone = {
    "badmagicresistancezone",
    CURSETYP_NORM, 0, NO_MERGE,
    cinfo_simple
};

/* erhöht Magieresistenz von aliierten Einheiten, wirkt nur 1x pro
* Einheit */
static struct curse_type ct_goodmagicresistancezone = {
    "goodmagicresistancezone",
    CURSETYP_NORM, 0, NO_MERGE,
    cinfo_simple
};

static struct curse_type ct_riotzone = {
    "riotzone",
    CURSETYP_NORM, 0, (M_DURATION),
    cinfo_simple
};

static struct curse_type ct_holyground = {
    "holyground",
    CURSETYP_NORM, CURSE_NOAGE, (M_VIGOUR_ADD),
    cinfo_simple
};

static struct curse_type ct_healing = {
    "healingzone",
    CURSETYP_NORM, 0, (M_VIGOUR | M_DURATION),
    cinfo_simple
};

void register_regioncurse(void)
{
    ct_register(&ct_fogtrap);
    ct_register(&ct_antimagiczone);
    ct_register(&ct_farvision);
    ct_register(&ct_gbdream);
    ct_register(&ct_maelstrom);
    ct_register(&ct_blessedharvest);
    ct_register(&ct_drought);
    ct_register(&ct_badlearn);
    ct_register(&ct_depression);
    ct_register(&ct_astralblock);
    ct_register(&ct_generous);
    ct_register(&ct_peacezone);
    ct_register(&ct_magicstreet);
    ct_register(&ct_badmagicresistancezone);
    ct_register(&ct_goodmagicresistancezone);
    ct_register(&ct_riotzone);
    ct_register(&ct_godcursezone);
    ct_register(&ct_holyground);
    ct_register(&ct_healing);
}
