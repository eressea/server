#include <kernel/config.h>

#include "curse.h"
#include "region.h"
#include "unit.h"

#include <kernel/attrib.h>
#include <kernel/skills.h>

#include <util/goodies.h>
#include <util/language.h>
#include <util/log.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <math.h>
#include <stdbool.h>
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

attrib *make_skillmod(enum skill_t sk, skillmod_fun special, double multiplier, int bonus)
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
skillmod(const unit * u, const region * r, enum skill_t sk, int value)
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

static bool rule_random_progress(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("study.random_progress", 0);
    }
    return rule != 0;
}

static int progress_weeks(unsigned int level, bool random_progress)
/* how many weeks must i study to get from level-1 to level */
{
    assert(level > 0);
    if (random_progress) {
        unsigned int coins = MAX_WEEKS_TO_NEXT_LEVEL(level - 1) - 1;
        int heads = 1;
        while (coins--) {
            heads += rng_int() % 2;
        }
        return heads;
    }
    return level;
}

static void skill_set(skill *sv, unsigned int level, unsigned int weeks)
{
    assert(weeks <= MAX_WEEKS_TO_NEXT_LEVEL(level));
    sv->level = level;
    sv->weeks = weeks;
    assert(sv->weeks == weeks && sv->level == level);
}

void sk_set_level(skill *sv, unsigned int level)
{
    int weeks = progress_weeks(level + 1, rule_random_progress());
    skill_set(sv, level, weeks);
}

void increase_skill(unit * u, enum skill_t sk, unsigned int weeks)
{
    skill *sv = unit_skill(u, sk);
    if (!sv) {
        sv = add_skill(u, sk);
    }
    while (sv->weeks <= weeks) {
        weeks -= sv->weeks;
        sk_set_level(sv, sv->level + 1);
    }
    sv->weeks -= weeks;
    assert(sv->weeks <= MAX_WEEKS_TO_NEXT_LEVEL(sv->level));
}

void reduce_skill(unit * u, skill * sv, unsigned int weeks)
{
    unsigned int max_weeks = MAX_WEEKS_TO_NEXT_LEVEL(sv->level);

    sv->weeks += weeks;
    while (sv->level > 0 && sv->weeks > max_weeks) {
        sv->weeks -= sv->level;
        --sv->level;
        max_weeks -= 2;
    }
    if (sv->level == 0) {
        /* reroll */
        sk_set_level(sv, sv->level + 1);
    }
    assert(sv->weeks <= MAX_WEEKS_TO_NEXT_LEVEL(sv->level));
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

static int weeks_from_level(int level)
{
    return level * (level + 1) / 2;
}

static int level_from_weeks(int weeks, int n)
{
    return (int)(sqrt(1.0 + (weeks * 8.0 / n)) - 1) / 2;
}

static int weeks_studied(const skill* sv)
{
    int expect = progress_weeks(sv->level + 1, false);
    return expect - sv->weeks;
}

int merge_skill(const skill* sv, const skill* sn, skill* result, int n, int add)
{
    /* number of person-weeks the units have studied: */
    int total = add + n;
    int weeks = (sn ? weeks_from_level(sn->level) : 0) * n
        + (sv ? weeks_from_level(sv->level) : 0) * add;
    /* level that combined unit should be at: */
    int level = level_from_weeks(weeks, total);

    result->level = level;
    /* how long it should take to the next level: */
    result->weeks = progress_weeks(level + 1, false);

    /* see if we have any remaining weeks: */
    weeks -= weeks_from_level(level) * total;
    /* adjusted by how much we already studied: */
    if (sv) weeks += weeks_studied(sv) * add;
    if (sn) weeks += weeks_studied(sn) * n;
    if (weeks / total) {
        weeks = result->weeks - weeks / total;
        while (weeks < 0) {
            ++result->level;
            weeks += progress_weeks(result->level, false);
        }
        result->weeks = weeks;
    }
    return result->level;
}
