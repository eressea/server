#include <platform.h>
#include <kernel/config.h>
#include <kernel/spell.h>
#include <selist.h>
#include <util/log.h>
#include <util/gamedata.h>

#include "spellbook.h"

#include <storage.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

spellbook * create_spellbook(const char * name)
{
    spellbook *result = (spellbook *)malloc(sizeof(spellbook));
    result->name = name ? _strdup(name) : 0;
    result->spells = 0;
    return result;
}

void read_spellbook(spellbook **bookp, gamedata *data, int(*get_level)(const spell * sp, void *), void * cbdata)
{
    for (;;) {
        spell *sp = 0;
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
    selist *ql;
    int qi;

    if (book) {
        for (ql = book->spells, qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
            WRITE_TOK(store, sbe->sp->sname);
            WRITE_INT(store, sbe->level);
        }
    }
    WRITE_TOK(store, "end");
}

void spellbook_add(spellbook *sb, struct spell * sp, int level)
{
    spellbook_entry * sbe;

    assert(sb && sp && level > 0);
#ifndef NDEBUG
    if (spellbook_get(sb, sp)) {
        log_error("duplicate spell in spellbook '%s': '%s'\n", sb->name, sp->sname);
    }
#endif  
    sbe = (spellbook_entry *)malloc(sizeof(spellbook_entry));
    sbe->sp = sp;
    sbe->level = level;
    selist_push(&sb->spells, sbe);
}

void spellbook_clear(spellbook *sb)
{
    selist *ql;
    int qi;

    assert(sb);
    for (qi = 0, ql = sb->spells; ql; selist_advance(&ql, &qi, 1)) {
        spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
        free(sbe);
    }
    selist_free(sb->spells);
    free(sb->name);
}

int spellbook_foreach(spellbook *sb, int(*callback)(spellbook_entry *, void *), void * data)
{
    selist *ql;
    int qi;

    for (qi = 0, ql = sb->spells; ql; selist_advance(&ql, &qi, 1)) {
        spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
        int result = callback(sbe, data);
        if (result) {
            return result;
        }
    }
    return 0;
}

spellbook_entry * spellbook_get(spellbook *sb, const struct spell * sp)
{
    if (sb) {
        selist *ql;
        int qi;

        for (qi = 0, ql = sb->spells; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
            if (sp == sbe->sp) {
                return sbe;
            }
        }
    }
    return 0;
}

