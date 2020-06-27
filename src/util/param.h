#pragma once

#ifndef H_PARAM_H
#define H_PARAM_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct locale;

    typedef enum param_t {
        P_LOCALE,
        P_ANY,
        P_EACH,
        P_PEASANT,
        P_BUILDING,
        P_UNIT,
        P_PRIVAT,
        P_BEHIND,
        P_CONTROL,
        P_HERBS,
        P_NOT,
        P_NEXT,
        P_FACTION,
        P_GAMENAME,
        P_PERSON,
        P_REGION,
        P_SHIP,
        P_MONEY,
        P_ROAD,
        P_TEMP,
        P_FLEE,
        P_GEBAEUDE,
        P_GIVE,
        P_FIGHT,
        P_TRAVEL,
        P_GUARD,
        P_ZAUBER,
        P_PAUSE,
        P_VORNE,
        P_AGGRO,
        P_CHICKEN,
        P_LEVEL,
        P_HELP,
        P_FOREIGN,
        P_AURA,
        P_AFTER,
        P_BEFORE,
        P_NUMBER,
        P_ITEMS,
        P_POTIONS,
        P_GROUP,
        P_FACTIONSTEALTH,
        P_TREES,
        P_ALLIANCE,
        P_AUTO,
        MAXPARAMS,
        NOPARAM
    } param_t;

    extern const char *parameters[MAXPARAMS];

    param_t findparam(const char *s, const struct locale *lang);
    param_t findparam_block(const char *s, const struct locale *lang, bool any_locale);
    bool isparam(const char *s, const struct locale * lang, param_t param);
    param_t getparam(const struct locale *lang);

    void init_parameters(struct locale *lang);

#ifdef __cplusplus
}
#endif
#endif
