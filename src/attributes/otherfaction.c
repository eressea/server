#include <platform.h>
#include <kernel/config.h>
#include "otherfaction.h"

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/attrib.h>
#include <kernel/gamedata.h>

#include <storage.h>
#include <assert.h>

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

struct attrib *make_otherfaction(struct faction *f)
{
    attrib *a = a_new(&at_otherfaction);
    a->data.v = (void *)f;
    return a;
}

faction *visible_faction(const faction * f, const unit * u)
{
    if (f == NULL || !alliedunit(u, f, HELP_FSTEALTH)) {
        faction *fv = get_otherfaction(u);
        if (fv) return fv;
    }
    return u->faction;
}
