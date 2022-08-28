#include <kernel/config.h>
#include "give.h"

#include "contact.h"
#include "economy.h"
#include "laws.h"

#include <spells/unitcurse.h>

 /* attributes includes */
#include <attributes/racename.h>
#include <attributes/otherfaction.h>

 /* kernel includes */
#include <kernel/attrib.h>
#include <kernel/event.h>
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
#include <util/base36.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/param.h>
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
        if (u2->number==0 && fval(u2, UFL_DEAD)) {
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
    if (!limited_give(type)) {
        return n;
    }
    if (dst && src && src->faction != dst->faction) {
        static int config;
        static int divisor = 1;
        if (config_changed(&config)) {
            divisor = config_get_int("rules.items.give_divisor", divisor);
        }
        if (divisor >= 1) {
            /* predictable > correct: */
            return n / divisor;
        }
    }
    return n;
}

static void
give_horses(unit * s, const item_type * itype, int n)
{
    region *r = s->region;
    UNUSED_ARG(itype);
    if (r->land) {
        rsethorses(r, rhorses(r) + n);
    }
}

static void
give_money(unit * s, const item_type * itype, int n)
{
    region *r = s->region;
    UNUSED_ARG(itype);
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
    if (n > want) n = want;
    delta = n;
    if (dest && src->faction != dest->faction
        && faction_age(src->faction) < GiveRestriction()) {
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
    else if ((itype->flags & ITF_CURSED) && (dest == NULL || src->faction != dest->faction)) {
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

bool can_give_men(const unit *u, const unit *dst, order *ord, message **msg)
{
    UNUSED_ARG(dst);
    if (has_skill(u, SK_MAGIC)) {
        /* cannot give units to and from magicians */
        if (msg) *msg = msg_error(u, ord, 158);
    }
    else if (dst && fval(u, UFL_HUNGER)) {
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

bool rule_transfermen(void)
{
    int rule = config_get_int("rules.transfermen", 1);
    return rule != 0;
}

static void transfer_ships(ship *s1, ship *s2, int n)
{
    assert(n <= s1->number);
    s2->damage += s1->damage * n / s1->number;
    s2->size += s1->size * n / s1->number;
    s2->number += n;
    if (s1->coast != NODIRECTION) {
        s2->coast = s1->coast;
    }
    if (n == s1->number) {
        s1->number = 0;
    }
    else {
        scale_ship(s1, s1->number - n);
    }
}

static void transfer_units(ship *s1, ship *s2)
{
    region * r = s1->region;
    unit *u;
    for (u = r->units; u; u = u->next) {
        if (u->ship == s1) {
            leave_ship(u);
            u_set_ship(u, s2);
        }
    }
}

static bool ship_cursed(const ship *sh) {
    return a_find(sh->attribs, &at_curse) != NULL;
}

message * give_ship(unit *u1, unit *u2, int n, order *ord)
{
    assert(u1->ship);
    assert(n > 0 && n <= u1->ship->number);
    if (u1->ship->type->range < 3) {
        /* Keine Boote und anderes Kleinzeug erlaubt */
        return msg_error(u1, ord, 326);
    }
    if (ship_cursed(u1->ship)) {
        return msg_error(u1, ord, 323);
    }
    if (u1 != ship_owner(u1->ship)) {
        return msg_error(u1, ord, 146);
    }
    if (u2 == NULL) {
        if (u1->region->land || n < u1->ship->number) {
            ship * sh = new_ship(u1->ship->type, u1->region, u1->faction->locale);
            scale_ship(sh, 0);
            transfer_ships(u1->ship, sh, n);
        }
        else {
            return msg_error(u1, ord, 327);
        }
    } else {
        if (u1->faction != u2->faction) {
            return msg_error(u1, ord, 324);
        }
        if (fval(u_race(u2), RCF_CANSAIL)) {
            if (u2->ship) {
                if (u2->ship == u1->ship) {
                    ship * sh = new_ship(u1->ship->type, u1->region, u1->faction->locale);
                    scale_ship(sh, 0);
                    leave_ship(u2);
                    u_set_ship(u2, sh);
                } else {
                    if (u2 != ship_owner(u2->ship)) {
                        return msg_error(u1, ord, 146);
                    }
                    if (u2->ship->type != u1->ship->type) {
                        return msg_error(u1, ord, 322);
                    }
                    if (ship_cursed(u2->ship)) {
                        return msg_error(u1, ord, 323);
                    }
                    if (u1->ship->coast != u2->ship->coast) {
                        if (u1->ship->coast != NODIRECTION) {
                            if (u2->ship->coast == NODIRECTION) {
                                u2->ship->coast = u1->ship->coast;
                            }
                            else {
                                return msg_error(u1, ord, 328);
                            }
                        }
                    }
                }
                if (n < u1->ship->number) {
                    transfer_ships(u1->ship, u2->ship, n);
                }
                else {
                    transfer_ships(u1->ship, u2->ship, n);
                    transfer_units(u1->ship, u2->ship);
                }
            }
            else {
                if (u2->building) {
                    leave_building(u2);
                }
                if (n < u1->ship->number) {
                    ship * sh = new_ship(u1->ship->type, u1->region, u1->faction->locale);
                    scale_ship(sh, 0);
                    u_set_ship(u2, sh);
                    transfer_ships(u1->ship, sh, n);
                }
                else {
                    u_set_ship(u2, u1->ship);
                    ship_set_owner(u2);
                }
            }
        }
        else {
            return msg_error(u1, ord, 233);
        }
    }
    return NULL;
}

message * give_men(int n, unit * u, unit * u2, struct order *ord)
{
    int error = 0;
    message * msg;
    int maxt = max_transfers();

    assert(u2); /* use disband_men for GIVE 0 */

    if (!can_give_men(u, u2, ord, &msg)) {
        return msg;
    }

    if (u->faction != u2->faction && faction_age(u->faction) < GiveRestriction()) {
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
        int k = faction_count_skill(u2->faction, SK_ALCHEMY);

        /* Falls die Zieleinheit keine Alchemisten sind, werden sie nun
         * welche. */
        if (!has_skill(u2, SK_ALCHEMY) && has_skill(u, SK_ALCHEMY))
            k += u2->number;

        /* Wenn in eine Alchemisteneinheit Personen verschoben werden */
        if (has_skill(u2, SK_ALCHEMY) && !has_skill(u, SK_ALCHEMY))
            k += n;

        /* Wenn Parteigrenzen ueberschritten werden */
        if (u2->faction != u->faction)
            k += n;

        /* wird das Alchemistenmaximum ueberschritten ? */

        if (k > faction_skill_limit(u2->faction, SK_ALCHEMY)) {
            error = 156;
        }
    }

    if (error == 0) {
        ship *sh = leftship(u);

        /* Einheiten von Schiffen koennen nicht NACH in von
        * Nicht-alliierten bewachten Regionen ausfuehren */
        if (sh) {
            set_leftship(u2, sh);
        }

        if (u2->number == 0) {
            const race* rc = u_race(u);
            u_setrace(u2, rc);
            if (rc == get_race(RC_DAEMON)) {
                u2->irace = u->irace;
            }
            if (fval(u, UFL_HERO)) {
                fset(u2, UFL_HERO);
            }
            else {
                freset(u2, UFL_HERO);
            }
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
        return add_message(&u2->faction->msgs,
            msg_message("give_person", "unit target amount", u, u2, n));
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

int give_unit_allowed(const unit * u)
{
    if (unit_has_cursed_item(u)) {
        return 78;
    }
    if (fval(u, UFL_HERO)) {
        return 75;
    }
    if (fval(u, UFL_LOCKED) || fval(u, UFL_HUNGER)) {
        return 74;
    }
    return 0;
}

static void transfer_unit(unit *u, unit *u2)
{
    faction *f = get_otherfaction(u);
    if (f == u2->faction) {
        set_otherfaction(u, NULL);
    }
    u_setfaction(u, u2->faction);
    u_freeorders(u);
    u2->faction->newbies += u->number;
}

void give_unit(unit * u, unit * u2, order * ord)
{
    int err, maxt = max_transfers();

    assert(u);
    if (!rule_transfermen() && u2 && u->faction != u2->faction) {
        cmistake(u, ord, 74, MSG_COMMERCE);
        return;
    }

    err = give_unit_allowed(u);
    if (err != 0) {
        cmistake(u, ord, err, MSG_COMMERCE);
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
        if (faction_count_skill(u2->faction, SK_MAGIC) + u->number >
            faction_skill_limit(u2->faction, SK_MAGIC)) {
            cmistake(u, ord, 155, MSG_COMMERCE);
            return;
        }
        if (u2->faction->magiegebiet != unit_get_magic(u)) {
            cmistake(u, ord, 157, MSG_COMMERCE);
            return;
        }
    }
    if (has_skill(u, SK_ALCHEMY)
        && faction_count_skill(u2->faction, SK_ALCHEMY) + u->number >
        faction_skill_limit(u2->faction, SK_ALCHEMY)) {
        cmistake(u, ord, 156, MSG_COMMERCE);
        return;
    }
    add_give_person(u, u2, u->number, ord, 0);
    transfer_unit(u, u2);
}

bool can_give_to(unit *u, unit *u2) {
    /* Damit Tarner nicht durch die Fehlermeldung enttarnt werden koennen */
    if (!u2 || u2->region != u->region) {
        return false;
    }
    if (u2 && !alliedunit(u2, u->faction, HELP_GIVE)
        && !cansee(u->faction, u->region, u2, 0) && !ucontact(u2, u)
        && !fval(u2, UFL_TAKEALL)) {
        return false;
    }
    return true;
}

static void give_all_items(unit *u, unit *u2, order *ord) { 
    char token[128];
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

        /* fuer alle items einmal pruefen, ob wir mehr als von diesem Typ
        * reserviert ist besitzen und diesen Teil dann uebergeben */
        if (u->items) {
            item **itmp = &u->items;
            while (*itmp) {
                item *itm = *itmp;
                const item_type *itype = itm->type;
                if (itm->number > 0
                    && itm->number - get_reservation(u, itype) > 0) {
                    int n = itm->number - get_reservation(u, itype);
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
        param_t p = get_param(s, u->faction->locale);
        if (p == P_PERSON) {
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
            const item_type *itype = finditemtype(s, u->faction->locale);
            if (itype != NULL) {
                item *i = *i_find(&u->items, itype);
                if (i != NULL) {
                    if (can_give(u, u2, itype, 0)) {
                        int n = i->number - get_reservation(u, itype);
                        give_item(n, itype, u, u2, ord);
                    }
                    else {
                        feedback_give_not_allowed(u, ord);
                    }
                }
            }
            else {
                cmistake(u, ord, 123, MSG_COMMERCE);
            }
        }
    }
}

void give_unit_cmd(unit* u, order* ord)
{
    char token[64];
    unit* u2;
    message* msg = NULL;
    int err;

    init_order(ord, NULL);
    err = getunit(u->region, u->faction, &u2);

    if (err == GET_NOTFOUND || (err != GET_PEASANTS && !can_give_to(u, u2))) {
        msg = msg_feedback(u, ord, "feedback_unit_not_found", NULL);
    }
    else {
        msg = check_give(u, u2, ord);
    }

    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
    }
    else if (!(u_race(u)->ec_flags & ECF_GIVEUNIT)) {
        cmistake(u, ord, 167, MSG_COMMERCE);
    }
    else {
        param_t p = get_param(gettoken(token, sizeof(token)), u->faction->locale);
        if (p == P_UNIT) {
            give_unit(u, u2, ord);
        }
    }
}

param_t give_cmd(unit * u, order * ord)
{
    char token[64];
    region *r = u->region;
    unit *u2;
    const char *s;
    int err, n;
    const item_type *itype;
    param_t p;
    plane *pl;
    message *msg;

    init_order(ord, NULL);

    err = getunit(r, u->faction, &u2);
    s = gettoken(token, sizeof(token));
    n = s ? atoip(s) : 0;
    p = (n > 0) ? NOPARAM : get_param(s, u->faction->locale);

    /* quick exit before any errors are generated: */
    if (p == P_UNIT || p == P_CONTROL) {
        /* handled in give_unit_cmd */
        return p;
    }

    if (err == GET_NOTFOUND || (err != GET_PEASANTS && !can_give_to(u, u2))) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "feedback_unit_not_found", NULL));
        return NOPARAM;
    }
    if (u == u2) {
        cmistake(u, ord, 8, MSG_COMMERCE);
        return NOPARAM;
    }

    /* first, do all the ones that do not require HELP_GIVE or CONTACT */
    if (p == P_SHIP) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "give_number_missing", "resource", "ship_p"));
        return p;
    }
    else if (p == P_PERSON) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "give_number_missing", "resource", "person_p"));
        return p;
    }

    msg = check_give(u, u2, ord);
    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
        return p;
    }

    /* UFL_TAKEALL ist ein grober Hack. Generalisierung tut not, ist aber nicht
    * wirklich einfach. */
    pl = rplane(r);
    if (pl && fval(pl, PFL_NOGIVE) && (!u2 || !fval(u2, UFL_TAKEALL))) {
        cmistake(u, ord, 268, MSG_COMMERCE);
        return p;
    }

    if (u2 && !alliedunit(u2, u->faction, HELP_GIVE) && !ucontact(u2, u)) {
        cmistake(u, ord, 40, MSG_COMMERCE);
        return p;
    }
    else if (p == NOPARAM) {
        /* the most likely case: giving items, ships, or persons.
         * let's catch this and save ourselves the rest of the param_t checks.
         */
    } 
    else if (p == P_HERBS) {
        bool given = false;
        if (!can_give(u, u2, NULL, GIVE_HERBS)) {
            feedback_give_not_allowed(u, ord);
        }
        else if (u2 && !(u_race(u2)->ec_flags & ECF_GETITEM)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "race_notake", "race", u_race(u2)));
        }
        else if (u->items) {
            item** itmp = &u->items;
            while (*itmp) {
                item* itm = *itmp;
                if (fval(itm->type, ITF_HERB) && itm->number > 0) {
                    /* give_item aendert im fall,das man alles uebergibt, die
                    * item-liste der unit, darum continue vor pointerumsetzten */
                    if (give_item(itm->number, itm->type, u, u2, ord) == 0) {
                        given = true;
                        continue;
                    }
                }
                itmp = &itm->next;
            }
            if (!given) {
                cmistake(u, ord, 38, MSG_COMMERCE);
            }
        }
        return p;
    }

    else if (p == P_ZAUBER) {
        cmistake(u, ord, 7, MSG_COMMERCE);
        /* geht nimmer */
        return p;
    }
    else if (p == P_ANY) {
        give_all_items(u, u2, ord);
        return p;
    }
    else if (p == P_EACH) {
        if (u2 == NULL) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "peasants_give_invalid", ""));
            return p;
        }
        s = gettoken(token, sizeof(token));          /* skip one ahead to get the amount. */
        n = s ? atoip(s) : 0;   /* n: Anzahl */
        n *= u2->number;
    }
    s = gettoken(token, sizeof(token));

    if (s == NULL) {
        cmistake(u, ord, 113, MSG_COMMERCE);
    }
    else {
        p = get_param(s, u->faction->locale);
        if (p == P_SHIP) {
            if (u->ship) {
                message* msg;
                if (n > u->ship->number) {
                    n = u->ship->number;
                }
                msg = give_ship(u, u2, n, ord);
                if (msg) {
                    ADDMSG(&u->faction->msgs, msg);
                }
            }
            else {
                cmistake(u, ord, 144, MSG_COMMERCE);
            }
        }
        else if (p == P_PERSON) {
            if (!(u_race(u)->ec_flags & ECF_GIVEPERSON)) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, ord, "race_noregroup", "race", u_race(u)));
            }
            else {
                if (n > u->number) {
                    n = u->number;
                }
                msg = u2 ? give_men(n, u, u2, ord) : disband_men(n, u, ord);
                if (msg) {
                    ADDMSG(&u->faction->msgs, msg);
                }
            }
        }
        else if (u2 != NULL && !(u_race(u2)->ec_flags & ECF_GETITEM)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "race_notake", "race", u_race(u2)));
        }
        else {
            itype = finditemtype(s, u->faction->locale);
            if (itype != NULL) {
                if (can_give(u, u2, itype, 0)) {
                    give_item(n, itype, u, u2, ord);
                }
                else {
                    feedback_give_not_allowed(u, ord);
                }
            }
            else {
                cmistake(u, ord, 123, MSG_COMMERCE);
            }
        }
    }
    return p;
}

