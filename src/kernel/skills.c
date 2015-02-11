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
#include "skill.h"

#include "curse.h"
#include "item.h"
#include "race.h"
#include "region.h"
#include "terrain.h"
#include "terrainid.h"
#include "unit.h"

#include <util/attrib.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/** skillmod attribut **/
static void init_skillmod(attrib * a)
{
    a->data.v = calloc(sizeof(skillmod_data), 1);
}

static void finalize_skillmod(attrib * a)
{
    free(a->data.v);
}

/** temporary skill modification (NOT SAVED!). */
attrib_type at_skillmod = {
    "skillmod",
    init_skillmod,
    finalize_skillmod,
    NULL,
    NULL,                         /* can't write function pointers */
    NULL,                         /* can't read function pointers */
    ATF_PRESERVE
};

attrib *make_skillmod(skill_t sk, unsigned int flags, skillmod_fun special,
    double multiplier, int bonus)
{
    attrib *a = a_new(&at_skillmod);
    skillmod_data *smd = (skillmod_data *)a->data.v;

    smd->skill = sk;
    smd->special = special;
    smd->bonus = bonus;
    smd->multiplier = multiplier;
    smd->flags = flags;

    return a;
}

int
skillmod(const attrib * a, const unit * u, const region * r, skill_t sk,
int value, int flags)
{
    for (a = a_find((attrib *)a, &at_skillmod); a && a->type == &at_skillmod;
        a = a->next) {
        skillmod_data *smd = (skillmod_data *)a->data.v;
        if (smd->skill != NOSKILL && smd->skill != sk)
            continue;
        if (flags != SMF_ALWAYS && (smd->flags & flags) == 0)
            continue;
        if (smd->special) {
            value = smd->special(u, r, sk, value);
            if (value < 0)
                return value;           /* pass errors back to caller */
        }
        if (smd->multiplier)
            value = (int)(value * smd->multiplier);
        value += smd->bonus;
    }
    return value;
}

int skill_mod(const race * rc, skill_t sk, const struct terrain_type *terrain)
{
    int result = 0;

    result = rc->bonus[sk];

    if (rc == get_race(RC_DWARF)) {
        if (sk == SK_TACTICS) {
            if (terrain == newterrain(T_MOUNTAIN) || fval(terrain, ARCTIC_REGION))
                ++result;
        }
    }
    else if (rc == get_race(RC_INSECT)) {
        if (terrain == newterrain(T_MOUNTAIN) || fval(terrain, ARCTIC_REGION))
            --result;
        else if (terrain == newterrain(T_DESERT) || terrain == newterrain(T_SWAMP))
            ++result;
    }

    return result;
}

#define RCMODMAXHASH 31
#ifdef FASTER_SKILLMOD
static struct skillmods {
    struct skillmods *next;
    const struct race *race;
    struct modifiers {
        int value[MAXSKILLS];
    } mod[MAXTERRAINS];
} *modhash[RCMODMAXHASH];

static struct skillmods *init_skills(const race * rc)
{
    terrain_t t;
    struct skillmods *mods =
        (struct skillmods *)calloc(1, sizeof(struct skillmods));
    mods->race = rc;

    for (t = 0; t != MAXTERRAINS; ++t) {
        skill_t sk;
        for (sk = 0; sk != MAXSKILLS; ++sk) {
            mods->mod[t].value[sk] = skill_mod(rc, sk, newterrain(t));
        }
    }
    return mods;
}
#endif

int rc_skillmod(const struct race *rc, const region * r, skill_t sk)
{
    int mods = 0;

    if (!skill_enabled(sk)) {
        return 0;
    }
#ifdef FASTER_SKILLMOD
    unsigned int index = hashstring(rc->_name) % RCMODMAXHASH;
    struct skillmods **imods = &modhash[index];
    while (*imods && (*imods)->race != rc) {
        imods = &(*imods)->next;
    }
    if (*imods == NULL) {
        *imods = init_skills(rc);
    }
    mods = (*imods)->mod[rterrain(r)].value[sk];
#else
    if (r) {
        mods = skill_mod(rc, sk, r->terrain);
    }
#endif
    if (rc == get_race(RC_ELF) && r && r_isforest(r)) {
        if (sk == SK_PERCEPTION || sk == SK_STEALTH) {
            ++mods;
        }
        else if (sk == SK_TACTICS) {
            mods += 2;
        }
    }
    return mods;
}

int level_days(int level)
{
    return 30 * ((level + 1) * level / 2);
}

int level(int days)
{
    int i;
    static int ldays[64];
    static bool init = false;
    if (!init) {
        init = true;
        for (i = 0; i != 64; ++i)
            ldays[i] = level_days(i + 1);
    }
    for (i = 0; i != 64; ++i)
        if (ldays[i] > days)
            return i;
    return i;
}

void sk_set(skill * sv, int level)
{
    assert(level != 0);
    sv->weeks = (unsigned char)skill_weeks(level);
    sv->level = (unsigned char)level;
}

static int rule_random_progress(void)
{
    return get_param_int(global.parameters, "study.random_progress", 1);
}

int skill_weeks(int level)
/* how many weeks must i study to get from level to level+1 */
{
    if (rule_random_progress()) {
        int coins = 2 * level;
        int heads = 1;
        while (coins--) {
            heads += rng_int() % 2;
        }
        return heads;
    }
    return level + 1;
}

void reduce_skill(unit * u, skill * sv, unsigned int weeks)
{
    sv->weeks += weeks;
    while (sv->level > 0 && sv->level * 2 + 1 < sv->weeks) {
        sv->weeks -= sv->level;
        --sv->level;
    }
    if (sv->level == 0) {
        /* reroll */
        sv->weeks = (unsigned char)skill_weeks(sv->level);
    }
}

int skill_compare(const skill * sk, const skill * sc)
{
    if (sk->level > sc->level)
        return 1;
    if (sk->level < sc->level)
        return -1;
    if (sk->weeks < sc->weeks)
        return 1;
    if (sk->weeks > sc->weeks)
        return -1;
    return 0;
}
