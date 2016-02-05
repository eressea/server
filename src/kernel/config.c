/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>

/* attributes includes */
#include <attributes/reduceproduction.h>

/* kernel includes */
#include "alliance.h"
#include "ally.h"
#include "alchemy.h"
#include "curse.h"
#include "connection.h"
#include "building.h"
#include "calendar.h"
#include "direction.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "keyword.h"
#include "messages.h"
#include "move.h"
#include "objtypes.h"
#include "order.h"
#include "plane.h"
#include "pool.h"
#include "race.h"
#include "reports.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "types.h"
#include "unit.h"


#include <kernel/spell.h>
#include <kernel/spellbook.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/crmessage.h>
#include <util/event.h>
#include <util/language.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/lists.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/translation.h>
#include <util/unicode.h>
#include <util/umlaut.h>
#include <util/xml.h>

#include "donations.h"
#include "guard.h"
#include "prefix.h"

#ifdef USE_LIBXML2
/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#endif

/* external libraries */
#include <iniparser.h>
#include <critbit.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <errno.h>

struct settings global = {
    "Eressea",                    /* gamename */
};

bool lomem = false;
FILE *logfile;
bool battledebug = false;
int turn = -1;

int NewbieImmunity(void)
{
    return config_get_int("NewbieImmunity", 0);
}

bool IsImmune(const faction * f)
{
    return !fval(f, FFL_NPC) && f->age < NewbieImmunity();
}

bool ExpensiveMigrants(void)
{
    return config_get_int("study.expensivemigrants", 0) != 0;
}

int LongHunger(const struct unit *u)
{
    if (u != NULL) {
        if (!fval(u, UFL_HUNGER))
            return false;
        if (u_race(u) == get_race(RC_DAEMON))
            return false;
    }
    return config_get_int("hunger.long", 0);
}

int NMRTimeout(void)
{
    return config_get_int("nmr.timeout", 0);
}

race_t old_race(const struct race * rc)
{
    race_t i;
    for (i = 0; i != MAXRACES; ++i) {
        if (get_race(i) == rc)  return i;
    }
    return NORACE;
}

helpmode helpmodes[] = {
    { "all", HELP_ALL }
    ,
    { "money", HELP_MONEY }
    ,
    { "fight", HELP_FIGHT }
    ,
    { "observe", HELP_OBSERVE }
    ,
    { "give", HELP_GIVE }
    ,
    { "guard", HELP_GUARD }
    ,
    { "stealth", HELP_FSTEALTH }
    ,
    { "travel", HELP_TRAVEL }
    ,
    { NULL, 0 }
};

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
    "GIB",                        /* Für HELFE */
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
    "ALLIANZ"
};

const char *report_options[MAX_MSG] = {
    "Kampf",
    "Ereignisse",
    "Bewegung",
    "Einkommen",
    "Handel",
    "Produktion",
    "Orkvermehrung",
    "Zauber",
    "",
    ""
};

const char *message_levels[ML_MAX] = {
    "Wichtig",
    "Debug",
    "Fehler",
    "Warnungen",
    "Infos"
};

const char *options[MAXOPTIONS] = {
    "AUSWERTUNG",
    "COMPUTER",
    "ZUGVORLAGE",
    NULL,
    "STATISTIK",
    "DEBUG",
    "ZIPPED",
    "ZEITUNG",                    /* Option hat Sonderbehandlung! */
    NULL,
    "ADRESSEN",
    "BZIP2",
    "PUNKTE",
    "SHOWSKCHANGE"
};

FILE *debug;

void
parse(keyword_t kword, int(*dofun) (unit *, struct order *), bool thisorder)
{
    region *r;

    for (r = regions; r; r = r->next) {
        unit **up = &r->units;
        while (*up) {
            unit *u = *up;
            order **ordp = &u->orders;
            if (thisorder)
                ordp = &u->thisorder;
            while (*ordp) {
                order *ord = *ordp;
                if (getkeyword(ord) == kword) {
                    if (dofun(u, ord) != 0)
                        break;
                    if (u->orders == NULL)
                        break;
                }
                if (thisorder)
                    break;
                if (*ordp == ord)
                    ordp = &ord->next;
            }
            if (*up == u)
                up = &u->next;
        }
    }
}

