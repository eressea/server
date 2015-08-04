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
#include "save.h"
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

#ifdef USE_LIBXML2
/* libxml includes */
#include <libxml/tree.h>
#include <libxml/xpath.h>
#endif

/* external libraries */
#include <storage.h>
#include <iniparser.h>
#include <critbit.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>
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
    static int value = -1;
    static int gamecookie = -1;
    if (value < 0 || gamecookie != global.cookie) {
        gamecookie = global.cookie;
        value = get_param_int(global.parameters, "NewbieImmunity", 0);
    }
    return value;
}

bool IsImmune(const faction * f)
{
    return !fval(f, FFL_NPC) && f->age < NewbieImmunity();
}

static int ally_flag(const char *s, int help_mask)
{
    if ((help_mask & HELP_MONEY) && strcmp(s, "money") == 0)
        return HELP_MONEY;
    if ((help_mask & HELP_FIGHT) && strcmp(s, "fight") == 0)
        return HELP_FIGHT;
    if ((help_mask & HELP_GIVE) && strcmp(s, "give") == 0)
        return HELP_GIVE;
    if ((help_mask & HELP_GUARD) && strcmp(s, "guard") == 0)
        return HELP_GUARD;
    if ((help_mask & HELP_FSTEALTH) && strcmp(s, "stealth") == 0)
        return HELP_FSTEALTH;
    if ((help_mask & HELP_TRAVEL) && strcmp(s, "travel") == 0)
        return HELP_TRAVEL;
    return 0;
}

bool ExpensiveMigrants(void)
{
    static int value = -1;
    static int gamecookie = -1;
    if (value < 0 || gamecookie != global.cookie) {
        gamecookie = global.cookie;
        value = get_param_int(global.parameters, "study.expensivemigrants", 0);
    }
    return value != 0;
}

/** Specifies automatic alliance modes.
 * If this returns a value then the bits set are immutable between alliance
 * partners (faction::alliance) and cannot be changed with the HELP command.
 */
int AllianceAuto(void)
{
    static int value = -1;
    static int gamecookie = -1;
    if (value < 0 || gamecookie != global.cookie) {
        const char *str = get_param(global.parameters, "alliance.auto");
        gamecookie = global.cookie;
        value = 0;
        if (str != NULL) {
            char *sstr = _strdup(str);
            char *tok = strtok(sstr, " ");
            while (tok) {
                value |= ally_flag(tok, -1);
                tok = strtok(NULL, " ");
            }
            free(sstr);
        }
    }
    return value & HelpMask();
}

/** Limits the available help modes
 * The bitfield returned by this function specifies the available help modes
 * in this game (so you can, for example, disable HELP GIVE globally).
 * Disabling a status will disable the command sequence entirely (order parsing
 * uses this function).
 */
int HelpMask(void)
{
    static int rule = -1;
    static int gamecookie = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        const char *str = get_param(global.parameters, "rules.help.mask");
        gamecookie = global.cookie;
        rule = 0;
        if (str != NULL) {
            char *sstr = _strdup(str);
            char *tok = strtok(sstr, " ");
            while (tok) {
                rule |= ally_flag(tok, -1);
                tok = strtok(NULL, " ");
            }
            free(sstr);
        }
        else {
            rule = HELP_ALL;
        }
    }
    return rule;
}

int AllianceRestricted(void)
{
    static int rule = -1;
    static int gamecookie = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        const char *str = get_param(global.parameters, "alliance.restricted");
        gamecookie = global.cookie;
        rule = 0;
        if (str != NULL) {
            char *sstr = _strdup(str);
            char *tok = strtok(sstr, " ");
            while (tok) {
                rule |= ally_flag(tok, -1);
                tok = strtok(NULL, " ");
            }
            free(sstr);
        }
        rule &= HelpMask();
    }
    return rule;
}

int LongHunger(const struct unit *u)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (u != NULL) {
        if (!fval(u, UFL_HUNGER))
            return false;
        if (u_race(u) == get_race(RC_DAEMON))
            return false;
    }
    if (rule < 0 || gamecookie != global.cookie) {
        gamecookie = global.cookie;
        rule = get_param_int(global.parameters, "hunger.long", 0);
    }
    return rule;
}

