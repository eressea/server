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
        rule = config_get_int("study.random_progress", 1);
    }
    return rule != 0;
}

static int progress_weeks(unsigned int level)
/* how many weeks must i study to get from level-1 to level */
{
    unsigned int coins = MAX_WEEKS_TO_NEXT_LEVEL(level - 1) - 1;
    int heads = 1;
    while (coins--) {
        heads += rng_int() % 2;
    }
    return heads;
}

static void skill_set(skill *sv, unsigned int level, unsigned int days)
{
    assert(days <= MAX_DAYS_TO_NEXT_LEVEL(level));
    sv->level = level;
    sv->days = days;
    assert(sv->days == days && sv->level == level);
}

void sk_set_level(skill *sv, unsigned int level)
{
    int weeks = rule_random_progress() ? progress_weeks(level + 1) : (level + 1);
    skill_set(sv, level, weeks * SKILL_DAYS_PER_WEEK);
}

void increase_skill_weeks(unit * u, enum skill_t sk, const unsigned int weeks)
{
    skill *sv = unit_skill(u, sk);
    unsigned int days = weeks * SKILL_DAYS_PER_WEEK;
    if (!sv) {
        sv = add_skill(u, sk);
    }
    while (sv->days <= days) {
        days -= sv->days;
        sk_set_level(sv, sv->level + 1);
    }
    sv->days -= days;
    assert(sv->days <= MAX_DAYS_TO_NEXT_LEVEL(sv->level));
}

void reduce_skill_weeks(unit * u, skill * sv, const unsigned int weeks)
{
    unsigned int days = weeks * SKILL_DAYS_PER_WEEK;
    unsigned int max_days = MAX_DAYS_TO_NEXT_LEVEL(sv->level);

    sv->days += days;
    while (sv->level > 0 && sv->days > max_days) {
        sv->days -= sv->level * SKILL_DAYS_PER_WEEK;
        --sv->level;
        max_days -= 2 * SKILL_DAYS_PER_WEEK;
    }
    if (sv->level == 0) {
        /* reroll */
        sk_set_level(sv, sv->level + 1);
    }
    assert(sv->days <= MAX_DAYS_TO_NEXT_LEVEL(sv->level));
}

int skill_compare(const skill * sk, const skill * sc)
{
    if (sk->level > sc->level)
        return 1;
    if (sk->level < sc->level)
        return -1;
    if (sk->days < sc->days)
        return 1;
    if (sk->days > sc->days)
        return -1;
    return 0;
}

static int weeks_from_level(int level)
{
    return level * (level + 1) / 2;
}

static int level_from_weeks(int weeks, int n)
{
    /*
    * Wieso funktioniert diese Formel?
    * Discord User @djannan:
    * https://proofwiki.org/wiki/Difference_between_Odd_Squares_is_Divisible_by_8
    * Das hängt mit einer witzigen Eigenschaft der Quadtratzahlen zusammen.
    * Die Differenz der Quadrate zweier aufeinander folgenden ungerader
    * Zahlen ist durch 8 teilbar. Den Umstand macht sich die Formel zu nutze,
    * um ein lineares Wachstum an eine quadratisch steigende Bedingung zu 
    * knüpfen.
    * Um aus jeder ungeraden Zahl jede Zahl zu gewinnen kommt die
    * -1)/2 am Ende.
    */
    return (int)((sqrt(1.0 + (weeks * 8.0 / n)) - 1) / 2);
}

static int days_studied(const skill* sv)
{
    int expect = SKILL_DAYS_PER_WEEK * (sv->level + 1);
    return expect - (int)sv->days;
}

/**
  * `n` persons with skill `sn` are merged into `add` persons with skill `sv`
  */
int merge_skill(const skill* sv, const skill* sn, skill* result, int n, int add)
{
    int total = add + n;
    /* number of person-weeks the units have already studied: */
    int days, weeks = weeks_from_level(sn ? sn->level : 0) * n
        + weeks_from_level(sv ? sv->level : 0) * add;
    /* level that combined unit should be at: */
    int level = level_from_weeks(weeks, total);

    /* minimum new level, based only on weeks spent towards current levels: */
    result->level = level;
    /* average time to the next level: */
    result->days = (level + 1) * SKILL_DAYS_PER_WEEK;

    /* add any remaining weeks to the already studied ones and convert to days: */
    weeks -= weeks_from_level(level) * total;
    days = weeks * SKILL_DAYS_PER_WEEK;

    /* adjusted by how much we already studied: */
    if (sv) days += days_studied(sv) * add;
    if (sn) days += days_studied(sn) * n;
    if (days < 0) {
        result->days -= days / total;
    }
    else if (days >= total) {
        int pdays = total * result->days - days;
        while (pdays < 0) {
            ++result->level;
            pdays += total * (result->level + 1) * SKILL_DAYS_PER_WEEK;
        }
        result->days = (pdays + total - 1 ) / total;
    }
    return result->level;
}

int skill_level(unit *u, enum skill_t sk)
{
    const skill *sv = unit_skill(u, sk);
    return sv ? sv->level : 0;
}

int skill_days(unit *u, enum skill_t sk)
{
    const skill *sv = unit_skill(u, sk);
    return sv ? sv->days : 1;
}
