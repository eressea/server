#ifdef _MSC_VER
#include <platform.h>
#endif
#include <kernel/faction.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/macros.h>

#include "seenspell.h"

#include <selist.h>
#include <storage.h>

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------- */
/* Ausgabe der Spruchbeschreibungen
 * Anzeige des Spruchs nur, wenn die Stufe des besten Magiers vorher
 * kleiner war (u->faction->seenspells). Ansonsten muss nur geprueft
 * werden, ob dieser Magier den Spruch schon kennt, und andernfalls der
 * Spruch zu seiner List-of-known-spells hinzugefuegt werden.
 */

static int read_seenspells(variant *var, void *owner, struct gamedata *data)
{
    selist *ql = NULL;
    storage *store = data->store;
    char token[32];

    UNUSED_ARG(owner);
    READ_TOK(store, token, sizeof(token));
    while (token[0]) {
        spell *sp = find_spell(token);
        if (sp) {
            selist_push(&ql, sp);
        }
        else {
            log_info("read_seenspells: could not find spell '%s'\n", token);
        }
        READ_TOK(store, token, sizeof(token));
    }
    var->v = ql;
    return AT_READ_OK;
}

static bool cb_write_spell(void *data, void *more) {
    const spell *sp = (const spell *)data;
    storage *store = (storage *)more;
    WRITE_TOK(store, sp->sname);
    return true;
}

static void
write_seenspells(const variant *var, const void *owner, struct storage *store)
{
    UNUSED_ARG(owner);
    selist_foreach_ex((selist *)var->v, cb_write_spell, store);
    WRITE_TOK(store, "");
}

static int read_seenspell(variant *var, void *owner, struct gamedata *data)
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
        log_error("read_seenspell: could not find spell '%s'\n", token);
        return AT_READ_FAIL;
    }
    var->v = sp;
    return AT_READ_DEPR;
}

static int cmp_spell(const void *a, const void *b) {
    const spell *spa = (const spell *)a;
    const spell *spb = (const spell *)b;
    return strcmp(spa->sname, spb->sname);
}

static bool set_seen(attrib **alist, const struct spell *sp) {
    attrib *a = a_find(*alist, &at_seenspells);
    selist **sl;
    if (!a) {
        a = a_add(alist, a_new(&at_seenspells));
    }
    sl = (selist **)&a->data.v;
    return selist_set_insert(sl, (void *)sp, cmp_spell);
}

static void upgrade_seenspell(attrib **alist, attrib *abegin) {
    attrib *a, *ak;

    ak = a_find(*alist, &at_seenspells);
    if (ak) alist = &ak;
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        set_seen(alist, (const struct spell *)a->data.v);
    }
}

static void free_seenspells(variant *var) {
    selist *sl = (selist *)var->v;
    selist_free(sl);
}

attrib_type at_seenspells = {
    "seenspells", NULL, free_seenspells, NULL, write_seenspells, read_seenspells
};

attrib_type at_seenspell = {
    "seenspell", NULL, NULL, NULL, NULL, read_seenspell, upgrade_seenspell
};

static bool already_seen(const faction * f, const spell * sp)
{
    attrib *a;

    a = a_find(f->attribs, &at_seenspells);
    if (a) {
        selist *sl = (selist *)a->data.v;
        return selist_set_find(&sl, NULL, sp, cmp_spell);
    }
    return false;
}

attrib_type at_reportspell = {
    "reportspell", NULL
};

void show_spell(faction *f, spellbook_entry *sbe)
{
    const spell *sp = spellref_get(&sbe->spref);
    if (!already_seen(f, sp)) {
        /* mark the spell as seen by this faction: */
        if (set_seen(&f->attribs, sp)) {
            /* add the spell to the report: */
            attrib * a = a_new(&at_reportspell);
            a->data.v = (void *)sbe;
            a_add(&f->attribs, a);
        }
    }
}

void reset_seen_spells(faction *f, const struct spell *sp)
{
    a_removeall(&f->attribs, &at_seenspells);
}
