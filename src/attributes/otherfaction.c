#include "otherfaction.h"

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/attrib.h>
#include <kernel/gamedata.h>

#include <assert.h>
#include <stdbool.h>         // for true, false
#include <storage.h>

/*
 * simple attributes that do not yet have their own file
 */

void write_of(const variant *var, const void *owner, struct storage *store)
{
    const faction *f = (faction *)var->v;
    WRITE_INT(store, f->no);
}

int read_of(variant *var, void *owner, gamedata *data)
{                               /* return 1 on success, 0 if attrib needs removal */
    int of;

    READ_INT(data->store, &of);
    if (rule_stealth_other()) {
        var->v = findfaction(of);
        if (var->v) {
            return AT_READ_OK;
        }
    }
    return AT_READ_FAIL;
}

attrib_type at_otherfaction = {
    "otherfaction", NULL, NULL, NULL, write_of, read_of, NULL, ATF_UNIQUE
};

static struct attrib* make_otherfaction(struct faction* f)
{
    attrib* a = a_new(&at_otherfaction);
    a->data.v = (void*)f;
    return a;
}

void set_otherfaction(unit* u, faction* f)
{
    if (!f) {
        a_removeall(&u->attribs, &at_otherfaction);
    }
    else {
        attrib* a = a_find(u->attribs, &at_otherfaction);
        if (!a) {
            a = a_add(&u->attribs, make_otherfaction(f));
        } else {
            a->data.v = f;
        }
    }
}

faction *get_otherfaction(const unit * u)
{
    attrib *a = a_find(u->attribs, &at_otherfaction);
    if (a) {
        faction * f = (faction *)(a->data.v);
        if (f && f->_alive) {
            return f;
        }
    }
    return NULL;
}

faction *visible_faction(const faction * f, const unit * u, faction* of)
{
    if (f == NULL || !alliedunit(u, f, HELP_FSTEALTH)) {
        if (of) return of;
    }
    else {
        return u->faction;
    }
    return (u->flags & UFL_ANON_FACTION) ? NULL : u->faction;
}