int SkillCap(skill_t sk)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (sk == SK_MAGIC)
        return 0;                   /* no caps on magic */
    if (rule < 0 || gamecookie != global.cookie) {
        gamecookie = global.cookie;
        rule = get_param_int(global.parameters, "skill.maxlevel", 0);
    }
    return rule;
}

int NMRTimeout(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        gamecookie = global.cookie;
        rule = get_param_int(global.parameters, "nmr.timeout", 0);
    }
    return rule;
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

/** Returns the English name of the race, which is what the database uses.
 */
const char *dbrace(const struct race *rc)
{
    static char zText[32]; // FIXME: static return value
    char *zPtr = zText;

    /* the english names are all in ASCII, so we don't need to worry about UTF8 */
    strcpy(zText, (const char *)LOC(get_locale("en"), rc_name_s(rc, NAME_SINGULAR)));
    while (*zPtr) {
        *zPtr = (char)(toupper(*zPtr));
        ++zPtr;
    }
    return zText;
}

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

static void init_maxmagicians(struct attrib *a)
{
    a->data.i = MAXMAGICIANS;
}

static attrib_type at_maxmagicians = {
    "maxmagicians",
    init_maxmagicians,
    NULL,
    NULL,
    a_writeint,
    a_readint,
    ATF_UNIQUE
};

int max_magicians(const faction * f)
{
    int m =
        get_param_int(global.parameters, "rules.maxskills.magic", MAXMAGICIANS);
    attrib *a;

    if ((a = a_find(f->attribs, &at_maxmagicians)) != NULL) {
        m = a->data.i;
    }
    if (f->race == get_race(RC_ELF))
        ++m;
    return m;
}

static void init_npcfaction(struct attrib *a)
{
    a->data.i = 1;
}

static attrib_type at_npcfaction = {
    "npcfaction",
    init_npcfaction,
    NULL,
    NULL,
    a_writeint,
    a_readint,
    ATF_UNIQUE
};

int verbosity = 1;

FILE *debug;

/* ----------------------------------------------------------------------- */

int distribute(int old, int new_value, int n)
{
    int i;
    int t;
    assert(new_value <= old);

    if (old == 0)
        return 0;

    t = (n / old) * new_value;
    for (i = (n % old); i; i--)
        if (rng_int() % old < new_value)
            t++;

    return t;
}

bool unit_has_cursed_item(const unit * u)
{
    item *itm = u->items;
    while (itm) {
        if (fval(itm->type, ITF_CURSED) && itm->number > 0)
            return true;
        itm = itm->next;
    }
    return false;
}

static int
autoalliance(const plane * pl, const faction * sf, const faction * f2)
{
    if (pl && (pl->flags & PFL_FRIENDLY))
        return HELP_ALL;

    if (f_get_alliance(sf) != NULL && AllianceAuto()) {
        if (sf->alliance == f2->alliance)
            return AllianceAuto();
    }

    return 0;
}

static int ally_mode(const ally * sf, int mode)
{
    if (sf == NULL)
        return 0;
    return sf->status & mode;
}

int
alliedgroup(const struct plane *pl, const struct faction *f,
const struct faction *f2, const struct ally *sf, int mode)
{
    while (sf && sf->faction != f2)
        sf = sf->next;
    if (sf == NULL) {
        mode = mode & autoalliance(pl, f, f2);
    }
    mode = ally_mode(sf, mode) | (mode & autoalliance(pl, f, f2));
    if (AllianceRestricted()) {
        if (a_findc(f->attribs, &at_npcfaction)) {
            return mode;
        }
        if (a_findc(f2->attribs, &at_npcfaction)) {
            return mode;
        }
        if (f->alliance != f2->alliance) {
            mode &= ~AllianceRestricted();
        }
    }
    return mode;
}

int
alliedfaction(const struct plane *pl, const struct faction *f,
const struct faction *f2, int mode)
{
    return alliedgroup(pl, f, f2, f->allies, mode);
}

