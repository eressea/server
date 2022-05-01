#include "spy.h"
#include "guard.h"
#include "laws.h"
#include "move.h"
#include "reports.h"
#include "study.h"

/* kernel includes */
#include <kernel/attrib.h>
#include <kernel/config.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/skills.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* attributes includes */
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/stealth.h>

/* util includes */
#include <util/base36.h>
#include <util/message.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/strings.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* in spy steht der Unterschied zwischen Wahrnehmung des Opfers und
* Spionage des Spions */
void spy_message(int spy, const unit * u, const unit * target)
{
    char status[32];
    sbstring sbs;

    sbs_init(&sbs, status, sizeof(status));
    report_status(target, u->faction->locale, &sbs);

    ADDMSG(&u->faction->msgs, msg_message("spyreport", "spy target status", u,
        target, status));
    if (spy > 20) {
        magic_t mtype = unit_get_magic(target);
        /* for mages, spells and magic school */
        if (mtype != M_GRAY && mtype < MAXMAGIETYP && mtype != M_NONE) {
            ADDMSG(&u->faction->msgs, msg_message("spyreport_mage", "spy target type", u,
                target, magic_school[mtype]));
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
        char buf[4096];
        size_t s, n = arrlen(target->skills);

        buf[0] = 0;
        for (s = 0; s != n; ++s) {
            skill* sv = target->skills + s;

            if (sv->level > 0) {
                found++;
                if (first == 1) {
                    first = 0;
                }
                else {
                    str_strlcat(buf, ", ", sizeof(buf));
                }
                str_strlcat(buf, (const char *)skillname((skill_t)sv->id, u->faction->locale),
                    sizeof(buf));
                str_strlcat(buf, " ", sizeof(buf));
                str_strlcat(buf, itoa10(eff_skill(target, sv, target->region)),
                    sizeof(buf));
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

    init_order(ord, NULL);
    getunit(r, u->faction, &target);

    if (!target || !cansee(u->faction, r, target, 0)) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "feedback_unit_not_found", NULL));
        return 0;
    }
    if (effskill(u, SK_SPY, NULL) < 1) {
        cmistake(u, u->thisorder, 39, MSG_EVENT);
        return 0;
    }
    /* Die Grundchance fuer einen erfolgreichen Spionage-Versuch ist 10%.
     * Fuer jeden Talentpunkt, den das Spionagetalent das Tarnungstalent
     * des Opfers uebersteigt, erhoeht sich dieses um 5%*/
    spy = effskill(u, SK_SPY, NULL) - effskill(target, SK_STEALTH, r);
    spychance = 0.1 + fmax(spy * 0.05, 0.0);

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
        - (effskill(u, SK_STEALTH, NULL) + effskill(u, SK_SPY, NULL) / 2);

    if (invisible(u, target) >= u->number) {
        if (observe > 0) observe = 0;
    }

    /* Anschliessend wird - unabhaengig vom Erfolg - gewuerfelt, ob der
     * Spionageversuch bemerkt wurde. Die Wahrscheinlich dafuer ist (100 -
     * SpionageSpion*5 + WahrnehmungOpfer*2)%. */
    observechance = 1.0 - (effskill(u, SK_SPY, NULL) * 0.05)
        + (effskill(target, SK_PERCEPTION, NULL) * 0.02);

    if (chance(observechance)) {
        ADDMSG(&target->faction->msgs, msg_message("spydetect",
            "spy target", observe > 0 ? u : NULL, target));
    }
    return 0;
}

static bool can_set_factionstealth(const unit * u, const faction * f)
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
                            return true;
                    }
                }
                ru = ru->next;
            }
        }
        mu = mu->nextF;
    }
    return true;
}

void set_factionstealth(unit *u, faction *f) {
    attrib *a = a_find(u->attribs, &at_otherfaction);
    if (!a)
        a = a_add(&u->attribs, make_otherfaction(f));
    else
        a->data.v = f;
}

