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

#ifdef _MSC_VER
#include <platform.h>
#endif

#include "config.h"

/* kernel includes */
#include "alliance.h"
#include "ally.h"
#include "alchemy.h"
#include "curse.h"
#include "connection.h"
#include "building.h"
#include "direction.h"
#include "faction.h"
#include "group.h"
#include "item.h"
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

/* util includes */
#include <kernel/attrib.h>
#include <kernel/event.h>

#include <util/base36.h>
#include <util/crmessage.h>
#include <util/keyword.h>
#include <util/language.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/lists.h>
#include <util/macros.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/path.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/strings.h>
#include <util/translation.h>
#include <util/umlaut.h>

#include "donations.h"
#include "guard.h"
#include "prefix.h"

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

#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
struct settings global;

int findoption(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_OPTIONS);
    variant token;

    if (findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (direction_t)token.i;
    }
    return NODIRECTION;
}

/* -- Erschaffung neuer Einheiten ------------------------------ */

static const char *forbidden[] = { "t", "te", "tem", "temp", NULL };
static int *forbidden_ids;

int forbiddenid(int id)
{
    static size_t len;
    size_t i;
    if (id <= 0)
        return 1;
    if (!forbidden_ids) {
        while (forbidden[len])
            ++len;
        forbidden_ids = calloc(len, sizeof(int));
        for (i = 0; i != len; ++i) {
            forbidden_ids[i] = atoi36(forbidden[i]);
        }
    }
    for (i = 0; i != len; ++i)
        if (id == forbidden_ids[i])
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
            addtoken((struct tnode **)tokens, name, var);
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
                addtoken((struct tnode **)tokens, name, var);
            }
            else {
                log_debug("no translation for OPTION %s in locale %s", options[i], locale_name(lang));
            }
        }
    }
}

void init_races(struct locale *lang)
{
    const struct race *rc;
    void **tokens;

    tokens = get_translations(lang, UT_RACES);
    for (rc = races; rc; rc = rc->next) {
        const char *name;
        variant var;
        var.v = (void *)rc;
        name = locale_string(lang, rc_name_s(rc, NAME_PLURAL), false);
        if (name) addtoken((struct tnode **)tokens, name, var);
        name = locale_string(lang, rc_name_s(rc, NAME_SINGULAR), false);
        if (name) addtoken((struct tnode **)tokens, name, var);
    }
}

static void init_magic(struct locale *lang)
{
    void **tokens;
    tokens = get_translations(lang, UT_MAGIC);
    if (tokens) {
        const char *str = config_get("rules.magic.playerschools");
        char *sstr, *tok;
        if (str == NULL) {
            str = "gwyrrd illaun draig cerddor tybied";
        }

        sstr = str_strdup(str);
        tok = strtok(sstr, " ");
        while (tok) {
            variant var;
            const char *name;
            int i;
            for (i = 0; i != MAXMAGIETYP; ++i) {
                if (strcmp(tok, magic_school[i]) == 0) break;
            }
            assert(i != MAXMAGIETYP);
            var.i = i;
            name = LOC(lang, mkname("school", tok));
            if (name) {
                addtoken((struct tnode **)tokens, name, var);
            }
            else {
                log_warning("no translation for magic school %s in locale %s", tok, locale_name(lang));
            }
            tok = strtok(NULL, " ");
        }
        free(sstr);
    }
}
void init_locale(struct locale *lang)
{
    init_magic(lang);
    init_directions(lang);
    init_keywords(lang);
    init_skills(lang);
    init_races(lang);
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
    char *v, *p_value;
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

char * join_path(const char *p1, const char *p2, char *dst, size_t len) {
    return path_join(p1, p2, dst, len);
}

static const char * relpath(char *buf, size_t sz, const char *path) {
    if (g_basedir) {
        path_join(g_basedir, path, buf, sz);
    }
    else {
        str_strlcpy(buf, path, sz);
    }
    return buf;
}

static const char *g_datadir;
const char *datapath(void)
{
    static char zText[4096];
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
    static char zText[4096];
    if (g_reportdir)
        return g_reportdir;
    return relpath(zText, sizeof(zText), "reports");
}

void set_reportpath(const char *path)
{
    g_reportdir = path;
}

static int sys_mkdir(const char *path, int mode) {
#ifdef WIN32
    UNUSED_ARG(mode);
    return _mkdir(path);
#else
    return mkdir(path, mode);
#endif
}

int create_directories(void) {
    int err;
    err = sys_mkdir(datapath(), 0777);
    if (err) {
        if (errno == EEXIST) errno = 0;
        else return err;
    }
    err = sys_mkdir(reportpath(), 0777);
    if (err && errno == EEXIST) {
        errno = 0;
    }
    return err;
}

double get_param_flt(const struct param *p, const char *key, double def)
{
    const char *str = p ? get_param(p, key) : NULL;
    return str ? atof(str) : def;
}

void kernel_done(void)
{
    /* calling this function releases memory assigned to static variables, etc.
     * calling it is optional, e.g. a release server will most likely not do it.
     */
    attrib_done();
    item_done();
    message_done();
    reports_done();
    curses_done();
    crmessage_done();
    translation_done();
    mt_clear();
}

bool rule_stealth_other(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("stealth.faction.other", 1);
    }
    return rule != 0;
}