/* Die Gruppe von Einheit u hat helfe zu f2 gesetzt. */
int alliedunit(const unit * u, const faction * f2, int mode)
{
    ally *sf;
    int automode;

    assert(u);
    assert(f2);
    assert(u->region);            /* the unit should be in a region, but it's possible that u->number==0 (TEMP units) */
    if (u->faction == f2)
        return mode;
    if (u->faction != NULL && f2 != NULL) {
        plane *pl;

        if (mode & HELP_FIGHT) {
            if ((u->flags & UFL_DEFENDER) || (u->faction->flags & FFL_DEFENDER)) {
                faction *owner = region_get_owner(u->region);
                /* helps the owner of the region */
                if (owner == f2) {
                    return HELP_FIGHT;
                }
            }
        }

        pl = rplane(u->region);
        automode = mode & autoalliance(pl, u->faction, f2);

        if (pl != NULL && (pl->flags & PFL_NOALLIANCES))
            mode = (mode & automode) | (mode & HELP_GIVE);

        sf = u->faction->allies;
        if (fval(u, UFL_GROUP)) {
            const attrib *a = a_findc(u->attribs, &at_group);
            if (a != NULL)
                sf = ((group *)a->data.v)->allies;
        }
        return alliedgroup(pl, u->faction, f2, sf, mode);
    }
    return 0;
}

int count_faction(const faction * f, int flags)
{
    unit *u;
    int n = 0;
    for (u = f->units; u; u = u->nextF) {
        const race *rc = u_race(u);
        int x = (flags&COUNT_UNITS) ? 1 : u->number;
        if (f->race != rc) {
            if (!playerrace(rc)) {
                if (flags&COUNT_MONSTERS) {
                    n += x;
                }
            }
            else if (flags&COUNT_MIGRANTS) {
                if (!is_cursed(u->attribs, C_SLAVE, 0)) {
                    n += x;
                }
            }
        }
        else if (flags&COUNT_DEFAULT) {
            n += x;
        }
    }
    return n;
}

int count_units(const faction * f)
{
    return count_faction(f, COUNT_ALL | COUNT_UNITS);
}
int count_all(const faction * f)
{
    return count_faction(f, COUNT_ALL);
}
int count_migrants(const faction * f)
{
    return count_faction(f, COUNT_MIGRANTS);
}

int count_maxmigrants(const faction * f)
{
    static int migrants = -1;

    if (migrants < 0) {
        migrants = get_param_int(global.parameters, "rules.migrants", INT_MAX);
    }
    if (migrants == INT_MAX) {
        int x = 0;
        if (f->race == get_race(RC_HUMAN)) {
            int nsize = count_all(f);
            if (nsize > 0) {
                x = (int)(log10(nsize / 50.0) * 20);
                if (x < 0)
                    x = 0;
            }
        }
        return x;
    }
    return migrants;
}

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

faction *getfaction(void)
{
    return findfaction(getid());
}

unit *getnewunit(const region * r, const faction * f)
{
    int n;
    n = getid();

    return findnewunit(r, f, n);
}

static int read_newunitid(const faction * f, const region * r)
{
    int n;
    unit *u2;
    n = getid();
    if (n == 0)
        return -1;

    u2 = findnewunit(r, f, n);
    if (u2)
        return u2->no;

    return -1;
}

int read_unitid(const faction * f, const region * r)
{
    char token[16];
    const char *s = gettoken(token, sizeof(token));

    /* Da s nun nur einen string enthaelt, suchen wir ihn direkt in der
     * paramliste. machen wir das nicht, dann wird getnewunit in s nach der
     * nummer suchen, doch dort steht bei temp-units nur "temp" drinnen! */

    if (!s || *s == 0) {
        return -1;
    }
    if (isparam(s, f->locale, P_TEMP)) {
        return read_newunitid(f, r);
    }
    return atoi36((const char *)s);
}

