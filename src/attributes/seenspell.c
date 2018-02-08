/*
Copyright (c) 1998-2018,
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

attrib_type at_reportspell = {
    "reportspell", NULL
};

void show_spell(faction *f, const spellbook_entry *sbe)
{
    if (!already_seen(f, sbe->sp)) {
        attrib * a = a_new(&at_reportspell);
        a->data.v = (void *)sbe;
        a_add(&f->attribs, a);
        a_add(&f->attribs, a_new(&at_seenspell))->data.v = sbe->sp;
    }
}

void reset_seen_spells(faction *f, const struct spell *sp)
{
    if (sp) {
        attrib *a = a_find(f->attribs, &at_seenspell);
        while (a && a->type == &at_seenspell && a->data.v != sp) {
            a = a->next;
        }
        if (a) {
            a_remove(&f->attribs, a);
        }
    }
    else {
        a_removeall(&f->attribs, &at_seenspell);
    }
}