const struct race *findrace(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_RACES);
    variant token;

    assert(lang);
    if (tokens && findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (const struct race *)token.v;
    }
    return NULL;
}

int findoption(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_OPTIONS);
    variant token;

    if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (direction_t)token.i;
    }
    return NODIRECTION;
}

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
            log_error("no parameters defined in locale %s", locale_name(lang));
        }
        else if (cb_find_prefix(cb, str, strlen(str), &match, 1, 0)) {
            cb_get_kv(match, &i, sizeof(int));
            result = (param_t)i;
        }
    }
    return result;
}

param_t findparam_ex(const char *s, const struct locale * lang)
{
    param_t result = findparam(s, lang);

    if (result == NOPARAM) {
        const building_type *btype = findbuildingtype(s, lang);
        if (btype != NULL)
            return P_GEBAEUDE;
    }
    return (result == P_BUILDING) ? P_GEBAEUDE : result;
}

bool isparam(const char *s, const struct locale * lang, param_t param)
{
    assert(s);
    if (s[0] > '@') {
        param_t p = (param == P_GEBAEUDE) ? findparam_ex(s, lang) : findparam(s, lang);
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

unit *getnewunit(const region * r, const faction * f)
{
    int n;
    n = getid();

    return findnewunit(r, f, n);
}

/* - Namen der Strukturen -------------------------------------- */
char *untilde(char *ibuf)
{
    char *p = ibuf;

    while (*p) {
        if (*p == '~') {
            *p = ' ';
        }
        ++p;
    }
    return ibuf;
}

building *largestbuilding(const region * r, cmp_building_cb cmp_gt,
    bool imaginary)
{
    building *b, *best = NULL;

    for (b = rbuildings(r); b; b = b->next) {
        if (cmp_gt(b, best) <= 0)
            continue;
        if (!imaginary) {
            const attrib *a = a_find(b->attribs, &at_icastle);
            if (a)
                continue;
        }
        best = b;
    }
    return best;
}

/* -- Erschaffung neuer Einheiten ------------------------------ */

static const char *forbidden[] = { "t", "te", "tem", "temp", NULL };
// PEASANT: "b", "ba", "bau", "baue", "p", "pe", "pea", "peas"

int forbiddenid(int id)
{
    static int *forbid = NULL;
    static size_t len;
    size_t i;
    if (id <= 0)
        return 1;
    if (!forbid) {
        while (forbidden[len])
            ++len;
        forbid = calloc(len, sizeof(int));
        for (i = 0; i != len; ++i) {
            forbid[i] = atoi36(forbidden[i]);
        }
    }
    for (i = 0; i != len; ++i)
        if (id == forbid[i])
            return 1;
    return 0;
}

int newcontainerid(void)
{
    int random_no;
    int start_random_no;

    random_no = 1 + (rng_int() % MAX_CONTAINER_NR);
    start_random_no = random_no;

    while (findship(random_no) || findbuilding(random_no)) {
        random_no++;
        if (random_no == MAX_CONTAINER_NR + 1) {
            random_no = 1;
        }
        if (random_no == start_random_no) {
            random_no = (int)MAX_CONTAINER_NR + 1;
        }
    }
    return random_no;
}

int maxworkingpeasants(const struct region *r)
{
    int size = production(r);
    int treespace = (rtrees(r, 2) + rtrees(r, 1) / 2) * TREESIZE;
    return _max(size - treespace, _min(size / 10, 200));
}

static const char * parameter_key(int i)
{
    assert(i < MAXPARAMS && i >= 0);
    return parameters[i];
}

void init_parameters(struct locale *lang) {
    init_translations(lang, UT_PARAMS, parameter_key, MAXPARAMS);
}


void init_terrains_translation(const struct locale *lang) {
    void **tokens;
    const terrain_type *terrain;

    tokens = get_translations(lang, UT_TERRAINS);
    for (terrain = terrains(); terrain != NULL; terrain = terrain->next) {
        variant var;
        const char *name;
        var.v = (void *)terrain;
        name = locale_string(lang, terrain->_name, false);
        if (name) {
            addtoken(tokens, name, var);
        }
        else {
            log_debug("no translation for terrain %s in locale %s", terrain->_name, locale_name(lang));
        }
    }
}

void init_options_translation(const struct locale * lang) {
    void **tokens;
    int i;

    tokens = get_translations(lang, UT_OPTIONS);
    for (i = 0; i != MAXOPTIONS; ++i) {
        variant var;
        var.i = i;
        if (options[i]) {
            const char *name = locale_string(lang, options[i], false);
            if (name) {
                addtoken(tokens, name, var);
            }
            else {
                log_debug("no translation for OPTION %s in locale %s", options[i], locale_name(lang));
            }
        }
    }
}

void init_locale(struct locale *lang)
{
    variant var;
    int i;
    const struct race *rc;
    void **tokens;

    tokens = get_translations(lang, UT_MAGIC);
    if (tokens) {
        const char *str = config_get("rules.magic.playerschools");
        char *sstr, *tok;
        if (str == NULL) {
            str = "gwyrrd illaun draig cerddor tybied";
        }

        sstr = _strdup(str);
        tok = strtok(sstr, " ");
        while (tok) {
            const char *name;
            for (i = 0; i != MAXMAGIETYP; ++i) {
                if (strcmp(tok, magic_school[i]) == 0) break;
            }
            assert(i != MAXMAGIETYP);
            var.i = i;
            name = LOC(lang, mkname("school", tok));
            if (name) {
                addtoken(tokens, name, var);
            }
            else {
                log_warning("no translation for magic school %s in locale %s", tok, locale_name(lang));
            }
            tok = strtok(NULL, " ");
        }
        free(sstr);
    }

    init_directions(lang);
    init_keywords(lang);
    init_skills(lang);

    tokens = get_translations(lang, UT_RACES);
    for (rc = races; rc; rc = rc->next) {
        const char *name;
        var.v = (void *)rc;
        name = locale_string(lang, rc_name_s(rc, NAME_PLURAL), false);
        if (name) addtoken(tokens, name, var);
        name = locale_string(lang, rc_name_s(rc, NAME_SINGULAR), false);
        if (name) addtoken(tokens, name, var);
    }

    init_parameters(lang);

    init_options_translation(lang);
    init_terrains_translation(lang);
}

typedef struct param {
    critbit_tree cb;
} param;

size_t pack_keyval(const char *key, const char *value, char *data, size_t len) {
    size_t klen = strlen(key);
    size_t vlen = strlen(value);
    assert(klen + vlen + 2 + sizeof(vlen) <= len);
    memcpy(data, key, klen + 1);
    memcpy(data + klen + 1, value, vlen + 1);
    return klen + vlen + 2;
}

void set_param(struct param **p, const char *key, const char *value)
{
    struct param *par;
    assert(p);

    par = *p;
    if (!par && value) {
        *p = par = calloc(1, sizeof(param));
    }
    if (par) {
        void *match;
        size_t klen = strlen(key) + 1;
        if (cb_find_prefix(&par->cb, key, klen, &match, 1, 0) > 0) {
            const char * kv = (const char *)match;
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

void free_params(struct param **pp) {
    param *p = *pp;
    if (p) {
        cb_clear(&p->cb);
        free(p);
    }
    *pp = 0;
}

const char *get_param(const struct param *p, const char *key)
{
    void *match;
    if (p && cb_find_prefix(&p->cb, key, strlen(key) + 1, &match, 1, 0) > 0) {
        cb_get_kv_ex(match, &match);
        return (const char *)match;
    }
    return NULL;
}

int get_param_int(const struct param *p, const char *key, int def)
{
    const char * str = get_param(p, key);
    return str ? atoi(str) : def;
}

int check_param(const struct param *p, const char *key, const char *searchvalue)
{
    int result = 0;
    const char *value = get_param(p, key);
    if (!value) {
        return 0;
    }
    char *p_value = _strdup(value);
    const char *delimiter = " ,;";
    char *v = strtok(p_value, delimiter);

    while (v != NULL) {
        if (strcmp(v, searchvalue) == 0) {
            result = 1;
            break;
        }
        v = strtok(NULL, delimiter);
    }
    free(p_value);
    return result;
}

static const char *g_basedir;
const char *basepath(void)
{
    if (g_basedir)
        return g_basedir;
    return ".";
}

void set_basepath(const char *path)
{
    g_basedir = path;
}

#ifdef WIN32
#define PATH_DELIM '\\'
#else
#define PATH_DELIM '/'
#endif

char * join_path(const char *p1, const char *p2, char *dst, size_t len) {
    size_t sz;
    assert(p1 && p2);
    assert(p2 != dst);
    if (dst == p1) {
        sz = strlen(p1);
    }
    else {
        sz = strlcpy(dst, p1, len);
    }
    assert(sz < len);
    dst[sz++] = PATH_DELIM;
    strlcpy(dst + sz, p2, len - sz);
    return dst;
}

static const char * relpath(char *buf, size_t sz, const char *path) {
    if (g_basedir) {
        join_path(g_basedir, path, buf, sz);
    }
    else {
        strlcpy(buf, path, sz);
    }
    return buf;
}

static const char *g_datadir;
const char *datapath(void)
{
    static char zText[MAX_PATH]; // FIXME: static return value
    if (g_datadir)
        return g_datadir;
    return relpath(zText, sizeof(zText), "data");
}

void set_datapath(const char *path)
{
    g_datadir = path;
}

static const char *g_reportdir;
const char *reportpath(void)
{
    static char zText[MAX_PATH]; // FIXME: static return value
    if (g_reportdir)
        return g_reportdir;
    return relpath(zText, sizeof(zText), "reports");
}

void set_reportpath(const char *path)
{
    g_reportdir = path;
}

int create_directories(void) {
    int err;
    err = _mkdir(datapath());
    if (err) {
        if (errno == EEXIST) errno = 0;
        else return err;
    }
    err = _mkdir(reportpath());
    if (err && errno == EEXIST) {
        errno = 0;
    }
    return err;
}

double get_param_flt(const struct param *p, const char *key, double def)
{
    const char *str = get_param(p, key);
    return str ? atof(str) : def;
}

void kernel_done(void)
{
    /* calling this function releases memory assigned to static variables, etc.
     * calling it is optional, e.g. a release server will most likely not do it.
     */
    translation_done();
    free_attribs();
}

#ifndef HAVE_STRDUP
char *_strdup(const char *s)
{
    return strcpy((char *)malloc(sizeof(char) * (strlen(s) + 1)), s);
}
#endif

/* Lohn bei den einzelnen Burgstufen für Normale Typen, Orks, Bauern,
 * Modifikation für Städter. */

static const int wagetable[7][4] = {
    { 10, 10, 11, -7 },             /* Baustelle */
    { 10, 10, 11, -5 },             /* Handelsposten */
    { 11, 11, 12, -3 },             /* Befestigung */
    { 12, 11, 13, -1 },             /* Turm */
    { 13, 12, 14, 0 },              /* Burg */
    { 14, 12, 15, 1 },              /* Festung */
    { 15, 13, 16, 2 }               /* Zitadelle */
};

int cmp_wage(const struct building *b, const building * a)
{
    const struct building_type *bt_castle = bt_find("castle");
    if (b->type == bt_castle) {
        if (!a)
            return 1;
        if (b->size > a->size)
            return 1;
        if (b->size == a->size)
            return 0;
    }
    return -1;
}

bool is_owner_building(const struct building * b)
{
    region *r = b->region;
    if (b->type->taxes && r->land && r->land->ownership) {
        unit *u = building_owner(b);
        return u && u->faction == r->land->ownership->owner;
    }
    return false;
}

int cmp_taxes(const building * b, const building * a)
{
    faction *f = region_get_owner(b->region);
    if (b->type->taxes) {
        unit *u = building_owner(b);
        if (!u) {
            return -1;
        }
        else if (a) {
            int newsize = buildingeffsize(b, false);
            double newtaxes = b->type->taxes(b, newsize);
            int oldsize = buildingeffsize(a, false);
            double oldtaxes = a->type->taxes(a, oldsize);

            if (newtaxes < oldtaxes)
                return -1;
            else if (newtaxes > oldtaxes)
                return 1;
            else if (b->size < a->size)
                return -1;
            else if (b->size > a->size)
                return 1;
            else {
                if (u && u->faction == f) {
                    u = building_owner(a);
                    if (u && u->faction == f)
                        return -1;
                    return 1;
                }
            }
        }
        else {
            return 1;
        }
    }
    return -1;
}

int cmp_current_owner(const building * b, const building * a)
{
    faction *f = region_get_owner(b->region);

    assert(rule_region_owners());
    if (f && b->type->taxes) {
        unit *u = building_owner(b);
        if (!u || u->faction != f)
            return -1;
        if (a) {
            int newsize = buildingeffsize(b, false);
            double newtaxes = b->type->taxes(b, newsize);
            int oldsize = buildingeffsize(a, false);
            double oldtaxes = a->type->taxes(a, oldsize);

            if (newtaxes != oldtaxes)
                return (newtaxes > oldtaxes) ? 1 : -1;
            if (newsize != oldsize)
                return newsize - oldsize;
            return (b->size - a->size);
        }
        else {
            return 1;
        }
    }
    return -1;
}

bool rule_stealth_other(void)
{
    int rule = config_get_int("stealth.faction.other", 1);
    return rule != 0;
}

bool rule_stealth_anon(void)
{
    int rule = config_get_int("stealth.faction.anon", 1);
    return rule != 0;
}

bool rule_region_owners(void)
{
    int rule = config_get_int("rules.region_owners", 0);
    return rule != 0;
}

int rule_blessed_harvest(void)
{
    int rule = config_get_int("rules.blessed_harvest.flags",
        HARVEST_WORK);
    assert(rule >= 0);
    return rule;
}

int rule_alliance_limit(void)
{
    int rule = config_get_int("rules.limit.alliance", 0);
    assert(rule >= 0);
    return rule;
}

int rule_faction_limit(void)
{
    int rule = config_get_int("rules.limit.faction", 0);
    assert(rule >= 0);
    return rule;
}

static int
default_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    building *b = largestbuilding(r, &cmp_wage, false);
    int esize = 0;
    curse *c;
    double wage;
    attrib *a;
    const building_type *artsculpture_type = bt_find("artsculpture");
    static const curse_type *drought_ct, *blessedharvest_ct;
    static bool init;

    if (!init) {
        init = true;
        drought_ct = ct_find("drought");
        blessedharvest_ct = ct_find("blessedharvest");
    }

    if (b != NULL) {
        /* TODO: this reveals imaginary castles */
        esize = buildingeffsize(b, false);
    }

    if (f != NULL) {
        int index = 0;
        if (rc == get_race(RC_ORC) || rc == get_race(RC_SNOTLING)) {
            index = 1;
        }
        wage = wagetable[esize][index];
    }
    else {
        if (is_mourning(r, in_turn)) {
            wage = 10;
        }
        else if (fval(r->terrain, SEA_REGION)) {
            wage = 11;
        }
        else if (fval(r, RF_ORCIFIED)) {
            wage = wagetable[esize][1];
        }
        else {
            wage = wagetable[esize][2];
        }
        if (rule_blessed_harvest() == HARVEST_WORK) {
            /* E1 rules */
            wage += curse_geteffect(get_curse(r->attribs, blessedharvest_ct));
        }
    }

    /* Artsculpture: Income +5 */
    for (b = r->buildings; b; b = b->next) {
        if (b->type == artsculpture_type) {
            wage += 5;
        }
    }

    /* Godcurse: Income -10 */
    if (curse_active(get_curse(r->attribs, ct_find("godcursezone")))) {
        wage = _max(0, wage - 10);
    }

    /* Bei einer Dürre verdient man nur noch ein Viertel  */
    if (drought_ct) {
        c = get_curse(r->attribs, drought_ct);
        if (curse_active(c))
            wage /= curse_geteffect(c);
    }

    a = a_find(r->attribs, &at_reduceproduction);
    if (a)
        wage = (wage * a->data.sa[0]) / 100;

    return (int)wage;
}

static int
minimum_wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    if (f && rc) {
        return rc->maintenance;
    }
    return default_wage(r, f, rc, in_turn);
}

/* Gibt Arbeitslohn für entsprechende Rasse zurück, oder für
* die Bauern wenn f == NULL. */
int wage(const region * r, const faction * f, const race * rc, int in_turn)
{
    if (global.functions.wage) {
        return global.functions.wage(r, f, rc, in_turn);
    }
    return default_wage(r, f, rc, in_turn);
}

int lovar(double xpct_x2)
{
    int n = (int)(xpct_x2 * 500) + 1;
    if (n == 0)
        return 0;
    return (rng_int() % n + rng_int() % n) / 1000;
}

void kernel_init(void)
{
    register_reports();
    mt_clear();
    translation_init();
    register_function((pf_generic)minimum_wage, "minimum_wage");
}

static order * defaults[MAXLOCALES];
keyword_t default_keyword = NOKEYWORD;

void set_default_order(int kwd) {
    default_keyword = (keyword_t)kwd;
}

// TODO: outside of tests, default_keyword is never used, why is this here?
// see also test_long_order_hungry
order *default_order(const struct locale *lang)
{
    static int usedefault = 1;
    int i = locale_index(lang);
    order *result = 0;
    assert(i < MAXLOCALES);

    if (default_keyword != NOKEYWORD) {
        return create_order(default_keyword, lang, 0);
    }

    result = defaults[i];
    if (!result && usedefault) {
        const char * str = LOC(lang, "defaultorder");
        if (str) {
            result = defaults[i] = parse_order(str, lang);
        }
        else {
            usedefault = 0;
        }
    }
    return result ? copy_order(result) : 0;
}

int entertainmoney(const region * r)
{
    double n;

    if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
        return 0;
    }

    n = rmoney(r) / ENTERTAINFRACTION;

    if (is_cursed(r->attribs, C_GENEROUS, 0)) {
        n *= get_curseeffect(r->attribs, C_GENEROUS, 0);
    }

    return (int)n;
}

int rule_give(void)
{
    return config_get_int("rules.give.flags", GIVE_DEFAULT);
}

bool markets_module(void)
{
    return config_get_int("modules.markets", 0);
}

static struct param *configuration;

void config_set(const char *key, const char *value) {
    set_param(&configuration, key, value);
}

const char *config_get(const char *key) {
    return get_param(configuration, key);
}

int config_get_int(const char *key, int def) {
    return get_param_int(configuration, key, def);
}

double config_get_flt(const char *key, double def) {
    return get_param_flt(configuration, key, def);
}

bool config_token(const char *key, const char *tok) {
    return !!check_param(configuration, key, tok);
}

void free_config(void) {
    global.functions.maintenance = NULL;
    global.functions.wage = NULL;
    free_params(&configuration);
}

/** releases all memory associated with the game state.
 * call this function before calling read_game() to load a new game
 * if you have a previously loaded state in memory.
 */
void free_gamedata(void)
{
    int i;
    free_donations();

    for (i = 0; i != MAXLOCALES; ++i) {
        if (defaults[i]) {
            free_order(defaults[i]);
            defaults[i] = 0;
        }
    }
    free_factions();
    free_units();
    free_regions();
    free_borders();
    free_alliances();

    while (planes) {
        plane *pl = planes;
        planes = planes->next;
        free(pl->name);
        free(pl);
    }

    while (global.attribs) {
        a_remove(&global.attribs, global.attribs);
    }
}

const char * game_name(void) {
    const char * param = config_get("game.name");
    return param ? param : global.gamename;
}

int game_id(void) {
    return config_get_int("game.id", 0);
}

