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
#include "unitcurse.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/messages.h>
#include <kernel/race.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/objtypes.h>
#include <kernel/version.h>

/* util includes */
#include <util/language.h>
#include <util/nrmessage.h>
#include <util/message.h>
#include <util/base36.h>
#include <util/functions.h>

#include <storage.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ------------------------------------------------------------- */
/*
 * C_AURA
 */
/* erhöht/senkt regeneration und maxaura um effect% */
static message *cinfo_auraboost(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    struct unit *u = (struct unit *)obj;
    unused_arg(typ);
    assert(typ == TYP_UNIT);

    if (self != 0) {
        if (curse_geteffect(c) > 100) {
            return msg_message("curseinfo::auraboost_0", "unit id", u, c->no);
        }
        else {
            return msg_message("curseinfo::auraboost_1", "unit id", u, c->no);
        }
    }
    return NULL;
}

static struct curse_type ct_auraboost = {
    "auraboost",
    CURSETYP_NORM, CURSE_SPREADMODULO, (NO_MERGE),
    cinfo_auraboost
};

/* Magic Boost - Gabe des Chaos */
static struct curse_type ct_magicboost = {
    "magicboost",
    CURSETYP_UNIT, CURSE_SPREADMODULO | CURSE_IMMUNE, M_MEN, cinfo_simple
};

/* ------------------------------------------------------------- */
/*
 * C_SLAVE
 */
static message *cinfo_slave(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unit *u;
    unused_arg(typ);

    assert(typ == TYP_UNIT);
    u = (unit *)obj;

    if (self != 0) {
        return msg_message("curseinfo::slave_1", "unit duration id", u, c->duration,
            c->no);
    }
    return NULL;
}

static struct curse_type ct_slavery = { "slavery",
CURSETYP_NORM, 0, NO_MERGE,
cinfo_slave
};

/* ------------------------------------------------------------- */
/*
 * C_CALM
 */
static message *cinfo_calm(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    assert(typ == TYP_UNIT);

    if (c->magician && c->magician->faction) {
        faction *f = c->magician->faction;
        unit *u = (unit *)obj;

        if (f == NULL || self == 0) {
            const struct race *rc = u_irace(c->magician);
            return msg_message("curseinfo::calm_0", "unit race id", u, rc, c->no);
        }
        return msg_message("curseinfo::calm_1", "unit faction id", u, f, c->no);
    }
    return NULL;
}

static struct curse_type ct_calmmonster = {
    "calmmonster",
    CURSETYP_NORM, CURSE_SPREADNEVER | CURSE_ONLYONE, NO_MERGE,
    cinfo_calm
};

/* ------------------------------------------------------------- */
/*
 * C_SPEED
 */
static message *cinfo_speed(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    assert(typ == TYP_UNIT);

    if (self != 0) {
        unit *u = (unit *)obj;
        return msg_message("curseinfo::speed_1", "unit number duration id", u,
            c->data.i, c->duration, c->no);
    }
    return NULL;
}

static struct curse_type ct_speed = {
    "speed",
    CURSETYP_UNIT, CURSE_SPREADNEVER, M_MEN,
    cinfo_speed
};

/* ------------------------------------------------------------- */
/*
 * C_ORC
 */
message *cinfo_unit(const void *obj, objtype_t typ, const curse * c, int self)
{
    unused_arg(typ);
    assert(typ == TYP_UNIT);

    if (self != 0) {
        unit *u = (unit *)obj;
        return msg_message(mkname("curseinfo", c->type->cname), "unit id", u,
            c->no);
    }
    return NULL;
}

static struct curse_type ct_orcish = {
    "orcish",
    CURSETYP_UNIT, CURSE_SPREADMODULO | CURSE_ISNEW, M_MEN,
    cinfo_unit
};

/* ------------------------------------------------------------- */
/*
 * C_KAELTESCHUTZ
 */
static message *cinfo_kaelteschutz(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);
    assert(typ == TYP_UNIT);

    if (self != 0) {
        unit *u = (unit *)obj;
        return msg_message("curseinfo::warmth_1", "unit number id", u,
            get_cursedmen(u, c), c->no);
    }
    return NULL;
}

static struct curse_type ct_insectfur = {
    "insectfur",
    CURSETYP_UNIT, CURSE_SPREADMODULO, (M_MEN | M_DURATION),
    cinfo_kaelteschutz
};

/* ------------------------------------------------------------- */
/*
 * C_SPARKLE
 */
