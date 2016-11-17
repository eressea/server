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
#include "shock.h"

#include "magic.h"

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/gamedata.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/rng.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/***
 ** shock
 **/

/* ------------------------------------------------------------- */
/* do_shock - Schockt die Einheit, z.B. bei Verlust eines */
/* Vertrauten.               */
/* ------------------------------------------------------------- */

static void do_shock(unit * u, const char *reason)
{
    int i;

    if (u->number > 0) {
        /* HP - Verlust */
        int hp = (unit_max_hp(u) * u->number) / 10;
        hp = _min(u->hp, hp);
        u->hp = _max(1, hp);
    }

    /* Aura - Verlust */
    if (is_mage(u)) {
        int aura = max_spellpoints(u->region, u) / 10;
        int now = get_spellpoints(u);
        if (now > aura) {
            set_spellpoints(u, aura);
        }
    }

    /* Evt. Talenttageverlust */
    for (i = 0; i != u->skill_size; ++i)
        if (rng_int() % 5 == 0) {
            skill *sv = u->skills + i;
            int weeks = (sv->level * sv->level - sv->level) / 2;
            int change = (weeks + 9) / 10;
            reduce_skill(u, sv, change);
        }

    /* Dies ist ein Hack, um das skillmod und familiar-Attribut beim Mage
     * zu löschen wenn der Familiar getötet wird. Da sollten wir über eine
     * saubere Implementation nachdenken. */

    if (strcmp(reason, "trigger") == 0) {
        remove_familiar(u);
    }
    if (u->faction != NULL) {
        ADDMSG(&u->faction->msgs, msg_message("shock",
            "mage reason", u, reason));
    }
}

static int shock_handle(trigger * t, void *data)
{
    /* destroy the unit */
    unit *u = (unit *)t->data.v;
    if (u && u->number) {
        do_shock(u, "trigger");
    }
    unused_arg(data);
    return 0;
}

static void shock_write(const trigger * t, struct storage *store)
{
    unit *u = (unit *)t->data.v;
    trigger *next = t->next;
    while (next) {
        /* make sure it is unique! */
        if (next->type == t->type && next->data.v == t->data.v)
            break;
        next = next->next;
    }
    if (next && u) {
        log_error("more than one shock-attribut for %s on a unit. FIXED.\n", itoa36(u->no));
        write_unit_reference(NULL, store);
    }
    else {
        write_unit_reference(u, store);
    }
}

static int shock_read(trigger * t, gamedata *data)
{
    int result =
        read_reference(&t->data.v, data, read_unit_reference, resolve_unit);
    if (result == 0 && t->data.v == NULL) {
        return AT_READ_FAIL;
    }
    return AT_READ_OK;
}

trigger_type tt_shock = {
    "shock",
    NULL,
    NULL,
    shock_handle,
    shock_write,
    shock_read
};

trigger *trigger_shock(unit * u)
{
    trigger *t = t_new(&tt_shock);
    t->data.v = (void *)u;
    return t;
}
