#include <kernel/spell.h>

#include <util/log.h>
#include <kernel/gamedata.h>

#include "spellbook.h"

#include <stb_ds.h>
#include <storage.h>
#include <strings.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

spellbook * create_spellbook(const char * name)
{
    spellbook *result = (spellbook *)malloc(sizeof(spellbook));
    if (!result) abort();
    result->name = name ? str_strdup(name) : 0;
    result->spells = NULL;
    return result;
}

void read_spellbook(spellbook **bookp, gamedata *data, int(*get_level)(const spell * sp, void *), void * cbdata)
{
    for (;;) {
        spell *sp = NULL;
        char spname[64];
        int level = 0;

        READ_TOK(data->store, spname, sizeof(spname));
        if (strcmp(spname, "end") == 0)
            break;
        if (bookp) {
            sp = find_spell(spname);
            if (!sp) {
                log_error("read_spells: could not find spell '%s'", spname);
            }
        }
        if (data->version >= SPELLBOOK_VERSION) {
            READ_INT(data->store, &level);
        }
        if (sp) {
            spellbook * sb = *bookp;
            if (level <= 0 && get_level) {
                level = get_level(sp, cbdata);
            }
            if (!sb) {
                *bookp = create_spellbook(0);
                sb = *bookp;
            }
            if (level > 0 && (data->version >= SPELLBOOK_VERSION || !spellbook_get(sb, sp))) {
                spellbook_add(sb, sp, level);
            }
        }
    }
}

void write_spellbook(const struct spellbook *book, struct storage *store)
{
    if (book) {
        ptrdiff_t qi, ql = arrlen(book->spells);

        for (qi = 0; qi != ql; ++qi) {
            spellbook_entry *sbe = (spellbook_entry *)book->spells + qi;
            WRITE_TOK(store, spellref_name(&sbe->spref));
            WRITE_INT(store, sbe->level);
        }
    }
    WRITE_TOK(store, "end");
}

void spellbook_addref(spellbook *sb, const char *name, int level) {
    spellbook_entry * sbe;

    assert(sb && name && level > 0);
    sbe = arraddnptr(sb->spells, 1);
    spellref_init(&sbe->spref, NULL, name);
    sbe->level = level;
}

void spellbook_add(spellbook *sb, spell *sp, int level)
{
    spellbook_entry * sbe;

    assert(sb && sp && level > 0);
#ifndef NDEBUG
    if (spellbook_get(sb, sp)) {
        log_error("duplicate spell in spellbook '%s': '%s'\n", sb->name, sp->sname);
    }
#endif  
    sbe = arraddnptr(sb->spells, 1);
    spellref_init(&sbe->spref, sp, NULL);
    sbe->level = level;
}

void spellbook_clear(spellbook *sb)
{
    ptrdiff_t qi, ql = arrlen(sb->spells);

    for (qi = 0; qi != ql; ++qi) {
        spellbook_entry *sbe = (spellbook_entry *)sb->spells + qi;
        spellref_done(&sbe->spref);
    }
    arrfree(sb->spells);
    free(sb->name);
}

int spellbook_foreach(spellbook *sb, int(*callback)(spellbook_entry *, void *), void * data)
{
    ptrdiff_t qi, ql = arrlen(sb->spells);

    for (qi = 0; qi != ql; ++qi) {
        spellbook_entry *sbe = (spellbook_entry *)sb->spells + qi;
        int result = callback(sbe, data);
        if (result) {
            return result;
        }
    }
    return 0;
}

spellbook_entry * spellbook_get(spellbook *sb, const struct spell *sp)
{
    if (sb) {
        ptrdiff_t qi, ql = arrlen(sb->spells);

        for (qi = 0; qi != ql; ++qi) {
            spellbook_entry *sbe = (spellbook_entry *)sb->spells + qi;
            if (spellref_get(&sbe->spref) == sp) {
                return sbe;
            }
        }
    }
    return 0;
}
