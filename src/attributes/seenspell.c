/*
Copyright (c) 1998-2014,
Enno Rehling <enno@eressea.de>
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
#include <kernel/faction.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <util/attrib.h>
#include <util/gamedata.h>
#include <util/log.h>
#include <util/macros.h>

#include "seenspell.h"

#include <selist.h>
#include <storage.h>

#include <stdlib.h>

/* ------------------------------------------------------------- */
/* Ausgabe der Spruchbeschreibungen
* Anzeige des Spruchs nur, wenn die Stufe des besten Magiers vorher
* kleiner war (u->faction->seenspells). Ansonsten muss nur geprüft
* werden, ob dieser Magier den Spruch schon kennt, und andernfalls der
* Spruch zu seiner List-of-known-spells hinzugefügt werden.
*/

static int read_seenspell(attrib * a, void *owner, struct gamedata *data)
{
    storage *store = data->store;
    spell *sp = 0;
    char token[32];

    UNUSED_ARG(owner);
    READ_TOK(store, token, sizeof(token));
    if (data->version < UNIQUE_SPELLS_VERSION) {
        READ_INT(store, 0); /* ignore mtype */
    }
    sp = find_spell(token);
    if (!sp) {
        log_info("read_seenspell: could not find spell '%s'\n", token);
        return AT_READ_FAIL;
    }
    a->data.v = sp;
    return AT_READ_OK;
}

static void
write_seenspell(const attrib * a, const void *owner, struct storage *store)
{
    const spell *sp = (const spell *)a->data.v;
    UNUSED_ARG(owner);
    WRITE_TOK(store, sp->sname);
}

attrib_type at_seenspell = {
    "seenspell", NULL, NULL, NULL, write_seenspell, read_seenspell
};

static bool already_seen(const faction * f, const spell * sp)
{
    attrib *a;

    for (a = a_find(f->attribs, &at_seenspell); a && a->type == &at_seenspell;
        a = a->next) {
        if (a->data.v == sp)
            return true;
    }
    return false;
}

static void a_init_reportspell(struct attrib *a) {
    a->data.v = calloc(1, sizeof(spellbook_entry));
}

static void a_finalize_reportspell(struct attrib *a) {
    free(a->data.v);
}

attrib_type at_reportspell = {
    "reportspell",
    a_init_reportspell,
    a_finalize_reportspell,
    0, NO_WRITE, NO_READ
};

void show_new_spells(faction * f, int level, const spellbook *book)
{
    if (book) {
        selist *ql = book->spells;
        int qi;

        for (qi = 0; ql; selist_advance(&ql, &qi, 1)) {
            spellbook_entry *sbe = (spellbook_entry *)selist_get(ql, qi);
            if (sbe->level <= level) {
                if (!already_seen(f, sbe->sp)) {
                    attrib * a = a_new(&at_reportspell);
                    spellbook_entry * entry = (spellbook_entry *)a->data.v;
                    entry->level = sbe->level;
                    entry->sp = sbe->sp;
                    a_add(&f->attribs, a);
                    a_add(&f->attribs, a_new(&at_seenspell))->data.v = sbe->sp;
                }
            }
        }
    }
}

