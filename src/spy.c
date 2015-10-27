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
#include "spy.h"
#include "laws.h"
#include "move.h"
#include "reports.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/stealth.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/parser.h>
#include <quicklist.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* in spy steht der Unterschied zwischen Wahrnehmung des Opfers und
* Spionage des Spions */
void spy_message(int spy, const unit * u, const unit * target)
{
    const char *str = report_kampfstatus(target, u->faction->locale); // FIXME: use report_status instead

    ADDMSG(&u->faction->msgs, msg_message("spyreport", "spy target status", u,
        target, str));
    if (spy > 20) {
        sc_mage *mage = get_mage(target);
        /* for mages, spells and magic school */
        if (mage) {
            ADDMSG(&u->faction->msgs, msg_message("spyreport_mage", "spy target type", u,
                target, magic_school[mage->magietyp]));
        }
    }
    if (spy > 6) {
        faction *fv = visible_faction(u->faction, target);
        if (fv && fv != target->faction) {
            /* true faction */
            ADDMSG(&u->faction->msgs, msg_message("spyreport_faction",
                "spy target faction", u, target, target->faction));
            add_seen_faction(u->faction, target->faction);
        }
    }
    if (spy > 0) {
        int first = 1;
        int found = 0;
        skill *sv;
        char buf[4096];

        buf[0] = 0;
        for (sv = target->skills; sv != target->skills + target->skill_size; ++sv) {
            if (sv->level > 0) {
                found++;
                if (first == 1) {
                    first = 0;
                }
                else {
                    strncat(buf, ", ", sizeof(buf) - 1);
                }
                strncat(buf, (const char *)skillname((skill_t)sv->id, u->faction->locale),
                    sizeof(buf) - 1);
                strncat(buf, " ", sizeof(buf) - 1);
                strncat(buf, itoa10(eff_skill(target, sv, target->region)),
                    sizeof(buf) - 1);
            }
        }
        if (found) {
            ADDMSG(&u->faction->msgs, msg_message("spyreport_skills", "spy target skills", u,
                target, buf));
        }

        if (target->items) {
            ADDMSG(&u->faction->msgs, msg_message("spyreport_items", "spy target items", u,
                target, target->items));
        }
    }
}

int spy_cmd(unit * u, struct order *ord)
{
    unit *target;
    int spy, observe;
    double spychance, observechance;
    region *r = u->region;

    init_order(ord);
    getunit(r, u->faction, &target);

    if (!target) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "feedback_unit_not_found", ""));
        return 0;
    }
    if (!can_contact(r, u, target)) {
        cmistake(u, u->thisorder, 24, MSG_EVENT);
        return 0;
    }
    if (effskill(u, SK_SPY, 0) < 1) {
        cmistake(u, u->thisorder, 39, MSG_EVENT);
        return 0;
    }
    /* Die Grundchance fuer einen erfolgreichen Spionage-Versuch ist 10%.
     * Fuer jeden Talentpunkt, den das Spionagetalent das Tarnungstalent
     * des Opfers uebersteigt, erhoeht sich dieses um 5%*/
    spy = effskill(u, SK_SPY, 0) - effskill(target, SK_STEALTH, r);
    spychance = 0.1 + _max(spy * 0.05, 0.0);

    if (chance(spychance)) {
        produceexp(u, SK_SPY, u->number);
        spy_message(spy, u, target);
    }
    else {
        ADDMSG(&u->faction->msgs, msg_message("spyfail", "spy target", u, target));
    }

    /* der Spion kann identifiziert werden, wenn das Opfer bessere
     * Wahrnehmung als das Ziel Tarnung + Spionage/2 hat */
    observe = effskill(target, SK_PERCEPTION, r)
        - (effskill(u, SK_STEALTH, 0) + effskill(u, SK_SPY, 0) / 2);

    if (invisible(u, target) >= u->number) {
        observe = _min(observe, 0);
    }

    /* Anschliessend wird - unabhaengig vom Erfolg - gewuerfelt, ob der
     * Spionageversuch bemerkt wurde. Die Wahrscheinlich dafuer ist (100 -
     * SpionageSpion*5 + WahrnehmungOpfer*2)%. */
    observechance = 1.0 - (effskill(u, SK_SPY, 0) * 0.05)
        + (effskill(target, SK_PERCEPTION, 0) * 0.02);

    if (chance(observechance)) {
        ADDMSG(&target->faction->msgs, msg_message("spydetect",
            "spy target", observe > 0 ? u : NULL, target));
    }
    return 0;
}

void set_factionstealth(unit * u, faction * f)
{
    region *lastr = NULL;
    /* for all units mu of our faction, check all the units in the region
     * they are in, if their visible faction is f, it's ok. use lastr to
     * avoid testing the same region twice in a row. */
    unit *mu = u->faction->units;
    while (mu != NULL) {
        if (mu->number && mu->region != lastr) {
            unit *ru = mu->region->units;
            lastr = mu->region;
            while (ru != NULL) {
                if (ru->number) {
                    faction *fv = visible_faction(f, ru);
                    if (fv == f) {
                        if (cansee(f, lastr, ru, 0))
                            break;
                    }
                }
                ru = ru->next;
            }
            if (ru != NULL)
                break;
        }
        mu = mu->nextF;
    }
    if (mu != NULL) {
        attrib *a = a_find(u->attribs, &at_otherfaction);
        if (!a)
            a = a_add(&u->attribs, make_otherfaction(f));
        else
            a->data.v = f;
    }
}

