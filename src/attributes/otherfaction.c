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
#include "otherfaction.h"

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/attrib.h>
#include <util/gamedata.h>

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
