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

#include <platform.h>
#include <kernel/config.h>
#include "economy.h"

#include "laws.h"
#include "skill.h"
#include "study.h"

#include <util/message.h>

#include <kernel/build.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <assert.h>
#include <limits.h>
#include <stdlib.h>


void expandstealing(region * r, econ_request * stealorders)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    unsigned int j;
    unsigned int norders;
    econ_request *requests;

    assert(rsilver);

    norders = expand_production(r, stealorders, &requests);
    if (!norders) return;

    /* F�r jede unit in der Region wird Geld geklaut, wenn sie Opfer eines
     * Beklauen-Orders ist. Jedes Opfer muss einzeln behandelt werden.
     *
     * u ist die beklaute unit. oa.unit ist die klauende unit.
     */

    for (j = 0; j != norders; j++) {
        unit *u;
        int n = 0;

        if (requests[j].unit->n > requests[j].unit->wants) {
            break;
        }

        u = findunitg(requests[j].no, r);

        if (u && u->region == r) {
            n = get_pooled(u, rsilver, GET_ALL, INT_MAX);
        }
        if (n > 10 && rplane(r) && (rplane(r)->flags & PFL_NOALLIANCES)) {
            /* In Questen nur reduziertes Klauen */
            n = 10;
        }
        if (n > 0) {
            n = MIN(n, requests[j].unit->wants);
            use_pooled(u, rsilver, GET_ALL, n);
            requests[j].unit->n = n;
            change_money(requests[j].unit, n);
            ADDMSG(&u->faction->msgs, msg_message("stealeffect", "unit region amount",
                u, u->region, n));
        }
        add_income(requests[j].unit, IC_STEAL, requests[j].unit->wants, requests[j].unit->n);
        fset(requests[j].unit, UFL_LONGACTION | UFL_NOTMOVING);
    }
    free(requests);
}

static int max_skill(region * r, struct faction * f, skill_t sk)
{
    unit *u;
    int w = 0;

    for (u = r->units; u; u = u->next) {
        if (u->faction == f) {
            int effsk = effskill(u, sk, 0);
            if (effsk > w) {
                w = effsk;
            }
        }
    }

    return w;
}

message * steal_message(const unit * u, struct order *ord) {
    plane *pl;

    if (fval(u_race(u), RCF_NOSTEAL)) {
        return msg_feedback(u, ord, "race_nosteal", "race", u_race(u));
    }

    if (fval(u->region->terrain, SEA_REGION) && u_race(u) != get_race(RC_AQUARIAN)) {
        return msg_feedback(u, ord, "error_onlandonly", "");
    }

    pl = rplane(u->region);
    if (pl && fval(pl, PFL_NOATTACK)) {
        return msg_feedback(u, ord, "error270", "");
    }
    return 0;
}

void steal_cmd(unit * u, struct order *ord, econ_request ** stealorders)
{
    const resource_type *rring = get_resourcetype(R_RING_OF_NIMBLEFINGER);
    int n, i, id, effsk;
    bool goblin = false;
    econ_request *o;
    unit *u2 = NULL;
    region *r = u->region;
    faction *f = NULL;
    message * msg;
    keyword_t kwd;

    kwd = init_order_depr(ord);
    assert(kwd == K_STEAL);

    assert(skill_enabled(SK_PERCEPTION) && skill_enabled(SK_STEALTH));

    msg = steal_message(u, ord);
    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
        return;
    }
    id = read_unitid(u->faction, r);
    if (id > 0) {
        u2 = findunitr(r, id);
    }
    if (u2 && u2->region == u->region) {
        f = u2->faction;
    }
    else {
        /* TODO: is this really necessary? it's the only time we use faction.c/deadhash
         * it allows stealing from a unit in a dead faction, but why? */
        f = dfindhash(id);
    }

    for (u2 = r->units; u2; u2 = u2->next) {
        if (u2->faction == f && cansee(u->faction, r, u2, 0))
            break;
    }

    if (!u2) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_unit_not_found",
            ""));
        return;
    }

    if (IsImmune(u2->faction)) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "newbie_immunity_error", "turns", NewbieImmunity()));
        return;
    }

    if (u->faction->alliance && u->faction->alliance == u2->faction->alliance) {
        cmistake(u, ord, 47, MSG_INCOME);
        return;
    }

    assert(u->region == u2->region);
    if (!can_contact(r, u, u2)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error60", ""));
        return;
    }

    effsk = effskill(u, SK_STEALTH, 0);
    n = effsk - max_skill(r, f, SK_PERCEPTION);

    if (n <= 0) {
        /* Wahrnehmung == Tarnung */
        if (u_race(u) != get_race(RC_GOBLIN) || effsk <= 3) {
            ADDMSG(&u->faction->msgs, msg_message("stealfail", "unit target", u, u2));
            if (n == 0) {
                ADDMSG(&u2->faction->msgs, msg_message("stealdetect", "unit", u2));
            }
            else {
                ADDMSG(&u2->faction->msgs, msg_message("thiefdiscover", "unit target",
                    u, u2));
            }
            return;
        }
        else {
            ADDMSG(&u->faction->msgs, msg_message("stealfatal", "unit target", u,
                u2));
            ADDMSG(&u2->faction->msgs, msg_message("thiefdiscover", "unit target", u,
                u2));
            n = 1;
            goblin = true;
        }
    }

    i = MIN(u->number, i_get(u->items, rring->itype));
    if (i > 0) {
        n *= STEALINCOME * (u->number + i * (roqf_factor() - 1));
    }
    else {
        n *= u->number * STEALINCOME;
    }

    u->wants = n;

    /* wer dank unsichtbarkeitsringen klauen kann, muss nicht unbedingt ein
     * guter dieb sein, schliesslich macht man immer noch sehr viel laerm */

    o = (econ_request *)calloc(1, sizeof(econ_request));
    o->unit = u;
    o->qty = 1;                   /* Betrag steht in u->wants */
    o->no = u2->no;
    o->type.goblin = goblin;      /* Merken, wenn Goblin-Spezialklau */
    o->next = *stealorders;
    *stealorders = o;

    /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */

    produceexp(u, SK_STEALTH, MIN(n, u->number));
}
