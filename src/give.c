/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2014   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

 */
#include <platform.h>
#include <kernel/config.h>
#include "give.h"

#include "economy.h"
#include "laws.h"

#include <spells/unitcurse.h>

 /* attributes includes */
#include <attributes/racename.h>

 /* kernel includes */
#include <kernel/ally.h>
#include <kernel/build.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/log.h>
#include <util/parser.h>

/* libc includes */
#include <assert.h>
#include <limits.h>
#include <stdlib.h>

/* Wieviel Fremde eine Partei pro Woche aufnehmen kangiven */
#define RESERVE_DONATIONS       /* shall we reserve objects given to us by other factions? */
#define RESERVE_GIVE            /* reserve anything that's given from one unit to another? */

static int max_transfers(void) {
    return config_get_int("rules.give.max_men", 5);
}

static int GiveRestriction(void)
{
    return config_get_int("GiveRestriction", 0);
}

static void feedback_give_not_allowed(unit * u, order * ord)
{
    ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_give_forbidden",
        ""));
}

static bool can_give(const unit * u, const unit * u2, const item_type * itype, int mask)
{
    if (u2) {
        if (u2->number==0 && !fval(u2, UFL_ISNEW)) {
            /* https://bugs.eressea.de/view.php?id=2230
             * cannot give anything to dead units */
            return false;
        } else if (u->faction != u2->faction) {
            int rule = rule_give();
            if (itype) {
                assert(mask == 0);
                if (itype->rtype->ltype)
                    mask |= GIVE_LUXURIES;
                else if (fval(itype, ITF_HERB))
                    mask |= GIVE_HERBS;
                else
                    mask |= GIVE_GOODS;
            }
            return (rule & mask) != 0;
        }
    }
    else {
        int rule = rule_give();
        return (rule & GIVE_PEASANTS) != 0;
    }
    return true;
}

static void
add_give(unit * u, unit * u2, int given, int received,
const resource_type * rtype, struct order *ord, int error)
{
    if (error) {
        cmistake(u, ord, error, MSG_COMMERCE);
    }
    else if (u2 == NULL) {
        ADDMSG(&u->faction->msgs,
            msg_message("give_peasants", "unit resource amount", u, rtype, given));
    }
    else if (u2->faction != u->faction) {
        message *msg;

        msg =
            msg_message("give", "unit target resource amount", u, u2, rtype, given);
        add_message(&u->faction->msgs, msg);
        msg_release(msg);

        msg =
            msg_message("receive", "unit target resource amount", u, u2, rtype,
            received);
        add_message(&u2->faction->msgs, msg);
        msg_release(msg);
    }
}

static void add_give_person(unit * u, unit * u2, int given,
    struct order *ord, int error)
{
    assert(u2);
    if (error) {
        cmistake(u, ord, error, MSG_COMMERCE);
    }
    else if (u2->faction != u->faction) {
        message *msg;

        msg = msg_message("give_person", "unit target amount",
            u, u2, given);
        add_message(&u->faction->msgs, msg);
        msg_release(msg);

        msg = msg_message("receive_person", "unit target amount",
            u, u2, given);
        add_message(&u2->faction->msgs, msg);
        msg_release(msg);
    }
}

static bool limited_give(const item_type * type)
{
    /* trade only money 2:1, if at all */
    return (type->rtype == get_resourcetype(R_SILVER));
}

int give_quota(const unit * src, const unit * dst, const item_type * type,
    int n)
{
    double divisor;

    if (!limited_give(type)) {
        return n;
    }
    if (dst && src && src->faction != dst->faction) {
        divisor = config_get_flt("rules.items.give_divisor", 1);
        assert(divisor <= 0 || divisor >= 1);
        if (divisor >= 1) {
            /* predictable > correct: */
            int x = (int)(n / divisor);
            return x;
        }
    }
    return n;
}

static void
give_horses(unit * s, const item_type * itype, int n)
{
    region *r = s->region;
    if (r->land) {
        rsethorses(r, rhorses(r) + n);
    }
}