message *check_give(const unit *u, const unit *u2, order * ord) {
    if (!can_give(u, u2, NULL, GIVE_ALLITEMS)) {
        return msg_feedback(u, ord, "feedback_give_forbidden", "");
    }
    return NULL;
}

static int reserve_i(unit* u, struct order* ord, int flags)
{
    if (u->number > 0) {
        char token[128];
        int use, count, res;
        const item_type* itype;
        const char* s;
        param_t p = NOPARAM;

        init_order(ord, NULL);
        s = gettoken(token, sizeof(token));
        count = s ? atoip(s) : 0;
        if (count == 0) {
            p = get_param(s, u->faction->locale);
            if (p == P_EACH) {
                count = getint() * u->number;
            }
        }
        s = gettoken(token, sizeof(token));
        itype = s ? finditemtype(s, u->faction->locale) : 0;
        if (itype == NULL)
            return 0;

        res = get_reservation(u, itype);
        set_resvalue(u, itype, 0);      /* make sure the pool is empty */

        if (p == P_ANY) {
            count = get_resource(u, itype->rtype);
        }
        if (count > 0) {
            use = use_pooled(u, itype->rtype, flags, count);
            if (use) {
                if (use > res) res = use;
                set_resvalue(u, itype, res);
                change_resource(u, itype->rtype, use);
                return use;
            }
        }
    }
    return 0;
}

int reserve_cmd(unit* u, struct order* ord) {
    if ((u_race(u)->ec_flags & ECF_GETITEM) == 0) {
        /* race cannot be given anything, reserve would be a loophole */
        return 0;
    }

    return reserve_i(u, ord, GET_DEFAULT);
}

int reserve_self(unit* u, struct order* ord) {
    return reserve_i(u, ord, GET_RESERVE | GET_SLACK);
}

