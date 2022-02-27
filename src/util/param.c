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
    char * str = s ? transliterate(buffer, sizeof(buffer) - sizeof(int), s) : NULL;

    if (str && str[0] && str[1]) {
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
    assert(param != P_GEBAEUDE);
    assert(param != P_BUILDING);
    if (s && s[0] > '@') {
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

typedef struct param {
    critbit_tree cb;
} param;

size_t pack_keyval(const char* key, const char* value, char* data, size_t len) {
    size_t klen = strlen(key);
    size_t vlen = strlen(value);
    assert(klen + vlen + 2 + sizeof(vlen) <= len);
    memcpy(data, key, klen + 1);
    memcpy(data + klen + 1, value, vlen + 1);
    return klen + vlen + 2;
}

void set_param(struct param** p, const char* key, const char* value)
{
    struct param* par;
    assert(p);

    par = *p;
    if (!par && value) {
        *p = par = calloc(1, sizeof(param));
        if (!par) abort();
    }
    if (par) {
        void* match;
        size_t klen = strlen(key) + 1;
        if (cb_find_prefix(&par->cb, key, klen, &match, 1, 0) > 0) {
            const char* kv = (const char*)match;
            size_t vlen = strlen(kv + klen) + 1;
            cb_erase(&par->cb, kv, klen + vlen);
        }
    }
    if (value) {
        char data[512];
        size_t sz = pack_keyval(key, value, data, sizeof(data));
        cb_insert(&par->cb, data, sz);
    }
}

void free_params(struct param** pp) {
    param* p = *pp;
    if (p) {
        cb_clear(&p->cb);
        free(p);
    }
    *pp = 0;
}

const char* get_param(const struct param* p, const char* key)
{
    void* match;
    if (p && cb_find_prefix(&p->cb, key, strlen(key) + 1, &match, 1, 0) > 0) {
        cb_get_kv_ex(match, &match);
        return (const char*)match;
    }
    return NULL;
}

int get_param_int(const struct param* p, const char* key, int def)
{
    const char* str = get_param(p, key);
    return str ? atoi(str) : def;
}

int check_param(const struct param* p, const char* key, const char* searchvalue)
{
    int result = 0;
    const char* value = get_param(p, key);
    char* v, * p_value;
    if (!value) {
        return 0;
    }
    p_value = str_strdup(value);
    v = strtok(p_value, " ,;");

    while (v != NULL) {
        if (strcmp(v, searchvalue) == 0) {
            result = 1;
            break;
        }
        v = strtok(NULL, " ,;");
    }
    free(p_value);
    return result;
}