static void
give_money(unit * s, const item_type * itype, int n)
{
    region *r = s->region;
    if (r->land) {
        rsetmoney(r, rmoney(r) + n);
    }
}

int
give_item(int want, const item_type * itype, unit * src, unit * dest,
struct order *ord)
{
    short error = 0;
    int n, delta;

    assert(itype != NULL);
    n = get_pooled(src, item2resource(itype), GET_SLACK | GET_POOLED_SLACK, want);
    n = MIN(want, n);
    delta = n;
    if (dest && src->faction != dest->faction
        && src->faction->age < GiveRestriction()) {
        if (ord != NULL) {
            ADDMSG(&src->faction->msgs, msg_feedback(src, ord, "giverestriction",
                "turns", GiveRestriction()));
        }
        return -1;
    }
    else if (n == 0) {
        int reserve = get_reservation(src, itype);
        if (reserve) {
            ADDMSG(&src->faction->msgs, msg_feedback(src, ord, "nogive_reserved", "resource reservation",
                itype->rtype, reserve));
            return -1;
        }
        error = 36;
    }
    else if (itype->flags & ITF_CURSED) {
        error = 25;
    }
    else {
        int use = use_pooled(src, item2resource(itype), GET_SLACK, n);

        if (use < n)
            use +=
            use_pooled(src, item2resource(itype), GET_POOLED_SLACK,
            n - use);
        if (dest) {
            delta = give_quota(src, dest, itype, n);
            i_change(&dest->items, itype, delta);
#ifdef RESERVE_GIVE
#ifdef RESERVE_DONATIONS
            change_reservation(dest, itype, delta);
#else
            if (src->faction == dest->faction) {
                change_reservation(dest, item2resource(itype), r);
            }
#endif
#endif
#if MUSEUM_MODULE && defined(TODO)
            /* TODO: use a trigger for the museum warden! */
            if (a_find(dest->attribs, &at_warden)) {
                warden_add_give(src, dest, itype, delta);
            }
#endif
        }
        else {
            /* return horses to the region */
            if (itype->construction && itype->flags & ITF_ANIMAL) {
                if (itype->construction->skill == SK_HORSE_TRAINING) {
                    give_horses(src, itype, n);
                }
            }
            else if (itype->rtype == get_resourcetype(R_SILVER)) {
                give_money(src, itype, n);
            }
        }
    }
    add_give(src, dest, n, delta, item2resource(itype), ord, error);
    if (error)
        return -1;
    return 0;
}

static bool unit_has_cursed_item(const unit * u)
{
    item *itm = u->items;
    while (itm) {
        if (fval(itm->type, ITF_CURSED) && itm->number > 0)
            return true;
        itm = itm->next;
    }
    return false;
}

static bool can_give_men(const unit *u, const unit *dst, order *ord, message **msg) {
    if (unit_has_cursed_item(u)) {
        if (msg) *msg = msg_error(u, ord, 78);
    }
    else if (has_skill(u, SK_MAGIC)) {
        /* cannot give units to and from magicians */
        if (msg) *msg = msg_error(u, ord, 158);
    }
    else if (fval(u, UFL_HUNGER)) {
        /* hungry people cannot be given away */
        if (msg) *msg = msg_error(u, ord, 73);
    }
    else if (fval(u, UFL_LOCKED) || is_cursed(u->attribs, &ct_slavery)) {
        if (msg) *msg = msg_error(u, ord, 74);
    }
    else {
        return true;
    }
    return false;
}

static bool rule_transfermen(void)
{
    int rule = config_get_int("rules.transfermen", 1);
    return rule != 0;
}