int setstealth_cmd(unit * u, struct order *ord)
{
    char token[64];
    const char *s;
    int level, rule;

    init_order(ord);
    s = gettoken(token, sizeof(token));

    /* Tarne ohne Parameter: Setzt maximale Tarnung */

    if (s == NULL || *s == 0) {
        u_seteffstealth(u, -1);
        return 0;
    }

    if (isdigit(s[0])) {
        /* Tarnungslevel setzen */
        level = atoi((const char *)s);
        if (level > effskill(u, SK_STEALTH, 0)) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_lowstealth", ""));
            return 0;
        }
        u_seteffstealth(u, level);
        return 0;
    }

    if (skill_enabled(SK_STEALTH)) { /* hack! E3 erlaubt keine Tarnung */
        const race *trace;

        trace = findrace(s, u->faction->locale);
        if (trace) {
            /* demons can cloak as other player-races */
            if (u_race(u) == get_race(RC_DAEMON)) {
                race_t allowed[] = { RC_DWARF, RC_ELF, RC_ORC, RC_GOBLIN, RC_HUMAN,
                    RC_TROLL, RC_DAEMON, RC_INSECT, RC_HALFLING, RC_CAT, RC_AQUARIAN,
                    NORACE
                };
                int i;
                for (i = 0; allowed[i] != NORACE; ++i)
                    if (get_race(allowed[i]) == trace)
                        break;
                if (get_race(allowed[i]) == trace) {
                    u->irace = trace;
                    if (u_race(u)->flags & RCF_SHAPESHIFTANY && get_racename(u->attribs))
                        set_racename(&u->attribs, NULL);
                }
                return 0;
            }

            /* Singdrachen koennen sich nur als Drachen tarnen */
            if (u_race(u) == get_race(RC_SONGDRAGON)
                || u_race(u) == get_race(RC_BIRTHDAYDRAGON)) {
                if (trace == get_race(RC_SONGDRAGON) || trace == get_race(RC_FIREDRAGON)
                    || trace == get_race(RC_DRAGON) || trace == get_race(RC_WYRM)) {
                    u->irace = trace;
                    if (u_race(u)->flags & RCF_SHAPESHIFTANY && get_racename(u->attribs))
                        set_racename(&u->attribs, NULL);
                }
                return 0;
            }

            /* Daemomen und Illusionsparteien koennen sich als andere race tarnen */
            if (u_race(u)->flags & RCF_SHAPESHIFT) {
                if (playerrace(trace)) {
                    u->irace = trace;
                    if ((u_race(u)->flags & RCF_SHAPESHIFTANY) && get_racename(u->attribs))
                        set_racename(&u->attribs, NULL);
                }
            }
            return 0;
        }
    }

    switch (findparam(s, u->faction->locale)) {
    case P_FACTION:
        /* TARNE PARTEI [NICHT|NUMMER abcd] */
        rule = rule_stealth_faction();
        if (!rule) {
            /* TARNE PARTEI is disabled */
            break;
        }
        s = gettoken(token, sizeof(token));
        if (rule & 1) {
            if (!s || *s == 0) {
                fset(u, UFL_ANON_FACTION);
                break;
            }
            else if (findparam(s, u->faction->locale) == P_NOT) {
                freset(u, UFL_ANON_FACTION);
                break;
            }
        }
        if (rule & 2) {
            if (get_keyword(s, u->faction->locale) == K_NUMBER) {
                s = gettoken(token, sizeof(token));
                int nr = -1;

                if (s) {
                    nr = atoi36(s);
                }
                if (!s || *s == 0 || nr == u->faction->no) {
                    a_removeall(&u->attribs, &at_otherfaction);
                    break;
                }
                else {
                    struct faction *f = findfaction(nr);
                    if (f == NULL) {
                        cmistake(u, ord, 66, MSG_EVENT);
                        break;
                    }
                    else {
                        set_factionstealth(u, f);
                        break;
                    }
                }
            }
        }
        cmistake(u, ord, 289, MSG_EVENT);
        break;
    case P_ANY:
    case P_NOT:
        /* TARNE ALLES (was nicht so alles geht?) */
        u_seteffstealth(u, -1);
        break;
    default:
        if (u_race(u)->flags & RCF_SHAPESHIFTANY) {
            set_racename(&u->attribs, s);
        }
        else {
            cmistake(u, ord, 289, MSG_EVENT);
        }
    }
    return 0;
}

static int top_skill(region * r, faction * f, ship * sh, skill_t sk)
{
    int value = 0;
    unit *u;

    for (u = r->units; u; u = u->next) {
        if (u->ship == sh && u->faction == f) {
            int s = effskill(u, sk, 0);
            value = _max(s, value);
        }
    }
    return value;
}

