#ifdef _MSC_VER
# include <platform.h>
#endif

#include "shock.h"

#include <magic.h>

/* kernel includes */
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/spell.h>
#include <kernel/unit.h>

/* util includes */
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <util/macros.h>
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
        if (hp > u->hp) hp = u->hp;
        u->hp = (hp > 1) ? hp : 1;
    }

    /* Aura - Verlust */
    if (is_mage(u)) {
        int aura = max_spellpoints_depr(u->region, u) / 10;
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
     * zu loeschen wenn der Familiar getoetet wird. Da sollten wir ueber eine
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
    UNUSED_ARG(data);
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

static int shock_read(trigger * t, gamedata *data) {
    if (read_unit_reference(data, (unit **)&t->data.v, NULL) <= 0) {
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
