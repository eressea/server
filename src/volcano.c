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
#include "volcano.h"

#include "alchemy.h"

/* kernel includes */
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/terrainid.h>

/* attributes includes */
#include <attributes/reduceproduction.h>

/* util includes */
#include <util/attrib.h>
#include <util/log.h>
#include <util/rand.h>
#include <util/message.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>

static int nb_armor(const unit * u, int index)
{
    const item *itm;
    int av = 0;
    int s = 0, a = 0;

    if (!(u_race(u)->battle_flags & BF_EQUIPMENT))
        return 0;

    /* Normale Rüstung */

    for (itm = u->items; itm; itm = itm->next) {
        const armor_type *atype = itm->type->rtype->atype;
        if (atype != NULL) {
            int *schutz = &a;
            if (atype->flags & ATF_SHIELD)
                schutz = &s;
            if (*schutz <= index) {
                *schutz += itm->number;
                if (*schutz > index) {
                    av += atype->prot;
                }
            }
        }
    }
    return av;
}

static int
damage_unit(unit * u, const char *dam, bool physical, bool magic)
{
    int *hp, hpstack[20];
    int h;
    int i, dead = 0, hp_rem = 0, heiltrank;
    double magres = magic_resistance(u);

    assert(u->number);
    if (fval(u_race(u), RCF_ILLUSIONARY) || u_race(u) == get_race(RC_SPELL)) {
        return 0;
    }

    assert(u->number <= u->hp);
    h = u->hp / u->number;
    /* HP verteilen */
    if (u->number < 20) {
        hp = hpstack;
    }
    else {
        hp = malloc(u->number * sizeof(int));
    }
    for (i = 0; i < u->number; i++)
        hp[i] = h;
    h = u->hp - (u->number * h);
    for (i = 0; i < h; i++)
        hp[i]++;

    /* Schaden */
    for (i = 0; i < u->number; i++) {
        int damage = dice_rand(dam);
        if (magic)
            damage = (int)(damage * (1.0 - magres));
        if (physical)
            damage -= nb_armor(u, i);
        hp[i] -= damage;
    }

    /* Auswirkungen */
    for (i = 0; i < u->number; i++) {
        if (hp[i] <= 0) {
            heiltrank = 0;

            /* Sieben Leben */
            if (old_race(u_race(u)) == RC_CAT && (chance(1.0 / 7))) {
                hp[i] = u->hp / u->number;
                hp_rem += hp[i];
                continue;
            }

            /* Heiltrank */
            if (oldpotiontype[P_HEAL]) {
                if (get_effect(u, oldpotiontype[P_HEAL]) > 0) {
                    change_effect(u, oldpotiontype[P_HEAL], -1);
                    heiltrank = 1;
                }
                else if (i_get(u->items, oldpotiontype[P_HEAL]->itype) > 0) {
                    i_change(&u->items, oldpotiontype[P_HEAL]->itype, -1);
                    change_effect(u, oldpotiontype[P_HEAL], 3);
                    heiltrank = 1;
                }
                if (heiltrank && (chance(0.50))) {
                    hp[i] = u->hp / u->number;
                    hp_rem += hp[i];
                    continue;
                }
            }
            dead++;
        }
        else {
            hp_rem += hp[i];
        }
    }

    scale_number(u, u->number - dead);
    u->hp = hp_rem;

    if (hp != hpstack) {
        free(hp);
    }

    return dead;
}

static region *rrandneighbour(region * r)
{
    direction_t i;
    region *rc = NULL;
    int rr, c = 0;

    /* Nachsehen, wieviele Regionen in Frage kommen */

    for (i = 0; i != MAXDIRECTIONS; i++) {
        c++;
    }
    /* Zufällig eine auswählen */

    rr = rng_int() % c;

    /* Durchzählen */

    c = -1;
    for (i = 0; i != MAXDIRECTIONS; i++) {
        rc = rconnect(r, i);
        c++;
        if (c == rr)
            break;
    }
    assert(i != MAXDIRECTIONS);
    return rc;
}

static void
volcano_destruction(region * volcano, region * r, const char *damage)
{
    attrib *a;
    unit **up;
    int percent = 25, time = 6 + rng_int() % 12;

    rsettrees(r, 2, 0);
    rsettrees(r, 1, 0);
    rsettrees(r, 0, 0);

    a = a_find(r->attribs, &at_reduceproduction);
    if (!a) {
        a = a_add(&r->attribs, make_reduceproduction(percent, time));
    }
    else {
        /* Produktion vierteln ... */
        a->data.sa[0] = (short)percent;
        /* Für 6-17 Runden */
        a->data.sa[1] = (short)(a->data.sa[1] + time);
    }

    /* Personen bekommen 4W10 Punkte Schaden. */

    for (up = &r->units; *up;) {
        unit *u = *up;
        if (u->number) {
            int dead = damage_unit(u, damage, true, false);
            /* TODO create undead */
            if (dead) {
                ADDMSG(&u->faction->msgs, msg_message("volcano_dead",
                    "unit region dead", u, volcano, dead));
            }
            if (!fval(u->faction, FFL_SELECT)) {
                fset(u->faction, FFL_SELECT);
                ADDMSG(&u->faction->msgs, msg_message("volcanooutbreaknn",
                    "region", r));
            }
        }
        if (u == *up) {
            up = &u->next;
        }
    }

    if (r != volcano) {
        ADDMSG(&r->msgs, msg_message("volcanooutbreak",
            "regionv regionn", volcano, r));
    }
    remove_empty_units_in_region(r);
}

void volcano_outbreak(region * r)
{
    region *rn;
    unit *u;
    faction *f;

    for (f = NULL, u = r->units; u; u = u->next) {
        if (f != u->faction) {
            f = u->faction;
            freset(f, FFL_SELECT);
        }
    }

    /* Zufällige Nachbarregion verwüsten */
    rn = rrandneighbour(r);
    volcano_destruction(r, r, "4d10");
    if (rn) {
        volcano_destruction(r, rn, "3d10");
    }
}

void volcano_update(void) 
{
    region *r; 
    /* Vulkane qualmen, brechen aus ... */
    for (r = regions; r; r = r->next) {
        if (r->terrain == newterrain(T_VOLCANO_SMOKING)) {
            if (a_find(r->attribs, &at_reduceproduction)) {
                ADDMSG(&r->msgs, msg_message("volcanostopsmoke", "region", r));
                rsetterrain(r, T_VOLCANO);
            }
            else {
                // TODO: this code path is inactive. are we only keeping it for old data? fix data instead.
                if (rng_int() % 100 < 12) {
                    ADDMSG(&r->msgs, msg_message("volcanostopsmoke", "region", r));
                    rsetterrain(r, T_VOLCANO);
                }
                else if (r->age > 20 && rng_int() % 100 < 8) {
                    volcano_outbreak(r);
                    rsetterrain(r, T_VOLCANO);
                }
            }
        }
        else if (r->terrain == newterrain(T_VOLCANO)) {
            if (rng_int() % 100 < 4) {
                ADDMSG(&r->msgs, msg_message("volcanostartsmoke", "region", r));
                rsetterrain(r, T_VOLCANO_SMOKING);
            }
        }
    }

}
