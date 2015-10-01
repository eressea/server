#pragma once

#ifndef H_SKILL_H
#define H_SKILL_H

#include <platform.h>
struct locale;

typedef enum {
    SK_ALCHEMY,
    SK_CROSSBOW,
    SK_MINING,
    SK_LONGBOW,
    SK_BUILDING,
    SK_TRADE,
    SK_LUMBERJACK,
    SK_CATAPULT,
    SK_HERBALISM,
    SK_MAGIC,
    SK_HORSE_TRAINING,            /* 10 */
    SK_RIDING,
    SK_ARMORER,
    SK_SHIPBUILDING,
    SK_MELEE,
    SK_SAILING,
    SK_SPEAR,
    SK_SPY,
    SK_QUARRYING,
    SK_ROAD_BUILDING,
    SK_TACTICS,                   /* 20 */
    SK_STEALTH,
    SK_ENTERTAINMENT,
    SK_WEAPONSMITH,
    SK_CARTMAKER,
    SK_PERCEPTION,
    SK_TAXING,
    SK_STAMINA,
    SK_WEAPONLESS,
    MAXSKILLS,
    NOSKILL = -1
} skill_t;

extern const char *skillnames[];

skill_t get_skill(const char *s, const struct locale *lang);
const char *skillname(skill_t, const struct locale *);
skill_t findskill(const char *name);
void init_skills(const struct locale *lang);
void init_skill(const struct locale *lang, skill_t kwd, const char *str);
void enable_skill(skill_t sk, bool enabled);
bool skill_enabled(skill_t sk);

#endif