message * give_men(int n, unit * u, unit * u2, struct order *ord)
{
    ship *sh;
    int k = 0;
    int error = 0;
    message * msg;
    int maxt = max_transfers();

    assert(u2); /* use disband_men for GIVE 0 */

    if (!can_give_men(u, u2, ord, &msg)) {
        return msg;
    }

    if (u->faction != u2->faction && u->faction->age < GiveRestriction()) {
        return msg_feedback(u, ord, "giverestriction",
            "turns", GiveRestriction());
    }
    else if (u == u2) {
        error = 10;
    }
    else if (u2->number && (fval(u, UFL_HERO) != fval(u2, UFL_HERO))) {
        /* heroes may not be given to non-heroes and vice versa */
        error = 75;
    }
    else if (unit_has_cursed_item(u2)) {
        error = 78;
    }
    else if (fval(u2, UFL_LOCKED) || is_cursed(u2->attribs, &ct_slavery)) {
        error = 75;
    }
    else if (!ucontact(u2, u)) {
        return msg_feedback(u, ord, "feedback_no_contact",
            "target", u2);
    }
    else if (has_skill(u2, SK_MAGIC)) {
        /* cannot give units to and from magicians */
        error = 158;
    }
    else if (fval(u, UFL_WERE) != fval(u2, UFL_WERE)) {
        /* werewolves can't be given to non-werewolves and vice-versa */
        error = 312;
    }
    else if (u2->number != 0 && u_race(u2) != u_race(u)) {
        log_debug("faction %s attempts to give %s to %s.\n", itoa36(u->faction->no), u_race(u)->_name, u_race(u2)->_name);
        error = 139;
    }
    else if (get_racename(u2->attribs) || get_racename(u->attribs)) {
        error = 139;
    }
    else if (u2->faction != u->faction && !rule_transfermen()) {
        error = 74;
    }
    else {
        if (n > u->number)
            n = u->number;
        if (n + u2->number > UNIT_MAXSIZE) {
            n = UNIT_MAXSIZE - u2->number;
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_unit_size",
                "maxsize", UNIT_MAXSIZE));
            assert(n >= 0);
        }
        if (n == 0) {
            error = 96;
        }
        else if (u->faction != u2->faction) {
            if (maxt >= 0 && u2->faction->newbies + n > maxt) {
                error = 129;
            }
            else if (u_race(u) != u2->faction->race) {
                if (u2->faction->race != get_race(RC_HUMAN)) {
                    error = 120;
                }
                else if (count_migrants(u2->faction) + n >
                    count_maxmigrants(u2->faction)) {
                    error = 128;
                }
                else if (has_limited_skills(u) || has_limited_skills(u2)) {
                    error = 154;
                }
                else if (u2->number != 0) {
                    error = 139;
                }
            }
        }
    }

    if (has_skill(u, SK_ALCHEMY) || has_skill(u2, SK_ALCHEMY)) {
        k = count_skill(u2->faction, SK_ALCHEMY);

        /* Falls die Zieleinheit keine Alchemisten sind, werden sie nun
         * welche. */
        if (!has_skill(u2, SK_ALCHEMY) && has_skill(u, SK_ALCHEMY))
            k += u2->number;

        /* Wenn in eine Alchemisteneinheit Personen verschoben werden */
        if (has_skill(u2, SK_ALCHEMY) && !has_skill(u, SK_ALCHEMY))
            k += n;

        /* Wenn Parteigrenzen überschritten werden */
        if (u2->faction != u->faction)
            k += n;

        /* wird das Alchemistenmaximum ueberschritten ? */

        if (k > skill_limit(u2->faction, SK_ALCHEMY)) {
            error = 156;
        }
    }

    if (error == 0) {
        if (u2->number == 0) {
            set_racename(&u2->attribs, get_racename(u->attribs));
            u_setrace(u2, u_race(u));
            u2->irace = u->irace;
            if (fval(u, UFL_HERO)) {
                fset(u2, UFL_HERO);
            }
            else {
                freset(u2, UFL_HERO);
            }
        }

        /* Einheiten von Schiffen können nicht NACH in von
            * Nicht-alliierten bewachten Regionen ausführen */
        sh = leftship(u);
        if (sh) {
            set_leftship(u2, sh);
        }
        transfermen(u, u2, n);
        if (maxt >= 0 && u->faction != u2->faction) {
            u2->faction->newbies += n;
        }
    }
    if (error > 0) {
        return msg_error(u, ord, error);
    }
    else if (u2->faction != u->faction) {
        message *msg = msg_message("give_person", "unit target amount", u, u2, n);
        add_message(&u2->faction->msgs, msg);
        return msg;
    }
    return NULL;
}

