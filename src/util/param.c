#ifdef _MSC_VER
#include <platform.h>
#endif

#include "language.h"
#include "log.h"
#include "param.h"
#include "parser.h"
#include "umlaut.h"

#include <critbit.h>

#include <assert.h>
#include <string.h>

const char *parameters[MAXPARAMS] = {
    "LOCALE",
    "ALLES",
    "JEDEM",
    "BAUERN",
    "BURG",
    "EINHEIT",
    "PRIVAT",
    "HINTEN",
    "KOMMANDO",
    "KRAEUTER",
    "NICHT",
    "NAECHSTER",
    "PARTEI",
    "ERESSEA",
    "PERSONEN",
    "REGION",
    "SCHIFF",
    "SILBER",
    "STRASSEN",
    "TEMP",
    "FLIEHE",
    "GEBAEUDE",
    "GIB",                        /* HELFE GIB */
    "KAEMPFE",
    "DURCHREISE",
    "BEWACHE",
    "ZAUBER",
    "PAUSE",
    "VORNE",
    "AGGRESSIV",
    "DEFENSIV",
    "STUFE",
    "HELFE",
    "FREMDES",
    "AURA",
    "HINTER",
    "VOR",
    "ANZAHL",
    "GEGENSTAENDE",
    "TRAENKE",
    "GRUPPE",
    "PARTEITARNUNG",
    "BAEUME",
    "ALLIANZ",
    "AUTO"
};

param_t findparam(const char *s, const struct locale * lang)
{
    param_t result = NOPARAM;
    char buffer[64];
    char * str = s ? transliterate(buffer, sizeof(buffer) - sizeof(int), s) : 0;

    if (str && *str) {
        int i;
        void * match;
        void **tokens = get_translations(lang, UT_PARAMS);
        critbit_tree *cb = (critbit_tree *)*tokens;
        if (!cb) {
            log_warning("no parameters defined in locale %s", locale_name(lang));
        }
        else if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
            cb_get_kv(match, &i, sizeof(int));
            result = (param_t)i;
        }
    }
    return result;
}

param_t findparam_block(const char *s, const struct locale *lang, bool any_locale)
{
    param_t p;
    if (!s || s[0] == '@') {
        return NOPARAM;
    }
    p = findparam(s, lang);
    if (any_locale && p == NOPARAM) {
        const struct locale *loc;
        for (loc = locales; loc; loc = nextlocale(loc)) {
            if (loc != lang) {
                p = findparam(s, loc);
                if (p == P_FACTION || p == P_GAMENAME) {
                    break;
                }
            }
        }
    }
    return p;
}

bool isparam(const char *s, const struct locale * lang, param_t param)
{
    assert(s);
    assert(param != P_GEBAEUDE);
    assert(param != P_BUILDING);
    if (s[0] > '@') {
        param_t p = findparam(s, lang);
        return p == param;
    }
    return false;
}

param_t getparam(const struct locale * lang)
{
    char token[64];
    const char *s = gettoken(token, sizeof(token));
    return s ? findparam(s, lang) : NOPARAM;
}

static const char * parameter_key(int i)
{
    assert(i < MAXPARAMS && i >= 0);
    return parameters[i];
}

void init_parameters(struct locale *lang) {
    init_translations(lang, UT_PARAMS, parameter_key, MAXPARAMS);
}
