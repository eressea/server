#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "config.h"

/* kernel includes */
#include "ally.h"
#include "alchemy.h"
#include "curse.h"
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
#include "ship.h"
#include "skill.h"

/* util includes */
#include <kernel/attrib.h>
#include <kernel/event.h>

#include <util/base36.h>
#include <util/crmessage.h>
#include <util/language.h>
#include <util/functions.h>
#include <util/log.h>
#include <util/lists.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/path.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/translation.h>
#include <util/umlaut.h>

#include "guard.h"
#include "prefix.h"

/* external libraries */
#include <strings.h>
#include <iniparser.h>

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

static int *forbidden_ids;
static const char *forbidden[] = { "t", "te", "tem", "temp", NULL };

bool forbiddenid(int id)
{
    static size_t len;
    size_t i;
    if (id <= 0) {
        return true;
    }
    if (!forbidden_ids) {
        while (forbidden[len])
            ++len;
        forbidden_ids = malloc(len * sizeof(int));
        if (!forbidden_ids) abort();
        for (i = 0; i != len; ++i) {
            forbidden_ids[i] = atoi36(forbidden[i]);
        }
    }
    for (i = 0; i != len; ++i) {
        if (id == forbidden_ids[i]) {
            return true;
        }
    }
    return false;
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

int rule_give(void)
{
    static int config;
    static int rule;
    if (config_changed(&config)) {
        /* TODO: No game uses this. Eliminate? */
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

void free_ids(void)
{
    free(forbidden_ids);
    forbidden_ids = NULL;
}