message * disband_men(int n, unit * u, struct order *ord) {
    message * msg;
    static const race *rc_snotling;
    static int rccache;

    if (rc_changed(&rccache)) {
        rc_snotling = get_race(RC_SNOTLING);
    }

    if (u_race(u) == rc_snotling) {
        /* snotlings may not be given to the peasants. */
        return msg_error(u, ord, 307);
    }
    if (!can_give_men(u, NULL, ord, &msg)) {
        return msg;
    }
    transfermen(u, NULL, n);
    if (fval(u->region->terrain, SEA_REGION)) {
        return msg_message("give_person_ocean", "unit amount", u, n);
    }
    return msg_message("give_person_peasants", "unit amount", u, n);
}

void give_unit(unit * u, unit * u2, order * ord)
{
    int maxt = max_transfers();

    assert(u);
    if (!rule_transfermen() && u2 && u->faction != u2->faction) {
        cmistake(u, ord, 74, MSG_COMMERCE);
        return;
    }

    if (unit_has_cursed_item(u)) {
        cmistake(u, ord, 78, MSG_COMMERCE);
        return;
    }

    if (fval(u, UFL_HERO)) {
        cmistake(u, ord, 75, MSG_COMMERCE);
        return;
    }
    if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER)) {
        cmistake(u, ord, 74, MSG_COMMERCE);
        return;
    }

    if (u2 == NULL) {
        region *r = u->region;
        message *msg;
        if (fval(r->terrain, SEA_REGION)) {
            msg = disband_men(u->number, u, ord);
            if (msg) {
                ADDMSG(&u->faction->msgs, msg);
            }
            else {
                cmistake(u, ord, 152, MSG_COMMERCE);
            }
        }
        else {
            unit *u3;

            for (u3 = r->units; u3; u3 = u3->next)
                if (u3->faction == u->faction && u != u3)
                    break;

            if (u3) {
                while (u->items) {
                    item *iold = i_remove(&u->items, u->items);
                    item *inew = *i_find(&u3->items, iold->type);
                    if (inew == NULL)
                        i_add(&u3->items, iold);
                    else {
                        inew->number += iold->number;
                        i_free(iold);
                    }
                }
            }
            msg = disband_men(u->number, u, ord);
            if (msg) {
                ADDMSG(&u->faction->msgs, msg);
            }
            else {
                cmistake(u, ord, 153, MSG_COMMERCE);
            }
        }
        return;
    } else {
        int err = checkunitnumber(u2->faction, 1);
        if (err) {
            if (err == 1) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, ord,
                        "too_many_units_in_alliance",
                        "allowed", rule_alliance_limit()));
            }
            else {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, ord,
                        "too_many_units_in_faction",
                        "allowed", rule_faction_limit()));
            }
            return;
        }
    }

    if (!alliedunit(u2, u->faction, HELP_GIVE) && ucontact(u2, u) == 0) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "feedback_no_contact",
            "target", u2));
        return;
    }
    if (u->number == 0) {
        cmistake(u, ord, 105, MSG_COMMERCE);
        return;
    }
    if (maxt >= 0 && u->faction != u2->faction) {
        if (u2->faction->newbies + u->number > maxt) {
            cmistake(u, ord, 129, MSG_COMMERCE);
            return;
        }
    }
    if (u_race(u) != u2->faction->race) {
        if (u2->faction->race != get_race(RC_HUMAN)) {
            cmistake(u, ord, 120, MSG_COMMERCE);
            return;
        }
        if (count_migrants(u2->faction) + u->number >
            count_maxmigrants(u2->faction)) {
            cmistake(u, ord, 128, MSG_COMMERCE);
            return;
        }
        if (has_limited_skills(u)) {
            cmistake(u, ord, 154, MSG_COMMERCE);
            return;
        }
    }
    if (has_skill(u, SK_MAGIC)) {
        sc_mage *mage;
        if (count_skill(u2->faction, SK_MAGIC) + u->number >
            skill_limit(u2->faction, SK_MAGIC)) {
            cmistake(u, ord, 155, MSG_COMMERCE);
            return;
        }
        mage = get_mage_depr(u);
        if (!mage || u2->faction->magiegebiet != mage->magietyp) {
            cmistake(u, ord, 157, MSG_COMMERCE);
            return;
        }
    }
    if (has_skill(u, SK_ALCHEMY)
        && count_skill(u2->faction, SK_ALCHEMY) + u->number >
        skill_limit(u2->faction, SK_ALCHEMY)) {
        cmistake(u, ord, 156, MSG_COMMERCE);
        return;
    }
    add_give_person(u, u2, u->number, ord, 0);
    u_setfaction(u, u2->faction);
    u2->faction->newbies += u->number;
}

