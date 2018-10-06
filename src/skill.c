#include <platform.h>
#include <kernel/config.h>
#include "skill.h"

#include <util/umlaut.h>
#include <util/language.h>
#include <util/log.h>
#include <util/param.h>

#include <critbit.h>

#include <string.h>
#include <assert.h>

const char *skillnames[MAXSKILLS] = {
    "alchemy",
    "crossbow",
    "mining",
    "bow",
    "building",
    "trade",
    "forestry",
    "catapult",
    "herbalism",
    "magic",
    "training",
    "riding",
    "armorer",
    "shipcraft",
    "melee",
    "sailing",
    "polearm",
    "espionage",
    "quarrying",
    "roadwork",
    "tactics",
    "stealth",
    "entertainment",
    "weaponsmithing",
    "cartmaking",
    "perception",
    "taxation",
    "stamina",
    "unarmed"
};

bool skill_disabled[MAXSKILLS];

bool skill_enabled(skill_t sk) {
    assert(sk != NOSKILL);
    return !skill_disabled[sk];
}

static const char * skill_key(int sk) {
    assert(sk < MAXPARAMS && sk >= 0);
    return skill_disabled[sk] ? 0 : mkname("skill", skillnames[sk]);
}

void init_skill(const struct locale *lang, skill_t kwd, const char *str) {
    void **tokens = get_translations(lang, UT_SKILLS);
    struct critbit_tree **cb = (critbit_tree **)tokens;
    add_translation(cb, str, (int)kwd);
}

void init_skills(const struct locale *lang) {
    init_translations(lang, UT_SKILLS, skill_key, MAXSKILLS);
}

const char *skillname(skill_t sk, const struct locale *lang)
{
    if (skill_disabled[sk]) return 0;
    return LOC(lang, mkname("skill", skillnames[sk]));

}

void enable_skill(skill_t sk, bool value)
{
    skill_disabled[sk] = !value;
}

skill_t findskill(const char *name)
{
    skill_t i;
    if (name == NULL) return NOSKILL;
    if (strncmp(name, "sk_", 3) == 0) name += 3;
    for (i = 0; i != MAXSKILLS; ++i) {
        if (!skill_disabled[i] && strcmp(name, skillnames[i]) == 0) {
            return i;
        }
    }
    return NOSKILL;
}

skill_t get_skill(const char *s, const struct locale * lang)
{
    skill_t result = NOSKILL;
    char buffer[64];

    if (s) {
        char * str = transliterate(buffer, sizeof(buffer) - sizeof(int), s);
        if (str) {
            int i;
            void * match;
            void **tokens = get_translations(lang, UT_SKILLS);
            struct critbit_tree *cb = (critbit_tree *)*tokens;
            if (cb && cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
                cb_get_kv(match, &i, sizeof(int));
                result = (skill_t)i;
            }
        }
        else {
            log_warning("could not transliterate skill: %s", s);
        }
    }
    return result;
}

int skill_cost(skill_t sk) {
    static int config;
    static int costs[MAXSKILLS];
    int cost;
    switch (sk) {
    case SK_SPY:
        cost = 100;
        break;
    case SK_TACTICS:
    case SK_HERBALISM:
    case SK_ALCHEMY:
        cost = 200;
        break;
    default:
        cost = -1;
    }

    if (config_changed(&config)) {
        memset(costs, 0, sizeof(costs));
    }

    if (costs[sk] == 0) {
        char buffer[256];
        sprintf(buffer, "skills.cost.%s", skillnames[sk]);
        costs[sk] = config_get_int(buffer, cost);
    }
    if (costs[sk] >= 0) {
        return costs[sk];
    }
    return (cost > 0) ? cost : 0;
}

bool expensive_skill(skill_t sk) {
    return (sk == SK_MAGIC) || skill_cost(sk) > 0;
}
