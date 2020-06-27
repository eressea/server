#include <platform.h>
#include <kernel/config.h>
#include "skill.h"

#include "curse.h"
#include "region.h"
#include "unit.h"

#include <kernel/attrib.h>
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
static void init_skillmod(variant *var)
{
    var->v = calloc(1, sizeof(skillmod_data));
}

/** temporary skill modification (NOT SAVED!). */
attrib_type at_skillmod = {
    "skillmod",
    init_skillmod,
    a_free_voidptr,
    NULL,
    NULL,                         /* can't write function pointers */
    NULL,                         /* can't read function pointers */
    NULL,
    ATF_PRESERVE
};

attrib *make_skillmod(skill_t sk, skillmod_fun special, double multiplier, int bonus)
{
    attrib *a = a_new(&at_skillmod);
    skillmod_data *smd = (skillmod_data *)a->data.v;

    smd->skill = sk;
    smd->special = special;
    smd->bonus = bonus;
    smd->multiplier = multiplier;

    return a;
}

int
skillmod(const unit * u, const region * r, skill_t sk, int value)
{
    const attrib * a = u->attribs;
    for (a = a_find((attrib *)a, &at_skillmod); a && a->type == &at_skillmod;
        a = a->next) {
        skillmod_data *smd = (skillmod_data *)a->data.v;
        if (smd->skill != NOSKILL && smd->skill != sk)
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

int level_days(int level)
{
    /* FIXME STUDYDAYS * ((level + 1) * level / 2); */
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
    assert(sv && level != 0);
    sv->weeks = skill_weeks(level);
    sv->level = level;
    assert(sv->weeks <= sv->level * 2 + 1);
}

static bool rule_random_progress(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("study.random_progress", 1);
    }
    return rule != 0;
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

void increase_skill(unit * u, skill_t sk, int weeks)
{
    skill *sv = unit_skill(u, sk);
    assert(weeks >= 0);
    if (!sv) {
        sv = add_skill(u, sk);
    }
    while (sv->weeks <= weeks) {
        weeks -= sv->weeks;
        sk_set(sv, sv->level + 1);
    }
    sv->weeks -= weeks;
    assert(sv->weeks <= sv->level * 2 + 1);
}

void reduce_skill(unit * u, skill * sv, int weeks)
{
    int max_weeks = sv->level + 1;

    assert(weeks >= 0);
    if (rule_random_progress()) {
        max_weeks += sv->level;
    }
    sv->weeks += weeks;
    while (sv->level > 0 && sv->weeks > max_weeks) {
        sv->weeks -= sv->level;
        --sv->level;
        max_weeks -= 2;
    }
    if (sv->level == 0) {
        /* reroll */
        sv->weeks = skill_weeks(sv->level);
    }
    assert(sv->weeks <= sv->level * 2 + 1);
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
