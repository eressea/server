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
#include "spell.h"

/* util includes */
#include <critbit.h>
#include <util/strings.h>
#include <util/language.h>
#include <util/log.h>
#include <util/umlaut.h>
#include <selist.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

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

spell * create_spell(const char * name, unsigned int id)
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
    len = cb_new_kv(name, len, &sp, sizeof(sp), buffer);
    if (cb_insert(&cb_spells, buffer, len) == CB_SUCCESS) {
        sp->id = id ? id : hashstring(name);
        sp->sname = strdup(name);
        add_spell(&spells, sp);
        return sp;
    }
    free(sp);
    return 0;
}

static const char *sp_aliases[][2] = {
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
    return zname;
}

spell *find_spell(const char *name)
{
    const char * match;
    spell * sp = 0;
    const char * alias = sp_alias(name);

    match = cb_find_str(&cb_spells, alias);
    if (match) {
        cb_get_kv(match, &sp, sizeof(sp));
    }
    else {
        log_debug("find_spell: could not find spell '%s'\n", name);
    }
    return sp;
}

spell *find_spellbyid(unsigned int id)
{
    selist *ql;
    int qi;

    if (id == 0)
        return NULL;
    for (qi = 0, ql = spells; ql; selist_advance(&ql, &qi, 1)) {
        spell *sp = (spell *)selist_get(ql, qi);
        if (sp->id == id) {
            return sp;
        }
    }
    for (qi = 0, ql = spells; ql; selist_advance(&ql, &qi, 1)) {
        spell *sp = (spell *)selist_get(ql, qi);
        unsigned int hashid = hashstring(sp->sname);
        if (hashid == id) {
            return sp;
        }
    }

    log_warning("cannot find spell by id: %u\n", id);
    return NULL;
}

struct spellref *spellref_create(spell *sp, const char *name)
{
    spellref *spref = malloc(sizeof(spellref));

    if (sp) {
        spref->sp = sp;
        spref->name = strdup(sp->sname);
    }
    else if (name) {
        spref->name = strdup(name);
        spref->sp = NULL;
    }
    return spref;
}

void spellref_free(spellref *spref)
{
    if (spref) {
        free(spref->name);
        free(spref);
    }
}

struct spell *spellref_get(struct spellref *spref)
{
    if (!spref->sp) {
        assert(spref->name);
        spref->sp = find_spell(spref->name);
        if (spref->sp) {
            free(spref->name);
            spref->name = NULL;
        }
    }
    return spref->sp;
}
