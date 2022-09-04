#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "language.h"
#include "log.h"
#include "param.h"
#include "parser.h"
#include "umlaut.h"

#include <critbit.h>
#include <strings.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

const char *parameters[MAXPARAMS] = {
    "LOCALE",
    "ALLES",
    "JE",
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
    "NUMMER",
    "GEGENSTAENDE",
    "TRAENKE",
    "GRUPPE",
    "PARTEITARNUNG",
    "BAEUME",
    "ALLIANZ",
    "AUTO"
};

param_t findparam(const char* s)
{
    int i;
    for (i = 0; i != MAXPARAMS; ++i) {
        if (parameters[i] && (strcmp(s, parameters[i]) == 0)) {
            return (param_t)i;
        }
    }
    return NOPARAM;
}

param_t get_param(const char *s, const struct locale * lang)
{
    param_t result = NOPARAM;
    char buffer[64];
    char * str = s ? transliterate(buffer, sizeof(buffer) - sizeof(int), s) : NULL;

    if (str && str[0] && str[1]) {
        void * match;
        void **tokens = get_translations(lang, UT_PARAMS);
        critbit_tree *cb = (critbit_tree *)*tokens;
        if (!cb) {
            log_warning("no parameters defined in locale %s", locale_name(lang));
        }
        else if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
            const char* key = (const char*)match;
            if (str[2] || strlen(key) < 3) {
                int i;
                cb_get_kv(match, &i, sizeof(i));
                result = (param_t)i;
            }
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
    p = get_param(s, lang);
    if (any_locale && p == NOPARAM) {
        const struct locale *loc;
        for (loc = locales; loc; loc = nextlocale(loc)) {
            if (loc != lang) {
                p = get_param(s, loc);
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
    assert(param != P_GEBAEUDE);
    assert(param != P_BUILDING);
    if (s && s[0] > '@') {
        param_t p = get_param(s, lang);
        return p == param;
    }
    return false;
}

param_t getparam(const struct locale * lang)
{
    char token[64];
    const char *s = gettoken(token, sizeof(token));
    return s ? get_param(s, lang) : NOPARAM;
}

static const char * parameter_key(int i)
{
    assert(i < MAXPARAMS && i >= 0);
    return parameters[i];
}

void init_parameter(const struct locale* lang, param_t p, const char* str) {
    void** tokens = get_translations(lang, UT_PARAMS);
    struct critbit_tree** cb = (critbit_tree**)tokens;
    add_translation(cb, str, (int)p);
}

void init_parameters(const struct locale *lang) {
    init_translations(lang, UT_PARAMS, parameter_key, MAXPARAMS);
}
