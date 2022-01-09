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

static bool rule_random_progress(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("study.random_progress", 1);
    }
    return rule != 0;
}

static int progress_weeks(int level, bool random_progress)
/* how many weeks must i study to get from level-1 to level */
{
    if (random_progress) {
        int coins = 2 * level;
        int heads = 1;
        while (coins--) {
            heads += rng_int() % 2;
        }
        return heads;
    }
    return level;
}

void sk_set(skill* sv, int level)
{
    assert(sv && level != 0);
    sv->weeks = progress_weeks(level + 1, rule_random_progress());
    sv->level = level;
    assert(sv->weeks <= sv->level * 2 + 3);
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
    assert(sv->weeks <= sv->level * 2 + 3);
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
        sv->weeks = progress_weeks(sv->level + 1, rule_random_progress());
    }
    assert(sv->weeks <= sv->level * 2 + 3);
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

#if 1
static int weeks_from_level(int level)
{
    return level * (level + 1) / 2;
}

static int weeks_from_skill(const skill* sv)
{
    if (sv) {
        return weeks_from_level(sv->level + 1) - sv->weeks;
    }
    return 0;
}

static int level_from_weeks(int weeks, int n)
{
    return (int)(sqrt(1.0 + (weeks * 8.0 / n)) - 1) / 2;
}

static int min_level(const skill* sa, const skill* sb)
{
    int a = sa ? sa->level : 0;
    int b = sb ? sb->level : 0;
    return (a < b) ? a : b;
}

int merge_skill(const skill* sv, const skill* sn, skill* result, int n, int add)
{
    /* number of person-weeks the units have studied: */
    int weeks = (sn ? weeks_from_level(sn->level) : 0) * n
        + (sv ? weeks_from_level(sv->level) : 0) * add;
    /* level that combined unit should be at: */
    int level = level_from_weeks(weeks, n + add);

    result->level = level;
    /* how long it should take to the next level: */
    result->weeks = progress_weeks(level + 1, false);
    /* adjusted by how much we already studied: */
    // result->weeks -= weeks / (n + add);
    return level;
}
#else
int merge_skill(const skill* sv, const skill* sn, skill* result, int n, int add)
{
    int weeks = sv ? sv->weeks : 0, level = sv ? sv->level : 0;
    assert(result);
    if (sn != NULL && n > 0) {
        double dlevel = sv ? ((level + 1.0 - weeks / (level + 1.0)) * add) : 0.0;

        level *= add;
        if (sn && sn->level) {
            dlevel +=
                (sn->level + 1.0 - sn->weeks / (sn->level + 1.0)) * n;
            level += sn->level * n;
        }

        dlevel /= ((double)add + n);
        level /= (add + n);
        if (level <= dlevel) {
            /* apply the remaining fraction to the number of weeks to go.
             * subtract the according number of weeks, getting closer to the
             * next level */
            level = (int)dlevel;
            weeks = (level + 1) - (int)((dlevel - level) * (level + 1.0));
        }
        else {
            /* make it harder to reach the next level.
             * weeks+level is the max difficulty, 1 - the fraction between
             * level and dlevel applied to the number of weeks between this
             * and the previous level is the added difficutly */
            level = (int)dlevel + 1;
            weeks = 1 + 2 * level - (int)((1 + dlevel - level) * level);
        }
    }
    if (level) {
        assert(weeks > 0 && weeks <= level * 2 + 1);
        assert(n != 0 || (sv && level == sv->level
            && weeks == sv->weeks));
        result->level = level;
        result->weeks = weeks;
    }
    return level;
}
#endif