bool can_give_to(unit *u, unit *u2) {
    /* Damit Tarner nicht durch die Fehlermeldung enttarnt werden können */
    if (!u2) {
        return false;
    }
    if (u2 && !alliedunit(u2, u->faction, HELP_GIVE)
        && !cansee(u->faction, u->region, u2, 0) && !ucontact(u2, u)
        && !fval(u2, UFL_TAKEALL)) {
        return false;
    }
    return true;
}

void give_cmd(unit * u, order * ord)
{
    char token[128];
    region *r = u->region;
    unit *u2;
    const char *s;
    int err, n;
    const item_type *itype;
    param_t p;
    plane *pl;
    message *msg;
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_GIVE);
    err = getunit(r, u->faction, &u2);
    s = gettoken(token, sizeof(token));
    n = s ? atoip(s) : 0;
    p = (n > 0) ? NOPARAM : findparam(s, u->faction->locale);

    /* first, do all the ones that do not require HELP_GIVE or CONTACT */
    if (p == P_CONTROL) {
        /* handled in give_control_cmd */
        return;
    }

    if (err == GET_NOTFOUND || (err != GET_PEASANTS && !can_give_to(u, u2))) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "feedback_unit_not_found", ""));
        return;
    }
    if (u == u2) {
        cmistake(u, ord, 8, MSG_COMMERCE);
        return;
    }

    msg = check_give(u, u2, ord);
    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
        return;
    }

    /* UFL_TAKEALL ist ein grober Hack. Generalisierung tut not, ist aber nicht
    * wirklich einfach. */
    pl = rplane(r);
    if (pl && fval(pl, PFL_NOGIVE) && (!u2 || !fval(u2, UFL_TAKEALL))) {
        cmistake(u, ord, 268, MSG_COMMERCE);
        return;
    }

    if (u2 && !alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
        cmistake(u, ord, 40, MSG_COMMERCE);
        return;
    }
    else if (p == NOPARAM) {
        /* the most likely case: giving items to someone.
         * let's catch this and save ourselves the rest of the param_t checks.
         */
    } 
    else if (p == P_HERBS) {
        bool given = false;
        if (!can_give(u, u2, NULL, GIVE_HERBS)) {
            feedback_give_not_allowed(u, ord);
            return;
        }
        if (u2 && !(u_race(u2)->ec_flags & ECF_GETITEM)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "race_notake", "race", u_race(u2)));
            return;
        }
        if (u->items) {
            item **itmp = &u->items;
            while (*itmp) {
                item *itm = *itmp;
                const item_type *itype = itm->type;
                if (fval(itype, ITF_HERB) && itm->number > 0) {
                    /* give_item ändert im fall,das man alles übergibt, die
                    * item-liste der unit, darum continue vor pointerumsetzten */
                    if (give_item(itm->number, itm->type, u, u2, ord) == 0) {
                        given = true;
                        continue;
                    }
                }
                itmp = &itm->next;
            }
        }
        if (!given) {
            cmistake(u, ord, 38, MSG_COMMERCE);
        }
        return;
    }

    else if (p == P_ZAUBER) {
        cmistake(u, ord, 7, MSG_COMMERCE);
        /* geht nimmer */
        return;
    }

    else if (p == P_UNIT) {       /* Einheiten uebergeben */
        if (!(u_race(u)->ec_flags & ECF_GIVEUNIT)) {
            cmistake(u, ord, 167, MSG_COMMERCE);
            return;
        }

        give_unit(u, u2, ord);
        return;
    }

    else if (p == P_ANY) {
        const char *s;

        if (!can_give(u, u2, NULL, GIVE_ALLITEMS)) {
            feedback_give_not_allowed(u, ord);
            return;
        }
        s = gettoken(token, sizeof(token));
        if (!s || *s == 0) {              /* GIVE ALL items that you have */

            /* do these checks once, not for each item we have: */
            if (u2 && !(u_race(u2)->ec_flags & ECF_GETITEM)) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, ord, "race_notake", "race", u_race(u2)));
                return;
            }

            /* für alle items einmal prüfen, ob wir mehr als von diesem Typ
            * reserviert ist besitzen und diesen Teil dann übergeben */
            if (u->items) {
                item **itmp = &u->items;
                while (*itmp) {
                    item *itm = *itmp;
                    const item_type *itype = itm->type;
                    if (itm->number > 0
                        && itm->number - get_reservation(u, itype) > 0) {
                        n = itm->number - get_reservation(u, itype);
                        if (give_item(n, itype, u, u2, ord) == 0) {
                            if (*itmp != itm)
                                continue;
                        }
                    }
                    itmp = &itm->next;
                }
            }
        }
        else {
            if (isparam(s, u->faction->locale, P_PERSON)) {
                if (!(u_race(u)->ec_flags & ECF_GIVEPERSON)) {
                    ADDMSG(&u->faction->msgs,
                        msg_feedback(u, ord, "race_noregroup", "race", u_race(u)));
                }
                else {
                    message * msg;
                    msg = u2 ? give_men(u->number, u, u2, ord) : disband_men(u->number, u, ord);
                    if (msg) {
                        ADDMSG(&u->faction->msgs, msg);
                    }
                }
            }
            else if (u2 && !(u_race(u2)->ec_flags & ECF_GETITEM)) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, ord, "race_notake", "race", u_race(u2)));
            }
            else {
                itype = finditemtype(s, u->faction->locale);
                if (itype != NULL) {
                    item *i = *i_find(&u->items, itype);
                    if (i != NULL) {
                        if (can_give(u, u2, itype, 0)) {
                            n = i->number - get_reservation(u, itype);
                            give_item(n, itype, u, u2, ord);
                        }
                        else {
                            feedback_give_not_allowed(u, ord);
                        }
                    }
                }
            }
        }
        return;
    }
    else if (p == P_EACH) {
        if (u2 == NULL) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "peasants_give_invalid", ""));
            return;
        }
        s = gettoken(token, sizeof(token));          /* skip one ahead to get the amount. */
        n = s ? atoip(s) : 0;   /* n: Anzahl */
        n *= u2->number;
    }
    s = gettoken(token, sizeof(token));

    if (s == NULL) {
        cmistake(u, ord, 113, MSG_COMMERCE);
        return;
    }

    if (isparam(s, u->faction->locale, P_PERSON)) {
        message * msg;
        if (!(u_race(u)->ec_flags & ECF_GIVEPERSON)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "race_noregroup", "race", u_race(u)));
            return;
        }
        n = MIN(u->number, n);
        msg = u2 ? give_men(n, u, u2, ord) : disband_men(n, u, ord);
        if (msg) {
            ADDMSG(&u->faction->msgs, msg);
        }
        return;
    }

    if (u2 != NULL) {
        if (!(u_race(u2)->ec_flags & ECF_GETITEM)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "race_notake", "race", u_race(u2)));
            return;
        }
    }

    itype = finditemtype(s, u->faction->locale);
    if (itype != NULL) {
        if (can_give(u, u2, itype, 0)) {
            give_item(n, itype, u, u2, ord);
        }
        else {
            feedback_give_not_allowed(u, ord);
        }
        return;
    }
    cmistake(u, ord, 123, MSG_COMMERCE);
}

message *check_give(const unit *u, const unit *u2, order * ord) {
    if (!can_give(u, u2, NULL, GIVE_ALLITEMS)) {
        return msg_feedback(u, ord, "feedback_give_forbidden", "");
    }
    return 0;
}

