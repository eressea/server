#include <platform.h>
#include <kernel/config.h>
#include <kernel/spell.h>
#include <quicklist.h>
#include <util/log.h>

#include "spellbook.h"

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
    ql_push(&sb->spells, sbe);
}

void spellbook_clear(spellbook *sb)
{
    quicklist *ql;
    int qi;

    assert(sb);
    for (qi = 0, ql = sb->spells; ql; ql_advance(&ql, &qi, 1)) {
        spellbook_entry *sbe = (spellbook_entry *)ql_get(ql, qi);
        free(sbe);
    }
    ql_free(sb->spells);
    free(sb->name);
}

int spellbook_foreach(spellbook *sb, int(*callback)(spellbook_entry *, void *), void * data)
{
    quicklist *ql;
    int qi;

    for (qi = 0, ql = sb->spells; ql; ql_advance(&ql, &qi, 1)) {
        spellbook_entry *sbe = (spellbook_entry *)ql_get(ql, qi);
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
        quicklist *ql;
        int qi;

        for (qi = 0, ql = sb->spells; ql; ql_advance(&ql, &qi, 1)) {
            spellbook_entry *sbe = (spellbook_entry *)ql_get(ql, qi);
            if (sp == sbe->sp) {
                return sbe;
            }
        }
    }
    return 0;
}