static message *cinfo_sparkle(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    const char *effects[] = {
        NULL,                       /* end grau */
        "sparkle_1",
        "sparkle_2",
        NULL,                       /* end traum */
        "sparkle_3",
        "sparkle_4",
        NULL,                       /* end tybied */
        "sparkle_5",
        "sparkle_6",
        "sparkle_7",
        "sparkle_8",
        NULL,                       /* end cerrdor */
        "sparkle_9",
        "sparkle_10",
        "sparkle_11",
        "sparkle_12",
        NULL,                       /* end gwyrrd */
        "sparkle_13",
        "sparkle_14",
        "sparkle_15",
        "sparkle_16",
        "sparkle_17",
        "sparkle_18",
        NULL,                       /* end draig */
    };
    int m, begin = 0, end = 0;
    unit *u;
    unused_arg(typ);

    assert(typ == TYP_UNIT);
    u = (unit *)obj;

    if (!c->magician || !c->magician->faction)
        return NULL;

    for (m = 0; m != c->magician->faction->magiegebiet; ++m) {
        while (effects[end] != NULL)
            ++end;
        begin = end + 1;
        end = begin;
    }

    while (effects[end] != NULL)
        ++end;
    if (end == begin)
        return NULL;
    else {
        int index = begin + curse_geteffect_int(c) % (end - begin);
        return msg_message(mkname("curseinfo", effects[index]), "unit id", u,
            c->no);
    }
}

static struct curse_type ct_sparkle = { "sparkle",
CURSETYP_UNIT, CURSE_SPREADMODULO, (M_MEN | M_DURATION), cinfo_sparkle
};

/* ------------------------------------------------------------- */
/*
 * C_STRENGTH
 */
static struct curse_type ct_strength = { "strength",
CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN, cinfo_simple
};

/* ------------------------------------------------------------- */
/*
 * C_ALLSKILLS (Alp)
 */
static struct curse_type ct_worse = {
    "worse", CURSETYP_UNIT, CURSE_SPREADMODULO | CURSE_NOAGE, M_MEN, cinfo_unit
};

/* ------------------------------------------------------------- */

/*
 * C_ITEMCLOAK
 */
static struct curse_type ct_itemcloak = {
    "itemcloak", CURSETYP_UNIT, CURSE_SPREADNEVER, M_DURATION, cinfo_unit
};

/* ------------------------------------------------------------- */

static struct curse_type ct_fumble = {
    "fumble", CURSETYP_NORM, CURSE_SPREADNEVER | CURSE_ONLYONE, NO_MERGE,
    cinfo_unit
};

/* ------------------------------------------------------------- */

static struct curse_type ct_oldrace = {
    "oldrace", CURSETYP_NORM, CURSE_SPREADALWAYS, NO_MERGE, NULL
};

/* ------------------------------------------------------------- */
/*
 * C_SKILL
 */

static int read_skill(struct storage *store, curse * c, void *target)
{
    int skill;
    READ_INT(store, &skill);
    c->data.i = skill;
    return 0;
}

static int
write_skill(struct storage *store, const curse * c, const void *target)
{
    WRITE_INT(store, c->data.i);
    return 0;
}

static message *cinfo_skillmod(const void *obj, objtype_t typ, const curse * c,
    int self)
{
    unused_arg(typ);

    if (self != 0) {
        unit *u = (unit *)obj;
        int sk = c->data.i;
        if (c->effect > 0) {
            return msg_message("curseinfo::skill_1", "unit skill id", u, sk, c->no);
        }
        else if (c->effect < 0) {
            return msg_message("curseinfo::skill_2", "unit skill id", u, sk, c->no);
        }
    }
    return NULL;
}

static struct curse_type ct_skillmod = {
    "skillmod", CURSETYP_NORM, CURSE_SPREADMODULO, M_MEN, cinfo_skillmod,
    NULL, read_skill, write_skill
};

/* ------------------------------------------------------------- */
void register_unitcurse(void)
{
    ct_register(&ct_auraboost);
    ct_register(&ct_magicboost);
    ct_register(&ct_slavery);
    ct_register(&ct_calmmonster);
    ct_register(&ct_speed);
    ct_register(&ct_orcish);
    ct_register(&ct_insectfur);
    ct_register(&ct_sparkle);
    ct_register(&ct_strength);
    ct_register(&ct_worse);
    ct_register(&ct_skillmod);
    ct_register(&ct_itemcloak);
    ct_register(&ct_fumble);
    ct_register(&ct_oldrace);
}