int getunit(const region * r, const faction * f, unit **uresult)
{
    unit *u2 = NULL;
    int n = read_unitid(f, r);
    int result = GET_NOTFOUND;

    if (n == 0) {
        result = GET_PEASANTS;
    }
    else if (n > 0) {
        u2 = findunit(n);
        if (u2 != NULL && u2->region == r) {
            /* there used to be a 'u2->flags & UFL_ISNEW || u2->number>0' condition
            * here, but it got removed because of a bug that made units disappear:
            * http://eressea.upb.de/mantis/bug_view_page.php?bug_id=0000172
            */
            result = GET_UNIT;
        }
        else {
            u2 = NULL;
        }
    }
    if (uresult) {
        *uresult = u2;
    }
    return result;
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

extern faction *dfindhash(int i);

static const char *forbidden[] = { "t", "te", "tem", "temp", NULL };

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

/* ID's für Einheiten und Zauber */
int newunitid(void)
{
    int random_unit_no;
    int start_random_no;
    random_unit_no = 1 + (rng_int() % MAX_UNIT_NR);
    start_random_no = random_unit_no;

    while (ufindhash(random_unit_no) || dfindhash(random_unit_no)
        || cfindhash(random_unit_no)
        || forbiddenid(random_unit_no)) {
        random_unit_no++;
        if (random_unit_no == MAX_UNIT_NR + 1) {
            random_unit_no = 1;
        }
        if (random_unit_no == start_random_no) {
            random_unit_no = (int)MAX_UNIT_NR + 1;
        }
    }
    return random_unit_no;
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
        const char *str = get_param(global.parameters, "rules.magic.playerschools");
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

    init_translations(lang, UT_PARAMS, parameter_key, MAXPARAMS);

    init_options_translation(lang);
    init_terrains_translation(lang);
}

typedef struct param {
    struct param *next;
    char *name;
    char *data;
} param;

const char *get_param(const struct param *p, const char *key)
{
    while (p != NULL) {
        int cmp = strcmp(p->name, key);
        if (cmp == 0) {
            return p->data;
        }
        else if (cmp > 0) {
            break;
        }
        p = p->next;
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

static const char *g_datadir;
const char *datapath(void)
{
    static char zText[MAX_PATH]; // FIXME: static return value
    if (g_datadir)
        return g_datadir;
    return strcat(strcpy(zText, basepath()), "/data");
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
    return strcat(strcpy(zText, basepath()), "/reports");
}

void set_reportpath(const char *path)
{
    g_reportdir = path;
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

float get_param_flt(const struct param *p, const char *key, float def)
{
    const char *str = get_param(p, key);
    return str ? (float)atof(str) : def;
}

void set_param(struct param **p, const char *key, const char *data)
{
    struct param *par;

    ++global.cookie;
    while (*p != NULL) {
        int cmp = strcmp((*p)->name, key);
        if (cmp == 0) {
            par = *p;
            free(par->data);
            if (data) {
                par->data = _strdup(data);
            }
            else {
                *p = par->next;
                free(par);
            }
            return;
        }
        else if (cmp > 0) {
            break;
        }
        p = &(*p)->next;
    }
    par = malloc(sizeof(param));
    par->name = _strdup(key);
    par->data = _strdup(data);
    par->next = *p;
    *p = par;
}

void kernel_done(void)
{
    /* calling this function releases memory assigned to static variables, etc.
     * calling it is optional, e.g. a release server will most likely not do it.
     */
    translation_done();
    free_attribs();
}

attrib_type at_germs = {
    "germs",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeshorts,
    a_readshorts,
    ATF_UNIQUE
};

attrib_type at_guard = {
    "guard",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeint,
    a_readint,
    ATF_UNIQUE
};

void setstatus(struct unit *u, int status)
{
    assert(status >= ST_AGGRO && status <= ST_FLEE);
    if (u->status != status) {
        u->status = (status_t)status;
    }
}

void setguard(unit * u, unsigned int flags)
{
    /* setzt die guard-flags der Einheit */
    attrib *a = NULL;
    assert(flags == 0 || !fval(u, UFL_MOVED));
    assert(flags == 0 || u->status < ST_FLEE);
    if (fval(u, UFL_GUARD)) {
        a = a_find(u->attribs, &at_guard);
    }
    if (flags == GUARD_NONE) {
        freset(u, UFL_GUARD);
        if (a)
            a_remove(&u->attribs, a);
        return;
    }
    fset(u, UFL_GUARD);
    fset(u->region, RF_GUARDED);
    if (flags == guard_flags(u)) {
        if (a)
            a_remove(&u->attribs, a);
    }
    else {
        if (!a)
            a = a_add(&u->attribs, a_new(&at_guard));
        a->data.i = (int)flags;
    }
}

unsigned int getguard(const unit * u)
{
    attrib *a;

    assert(fval(u, UFL_GUARD) || (u->building && u == building_owner(u->building))
        || !"you're doing it wrong! check is_guard first");
    a = a_find(u->attribs, &at_guard);
    if (a) {
        return (unsigned int)a->data.i;
    }
    return guard_flags(u);
}

#ifndef HAVE_STRDUP
char *_strdup(const char *s)
{
    return strcpy((char *)malloc(sizeof(char) * (strlen(s) + 1)), s);
}
#endif

bool faction_id_is_unused(int id)
{
    return findfaction(id) == NULL;
}

unsigned int guard_flags(const unit * u)
{
    unsigned int flags =
        GUARD_CREWS | GUARD_LANDING | GUARD_TRAVELTHRU | GUARD_TAX;
#if GUARD_DISABLES_PRODUCTION == 1
    flags |= GUARD_PRODUCE;
#endif
#if GUARD_DISABLES_RECRUIT == 1
    flags |= GUARD_RECRUIT;
#endif
    switch (old_race(u_race(u))) {
    case RC_ELF:
        if (u->faction->race != u_race(u))
            break;
        /* else fallthrough */
    case RC_TREEMAN:
        flags |= GUARD_TREES;
        break;
    case RC_IRONKEEPER:
        flags = GUARD_MINING;
        break;
    default:
        /* TODO: This should be configuration variables, all of it */
        break;
    }
    return flags;
}

void guard(unit * u, unsigned int mask)
{
    unsigned int flags = guard_flags(u);
    setguard(u, flags & mask);
}

int besieged(const unit * u)
{
    /* belagert kann man in schiffen und burgen werden */
    return (u && !keyword_disabled(K_BESIEGE)
        && u->building && u->building->besieged
        && u->building->besieged >= u->building->size * SIEGEFACTOR);
}

bool has_horses(const struct unit * u)
{
    item *itm = u->items;
    for (; itm; itm = itm->next) {
        if (itm->type->flags & ITF_ANIMAL)
            return true;
    }
    return false;
}

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

int rule_stealth_faction(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule = get_param_int(global.parameters, "rules.stealth.faction", 0xFF);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
    return rule;
}

int rule_region_owners(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule = get_param_int(global.parameters, "rules.region_owners", 0);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
    return rule;
}

int rule_auto_taxation(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule =
            get_param_int(global.parameters, "rules.economy.taxation", TAX_ORDER);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
    return rule;
}

int rule_blessed_harvest(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule =
            get_param_int(global.parameters, "rules.magic.blessed_harvest",
            HARVEST_WORK);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
    return rule;
}

int rule_alliance_limit(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule = get_param_int(global.parameters, "rules.limit.alliance", 0);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
    return rule;
}

int rule_faction_limit(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule = get_param_int(global.parameters, "rules.limit.faction", 0);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
    return rule;
}

int rule_transfermen(void)
{
    static int gamecookie = -1;
    static int rule = -1;
    if (rule < 0 || gamecookie != global.cookie) {
        rule = get_param_int(global.parameters, "rules.transfermen", 1);
        gamecookie = global.cookie;
        assert(rule >= 0);
    }
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

#define MAINTENANCE 10
int maintenance_cost(const struct unit *u)
{
    if (u == NULL)
        return MAINTENANCE;
    if (global.functions.maintenance) {
        int retval = global.functions.maintenance(u);
        if (retval >= 0)
            return retval;
    }
    return u_race(u)->maintenance * u->number;
}

int produceexp(struct unit *u, skill_t sk, int n)
{
    if (global.producexpchance > 0.0F) {
        if (n == 0 || !playerrace(u_race(u)))
            return 0;
        learn_skill(u, sk, global.producexpchance);
    }
    return 0;
}

int lovar(double xpct_x2)
{
    int n = (int)(xpct_x2 * 500) + 1;
    if (n == 0)
        return 0;
    return (rng_int() % n + rng_int() % n) / 1000;
}

bool has_limited_skills(const struct unit * u)
{
    if (has_skill(u, SK_MAGIC) || has_skill(u, SK_ALCHEMY) ||
        has_skill(u, SK_TACTICS) || has_skill(u, SK_HERBALISM) ||
        has_skill(u, SK_SPY)) {
        return true;
    }
    else {
        return false;
    }
}

static int read_ext(attrib * a, void *owner, struct storage *store)
{
    int len;

    READ_INT(store, &len);
    store->api->r_bin(store->handle, NULL, (size_t)len);
    return AT_READ_OK;
}


void attrib_init(void)
{
    /* Alle speicherbaren Attribute müssen hier registriert werden */
    at_register(&at_shiptrail);
    at_register(&at_familiar);
    at_register(&at_familiarmage);
    at_register(&at_clone);
    at_register(&at_clonemage);
    at_register(&at_eventhandler);
    at_register(&at_mage);
    at_register(&at_countdown);
    at_register(&at_curse);

    at_register(&at_seenspell);

    /* neue REGION-Attribute */
    at_register(&at_moveblock);
    at_register(&at_deathcount);
    at_register(&at_woodcount);

    /* neue UNIT-Attribute */
    at_register(&at_siege);
    at_register(&at_effect);
    at_register(&at_private);

    at_register(&at_icastle);
    at_register(&at_guard);
    at_register(&at_group);

    at_register(&at_building_generic_type);
    at_register(&at_maxmagicians);
    at_register(&at_npcfaction);

    /* connection-typen */
    register_bordertype(&bt_noway);
    register_bordertype(&bt_fogwall);
    register_bordertype(&bt_wall);
    register_bordertype(&bt_illusionwall);
    register_bordertype(&bt_road);

    register_function((pf_generic)& minimum_wage, "minimum_wage");

    at_register(&at_germs);

    at_deprecate("xontormiaexpress", a_readint);    /* required for old datafiles */
    at_deprecate("lua", read_ext);    /* required for old datafiles */
}

void kernel_init(void)
{
    register_reports();
    if (!mt_find("missing_message")) {
        mt_register(mt_new_va("missing_message", "name:string", 0));
        mt_register(mt_new_va("missing_feedback", "unit:unit", "region:region", "command:order", "name:string", 0));
    }
    attrib_init();
    translation_init();
}

static order * defaults[MAXLOCALES];
keyword_t default_keyword = NOKEYWORD;

void set_default_order(int kwd) {
    default_keyword = (keyword_t)kwd;
}

order *default_order(const struct locale *lang)
{
    static int usedefault = 1;
    int i = locale_index(lang);
    order *result = 0;
    assert(i < MAXLOCALES);

    if (default_keyword!=NOKEYWORD) {
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
    return get_param_int(global.parameters, "rules.give", GIVE_DEFAULT);
}

int markets_module(void)
{
    return get_param_int(global.parameters, "modules.markets", 0);
}

/** releases all memory associated with the game state.
 * call this function before calling read_game() to load a new game
 * if you have a previously loaded state in memory.
 */
void free_gamedata(void)
{
    int i;
    free_units();
    free_regions();
    free_borders();

    for (i = 0; i != MAXLOCALES; ++i) {
        if (defaults[i]) {
            free_order(defaults[i]);
            defaults[i] = 0;
        }
    }
    free_alliances();
    while (factions) {
        faction *f = factions;
        factions = f->next;
        funhash(f);
        free_faction(f);
        free(f);
    }

    while (planes) {
        plane *pl = planes;
        planes = planes->next;
        free(pl->name);
        free(pl);
    }

    while (global.attribs) {
        a_remove(&global.attribs, global.attribs);
    }
    ++global.cookie;              /* readgame() already does this, but sjust in case */
}

const char * game_name(void) {
    const char * param = get_param(global.parameters, "game.name");
    return param ? param : global.gamename;
}

int game_id(void) {
    return get_param_int(global.parameters, "game.id", 0);
}