static int try_destruction(unit * u, unit * u2, const ship * sh, int skilldiff)
{
    const char *destruction_success_msg = "destroy_ship_0";
    const char *destruction_failed_msg = "destroy_ship_1";
    const char *destruction_detected_msg = "destroy_ship_2";
    const char *detect_failure_msg = "destroy_ship_3";
    const char *object_destroyed_msg = "destroy_ship_4";

    if (skilldiff == 0) {
        /* tell the unit that the attempt failed: */
        ADDMSG(&u->faction->msgs, msg_message(destruction_failed_msg, "ship unit",
            sh, u));
        /* tell the enemy about the attempt: */
        if (u2) {
            ADDMSG(&u2->faction->msgs, msg_message(detect_failure_msg, "ship", sh));
        }
        return 0;
    }
    else if (skilldiff < 0) {
        /* tell the unit that the attempt was detected: */
        ADDMSG(&u->faction->msgs, msg_message(destruction_detected_msg,
            "ship unit", sh, u));
        /* tell the enemy whodunit: */
        if (u2) {
            ADDMSG(&u2->faction->msgs, msg_message(detect_failure_msg, "ship", sh));
        }
        return 0;
    }
    else {
        /* tell the unit that the attempt succeeded */
        ADDMSG(&u->faction->msgs, msg_message(destruction_success_msg, "ship unit",
            sh, u));
        if (u2) {
            ADDMSG(&u2->faction->msgs, msg_message(object_destroyed_msg, "ship", sh));
        }
    }
    return 1;                     /* success */
}

static void sink_ship(region * r, ship * sh, unit * saboteur)
{
    unit **ui, *u;
    region *safety = r;
    int i;
    direction_t d;
    double probability = 0.0;
    message *sink_msg = NULL;
    faction *f;

    assert(r);
    assert(sh);
    assert(saboteur);
    for (f = NULL, u = r->units; u; u = u->next) {
        /* slight optimization to avoid dereferencing u->faction each time */
        if (f != u->faction) {
            f = u->faction;
            freset(f, FFL_SELECT);
        }
    }

    /* figure out what a unit's chances of survival are: */
    if (!fval(r->terrain, SEA_REGION)) {
        probability = CANAL_SWIMMER_CHANCE;
    }
    else {
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = rconnect(r, d);
            if (!fval(rn->terrain, SEA_REGION) && !move_blocked(NULL, r, rn)) {
                safety = rn;
                probability = OCEAN_SWIMMER_CHANCE;
                break;
            }
        }
    }
    for (ui = &r->units; *ui;) {
        unit *u = *ui;

        /* inform this faction about the sinking ship: */
        if (!fval(u->faction, FFL_SELECT)) {
            fset(u->faction, FFL_SELECT);
            if (sink_msg == NULL) {
                sink_msg = msg_message("sink_msg", "ship region", sh, r);
            }
            add_message(&f->msgs, sink_msg);
        }

        if (u->ship == sh) {
            int dead = 0;
            message *msg;

            /* if this fails, I misunderstood something: */
            for (i = 0; i != u->number; ++i)
                if (chance(probability))
                    ++dead;

            if (dead != u->number) {
                /* she will live. but her items get stripped */
                if (dead > 0) {
                    msg =
                        msg_message("sink_lost_msg", "dead region unit", dead, safety, u);
                }
                else {
                    msg = msg_message("sink_saved_msg", "region unit", safety, u);
                }
                leave_ship(u);
                if (r != safety) {
                    setguard(u, GUARD_NONE);
                }
                while (u->items) {
                    i_remove(&u->items, u->items);
                }
                move_unit(u, safety, NULL);
            }
            else {
                msg = msg_message("sink_lost_msg", "dead region unit", dead, NULL, u);
            }
            add_message(&u->faction->msgs, msg);
            msg_release(msg);
            if (dead == u->number) {
                if (remove_unit(ui, u) == 0) {
                    /* ui is already pointing at u->next */
                    continue;
                }
            }
        }
        ui = &u->next;
    }
    if (sink_msg)
        msg_release(sink_msg);
    /* finally, get rid of the ship */
    remove_ship(&sh->region->ships, sh);
}

int sabotage_cmd(unit * u, struct order *ord)
{
    const char *s;
    param_t p;
    ship *sh;
    unit *u2;
    int skdiff = INT_MAX;

    assert(u);
    assert(ord);

    init_order(ord);
    s = getstrtoken();

    p = findparam(s, u->faction->locale);

    switch (p) {
    case P_SHIP:
        sh = u->ship;
        if (!sh) {
            cmistake(u, u->thisorder, 144, MSG_EVENT);
            return 0;
        }
        u2 = ship_owner(sh);
        if (u2->faction != u->faction) {
            skdiff =
                effskill(u, SK_SPY, 0) - top_skill(u->region, u2->faction, sh, SK_PERCEPTION);
        }
        if (try_destruction(u, u2, sh, skdiff)) {
            sink_ship(u->region, sh, u);
        }
        break;
    default:
        cmistake(u, u->thisorder, 9, MSG_EVENT);
        return 0;
    }

    return 0;
}