static void stealth_race(unit *u, const char *s) {
    const race *trace;

    trace = findrace(s, u->faction->locale);
    if (trace) {
        /* demons can cloak as other player-races */
        if (u_race(u) == get_race(RC_DAEMON)) {
            if (playerrace(trace)) {
                u->irace = trace;
            }
        }
        /* Singdrachen k�nnen sich nur als Drachen tarnen */
        else if (u_race(u) == get_race(RC_SONGDRAGON)
            || u_race(u) == get_race(RC_BIRTHDAYDRAGON)) {
            if (trace == get_race(RC_SONGDRAGON) || trace == get_race(RC_FIREDRAGON)
                || trace == get_race(RC_DRAGON) || trace == get_race(RC_WYRM)) {
                u->irace = trace;
            }
        }

        /* Schablonen k�nnen sich als alles m�gliche tarnen */
        else if (u_race(u)->flags & RCF_SHAPESHIFT) {
            u->irace = trace;
            set_racename(&u->attribs, NULL);
        }
    }
    else {
        if (u_race(u)->flags & RCF_SHAPESHIFT) {
            set_racename(&u->attribs, s);
        }
    }
}

int setstealth_cmd(unit * u, struct order *ord)
{
    char token[64];
    const char *s;

    init_order(ord, NULL);
    s = gettoken(token, sizeof(token));

    /* Tarne ohne Parameter: Setzt maximale Tarnung */

    if (s == NULL || *s == 0) {
        u_seteffstealth(u, -1);
    }
    else if (isdigit(*(const unsigned char *)s)) {
        /* Tarnungslevel setzen */
        int level = atoi((const char *)s);
        if (level > effskill(u, SK_STEALTH, NULL)) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_lowstealth", ""));
        }
        else {
            u_seteffstealth(u, level);
        }
    }
    else {
        switch (findparam(s, u->faction->locale)) {
        case P_FACTION:
            /* TARNE PARTEI [NICHT|NUMMER abcd] */
            s = gettoken(token, sizeof(token));
            if (rule_stealth_anon()) {
                if (!s || *s == 0) {
                    u->flags |= UFL_ANON_FACTION;
                    break;
                }
                else if (findparam(s, u->faction->locale) == P_NOT) {
                    u->flags &= ~UFL_ANON_FACTION;
                    break;
                }
            }
            if (rule_stealth_other()) {
                if (get_keyword(s, u->faction->locale) == K_NUMBER) {
                    int nr = -1;

                    s = gettoken(token, sizeof(token));
                    if (s) {
                        nr = atoi36(s);
                    }
                    if (!s || *s == 0 || nr == u->faction->no) {
                        a_removeall(&u->attribs, &at_otherfaction);
                        break;
                    }
                    else {
                        struct faction *f = findfaction(nr);
                        if (f == NULL || !can_set_factionstealth(u, f)) {
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
            if (skill_enabled(SK_STEALTH)) { /* hack! E3 erlaubt keine Tarnung */
                stealth_race(u, s);
            }
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
            int s = effskill(u, sk, NULL);
            if (value < s) value = s;
        }
    }
    return value;
}

void sink_ship(ship * sh)
{
    unit *u;
    region *r;
    message *sink_msg = NULL;
    faction *f;

    assert(sh && sh->region);
    r = sh->region;

    for (f = NULL, u = r->units; u; u = u->next) {
        /* slight optimization to avoid dereferencing u->faction each time */
        if (f != u->faction) {
            f = u->faction;
            f->flags &= ~FFL_SELECT;
        }
    }

    for (f = NULL, u = r->units; u; u = u->next) {
        /* inform this faction about the sinking ship: */
        if (u->ship == sh) {
            if (f != u->faction) {
                f = u->faction;
                if (!(f->flags & FFL_SELECT)) {
                    f->flags |= FFL_SELECT;
                    if (sink_msg == NULL) {
                        sink_msg = msg_message("sink_msg", "ship region", sh, r);
                    }
                    add_message(&f->msgs, sink_msg);
                }
            }
        }
        else if (f != NULL) {
            break;
        }
    }
    if (sink_msg) {
        msg_release(sink_msg);
    }
}