bool rule_stealth_anon(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("stealth.faction.anon", 1);
    }
    return rule != 0;
}

bool rule_region_owners(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("rules.region_owners", 0);
    }
    return rule != 0;
}

int rule_blessed_harvest(void)
{
    static int rule, config;
    if (config_changed(&config)) {
        rule = config_get_int("rules.blessed_harvest.flags", HARVEST_WORK);
        assert(rule >= 0);
    }
    return rule;
}

int rule_alliance_limit(void)
{
    static int cache_token;
    static int rule = 0;

    if (config_changed(&cache_token)) {
        rule = config_get_int("rules.limit.alliance", 0);
    }
    assert(rule >= 0);
    return rule;
}

int rule_faction_limit(void)
{
    static int cache_token;
    static int rule = 0;
    if (config_changed(&cache_token)) {
        rule = config_get_int("rules.limit.faction", 0);
    }
    assert(rule >= 0);
    return rule;
}

void kernel_init(void)
{
    register_reports();
    mt_clear();
    translation_init();
}

order *default_order(const struct locale *lang)
{
    int i = locale_index(lang);
    keyword_t kwd;
    const char * str;
    order *result = 0;

    assert(i < MAXLOCALES);
    kwd = keyword_disabled(K_WORK) ? NOKEYWORD : K_WORK;
    str = config_get("orders.default");
    if (str) {
        kwd = findkeyword(str);
    }
    if (kwd != NOKEYWORD) {
        result = create_order(kwd, lang, NULL);
        return copy_order(result);
    }
    return NULL;
}

int rule_give(void)
{
    static int config;
    static int rule;
    if (config_changed(&config)) {
        rule = config_get_int("rules.give.flags", GIVE_DEFAULT);
    }
    return rule;
}

static struct param *configuration;
static int config_cache_key = 1;

bool config_changed(int *cache_key) {
    assert(cache_key);
    if (config_cache_key != *cache_key) {
        *cache_key = config_cache_key;
        return true;
    }
    return false;
}

#define MAXKEYS 16
void config_set_from(const dictionary *d, const char *valid_keys[])
{
    int s, nsec = iniparser_getnsec(d);
    for (s=0;s!=nsec;++s) {
        char key[128];
        const char *sec = iniparser_getsecname(d, s);
        int k, nkeys = iniparser_getsecnkeys(d, sec);
        const char *keys[MAXKEYS];
        size_t slen = strlen(sec);

        assert(nkeys <= MAXKEYS);
        assert(slen<sizeof(key));
        memcpy(key, sec, slen);
        key[slen] = '.';
        iniparser_getseckeys(d, sec, keys);
        for (k=0;k!=nkeys;++k) {
            const char *val, *orig;
            size_t klen = strlen(keys[k]);
            assert(klen+slen+1<sizeof(key));
            memcpy(key+slen+1, keys[k]+slen+1, klen-slen);
            orig = config_get(key);
            val = iniparser_getstring(d, keys[k], NULL);
            if (!orig) {
                if (val) {
                    if (valid_keys) {
                        int i;
                        for (i = 0; valid_keys[i]; ++i) {
                            size_t vlen = strlen(valid_keys[i]);
                            if (strncmp(key, valid_keys[i], vlen) == 0) break;
                        }
                        if (!valid_keys[i]) {
                            log_error("unknown key in ini-section %s: %s = %s", sec, key+slen+1, val);
                        }
                    }
                    config_set(key, val);
                }
            } else {
                log_debug("not overwriting %s=%s with %s", key, orig, val);
            }
        }
    }
}

void config_set(const char *key, const char *value)
{
    ++config_cache_key;
    set_param(&configuration, key, value);
}

void config_set_int(const char *key, int value)
{
    ++config_cache_key;
    set_param(&configuration, key, itoa10(value));
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
    free_params(&configuration);
    ++config_cache_key;
}

/** releases all memory associated with the game state.
 * call this function before calling read_game() to load a new game
 * if you have a previously loaded state in memory.
 */
void free_gamedata(void)
{
    free(forbidden_ids);
    forbidden_ids = NULL;

    free_factions();
    free_donations();
    free_units();
    free_regions();
    free_borders();
    free_alliances();

    while (planes) {
        plane *pl = planes;
        planes = planes->next;
        free_plane(pl);
    }
}

const char * game_name(void)
{
    const char * param = config_get("game.name");
    return param ? param : "Eressea";
}

const char * game_mailcmd(void)
{
    const char *param = config_get("game.mailcmd");
    if (!param) {
        static char result[32]; /* FIXME: static result */
        char *r = result;
        const char *c;
        param = game_name();
        for (c = param; *c && (result + sizeof(result)) > r; ++c) {
            *r++ = (char)toupper(*c);
        }
        *r = '\0';
        log_warning("game.mailcmd configuration is not set, using %s from game.name", result);
        config_set("game.mailcmd", result);
        return result;
    }
    return param;
}

int game_id(void) {
    return config_get_int("game.id", 0);
}
