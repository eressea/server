#ifdef _MSC_VER
#include <platform.h>
#endif
#include "spell.h"

/* util includes */
#include <critbit.h>
#include <util/strings.h>
#include <util/language.h>
#include <util/log.h>
#include <util/strings.h>
#include <util/umlaut.h>
#include <selist.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>


static critbit_tree cb_spell_fun;
void add_spellcast(const char *sname, spell_f fun)
{
    size_t len;
    char data[64];

    len = cb_new_kv(sname, strlen(sname), &fun, sizeof(fun), data);
    assert(len <= sizeof(data));
    cb_insert(&cb_spell_fun, data, len);
}

spell_f get_spellcast(const char *sname)
{
    void * match;
    spell_f result = NULL;

    if (cb_find_prefix(&cb_spell_fun, sname, strlen(sname) + 1, &match, 1, 0)) {
        cb_get_kv(match, &result, sizeof(result));
    }
    return result;
}

static critbit_tree cb_fumble_fun;
void add_fumble(const char *sname, fumble_f fun)
{
    size_t len;
    char data[64];

    len = cb_new_kv(sname, strlen(sname), &fun, sizeof(fun), data);
    assert(len <= sizeof(data));
    cb_insert(&cb_fumble_fun, data, len);
}

fumble_f get_fumble(const char *sname)
{
    void * match;
    fumble_f result = NULL;

    if (cb_find_prefix(&cb_fumble_fun, sname, strlen(sname) + 1, &match, 1, 0)) {
        cb_get_kv(match, &result, sizeof(result));
    }
    return result;
}

static critbit_tree cb_spells;
selist * spells;

static void free_spell(spell *sp) {
    free(sp->syntax);
    free(sp->parameter);
    free(sp->sname);
    free(sp->components);
    free(sp);
}

static void free_spell_cb(void *cbdata) {
    free_spell((spell *)cbdata);
}

void free_spells(void) {
    cb_clear(&cb_fumble_fun);
    cb_clear(&cb_spell_fun);
    cb_clear(&cb_spells);
    selist_foreach(spells, free_spell_cb);
    selist_free(spells);
    spells = 0;
}

void add_spell(struct selist **slistp, spell * sp)
{
    if (!selist_set_insert(slistp, sp, NULL)) {
        log_error("add_spell: the list already contains the spell '%s'.\n", sp->sname);
    }
}

spell * create_spell(const char * name)
{
    spell * sp;
    char buffer[64];
    size_t len = strlen(name);

    assert(len + sizeof(sp) < sizeof(buffer));

    if (cb_find_str(&cb_spells, name)) {
        log_error("create_spell: duplicate name '%s'", name);
        return 0;
    }
    sp = (spell *)calloc(1, sizeof(spell));
    if (!sp) abort();
    len = cb_new_kv(name, len, &sp, sizeof(sp), buffer);
    if (cb_insert(&cb_spells, buffer, len) == CB_SUCCESS) {
        sp->sname = str_strdup(name);
        add_spell(&spells, sp);
        return sp;
    }
    free(sp);
    return 0;
}

static const char *sp_aliases[][2] = {
    { "create_potion_p14", "create_potion_healing" },
    { "gwyrrdfamiliar", "summon_familiar" },
    { "illaunfamiliar", "summon_familiar" },
    { "draigfamiliar", "summon_familiar" },
    { "commonfamiliar", "summon_familiar" },
    { "cerrdorfumbleshield", "cerddorfumbleshield" },
    { NULL, NULL },
};

static const char *sp_alias(const char *zname)
{
    int i;
    for (i = 0; sp_aliases[i][0]; ++i) {
        if (strcmp(sp_aliases[i][0], zname) == 0)
            return sp_aliases[i][1];
    }
    return NULL;
}

spell *find_spell(const char *name)
{
    const char * match;
    spell * sp = 0;
    match = cb_find_str(&cb_spells, name);
    if (!match) {
        const char * alias = sp_alias(name);
        if (alias) {
            match = cb_find_str(&cb_spells, alias);
        }
    }
    if (match) {
        cb_get_kv(match, &sp, sizeof(sp));
    }
    else {
        log_debug("find_spell: could not find spell '%s'\n", name);
    }
    return sp;
}

struct spellref *spellref_create(spell *sp, const char *name)
{
    spellref *spref = malloc(sizeof(spellref));
    if (!spref) abort();

    if (sp) {
        spref->sp = sp;
        spref->_name = str_strdup(sp->sname);
    }
    else if (name) {
        spref->_name = str_strdup(name);
        spref->sp = NULL;
    }
    return spref;
}

void spellref_init(spellref *spref, spell *sp, const char *name)
{
    if (sp) {
        spref->sp = sp;
        spref->_name = str_strdup(sp->sname);
    }
    else if (name) {
        spref->_name = str_strdup(name);
        spref->sp = NULL;
    }
}

void spellref_done(spellref *spref) {
    if (spref) {
        free(spref->_name);
    }
}

void spellref_free(spellref *spref)
{
    if (spref) {
        spellref_done(spref);
        free(spref);
    }
}

struct spell *spellref_get(struct spellref *spref)
{
    if (!spref->sp) {
        assert(spref->_name);
        spref->sp = find_spell(spref->_name);
        if (spref->sp) {
            free(spref->_name);
            spref->_name = NULL;
        }
    }
    return spref->sp;
}

const char *spellref_name(const struct spellref *spref)
{
    if (spref->_name) {
        return spref->_name;
    }
    assert(spref->sp);
    return spref->sp->sname;
}
