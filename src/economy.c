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

#include "alchemy.h"
#include "direction.h"
#include "give.h"
#include "laws.h"
#include "randenc.h"
#include "spy.h"
#include "move.h"
#include "monster.h"
#include "morale.h"
#include "reports.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/curse.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/resources.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rng.h>

#include <attributes/reduceproduction.h>
#include <attributes/racename.h>
#include <attributes/orcification.h>

/* libs includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

typedef struct request {
    struct request *next;
    struct unit *unit;
    struct order *ord;
    int qty;
    int no;
    union {
        bool goblin;             /* stealing */
        const struct luxury_type *ltype;    /* trading */
    } type;
} request;

static int working;

static request entertainers[1024];
static request *nextentertainer;
static int entertaining;

static int norders;
static request *oa;

#define RECRUIT_MERGE 1
static int rules_recruit = -1;

static void recruit_init(void)
{
    if (rules_recruit < 0) {
        rules_recruit = 0;
        if (get_param_int(global.parameters, "recruit.allow_merge", 1)) {
            rules_recruit |= RECRUIT_MERGE;
        }
    }
}

int income(const unit * u)
{
    switch (old_race(u_race(u))) {
    case RC_FIREDRAGON:
        return 150 * u->number;
    case RC_DRAGON:
        return 1000 * u->number;
    case RC_WYRM:
        return 5000 * u->number;
    default:
        return 20 * u->number;
    }
}

static void scramble(void *data, int n, size_t width)
{
    int j;
    char temp[64];
    assert(width <= sizeof(temp));
    for (j = 0; j != n; ++j) {
        int k = rng_int() % n;
        if (k == j)
            continue;
        memcpy(temp, (char *)data + j * width, width);
        memcpy((char *)data + j * width, (char *)data + k * width, width);
        memcpy((char *)data + k * width, temp, width);
    }
}

static void expandorders(region * r, request * requests)
{
    unit *u;
    request *o;

    /* Alle Units ohne request haben ein -1, alle units mit orders haben ein
     * 0 hier stehen */

    for (u = r->units; u; u = u->next)
        u->n = -1;

    norders = 0;

    for (o = requests; o; o = o->next) {
        if (o->qty > 0) {
            norders += o->qty;
        }
    }

    if (norders > 0) {
        int i = 0;
        oa = (request *)calloc(norders, sizeof(request));
        for (o = requests; o; o = o->next) {
            if (o->qty > 0) {
                int j;
                for (j = o->qty; j; j--) {
                    oa[i] = *o;
                    oa[i].unit->n = 0;
                    i++;
                }
            }
        }
        scramble(oa, norders, sizeof(request));
    }
    else {
        oa = NULL;
    }
    while (requests) {
        request *o = requests->next;
        free_order(requests->ord);
        free(requests);
        requests = o;
    }
}

/* ------------------------------------------------------------- */

static void change_level(unit * u, skill_t sk, int bylevel)
{
    skill *sv = unit_skill(u, sk);
    assert(bylevel > 0);
    if (sv == 0)
        sv = add_skill(u, sk);
    sk_set(sv, sv->level + bylevel);
}

typedef struct recruitment {
    struct recruitment *next;
    faction *f;
    request *requests;
    int total, assigned;
} recruitment;

/** Creates a list of recruitment structs, one for each faction. Adds every quantifyable request
 * to the faction's struct and to total.
 */
static recruitment *select_recruitment(request ** rop,
    int(*quantify) (const struct race *, int), int *total)
{
    recruitment *recruits = NULL;

    while (*rop) {
        recruitment *rec = recruits;
        request *ro = *rop;
        unit *u = ro->unit;
        const race *rc = u_race(u);
        int qty = quantify(rc, ro->qty);

        if (qty < 0) {
            rop = &ro->next;          /* skip this one */
        }
        else {
            *rop = ro->next;          /* remove this one */
            while (rec && rec->f != u->faction)
                rec = rec->next;
            if (rec == NULL) {
                rec = malloc(sizeof(recruitment));
                rec->f = u->faction;
                rec->total = 0;
                rec->assigned = 0;
                rec->requests = NULL;
                rec->next = recruits;
                recruits = rec;
            }
            *total += qty;
            rec->total += qty;
            ro->next = rec->requests;
            rec->requests = ro;
        }
    }
    return recruits;
}

static void add_recruits(unit * u, int number, int wanted)
{
    region *r = u->region;
    assert(number <= wanted);
    if (number > 0) {
        unit *unew;
        char equipment[64];

        if (u->number == 0) {
            set_number(u, number);
            u->hp = number * unit_max_hp(u);
            unew = u;
        }
        else {
            unew = create_unit(r, u->faction, number, u_race(u), 0, NULL, u);
        }

        strlcpy(equipment, "new_", sizeof(equipment));
        strlcat(equipment, u_race(u)->_name, sizeof(equipment));
        strlcat(equipment, "_unit", sizeof(equipment));
        equip_unit(unew, get_equipment(equipment));

        if (u_race(unew)->ec_flags & ECF_REC_HORSES) {
            change_level(unew, SK_RIDING, 1);
        }

        if (unew != u) {
            transfermen(unew, u, unew->number);
            remove_unit(&r->units, unew);
        }
    }
    if (number < wanted) {
        ADDMSG(&u->faction->msgs, msg_message("recruit",
            "unit region amount want", u, r, number, wanted));
    }
}

static int any_recruiters(const struct race *rc, int qty)
{
    return (int)(qty * 2 * rc->recruit_multi);
}

/*static int peasant_recruiters(const struct race *rc, int qty)
{
if (rc->ec_flags & ECF_REC_ETHEREAL)
return -1;
if (rc->ec_flags & ECF_REC_HORSES)
return -1;
return (int)(qty * 2 * rc->recruit_multi);
}*/

static int horse_recruiters(const struct race *rc, int qty)
{
    if (rc->ec_flags & ECF_REC_ETHEREAL)
        return -1;
    if (rc->ec_flags & ECF_REC_HORSES)
        return (int)(qty * 2 * rc->recruit_multi);
    return -1;
}

static int do_recruiting(recruitment * recruits, int available)
{
    recruitment *rec;
    int recruited = 0;

    /* try to assign recruits to factions fairly */
    while (available > 0) {
        int n = 0;
        int rest, mintotal = INT_MAX;

        /* find smallest request */
        for (rec = recruits; rec != NULL; rec = rec->next) {
            int want = rec->total - rec->assigned;
            if (want > 0) {
                if (mintotal > want)
                    mintotal = want;
                ++n;
            }
        }
        if (n == 0)
            break;
        if (mintotal * n > available) {
            mintotal = available / n;
        }
        rest = available - mintotal * n;

        /* assign size of smallest request for everyone if possible; in the end roll dice to assign
         * small rest */
        for (rec = recruits; rec != NULL; rec = rec->next) {
            int want = rec->total - rec->assigned;

            if (want > 0) {
                int get = mintotal;
                if (want > mintotal && rest < n && (rng_int() % n) < rest) {
                    --rest;
                    ++get;
                }
                assert(get <= want);
                available -= get;
                rec->assigned += get;
            }
        }
    }

    /* do actual recruiting */
    for (rec = recruits; rec != NULL; rec = rec->next) {
        request *req;
        int get = rec->assigned;

        for (req = rec->requests; req; req = req->next) {
            unit *u = req->unit;
            const race *rc = u_race(u); /* race is set in recruit() */
            int number, dec;
            float multi = 2.0F * rc->recruit_multi;

            number = _min(req->qty, (int)(get / multi));
            if (rc->recruitcost) {
                int afford = get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT,
                    number * rc->recruitcost) / rc->recruitcost;
                number = _min(number, afford);
            }
            if (u->number + number > UNIT_MAXSIZE) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, req->ord, "error_unit_size",
                    "maxsize", UNIT_MAXSIZE));
                number = UNIT_MAXSIZE - u->number;
                assert(number >= 0);
            }
            if (rc->recruitcost) {
                use_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT,
                    rc->recruitcost * number);
            }
            if (u->number == 0 && fval(u, UFL_DEAD)) {
                /* unit is empty, dead, and cannot recruit */
                number = 0;
            }
            if (number > 0) {
                add_recruits(u, number, req->qty);
                dec = (int)(number * multi);
                if ((rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
                    recruited += dec;
                }

                get -= dec;
            }
        }
    }
    return recruited;
}

void free_recruitments(recruitment * recruits)
{
    while (recruits) {
        recruitment *rec = recruits;
        recruits = rec->next;
        while (rec->requests) {
            request *req = rec->requests;
            rec->requests = req->next;
            free(req);
        }
        free(rec);
    }
}

/* Rekrutierung */
static void expandrecruit(region * r, request * recruitorders)
{
    recruitment *recruits = NULL;

    int orc_total = 0;

    /* centaurs: */
    recruits = select_recruitment(&recruitorders, horse_recruiters, &orc_total);
    if (recruits) {
        int recruited, horses = rhorses(r) * 2;
        if (orc_total < horses)
            horses = orc_total;
        recruited = do_recruiting(recruits, horses);
        rsethorses(r, (horses - recruited) / 2);
        free_recruitments(recruits);
    }

    /* peasant limited: */
    recruits = select_recruitment(&recruitorders, any_recruiters, &orc_total);
    if (recruits) {
        int orc_recruited, orc_peasants = rpeasants(r) * 2;
        int orc_frac = orc_peasants / RECRUITFRACTION;      /* anzahl orks. 2 ork = 1 bauer */
        if (orc_total < orc_frac)
            orc_frac = orc_total;
        orc_recruited = do_recruiting(recruits, orc_frac);
        assert(orc_recruited <= orc_frac);
        rsetpeasants(r, (orc_peasants - orc_recruited) / 2);
        free_recruitments(recruits);
    }

    /* no limit: */
    recruits = select_recruitment(&recruitorders, any_recruiters, &orc_total);
    if (recruits) {
        int recruited, peasants = rpeasants(r) * 2;
        recruited = do_recruiting(recruits, INT_MAX);
        if (recruited > 0) {
            rsetpeasants(r, (peasants - recruited) / 2);
        }
        free_recruitments(recruits);
    }

    assert(recruitorders == NULL);
}

static int recruit_cost(const faction * f, const race * rc)
{
    if (is_monsters(f) || f->race == rc) {
        return rc->recruitcost;
    }
    else if (valid_race(f, rc)) {
        return rc->recruitcost;
        /*      return get_param_int(f->race->parameters, "other_cost", -1); */
    }
    return -1;
}

static void recruit(unit * u, struct order *ord, request ** recruitorders)
{
    int n;
    region *r = u->region;
    plane *pl;
    request *o;
    int recruitcost = -1;
    const faction *f = u->faction;
    const struct race *rc = u_race(u);
    const char *str;

    init_order(ord);
    n = getuint();

    if (u->number == 0) {
        str = getstrtoken();
        if (str && str[0]) {
            /* Monsters can RECRUIT 15 DRACOID
             * also: secondary race */
            rc = findrace(str, f->locale);
            if (rc != NULL) {
                recruitcost = recruit_cost(f, rc);
            }
        }
    }
    if (recruitcost < 0) {
        rc = u_race(u);
        recruitcost = recruit_cost(f, rc);
        if (recruitcost < 0) {
            recruitcost = INT_MAX;
        }
    }
    assert(rc);
    u_setrace(u, rc);

#if GUARD_DISABLES_RECRUIT
    /* this is a very special case because the recruiting unit may be empty
     * at this point and we have to look at the creating unit instead. This
     * is done in cansee, which is called indirectly by is_guarded(). */
    if (is_guarded(r, u, GUARD_RECRUIT)) {
        cmistake(u, ord, 70, MSG_EVENT);
        return;
    }
#endif

    if (rc == get_race(RC_INSECT)) {
        gamedate date;
        get_gamedate(turn, &date);
        if (date.season == 0 && r->terrain != newterrain(T_DESERT)) {
#ifdef INSECT_POTION
            bool usepotion = false;
            unit *u2;

            for (u2 = r->units; u2; u2 = u2->next)
                if (fval(u2, UFL_WARMTH)) {
                usepotion = true;
                break;
                }
            if (!usepotion)
#endif
            {
                cmistake(u, ord, 98, MSG_EVENT);
                return;
            }
        }
        /* in Gletschern, Eisbergen gar nicht rekrutieren */
        if (r_insectstalled(r)) {
            cmistake(u, ord, 97, MSG_EVENT);
            return;
        }
    }
    if (is_cursed(r->attribs, C_RIOT, 0)) {
        /* Die Region befindet sich in Aufruhr */
        cmistake(u, ord, 237, MSG_EVENT);
        return;
    }

    if (!(rc->ec_flags & ECF_REC_HORSES) && fval(r, RF_ORCIFIED)) {
        if (rc != get_race(RC_ORC)) {
            cmistake(u, ord, 238, MSG_EVENT);
            return;
        }
    }

    if (recruitcost) {
        pl = getplane(r);
        if (pl && fval(pl, PFL_NORECRUITS)) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_pflnorecruit", ""));
            return;
        }

        if (get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT,
            recruitcost) < recruitcost) {
            cmistake(u, ord, 142, MSG_EVENT);
            return;
        }
    }
    if (!playerrace(rc) || idle(u->faction)) {
        cmistake(u, ord, 139, MSG_EVENT);
        return;
    }

    if (has_skill(u, SK_MAGIC)) {
        /* error158;de;{unit} in {region}: '{command}' - Magier arbeiten
         * grundsätzlich nur alleine! */
        cmistake(u, ord, 158, MSG_EVENT);
        return;
    }
    if (has_skill(u, SK_ALCHEMY)
        && count_skill(u->faction, SK_ALCHEMY) + n >
        skill_limit(u->faction, SK_ALCHEMY)) {
        cmistake(u, ord, 156, MSG_EVENT);
        return;
    }
    if (recruitcost > 0) {
        int pooled =
            get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, recruitcost * n);
        n = _min(n, pooled / recruitcost);
    }

    u->wants = n;

    if (!n) {
        cmistake(u, ord, 142, MSG_EVENT);
        return;
    }

    o = (request *)calloc(1, sizeof(request));
    o->qty = n;
    o->unit = u;
    o->ord = copy_order(ord);
    addlist(recruitorders, o);
}

static void friendly_takeover(region * r, faction * f)
{
    region_set_owner(r, f, turn);
    morale_change(r, MORALE_TRANSFER);
}

void give_control(unit * u, unit * u2)
{
    if (u->building) {
        if (u->faction != u2->faction && rule_region_owners()) {
            region *r = u->region;
            faction *f = region_get_owner(r);

            assert(u->building == u2->building);
            if (f == u->faction) {
                building *b = largestbuilding(r, &cmp_current_owner, false);
                if (b == u->building) {
                    friendly_takeover(r, u2->faction);
                }
            }
        }
        building_set_owner(u2);
    }
    if (u->ship) {
        assert(u->ship == u2->ship);
        ship_set_owner(u2);
    }
}

int give_control_cmd(unit * u, order * ord)
{
    region *r = u->region;
    unit *u2;
    const char *s;

    init_order(ord);
    getunit(r, u->faction, &u2);

    s = getstrtoken();
    if (s && isparam(s, u->faction->locale, P_CONTROL)) {
        message *msg = 0;

        if (!can_give_to(u, u2)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "feedback_unit_not_found", ""));
            return 0;
        }
        else if (!u->building && !u->ship) {
            msg = cmistake(u, ord, 140, MSG_EVENT);
        }
        else if (u->building) {
            if (u != building_owner(u->building)) {
                msg = cmistake(u, ord, 49, MSG_EVENT);
            }
            else if (u2->building != u->building) {
                msg = cmistake(u, ord, 33, MSG_EVENT);
            }
        }
        else if (u->ship) {
            if (u != ship_owner(u->ship)) {
                msg = cmistake(u, ord, 49, MSG_EVENT);
            }
            else if (u2->ship != u->ship) {
                msg = cmistake(u, ord, 32, MSG_EVENT);
            }
        }
        if (!msg) {
            give_control(u, u2);
            msg = msg_message("givecommand", "unit recipient", u, u2);
            add_message(&u->faction->msgs, msg);
            if (u->faction != u2->faction) {
                add_message(&u2->faction->msgs, msg);
            }
            msg_release(msg);
        }
    }
    return 0;
}

static int forget_cmd(unit * u, order * ord)
{
    skill_t sk;
    const char *s;

    if (is_cursed(u->attribs, C_SLAVE, 0)) {
        /* charmed units shouldn't be losing their skills */
        return 0;
    }

    init_order(ord);
    s = getstrtoken();

    if ((sk = get_skill(s, u->faction->locale)) != NOSKILL) {
        ADDMSG(&u->faction->msgs, msg_message("forget", "unit skill", u, sk));
        set_level(u, sk, 0);
    }
    return 0;
}

void add_spende(faction * f1, faction * f2, int amount, region * r)
{
    donation *sp;

    sp = r->donations;

    while (sp) {
        if (sp->f1 == f1 && sp->f2 == f2) {
            sp->amount += amount;
            return;
        }
        sp = sp->next;
    }

    sp = calloc(1, sizeof(donation));
    sp->f1 = f1;
    sp->f2 = f2;
    sp->amount = amount;
    sp->next = r->donations;
    r->donations = sp;
}

static bool maintain(building * b, bool first)
/* first==false -> take money from wherever you can */
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    int c;
    region *r = b->region;
    bool paid = true, work = first;
    unit *u;

    if (fval(b, BLD_MAINTAINED) || b->type == NULL || b->type->maintenance == NULL || is_cursed(b->attribs, C_NOCOST, 0)) {
        fset(b, BLD_MAINTAINED);
        fset(b, BLD_WORKING);
        return true;
    }
    if (fval(b, BLD_DONTPAY)) {
        return false;
    }
    u = building_owner(b);
    if (u == NULL) {
        /* no owner - send a message to the entire region */
        ADDMSG(&r->msgs, msg_message("maintenance_noowner", "building", b));
        return false;
    }
    /* If the owner is the region owner, check if dontpay flag is set for the building where he is in */
    if (check_param(global.parameters, "rules.region_owner_pay_building", b->type->_name)) {
        if (fval(u->building, BLD_DONTPAY)) {
            return false;
        }
    }
    for (c = 0; b->type->maintenance[c].number; ++c) {
        const maintenance *m = b->type->maintenance + c;
        int need = m->number;

        if (fval(m, MTF_VARIABLE))
            need = need * b->size;
        if (u) {
            /* first ist im ersten versuch true, im zweiten aber false! Das
             * bedeutet, das in der Runde in die Region geschafften Resourcen
             * nicht genutzt werden können, weil die reserviert sind! */
            if (!first)
                need -= get_pooled(u, m->rtype, GET_ALL, need);
            else
                need -= get_pooled(u, m->rtype, GET_DEFAULT, need);
            if (!first && need > 0) {
                unit *ua;
                for (ua = r->units; ua; ua = ua->next)
                    freset(ua->faction, FFL_SELECT);
                fset(u->faction, FFL_SELECT);   /* hat schon */
                for (ua = r->units; ua; ua = ua->next) {
                    if (!fval(ua->faction, FFL_SELECT) && (ua->faction == u->faction
                        || alliedunit(ua, u->faction, HELP_MONEY))) {
                        need -= get_pooled(ua, m->rtype, GET_ALL, need);
                        fset(ua->faction, FFL_SELECT);
                        if (need <= 0)
                            break;
                    }
                }
            }
        }
        if (need > 0) {
            if (!fval(m, MTF_VITAL))
                work = false;
            else {
                paid = false;
                break;
            }
        }
    }
    if (fval(b, BLD_DONTPAY)) {
        return false;
    }
    u = building_owner(b);
    if (u == NULL)
        return false;
    for (c = 0; b->type->maintenance[c].number; ++c) {
        const maintenance *m = b->type->maintenance + c;
        int need = m->number;

        if (fval(m, MTF_VARIABLE))
            need = need * b->size;
        if (u) {
            /* first ist im ersten versuch true, im zweiten aber false! Das
             * bedeutet, das in der Runde in die Region geschafften Resourcen
             * nicht genutzt werden können, weil die reserviert sind! */
            if (!first)
                need -= get_pooled(u, m->rtype, GET_ALL, need);
            else
                need -= get_pooled(u, m->rtype, GET_DEFAULT, need);
            if (!first && need > 0) {
                unit *ua;
                for (ua = r->units; ua; ua = ua->next)
                    freset(ua->faction, FFL_SELECT);
                fset(u->faction, FFL_SELECT);   /* hat schon */
                for (ua = r->units; ua; ua = ua->next) {
                    if (!fval(ua->faction, FFL_SELECT) && (ua->faction == u->faction
                        || alliedunit(ua, u->faction, HELP_MONEY))) {
                        need -= get_pooled(ua, m->rtype, GET_ALL, need);
                        fset(ua->faction, FFL_SELECT);
                        if (need <= 0)
                            break;
                    }
                }
            }
            if (need > 0) {
                work = false;
                if (fval(m, MTF_VITAL))
                {
                    paid = false;
                    break;
                }
            }
        }
    }
    if (paid && c > 0) {
        /* TODO: wieviel von was wurde bezahlt */
        if (first && work) {
            ADDMSG(&u->faction->msgs, msg_message("maintenance", "unit building", u, b));
            fset(b, BLD_WORKING);
            fset(b, BLD_MAINTAINED);
        }
        if (!first) {
            ADDMSG(&u->faction->msgs, msg_message("maintenance_late", "building", b));
            fset(b, BLD_MAINTAINED);
        }

        if (first && !work) {
            ADDMSG(&u->faction->msgs, msg_message("maintenancefail", "unit building", u, b));
            return false;
        }

        for (c = 0; b->type->maintenance[c].number; ++c) {
            const maintenance *m = b->type->maintenance + c;
            int cost = m->number;

            if (!fval(m, MTF_VITAL) && !work)
                continue;
            if (fval(m, MTF_VARIABLE))
                cost = cost * b->size;

            if (!first)
                cost -= use_pooled(u, m->rtype, GET_ALL, cost);
            else
                cost -=
                use_pooled(u, m->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
                cost);
            if (!first && cost > 0) {
                unit *ua;
                for (ua = r->units; ua; ua = ua->next)
                    freset(ua->faction, FFL_SELECT);
                fset(u->faction, FFL_SELECT);   /* hat schon */
                for (ua = r->units; ua; ua = ua->next) {
                    if (!fval(ua->faction, FFL_SELECT)
                        && alliedunit(ua, u->faction, HELP_MONEY)) {
                        int give = use_pooled(ua, m->rtype, GET_ALL, cost);
                        if (!give)
                            continue;
                        cost -= give;
                        fset(ua->faction, FFL_SELECT);
                        if (m->rtype == rsilver)
                            add_spende(ua->faction, u->faction, give, r);
                        if (cost <= 0)
                            break;
                    }
                }
            }
            assert(cost == 0);
        }
    }
    else {
        ADDMSG(&u->faction->msgs, msg_message("maintenancefail", "unit building", u, b));
        return false;
    }
    return true;
}

void maintain_buildings(region * r, bool crash)
{
    building **bp = &r->buildings;
    while (*bp) {
        building *b = *bp;
        bool maintained = maintain(b, !crash);

        /* the second time, send a message */
        if (crash) {
            if (!fval(b, BLD_WORKING)) {
                unit *u = building_owner(b);
                const char *msgtype =
                    maintained ? "maintenance_nowork" : "maintenance_none";
                struct message *msg = msg_message(msgtype, "building", b);

                if (u) {
                    add_message(&u->faction->msgs, msg);
                    r_addmessage(r, u->faction, msg);
                }
                else {
                    add_message(&r->msgs, msg);
                }
                msg_release(msg);
            }
        }
        bp = &b->next;
    }
}

void economics(region * r)
{
    unit *u;
    request *recruitorders = NULL;

    /* Geben vor Selbstmord (doquit)! Hier alle unmittelbaren Befehle.
     * Rekrutieren vor allen Einnahmequellen. Bewachen JA vor Steuern
     * eintreiben. */

    for (u = r->units; u; u = u->next) {
        order *ord;
        if (u->number > 0) {
            for (ord = u->orders; ord; ord = ord->next) {
                keyword_t kwd = getkeyword(ord);
                if (kwd == K_GIVE) {
                    give_cmd(u, ord);
                }
                else if (kwd == K_FORGET) {
                    forget_cmd(u, ord);
                }
                if (u->orders == NULL) {
                    break;
                }
            }
        }
    }
    /* RECRUIT orders */

    if (rules_recruit < 0)
        recruit_init();
    for (u = r->units; u; u = u->next) {
        order *ord;

        if ((rules_recruit & RECRUIT_MERGE) || u->number == 0) {
            for (ord = u->orders; ord; ord = ord->next) {
                if (getkeyword(ord) == K_RECRUIT) {
                    recruit(u, ord, &recruitorders);
                    break;
                }
            }
        }
    }

    if (recruitorders)
        expandrecruit(r, recruitorders);
    remove_empty_units_in_region(r);

    for (u = r->units; u; u = u->next) {
        order *ord;
        bool destroyed = false;
        if (u->number > 0) {
            for (ord = u->orders; ord; ord = ord->next) {
                keyword_t kwd = getkeyword(ord);
                if (kwd == K_DESTROY) {
                    if (!destroyed) {
                        if (destroy_cmd(u, ord) != 0)
                            ord = NULL;
                        destroyed = true;
                    }
                }
                if (u->orders == NULL) {
                    break;
                }
            }
        }
    }

}

/* ------------------------------------------------------------- */

static void manufacture(unit * u, const item_type * itype, int want)
{
    int n;
    int skill;
    int minskill = itype->construction->minskill;
    skill_t sk = itype->construction->skill;

    skill = effskill(u, sk);
    skill =
        skillmod(itype->rtype->attribs, u, u->region, sk, skill, SMF_PRODUCTION);

    if (skill < 0) {
        /* an error occured */
        int err = -skill;
        cmistake(u, u->thisorder, err, MSG_PRODUCE);
        return;
    }

    if (want == 0) {
        want = maxbuild(u, itype->construction);
    }
    n = build(u, itype->construction, 0, want);
    switch (n) {
    case ENEEDSKILL:
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "skill_needed", "skill", sk));
        return;
    case EBUILDINGREQ:
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "building_needed", "building",
            itype->construction->btype->_name));
        return;
    case ELOWSKILL:
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "manufacture_skills",
            "skill minskill product", sk, minskill, itype->rtype, 1));
        return;
    case ENOMATERIALS:
        ADDMSG(&u->faction->msgs, msg_materials_required(u, u->thisorder,
            itype->construction, want));
        return;
    }
    if (n > 0) {
        i_change(&u->items, itype, n);
        if (want == INT_MAX)
            want = n;
        ADDMSG(&u->faction->msgs, msg_message("manufacture",
            "unit region amount wanted resource", u, u->region, n, want,
            itype->rtype));
    }
    else {
        ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder, "error_cannotmake",
            ""));
    }
}

typedef struct allocation {
    struct allocation *next;
    int want, get;
    double save;
    unsigned int flags;
    unit *unit;
} allocation;
#define new_allocation() calloc(sizeof(allocation), 1)
#define free_allocation(a) free(a)

typedef struct allocation_list {
    struct allocation_list *next;
    allocation *data;
    const resource_type *type;
} allocation_list;

static allocation_list *allocations;

static bool can_guard(const unit * guard, const unit * u)
{
    if (fval(guard, UFL_ISNEW))
        return false;
    if (guard->number <= 0 || !cansee(guard->faction, guard->region, u, 0))
        return false;
    if (besieged(guard) || !(fval(u_race(guard), RCF_UNARMEDGUARD)
        || armedmen(guard, true)))
        return false;

    return !alliedunit(guard, u->faction, HELP_GUARD);
}

enum {
    AFL_DONE = 1 << 0,
    AFL_LOWSKILL = 1 << 1
};

static void allocate_resource(unit * u, const resource_type * rtype, int want)
{
    const item_type *itype = resource2item(rtype);
    region *r = u->region;
    int dm = 0;
    allocation_list *alist;
    allocation *al;
    attrib *a = a_find(rtype->attribs, &at_resourcelimit);
    resource_limit *rdata = (resource_limit *)a->data.v;
    const resource_type *rring;
    int amount, skill;

    /* momentan kann man keine ressourcen abbauen, wenn man dafür
     * Materialverbrauch hat: */
    assert(itype != NULL && (itype->construction == NULL
        || itype->construction->materials == NULL));
    assert(rdata != NULL);

    if (rdata->limit != NULL) {
        int avail = rdata->limit(r, rtype);
        if (avail <= 0) {
            cmistake(u, u->thisorder, 121, MSG_PRODUCE);
            return;
        }
    }

    if (besieged(u)) {
        cmistake(u, u->thisorder, 60, MSG_PRODUCE);
        return;
    }

    if (rdata->modifiers) {
        resource_mod *mod = rdata->modifiers;
        for (; mod->flags != 0; ++mod) {
            if (mod->flags & RMF_REQUIREDBUILDING) {
                struct building *b = inside_building(u);
                const struct building_type *btype = b ? b->type : NULL;
                if (mod->btype && mod->btype != btype) {
                    cmistake(u, u->thisorder, 104, MSG_PRODUCE);
                    return;
                }
            }
        }
    }

    if (rdata->guard != 0) {
        unit *u2;
        for (u2 = r->units; u2; u2 = u2->next) {
            if (is_guard(u2, rdata->guard) != 0 && can_guard(u2, u)) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, u->thisorder, "region_guarded", "guard", u2));
                return;
            }
        }
    }

    /* Bergwächter können Abbau von Eisen/Laen durch Bewachen verhindern.
     * Als magische Wesen 'sehen' Bergwächter alles und werden durch
     * Belagerung nicht aufgehalten.  (Ansonsten wie oben bei Elfen anpassen).
     */
    if (itype->rtype && (itype->rtype == get_resourcetype(R_IRON) || itype->rtype == rt_find("laen"))) {
        unit *u2;
        for (u2 = r->units; u2; u2 = u2->next) {
            if (is_guard(u, GUARD_MINING)
                && !fval(u2, UFL_ISNEW)
                && u2->number && !alliedunit(u2, u->faction, HELP_GUARD)) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, u->thisorder, "region_guarded", "guard", u2));
                return;
            }
        }
    }

    assert(itype->construction->skill != 0
        || "limited resource needs a required skill for making it");
    skill = eff_skill(u, itype->construction->skill, u->region);
    if (skill == 0) {
        skill_t sk = itype->construction->skill;
        add_message(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "skill_needed", "skill", sk));
        return;
    }
    if (skill < itype->construction->minskill) {
        skill_t sk = itype->construction->skill;
        add_message(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "manufacture_skills",
            "skill minskill product", sk, itype->construction->minskill,
            itype->rtype));
        return;
    }
    else {
        struct building *b = inside_building(u);
        const struct building_type *btype = b ? b->type : NULL;

        if (rdata->modifiers) {
            resource_mod *mod = rdata->modifiers;
            for (; mod->flags != 0; ++mod) {
                if (mod->flags & RMF_SKILL) {
                    if (mod->btype == NULL || mod->btype == btype) {
                        if (mod->race == NULL || mod->race == u_race(u)) {
                            skill += mod->value.i;
                        }
                    }
                }
            }
        }
    }
    amount = skill * u->number;
    /* nun ist amount die Gesamtproduktion der Einheit (in punkten) */

    /* mit Flinkfingerring verzehnfacht sich die Produktion */
    rring = get_resourcetype(R_RING_OF_NIMBLEFINGER);
    if (rring) {
        int dm = i_get(u->items, rring->itype);
        amount += skill * _min(u->number, dm) * (roqf_factor() - 1);
    }

    /* Schaffenstrunk: */
    if ((dm = get_effect(u, oldpotiontype[P_DOMORE])) != 0) {
        dm = _min(dm, u->number);
        change_effect(u, oldpotiontype[P_DOMORE], -dm);
        amount += dm * skill;       /* dm Personen produzieren doppelt */
    }

    amount /= itype->construction->minskill;

    /* Limitierung durch Parameter m. */
    if (want > 0 && want < amount)
        amount = want;

    alist = allocations;
    while (alist && alist->type != rtype)
        alist = alist->next;
    if (!alist) {
        alist = calloc(sizeof(struct allocation_list), 1);
        alist->next = allocations;
        alist->type = rtype;
        allocations = alist;
    }
    al = new_allocation();
    al->want = amount;
    al->save = 1.0;
    al->next = alist->data;
    al->unit = u;
    alist->data = al;

    if (rdata->modifiers) {
        struct building *b = inside_building(u);
        const struct building_type *btype = b ? b->type : NULL;

        resource_mod *mod = rdata->modifiers;
        for (; mod->flags != 0; ++mod) {
            if (mod->flags & RMF_SAVEMATERIAL) {
                if (mod->btype == NULL || mod->btype == btype) {
                    if (mod->race == NULL || mod->race == u_race(u)) {
                        al->save *= mod->value.f;
                    }
                }
            }
        }
    }
}

static int required(int want, double save)
{
    int norders = (int)(want * save);
    if (norders < want * save)
        ++norders;
    return norders;
}

static void
leveled_allocation(const resource_type * rtype, region * r, allocation * alist)
{
    const item_type *itype = resource2item(rtype);
    rawmaterial *rm = rm_get(r, rtype);
    int need;
    bool first = true;

    if (rm != NULL) {
        do {
            int avail = rm->amount;
            int norders = 0;
            allocation *al;

            if (avail <= 0) {
                for (al = alist; al; al = al->next) {
                    al->get = 0;
                }
                break;
            }

            assert(avail > 0);

            for (al = alist; al; al = al->next)
                if (!fval(al, AFL_DONE)) {
                int req = required(al->want - al->get, al->save);
                assert(al->get <= al->want && al->get >= 0);
                if (eff_skill(al->unit, itype->construction->skill, r)
                    >= rm->level + itype->construction->minskill - 1) {
                    if (req) {
                        norders += req;
                    }
                    else {
                        fset(al, AFL_DONE);
                    }
                }
                else {
                    fset(al, AFL_DONE);
                    if (first)
                        fset(al, AFL_LOWSKILL);
                }
                }
            need = norders;

            avail = _min(avail, norders);
            if (need > 0) {
                int use = 0;
                for (al = alist; al; al = al->next)
                    if (!fval(al, AFL_DONE)) {
                    if (avail > 0) {
                        int want = required(al->want - al->get, al->save);
                        int x = avail * want / norders;
                        /* Wenn Rest, dann würfeln, ob ich was bekomme: */
                        if (rng_int() % norders < (avail * want) % norders)
                            ++x;
                        avail -= x;
                        use += x;
                        norders -= want;
                        need -= x;
                        al->get = _min(al->want, al->get + (int)(x / al->save));
                    }
                    }
                if (use) {
                    assert(use <= rm->amount);
                    rm->type->use(rm, r, use);
                }
                assert(avail == 0 || norders == 0);
            }
            first = false;
        } while (need > 0);
    }
}

static void
attrib_allocation(const resource_type * rtype, region * r, allocation * alist)
{
    allocation *al;
    int norders = 0;
    attrib *a = a_find(rtype->attribs, &at_resourcelimit);
    resource_limit *rdata = (resource_limit *)a->data.v;
    int avail = rdata->value;

    for (al = alist; al; al = al->next) {
        norders += required(al->want, al->save);
    }

    if (rdata->limit) {
        avail = rdata->limit(r, rtype);
        if (avail < 0)
            avail = 0;
    }

    avail = _min(avail, norders);
    for (al = alist; al; al = al->next) {
        if (avail > 0) {
            int want = required(al->want, al->save);
            int x = avail * want / norders;
            /* Wenn Rest, dann würfeln, ob ich was bekomme: */
            if (rng_int() % norders < (avail * want) % norders)
                ++x;
            avail -= x;
            norders -= want;
            al->get = _min(al->want, (int)(x / al->save));
            if (rdata->produce) {
                int use = required(al->get, al->save);
                if (use)
                    rdata->produce(r, rtype, use);
            }
        }
    }
    assert(avail == 0 || norders == 0);
}

typedef void(*allocate_function) (const resource_type *, struct region *,
struct allocation *);

static allocate_function get_allocator(const struct resource_type *rtype)
{
    attrib *a = a_find(rtype->attribs, &at_resourcelimit);

    if (a != NULL) {
        resource_limit *rdata = (resource_limit *)a->data.v;
        if (rdata->value > 0 || rdata->limit != NULL) {
            return attrib_allocation;
        }
        return leveled_allocation;
    }
    return NULL;
}

void split_allocations(region * r)
{
    allocation_list **p_alist = &allocations;
    freset(r, RF_SELECT);
    while (*p_alist) {
        allocation_list *alist = *p_alist;
        const resource_type *rtype = alist->type;
        allocate_function alloc = get_allocator(rtype);
        const item_type *itype = resource2item(rtype);
        allocation **p_al = &alist->data;

        freset(r, RF_SELECT);
        alloc(rtype, r, alist->data);

        while (*p_al) {
            allocation *al = *p_al;
            if (al->get) {
                assert(itype || !"not implemented for non-items");
                i_change(&al->unit->items, itype, al->get);
                produceexp(al->unit, itype->construction->skill, al->unit->number);
                fset(r, RF_SELECT);
            }
            if (al->want == INT_MAX)
                al->want = al->get;
            ADDMSG(&al->unit->faction->msgs, msg_message("produce",
                "unit region amount wanted resource",
                al->unit, al->unit->region, al->get, al->want, rtype));
            *p_al = al->next;
            free_allocation(al);
        }
        *p_alist = alist->next;
        free(alist);
    }
    allocations = NULL;
}

static void create_potion(unit * u, const potion_type * ptype, int want)
{
    int built;

    if (want == 0) {
        want = maxbuild(u, ptype->itype->construction);
    }
    built = build(u, ptype->itype->construction, 0, want);
    switch (built) {
    case ELOWSKILL:
    case ENEEDSKILL:
        /* no skill, or not enough skill points to build */
        cmistake(u, u->thisorder, 50, MSG_PRODUCE);
        break;
    case EBUILDINGREQ:
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "building_needed", "building",
            ptype->itype->construction->btype->_name));
        break;
    case ECOMPLETE:
        assert(0);
        break;
    case ENOMATERIALS:
        /* something missing from the list of materials */
        ADDMSG(&u->faction->msgs, msg_materials_required(u, u->thisorder,
            ptype->itype->construction, want));
        return;
        break;
    default:
        i_change(&u->items, ptype->itype, built);
        if (want == INT_MAX)
            want = built;
        ADDMSG(&u->faction->msgs, msg_message("manufacture",
            "unit region amount wanted resource", u, u->region, built, want,
            ptype->itype->rtype));
        break;
    }
}

static void create_item(unit * u, const item_type * itype, int want)
{
    if (itype->construction && fval(itype->rtype, RTF_LIMITED)) {
#if GUARD_DISABLES_PRODUCTION == 1
        if (is_guarded(u->region, u, GUARD_PRODUCE)) {
            cmistake(u, u->thisorder, 70, MSG_EVENT);
            return;
        }
#endif
        allocate_resource(u, itype->rtype, want);
    }
    else {
        const potion_type *ptype = resource2potion(itype->rtype);
        if (ptype != NULL)
            create_potion(u, ptype, want);
        else if (itype->construction && itype->construction->materials)
            manufacture(u, itype, want);
        else {
            ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
                "error_cannotmake", ""));
        }
    }
}

int make_cmd(unit * u, struct order *ord)
{
    char token[128];
    region *r = u->region;
    const building_type *btype = 0;
    const ship_type *stype = 0;
    const item_type *itype = 0;
    param_t p = NOPARAM;
    int m = INT_MAX;
    const char *s;
    const struct locale *lang = u->faction->locale;
    char ibuf[16];
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_MAKE);
    s = getstrtok(token, sizeof(token));

    if (s) {
        m = atoi((const char *)s);
        sprintf(ibuf, "%d", m);
        if (!strcmp(ibuf, (const char *)s)) {
            /* a quantity was given */
            s = getstrtok(token, sizeof(token));
        }
        else {
            m = INT_MAX;
        }
        if (s) {
            p = findparam(s, u->faction->locale);
        }
    }

    if (p == P_ROAD) {
        plane *pl = rplane(r);
        if (pl && fval(pl, PFL_NOBUILD)) {
            cmistake(u, ord, 275, MSG_PRODUCE);
        }
        else {
            const char * s = getstrtok(token, sizeof(token));
            direction_t d = s ? get_direction(s, u->faction->locale) : NODIRECTION;
            if (d != NODIRECTION) {
                build_road(r, u, m, d);
            }
            else {
                /* Die Richtung wurde nicht erkannt */
                cmistake(u, ord, 71, MSG_PRODUCE);
            }
        }
        return 0;
    }
    else if (p == P_SHIP) {
        plane *pl = rplane(r);
        if (pl && fval(pl, PFL_NOBUILD)) {
            cmistake(u, ord, 276, MSG_PRODUCE);
        }
        else {
            continue_ship(r, u, m);
        }
        return 0;
    }
    else if (p == P_HERBS) {
        herbsearch(r, u, m);
        return 0;
    }

    /* since the string can match several objects, like in 'academy' and
     * 'academy of arts', we need to figure out what the player meant.
     * This is not 100% safe.
     */
    if (s) {
        stype = findshiptype(s, lang);
        btype = findbuildingtype(s, lang);
        itype = finditemtype(s, lang);
    }
    if (itype != NULL && (btype != NULL || stype != NULL)) {
        if (itype->construction == NULL) {
            /* if the item cannot be made, we probably didn't mean to make it */
            itype = NULL;
        }
        else if (stype != NULL) {
            const char *sname = LOC(lang, stype->_name);
            const char *iname = LOC(lang, resourcename(itype->rtype, 0));
            if (strlen(iname) < strlen(sname))
                stype = NULL;
            else
                itype = NULL;
        }
        else {
            const char *bname = LOC(lang, btype->_name);
            const char *iname = LOC(lang, resourcename(itype->rtype, 0));
            if (strlen(iname) < strlen(bname))
                btype = NULL;
            else
                itype = NULL;
        }
    }

    if (btype != NULL && stype != NULL) {
        const char *bname = LOC(lang, btype->_name);
        const char *sname = LOC(lang, stype->_name);
        if (strlen(sname) < strlen(bname))
            btype = NULL;
        else
            stype = NULL;
    }

    if (stype != NOSHIP) {
        plane *pl = rplane(r);
        if (pl && fval(pl, PFL_NOBUILD)) {
            cmistake(u, ord, 276, MSG_PRODUCE);
        }
        else {
            create_ship(r, u, stype, m, ord);
        }
    }
    else if (btype != NOBUILDING) {
        plane *pl = rplane(r);
        if (pl && fval(pl, PFL_NOBUILD)) {
            cmistake(u, ord, 275, MSG_PRODUCE);
        }
        else if (btype->construction) {
            int id = getid();
            build_building(u, btype, id, m, ord);
        }
        else {
            cmistake(u, ord, 275, MSG_PRODUCE);
        }
    }
    else if (itype != NULL) {
        create_item(u, itype, m);
    }
    else {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_cannotmake", ""));
    }

    return 0;
}

/* ------------------------------------------------------------- */

static void free_luxuries(struct attrib *a)
{
    item *itm = (item *)a->data.v;
    a->data.v = NULL;
    i_freeall(&itm);
}

const attrib_type at_luxuries = {
    "luxuries", NULL, free_luxuries, NULL, NULL, NULL
};

static void expandbuying(region * r, request * buyorders)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    int max_products;
    unit *u;
    static struct trade {
        const luxury_type *type;
        int number;
        int multi;
    } *trades, *trade;
    static int ntrades = 0;
    int i, j;
    const luxury_type *ltype;

    if (ntrades == 0) {
        for (ltype = luxurytypes; ltype; ltype = ltype->next)
            ++ntrades;
        trades = gc_add(calloc(sizeof(struct trade), ntrades));
        for (i = 0, ltype = luxurytypes; i != ntrades; ++i, ltype = ltype->next)
            trades[i].type = ltype;
    }
    for (i = 0; i != ntrades; ++i) {
        trades[i].number = 0;
        trades[i].multi = 1;
    }

    if (!buyorders)
        return;

    /* Initialisation. multiplier ist der Multiplikator auf den
     * Verkaufspreis. Für max_products Produkte kauft man das Produkt zum
     * einfachen Verkaufspreis, danach erhöht sich der Multiplikator um 1.
     * counter ist ein Zähler, der die gekauften Produkte zählt. money
     * wird für die debug message gebraucht. */

    max_products = rpeasants(r) / TRADE_FRACTION;

    /* Kauf - auch so programmiert, daß er leicht erweiterbar auf mehrere
     * Güter pro Monat ist. j sind die Befehle, i der Index des
     * gehandelten Produktes. */
    if (max_products > 0) {
        expandorders(r, buyorders);
        if (!norders)
            return;

        for (j = 0; j != norders; j++) {
            int price, multi;
            ltype = oa[j].type.ltype;
            trade = trades;
            while (trade->type != ltype)
                ++trade;
            multi = trade->multi;
            price = ltype->price * multi;

            if (get_pooled(oa[j].unit, rsilver, GET_DEFAULT,
                price) >= price) {
                unit *u = oa[j].unit;
                item *items;

                /* litems zählt die Güter, die verkauft wurden, u->n das Geld, das
                 * verdient wurde. Dies muß gemacht werden, weil der Preis ständig sinkt,
                 * man sich also das verdiente Geld und die verkauften Produkte separat
                 * merken muß. */
                attrib *a = a_find(u->attribs, &at_luxuries);
                if (a == NULL)
                    a = a_add(&u->attribs, a_new(&at_luxuries));

                items = a->data.v;
                i_change(&items, ltype->itype, 1);
                a->data.v = items;
                i_change(&oa[j].unit->items, ltype->itype, 1);
                use_pooled(u, rsilver, GET_DEFAULT, price);
                if (u->n < 0)
                    u->n = 0;
                u->n += price;

                rsetmoney(r, rmoney(r) + price);

                /* Falls mehr als max_products Bauern ein Produkt verkauft haben, steigt
                 * der Preis Multiplikator für das Produkt um den Faktor 1. Der Zähler
                 * wird wieder auf 0 gesetzt. */
                if (++trade->number == max_products) {
                    trade->number = 0;
                    ++trade->multi;
                }
                fset(u, UFL_LONGACTION | UFL_NOTMOVING);
            }
        }
        free(oa);

        /* Ausgabe an Einheiten */

        for (u = r->units; u; u = u->next) {
            attrib *a = a_find(u->attribs, &at_luxuries);
            item *itm;
            if (a == NULL)
                continue;
            ADDMSG(&u->faction->msgs, msg_message("buy", "unit money", u, u->n));
            for (itm = (item *)a->data.v; itm; itm = itm->next) {
                if (itm->number) {
                    ADDMSG(&u->faction->msgs, msg_message("buyamount",
                        "unit amount resource", u, itm->number, itm->type->rtype));
                }
            }
            a_remove(&u->attribs, a);
        }
    }
}

attrib_type at_trades = {
    "trades",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

static void buy(unit * u, request ** buyorders, struct order *ord)
{
    region *r = u->region;
    int n, k;
    request *o;
    attrib *a;
    const item_type *itype = NULL;
    const luxury_type *ltype = NULL;
    keyword_t kwd;
    const char *s;

    if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return;
    }
    if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return;
    }
    /* Im Augenblick kann man nur 1 Produkt kaufen. expandbuying ist aber
     * schon dafür ausgerüstet, mehrere Produkte zu kaufen. */

    kwd = init_order(ord);
    assert(kwd == K_BUY);
    n = getuint();
    if (!n) {
        cmistake(u, ord, 26, MSG_COMMERCE);
        return;
    }
    if (besieged(u)) {
        /* Belagerte Einheiten können nichts kaufen. */
        cmistake(u, ord, 60, MSG_COMMERCE);
        return;
    }

    if (u_race(u) == get_race(RC_INSECT)) {
        /* entweder man ist insekt, oder... */
        if (r->terrain != newterrain(T_SWAMP) && r->terrain != newterrain(T_DESERT)
            && !rbuildings(r)) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return;
        }
    }
    else {
        /* ...oder in der Region muß es eine Burg geben. */
        building *b = 0;
        if (r->buildings) {
            const struct building_type *bt_castle = bt_find("castle");

            for (b = r->buildings; b; b = b->next) {
                if (b->type == bt_castle && b->size >= 2) {
                    break;
                }
            }
        }
        if (b == NULL) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return;
        }
    }

    /* Ein Händler kann nur 10 Güter pro Talentpunkt handeln. */
    k = u->number * 10 * eff_skill(u, SK_TRADE, r);

    /* hat der Händler bereits gehandelt, muss die Menge der bereits
     * verkauften/gekauften Güter abgezogen werden */
    a = a_find(u->attribs, &at_trades);
    if (!a) {
        a = a_add(&u->attribs, a_new(&at_trades));
    }
    else {
        k -= a->data.i;
    }

    n = _min(n, k);

    if (!n) {
        cmistake(u, ord, 102, MSG_COMMERCE);
        return;
    }

    assert(n >= 0);
    /* die Menge der verkauften Güter merken */
    a->data.i += n;

    s = getstrtoken();
    itype = s ? finditemtype(s, u->faction->locale) : 0;
    if (itype != NULL) {
        ltype = resource2luxury(itype->rtype);
        if (ltype == NULL) {
            cmistake(u, ord, 124, MSG_COMMERCE);
            return;
        }
    }
    if (r_demand(r, ltype)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "luxury_notsold", ""));
        return;
    }
    o = (request *)calloc(1, sizeof(request));
    o->type.ltype = ltype;        /* sollte immer gleich sein */

    o->unit = u;
    o->qty = n;
    addlist(buyorders, o);
}

/* ------------------------------------------------------------- */
static void add_income(unit * u, int type, int want, int qty)
{
    if (want == INT_MAX)
        want = qty;
    ADDMSG(&u->faction->msgs, msg_message("income",
        "unit region mode wanted amount", u, u->region, type, want, qty));
}

/* Steuersätze in % bei Burggröße */
static int tax_per_size[7] = { 0, 6, 12, 18, 24, 30, 36 };

static void expandselling(region * r, request * sellorders, int limit)
{
    int money, price, j, max_products;
    /* int m, n = 0; */
    int maxsize = 0, maxeffsize = 0;
    int taxcollected = 0;
    int hafencollected = 0;
    unit *maxowner = (unit *)NULL;
    building *maxb = (building *)NULL;
    building *b;
    unit *u;
    unit *hafenowner;
    static int *counter;
    static int ncounter = 0;

    if (ncounter == 0) {
        const luxury_type *ltype;
        for (ltype = luxurytypes; ltype; ltype = ltype->next)
            ++ncounter;
        counter = (int *)gc_add(calloc(sizeof(int), ncounter));
    }
    else {
        memset(counter, 0, sizeof(int) * ncounter);
    }

    if (!sellorders) {            /* NEIN, denn Insekten können in   || !r->buildings) */
        return;                     /* Sümpfen und Wüsten auch so handeln */
    }
    /* Stelle Eigentümer der größten Burg fest. Bekommt Steuern aus jedem
     * Verkauf. Wenn zwei Burgen gleicher Größe bekommt gar keiner etwas. */

    for (b = rbuildings(r); b; b = b->next) {
        if (b->size > maxsize && building_owner(b) != NULL
            && b->type == bt_find("castle")) {
            maxb = b;
            maxsize = b->size;
            maxowner = building_owner(b);
        }
        else if (b->size == maxsize && b->type == bt_find("castle")) {
            maxb = (building *)NULL;
            maxowner = (unit *)NULL;
        }
    }

    hafenowner = owner_buildingtyp(r, bt_find("harbour"));

    if (maxb != (building *)NULL && maxowner != (unit *)NULL) {
        maxeffsize = buildingeffsize(maxb, false);
        if (maxeffsize == 0) {
            maxb = (building *)NULL;
            maxowner = (unit *)NULL;
        }
    }
    /* Die Region muss genug Geld haben, um die Produkte kaufen zu können. */

    money = rmoney(r);

    /* max_products sind 1/100 der Bevölkerung, falls soviele Produkte
     * verkauft werden - counter[] - sinkt die Nachfrage um 1 Punkt.
     * multiplier speichert r->demand für die debug message ab. */

    max_products = rpeasants(r) / TRADE_FRACTION;
    if (max_products <= 0)
        return;

    if (r->terrain == newterrain(T_DESERT)
        && buildingtype_exists(r, bt_find("caravan"), true)) {
        max_products = rpeasants(r) * 2 / TRADE_FRACTION;
    }
    /* Verkauf: so programmiert, dass er leicht auf mehrere Gueter pro
     * Runde erweitert werden kann. */

    expandorders(r, sellorders);
    if (!norders)
        return;

    for (j = 0; j != norders; j++) {
        const luxury_type *search = NULL;
        const luxury_type *ltype = oa[j].type.ltype;
        int multi = r_demand(r, ltype);
        int i;
        int use = 0;
        for (i = 0, search = luxurytypes; search != ltype; search = search->next) {
            ++i;
        }
        if (counter[i] >= limit)
            continue;
        if (counter[i] + 1 > max_products && multi > 1)
            --multi;
        price = ltype->price * multi;

        if (money >= price) {
            int abgezogenhafen = 0;
            int abgezogensteuer = 0;
            unit *u = oa[j].unit;
            item *itm;
            attrib *a = a_find(u->attribs, &at_luxuries);
            if (a == NULL)
                a = a_add(&u->attribs, a_new(&at_luxuries));
            itm = (item *)a->data.v;
            i_change(&itm, ltype->itype, 1);
            a->data.v = itm;
            ++use;
            if (u->n < 0)
                u->n = 0;

            if (hafenowner != NULL) {
                if (hafenowner->faction != u->faction) {
                    abgezogenhafen = price / 10;
                    hafencollected += abgezogenhafen;
                    price -= abgezogenhafen;
                    money -= abgezogenhafen;
                }
            }
            if (maxb != NULL) {
                if (maxowner->faction != u->faction) {
                    abgezogensteuer = price * tax_per_size[maxeffsize] / 100;
                    taxcollected += abgezogensteuer;
                    price -= abgezogensteuer;
                    money -= abgezogensteuer;
                }
            }
            u->n += price;
            change_money(u, price);
            fset(u, UFL_LONGACTION | UFL_NOTMOVING);

            /* r->money -= price; --- dies wird eben nicht ausgeführt, denn die
             * Produkte können auch als Steuern eingetrieben werden. In der Region
             * wurden Silberstücke gegen Luxusgüter des selben Wertes eingetauscht!
             * Falls mehr als max_products Kunden ein Produkt gekauft haben, sinkt
             * die Nachfrage für das Produkt um 1. Der Zähler wird wieder auf 0
             * gesetzt. */

            if (++counter[i] > max_products) {
                int d = r_demand(r, ltype);
                if (d > 1) {
                    r_setdemand(r, ltype, d - 1);
                }
                counter[i] = 0;
            }
        }
        if (use > 0) {
#ifdef NDEBUG
            use_pooled(oa[j].unit, ltype->itype->rtype, GET_DEFAULT, use);
#else
            /* int i = */ use_pooled(oa[j].unit, ltype->itype->rtype, GET_DEFAULT,
                use);
            /* assert(i==use); */
#endif
        }
    }
    free(oa);

    /* Steuern. Hier werden die Steuern dem Besitzer der größten Burg gegeben. */
    if (maxowner) {
        if (taxcollected > 0) {
            change_money(maxowner, (int)taxcollected);
            add_income(maxowner, IC_TRADETAX, taxcollected, taxcollected);
            /* TODO: Meldung
             * "%s verdient %d Silber durch den Handel in %s.",
             * unitname(maxowner), (int) taxcollected, regionname(r)); */
        }
    }
    if (hafenowner) {
        if (hafencollected > 0) {
            change_money(hafenowner, (int)hafencollected);
            add_income(hafenowner, IC_TRADETAX, hafencollected, hafencollected);
        }
    }
    /* Berichte an die Einheiten */

    for (u = r->units; u; u = u->next) {

        attrib *a = a_find(u->attribs, &at_luxuries);
        item *itm;
        if (a == NULL)
            continue;
        for (itm = (item *)a->data.v; itm; itm = itm->next) {
            if (itm->number) {
                ADDMSG(&u->faction->msgs, msg_message("sellamount",
                    "unit amount resource", u, itm->number, itm->type->rtype));
            }
        }
        a_remove(&u->attribs, a);
        add_income(u, IC_TRADE, u->n, u->n);
    }
}

static bool sell(unit * u, request ** sellorders, struct order *ord)
{
    bool unlimited = true;
    const item_type *itype;
    const luxury_type *ltype;
    int n;
    region *r = u->region;
    const char *s;
    keyword_t kwd;

    if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return false;
    }
    /* sellorders sind KEIN array, weil für alle items DIE SELBE resource
     * (das geld der region) aufgebraucht wird. */

    kwd = init_order(ord);
    assert(kwd == K_SELL);
    s = getstrtoken();

    if (findparam(s, u->faction->locale) == P_ANY) {
        unlimited = false;
        n = rpeasants(r) / TRADE_FRACTION;
        if (r->terrain == newterrain(T_DESERT)
            && buildingtype_exists(r, bt_find("caravan"), true))
            n *= 2;
        if (n == 0) {
            cmistake(u, ord, 303, MSG_COMMERCE);
            return false;
        }
    }
    else {
        n = s ? atoi(s) : 0;
        if (n == 0) {
            cmistake(u, ord, 27, MSG_COMMERCE);
            return false;
        }
    }
    /* Belagerte Einheiten können nichts verkaufen. */

    if (besieged(u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error60", ""));
        return false;
    }
    /* In der Region muß es eine Burg geben. */

    if (u_race(u) == get_race(RC_INSECT)) {
        if (r->terrain != newterrain(T_SWAMP) && r->terrain != newterrain(T_DESERT)
            && !rbuildings(r)) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return false;
        }
    }
    else {
        /* ...oder in der Region muß es eine Burg geben. */
        building *b = 0;
        if (r->buildings) {
            const struct building_type *bt_castle = bt_find("castle");
            for (b = r->buildings; b; b = b->next) {
                if (b->type == bt_castle && b->size >= 2) break;
            }
        }
        if (!b) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return false;
        }
    }

    /* Ein Händler kann nur 10 Güter pro Talentpunkt verkaufen. */

    n = _min(n, u->number * 10 * eff_skill(u, SK_TRADE, r));

    if (!n) {
        cmistake(u, ord, 54, MSG_COMMERCE);
        return false;
    }
    s = getstrtoken();
    itype = s ? finditemtype(s, u->faction->locale) : 0;
    ltype = itype ? resource2luxury(itype->rtype) : 0;
    if (ltype == NULL) {
        cmistake(u, ord, 126, MSG_COMMERCE);
        return false;
    }
    else {
        attrib *a;
        request *o;
        int k, available;

        if (!r_demand(r, ltype)) {
            cmistake(u, ord, 263, MSG_COMMERCE);
            return false;
        }
        available = get_pooled(u, itype->rtype, GET_DEFAULT, INT_MAX);

        /* Wenn andere Einheiten das selbe verkaufen, muß ihr Zeug abgezogen
         * werden damit es nicht zweimal verkauft wird: */
        for (o = *sellorders; o; o = o->next) {
            if (o->type.ltype == ltype && o->unit->faction == u->faction) {
                int fpool =
                    o->qty - get_pooled(o->unit, itype->rtype, GET_RESERVE, INT_MAX);
                available -= _max(0, fpool);
            }
        }

        n = _min(n, available);

        if (n <= 0) {
            cmistake(u, ord, 264, MSG_COMMERCE);
            return false;
        }
        /* Hier wird request->type verwendet, weil die obere limit durch
         * das silber gegeben wird (region->money), welches für alle
         * (!) produkte als summe gilt, als nicht wie bei der
         * produktion, wo für jedes produkt einzeln eine obere limite
         * existiert, so dass man arrays von orders machen kann. */

        /* Ein Händler kann nur 10 Güter pro Talentpunkt handeln. */
        k = u->number * 10 * eff_skill(u, SK_TRADE, r);

        /* hat der Händler bereits gehandelt, muss die Menge der bereits
         * verkauften/gekauften Güter abgezogen werden */
        a = a_find(u->attribs, &at_trades);
        if (!a) {
            a = a_add(&u->attribs, a_new(&at_trades));
        }
        else {
            k -= a->data.i;
        }

        n = _min(n, k);
        assert(n >= 0);
        /* die Menge der verkauften Güter merken */
        a->data.i += n;
        o = (request *)calloc(1, sizeof(request));
        o->unit = u;
        o->qty = n;
        o->type.ltype = ltype;
        addlist(sellorders, o);

        return unlimited;
    }
}

/* ------------------------------------------------------------- */

static void expandstealing(region * r, request * stealorders)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    int i;

    assert(rsilver);

    expandorders(r, stealorders);
    if (!norders) return;

    /* Für jede unit in der Region wird Geld geklaut, wenn sie Opfer eines
     * Beklauen-Orders ist. Jedes Opfer muß einzeln behandelt werden.
     *
     * u ist die beklaute unit. oa.unit ist die klauende unit.
     */

    for (i = 0; i != norders && oa[i].unit->n <= oa[i].unit->wants; i++) {
        unit *u = findunitg(oa[i].no, r);
        int n = 0;
        if (u && u->region == r) {
            n = get_pooled(u, rsilver, GET_ALL, INT_MAX);
        }
#ifndef GOBLINKILL
        if (oa[i].type.goblin) {    /* Goblin-Spezialklau */
            int uct = 0;
            unit *u2;
            assert(effskill(oa[i].unit, SK_STEALTH) >= 4
                || !"this goblin\'s skill is too low");
            for (u2 = r->units; u2; u2 = u2->next) {
                if (u2->faction == u->faction) {
                    uct += maintenance_cost(u2);
                }
            }
            n -= uct * 2;
        }
#endif
        if (n > 10 && rplane(r) && (rplane(r)->flags & PFL_NOALLIANCES)) {
            /* In Questen nur reduziertes Klauen */
            n = 10;
        }
        if (n > 0) {
            n = _min(n, oa[i].unit->wants);
            use_pooled(u, rsilver, GET_ALL, n);
            oa[i].unit->n = n;
            change_money(oa[i].unit, n);
            ADDMSG(&u->faction->msgs, msg_message("stealeffect", "unit region amount",
                u, u->region, n));
        }
        add_income(oa[i].unit, IC_STEAL, oa[i].unit->wants, oa[i].unit->n);
        fset(oa[i].unit, UFL_LONGACTION | UFL_NOTMOVING);
    }
    free(oa);
}

/* ------------------------------------------------------------- */
static void plant(region * r, unit * u, int raw)
{
    int n, i, skill, planted = 0;
    const item_type *itype;
    const resource_type *rt_water = get_resourcetype(R_WATER_OF_LIFE);

    assert(rt_water != NULL);
    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }
    if (rherbtype(r) == NULL) {
        cmistake(u, u->thisorder, 108, MSG_PRODUCE);
        return;
    }

    /* Skill prüfen */
    skill = eff_skill(u, SK_HERBALISM, r);
    itype = rherbtype(r);
    if (skill < 6) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "plant_skills",
            "skill minskill product", SK_HERBALISM, 6, itype->rtype, 1));
        return;
    }
    /* Wasser des Lebens prüfen */
    if (get_pooled(u, rt_water, GET_DEFAULT, 1) == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rt_water));
        return;
    }
    n = get_pooled(u, itype->rtype, GET_DEFAULT, skill * u->number);
    /* Kräuter prüfen */
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing",
            itype->rtype));
        return;
    }

    n = _min(skill * u->number, n);
    n = _min(raw, n);
    /* Für jedes Kraut Talent*10% Erfolgschance. */
    for (i = n; i > 0; i--) {
        if (rng_int() % 10 < skill)
            planted++;
    }
    produceexp(u, SK_HERBALISM, u->number);

    /* Alles ok. Abziehen. */
    use_pooled(u, rt_water, GET_DEFAULT, 1);
    use_pooled(u, itype->rtype, GET_DEFAULT, n);
    rsetherbs(r, rherbs(r) + planted);
    ADDMSG(&u->faction->msgs, msg_message("plant", "unit region amount herb",
        u, r, planted, itype->rtype));
}

static void planttrees(region * r, unit * u, int raw)
{
    int n, i, skill, planted = 0;
    const resource_type *rtype;

    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }

    /* Mallornbäume kann man nur in Mallornregionen züchten */
    rtype = get_resourcetype(fval(r, RF_MALLORN) ? R_MALLORNSEED : R_SEED);

    /* Skill prüfen */
    skill = eff_skill(u, SK_HERBALISM, r);
    if (skill < 6) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "plant_skills",
            "skill minskill product", SK_HERBALISM, 6, rtype, 1));
        return;
    }
    if (fval(r, RF_MALLORN) && skill < 7) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "plant_skills",
            "skill minskill product", SK_HERBALISM, 7, rtype, 1));
        return;
    }

    /* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
    raw = _min(raw, skill * u->number);
    n = get_pooled(u, rtype, GET_DEFAULT, raw);
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
        return;
    }
    n = _min(raw, n);

    /* Für jeden Samen Talent*10% Erfolgschance. */
    for (i = n; i > 0; i--) {
        if (rng_int() % 10 < skill)
            planted++;
    }
    rsettrees(r, 0, rtrees(r, 0) + planted);

    /* Alles ok. Abziehen. */
    produceexp(u, SK_HERBALISM, u->number);
    use_pooled(u, rtype, GET_DEFAULT, n);

    ADDMSG(&u->faction->msgs, msg_message("plant",
        "unit region amount herb", u, r, planted, rtype));
}

/* züchte bäume */
static void breedtrees(region * r, unit * u, int raw)
{
    int n, i, skill, planted = 0;
    const resource_type *rtype;
    static int gamecookie = -1;
    static int current_season;

    if (gamecookie != global.cookie) {
        gamedate date;
        get_gamedate(turn, &date);
        current_season = date.season;
        gamecookie = global.cookie;
    }

    /* Bäume züchten geht nur im Frühling */
    if (current_season != SEASON_SPRING) {
        planttrees(r, u, raw);
        return;
    }

    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }

    /* Mallornbäume kann man nur in Mallornregionen züchten */
    rtype = get_resourcetype(fval(r, RF_MALLORN) ? R_MALLORNSEED : R_SEED);

    /* Skill prüfen */
    skill = eff_skill(u, SK_HERBALISM, r);
    if (skill < 12) {
        planttrees(r, u, raw);
        return;
    }

    /* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
    raw = _min(skill * u->number, raw);
    n = get_pooled(u, rtype, GET_DEFAULT, raw);
    /* Samen prüfen */
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
        return;
    }
    n = _min(raw, n);

    /* Für jeden Samen Talent*5% Erfolgschance. */
    for (i = n; i > 0; i--) {
        if (rng_int() % 100 < skill * 5)
            planted++;
    }
    rsettrees(r, 1, rtrees(r, 1) + planted);

    /* Alles ok. Abziehen. */
    produceexp(u, SK_HERBALISM, u->number);
    use_pooled(u, rtype, GET_DEFAULT, n);

    ADDMSG(&u->faction->msgs, msg_message("plant",
        "unit region amount herb", u, r, planted, rtype));
}

/* züchte pferde */
static void breedhorses(region * r, unit * u)
{
    int n, c, breed = 0;
    struct building *b = inside_building(u);
    const struct building_type *btype = b ? b->type : NULL;
    const struct resource_type *rhorse = get_resourcetype(R_HORSE);
    int horses;
    assert(rhorse && rhorse->itype);
    if (btype != bt_find("stables")) {
        cmistake(u, u->thisorder, 122, MSG_PRODUCE);
        return;
    }
    horses = i_get(u->items, rhorse->itype);
    if (horses < 2) {
        cmistake(u, u->thisorder, 107, MSG_PRODUCE);
        return;
    }
    n = u->number * eff_skill(u, SK_HORSE_TRAINING, r);
    n = _min(n, horses);

    for (c = 0; c < n; c++) {
        if (rng_int() % 100 < eff_skill(u, SK_HORSE_TRAINING, r)) {
            i_change(&u->items, rhorse->itype, 1);
            ++breed;
        }
    }

    produceexp(u, SK_HORSE_TRAINING, u->number);

    ADDMSG(&u->faction->msgs, msg_message("raised", "unit amount", u, breed));
}

static void breed_cmd(unit * u, struct order *ord)
{
    int m;
    const char *s;
    param_t p;
    region *r = u->region;
    const resource_type *rtype = NULL;

    if (r->land == NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_onlandonly", ""));
        return;
    }

    /* züchte [<anzahl>] <parameter> */
    (void)init_order(ord);
    s = getstrtoken();

    m = s ? atoi((const char *)s) : 0;
    if (m != 0) {
        /* first came a want-paramter */
        s = getstrtoken();
    }
    else {
        m = INT_MAX;
    }

    if (!s || !s[0]) {
        p = P_ANY;
    }
    else {
        p = findparam(s, u->faction->locale);
    }

    switch (p) {
    case P_HERBS:
        plant(r, u, m);
        break;
    case P_TREES:
        breedtrees(r, u, m);
        break;
    default:
        if (p != P_ANY) {
            rtype = findresourcetype(s, u->faction->locale);
            if (rtype == get_resourcetype(R_SEED) || rtype == get_resourcetype(R_MALLORNSEED)) {
                breedtrees(r, u, m);
                break;
            }
            else if (rtype != get_resourcetype(R_HORSE)) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_cannotmake", ""));
                break;
            }
        }
        breedhorses(r, u);
        break;
    }
}

static const char *rough_amount(int a, int m)
{
    int p = (a * 100) / m;

    if (p < 10) {
        return "sehr wenige";
    }
    else if (p < 30) {
        return "wenige";
    }
    else if (p < 60) {
        return "relativ viele";
    }
    else if (p < 90) {
        return "viele";
    }
    return "sehr viele";
}

static void research_cmd(unit * u, struct order *ord)
{
    region *r = u->region;
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_RESEARCH);
    /*
       const char *s = getstrtoken();

       if (findparam(s, u->faction->locale) == P_HERBS) { */

    if (eff_skill(u, SK_HERBALISM, r) < 7) {
        cmistake(u, ord, 227, MSG_EVENT);
        return;
    }

    produceexp(u, SK_HERBALISM, u->number);

    if (rherbs(r) > 0) {
        const item_type *itype = rherbtype(r);

        if (itype != NULL) {
            ADDMSG(&u->faction->msgs, msg_message("researchherb",
                "unit region amount herb",
                u, r, rough_amount(rherbs(r), 100), itype->rtype));
        }
        else {
            ADDMSG(&u->faction->msgs, msg_message("researchherb_none",
                "unit region", u, r));
        }
    }
    else {
        ADDMSG(&u->faction->msgs, msg_message("researchherb_none",
            "unit region", u, r));
    }
}

static int max_skill(region * r, faction * f, skill_t sk)
{
    unit *u;
    int w = 0;

    for (u = r->units; u; u = u->next) {
        if (u->faction == f) {
            if (eff_skill(u, sk, r) > w) {
                w = eff_skill(u, sk, r);
            }
        }
    }

    return w;
}

message * check_steal(const unit * u, struct order *ord) {
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

static void steal_cmd(unit * u, struct order *ord, request ** stealorders)
{
    const resource_type *rring = get_resourcetype(R_RING_OF_NIMBLEFINGER);
    int n, i, id;
    bool goblin = false;
    request *o;
    unit *u2 = NULL;
    region *r = u->region;
    faction *f = NULL;
    message * msg;
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_STEAL);

    assert(skill_enabled(SK_PERCEPTION) && skill_enabled(SK_STEALTH));

    msg = check_steal(u, ord);
    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
        return;
    }
    id = read_unitid(u->faction, r);
    u2 = findunitr(r, id);

    if (u2 && u2->region == u->region) {
        f = u2->faction;
    }
    else {
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

    n = eff_skill(u, SK_STEALTH, r) - max_skill(r, f, SK_PERCEPTION);

    if (n <= 0) {
        /* Wahrnehmung == Tarnung */
        if (u_race(u) != get_race(RC_GOBLIN) || eff_skill(u, SK_STEALTH, r) <= 3) {
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

    i = _min(u->number, i_get(u->items, rring->itype));
    if (i > 0) {
        n *= STEALINCOME * (u->number + i * (roqf_factor() - 1));
    }
    else {
        n *= u->number * STEALINCOME;
    }

    u->wants = n;

    /* wer dank unsichtbarkeitsringen klauen kann, muss nicht unbedingt ein
     * guter dieb sein, schliesslich macht man immer noch sehr viel laerm */

    o = (request *)calloc(1, sizeof(request));
    o->unit = u;
    o->qty = 1;                   /* Betrag steht in u->wants */
    o->no = u2->no;
    o->type.goblin = goblin;      /* Merken, wenn Goblin-Spezialklau */
    addlist(stealorders, o);

    /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */

    produceexp(u, SK_STEALTH, _min(n, u->number));
}

/* ------------------------------------------------------------- */

static void expandentertainment(region * r)
{
    unit *u;
    int m = entertainmoney(r);
    request *o;

    for (o = &entertainers[0]; o != nextentertainer; ++o) {
        double part = m / (double)entertaining;
        u = o->unit;
        if (entertaining <= m)
            u->n = o->qty;
        else
            u->n = (int)(o->qty * part);
        change_money(u, u->n);
        rsetmoney(r, rmoney(r) - u->n);
        m -= u->n;
        entertaining -= o->qty;

        /* Nur soviel PRODUCEEXP wie auch tatsächlich gemacht wurde */
        produceexp(u, SK_ENTERTAINMENT, _min(u->n, u->number));
        add_income(u, IC_ENTERTAIN, o->qty, u->n);
        fset(u, UFL_LONGACTION | UFL_NOTMOVING);
    }
}

void entertain_cmd(unit * u, struct order *ord)
{
    region *r = u->region;
    int max_e;
    request *o;
    static int entertainbase = 0;
    static int entertainperlevel = 0;
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_ENTERTAIN);
    if (!entertainbase) {
        const char *str = get_param(global.parameters, "entertain.base");
        entertainbase = str ? atoi(str) : 0;
    }
    if (!entertainperlevel) {
        const char *str = get_param(global.parameters, "entertain.perlevel");
        entertainperlevel = str ? atoi(str) : 0;
    }
    if (fval(u, UFL_WERE)) {
        cmistake(u, ord, 58, MSG_INCOME);
        return;
    }
    if (!effskill(u, SK_ENTERTAINMENT)) {
        cmistake(u, ord, 58, MSG_INCOME);
        return;
    }
    if (besieged(u)) {
        cmistake(u, ord, 60, MSG_INCOME);
        return;
    }
    if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return;
    }
    if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
        cmistake(u, ord, 28, MSG_INCOME);
        return;
    }

    u->wants = u->number * (entertainbase + effskill(u, SK_ENTERTAINMENT)
        * entertainperlevel);

    max_e = getuint();
    if (max_e != 0) {
        u->wants = _min(u->wants, max_e);
    }
    o = nextentertainer++;
    o->unit = u;
    o->qty = u->wants;
    entertaining += o->qty;
}

/**
 * \return number of working spaces taken by players
 */
static void
expandwork(region * r, request * work_begin, request * work_end, int maxwork)
{
    int earnings;
    /* n: verbleibende Einnahmen */
    /* fishes: maximale Arbeiter */
    int jobs = maxwork;
    int p_wage = wage(r, NULL, NULL, turn);
    int money = rmoney(r);
    request *o;

    for (o = work_begin; o != work_end; ++o) {
        unit *u = o->unit;
        int workers;

        if (u->number == 0)
            continue;

        if (jobs >= working)
            workers = u->number;
        else {
            workers = u->number * jobs / working;
            if (rng_int() % working < (u->number * jobs) % working)
                workers++;
        }

        assert(workers >= 0);

        u->n = workers * wage(u->region, u->faction, u_race(u), turn);

        jobs -= workers;
        assert(jobs >= 0);

        change_money(u, u->n);
        working -= o->unit->number;
        add_income(u, IC_WORK, o->qty, u->n);
        fset(u, UFL_LONGACTION | UFL_NOTMOVING);
    }

    if (jobs > rpeasants(r)) {
        jobs = rpeasants(r);
    }
    earnings = jobs * p_wage;
    if (rule_blessed_harvest() == HARVEST_TAXES) {
        /* E3 rules */
        static const curse_type *blessedharvest_ct;
        if (!blessedharvest_ct) {
            blessedharvest_ct = ct_find("blessedharvest");
        }
        if (blessedharvest_ct && r->attribs) {
            int happy =
                (int)curse_geteffect(get_curse(r->attribs, blessedharvest_ct));
            happy = _min(happy, jobs);
            earnings += happy;
        }
    }
    rsetmoney(r, money + earnings);
}

static int do_work(unit * u, order * ord, request * o)
{
    if (playerrace(u_race(u))) {
        region *r = u->region;
        int w;

        if (fval(u, UFL_WERE)) {
            if (ord)
                cmistake(u, ord, 313, MSG_INCOME);
            return -1;
        }
        if (besieged(u)) {
            if (ord)
                cmistake(u, ord, 60, MSG_INCOME);
            return -1;
        }
        if (u->ship && is_guarded(r, u, GUARD_CREWS)) {
            if (ord)
                cmistake(u, ord, 69, MSG_INCOME);
            return -1;
        }
        w = wage(r, u->faction, u_race(u), turn);
        u->wants = u->number * w;
        o->unit = u;
        o->qty = u->number * w;
        working += u->number;
        return 0;
    }
    else if (ord && !is_monsters(u->faction)) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "race_cantwork", "race", u_race(u)));
    }
    return -1;
}

static void expandloot(region * r, request * lootorders)
{
    unit *u;
    int i;
    int looted = 0;
    int startmoney = rmoney(r);

    expandorders(r, lootorders);
    if (!norders)
        return;

    for (i = 0; i != norders && rmoney(r) > TAXFRACTION * 2; i++) {
        change_money(oa[i].unit, TAXFRACTION);
        oa[i].unit->n += TAXFRACTION;
        /*Looting destroys double the money*/
        rsetmoney(r, rmoney(r) - TAXFRACTION * 2);
        looted = looted + TAXFRACTION * 2;
    }
    free(oa);
    
    /* Lowering morale by 1 depending on the looted money (+20%) */
    if (rng_int() % 100 < ((looted / startmoney) + 0.2)) {
        int m = region_get_morale(r);
        if (m) {
            /*Nur Moral -1, turns is not changed, so the first time nothing happens if the morale is good*/
            region_set_morale(r, m-1, -1);
        }
    }
    
    for (u = r->units; u; u = u->next) {
        if (u->n >= 0) {
            add_income(u, IC_LOOT, u->wants, u->n);
            fset(u, UFL_LONGACTION | UFL_NOTMOVING);
        }
    }
}

static void expandtax(region * r, request * taxorders)
{
    unit *u;
    int i;

    expandorders(r, taxorders);
    if (!norders)
        return;

    for (i = 0; i != norders && rmoney(r) > TAXFRACTION; i++) {
        change_money(oa[i].unit, TAXFRACTION);
        oa[i].unit->n += TAXFRACTION;
        rsetmoney(r, rmoney(r) - TAXFRACTION);
    }
    free(oa);

    for (u = r->units; u; u = u->next) {
        if (u->n >= 0) {
            add_income(u, IC_TAX, u->wants, u->n);
            fset(u, UFL_LONGACTION | UFL_NOTMOVING);
        }
    }
}

void tax_cmd(unit * u, struct order *ord, request ** taxorders)
{
    /* Steuern werden noch vor der Forschung eingetrieben */
    region *r = u->region;
    unit *u2;
    int n;
    request *o;
    int max;
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_TAX);

    if (!humanoidrace(u_race(u)) && !is_monsters(u->faction)) {
        cmistake(u, ord, 228, MSG_INCOME);
        return;
    }

    if (fval(u, UFL_WERE)) {
        cmistake(u, ord, 228, MSG_INCOME);
        return;
    }

    if (besieged(u)) {
        cmistake(u, ord, 60, MSG_INCOME);
        return;
    }
    n = armedmen(u, false);

    if (!n) {
        cmistake(u, ord, 48, MSG_INCOME);
        return;
    }

    max = getuint();

    if (max == 0)
        max = INT_MAX;
    if (!playerrace(u_race(u))) {
        u->wants = _min(income(u), max);
    }
    else {
        u->wants = _min(n * eff_skill(u, SK_TAXING, r) * 20, max);
    }

    u2 = is_guarded(r, u, GUARD_TAX);
    if (u2) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "region_guarded", "guard", u2));
        return;
    }

    /* die einnahmen werden in fraktionen von 10 silber eingeteilt: diese
     * fraktionen werden dann bei eintreiben unter allen eintreibenden
     * einheiten aufgeteilt. */

    o = (request *)calloc(1, sizeof(request));
    o->qty = u->wants / TAXFRACTION;
    o->unit = u;
    addlist(taxorders, o);
    return;
}

void loot_cmd(unit * u, struct order *ord, request ** lootorders)
{
    region *r = u->region;
    unit *u2;
    int n;
    int max;
    request *o;
    keyword_t kwd;

    kwd = init_order(ord);
    assert(kwd == K_LOOT);

    if (get_param_int(global.parameters, "rules.enable_loot", 0) == 0 && !is_monsters(u->faction)) {
        return;
    }

    if (!humanoidrace(u_race(u)) && !is_monsters(u->faction)) {
        cmistake(u, ord, 228, MSG_INCOME);
        return;
    }

    if (fval(u, UFL_WERE)) {
        cmistake(u, ord, 228, MSG_INCOME);
        return;
    }

    if (besieged(u)) {
        cmistake(u, ord, 60, MSG_INCOME);
        return;
    }
    n = armedmen(u, false);

    if (!n) {
        cmistake(u, ord, 48, MSG_INCOME);
        return;
    }

    u2 = is_guarded(r, u, GUARD_TAX);
    if (u2) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "region_guarded", "guard", u2));
        return;
    }

    max = getuint();

    if (max == 0)
        max = INT_MAX;
    if (!playerrace(u_race(u))) {
        u->wants = _min(income(u), max);
    }
    else {
        /* For player start with 20 Silver +10 every 5 level of close combat skill*/
        int skbonus = (_max(eff_skill(u, SK_MELEE, r), eff_skill(u, SK_SPEAR, r)) * 2 / 10) + 2;
        u->wants = _min(n * skbonus * 10, max);
    }

    o = (request *)calloc(1, sizeof(request));
    o->qty = u->wants / TAXFRACTION;
    o->unit = u;
    addlist(lootorders, o);

    return;
}

#define MAX_WORKERS 2048
void auto_work(region * r)
{
    request workers[MAX_WORKERS];
    request *nextworker = workers;
    unit *u;

    for (u = r->units; u; u = u->next) {
        if (!(u->flags & UFL_LONGACTION) && !is_monsters(u->faction)) {
            if (do_work(u, NULL, nextworker) == 0) {
                assert(nextworker - workers < MAX_WORKERS);
                ++nextworker;
            }
        }
    }
    if (nextworker != workers) {
        expandwork(r, workers, nextworker, maxworkingpeasants(r));
    }
}

static void peasant_taxes(region * r)
{
    faction *f;
    unit *u;
    building *b;
    int money;
    int maxsize;

    f = region_get_owner(r);
    if (f == NULL || is_mourning(r, turn)) {
        return;
    }
    money = rmoney(r);
    if (money <= 0)
        return;

    b = largestbuilding(r, cmp_taxes, false);
    if (b == NULL)
        return;

    u = building_owner(b);
    if (u == NULL || u->faction != f)
        return;

    maxsize = buildingeffsize(b, false);
    if (maxsize > 0) {
        double taxfactor = money * b->type->taxes(b, maxsize);
        double morale = money * region_get_morale(r) * MORALE_TAX_FACTOR;
        if (taxfactor > morale)
            taxfactor = morale;
        if (taxfactor > 0) {
            int taxmoney = (int)taxfactor;
            change_money(u, taxmoney);
            rsetmoney(r, money - taxmoney);
            ADDMSG(&u->faction->msgs, msg_message("income_tax",
                "unit region amount", u, r, taxmoney));
        }
    }
}

void produce(struct region *r)
{
    request workers[MAX_WORKERS];
    request *taxorders, *lootorders, *sellorders, *stealorders, *buyorders;
    unit *u;
    static int rule_autowork = -1;
    bool limited = true;
    request *nextworker = workers;
    assert(r);

    /* das sind alles befehle, die 30 tage brauchen, und die in thisorder
     * stehen! von allen 30-tage befehlen wird einfach der letzte verwendet
     * (dosetdefaults).
     *
     * kaufen vor einnahmequellen. da man in einer region dasselbe produkt
     * nicht kaufen und verkaufen kann, ist die reihenfolge wegen der
     * produkte egal. nicht so wegen dem geld.
     *
     * lehren vor lernen. */

    if (rule_autowork < 0) {
        rule_autowork = get_param_int(global.parameters, "work.auto", 0);
    }

    assert(rmoney(r) >= 0);
    assert(rpeasants(r) >= 0);

    if (r->land && rule_auto_taxation()) {
        /* new taxation rules, region owners make money based on morale and building */
        peasant_taxes(r);
    }

    buyorders = 0;
    sellorders = 0;
    working = 0;
    nextentertainer = &entertainers[0];
    entertaining = 0;
    taxorders = 0;
    lootorders = 0;
    stealorders = 0;

    for (u = r->units; u; u = u->next) {
        order *ord;
        bool trader = false;
        keyword_t todo;

        if (u_race(u) == get_race(RC_SPELL) || fval(u, UFL_LONGACTION))
            continue;

        if (u_race(u) == get_race(RC_INSECT) && r_insectstalled(r) &&
            !is_cursed(u->attribs, C_KAELTESCHUTZ, 0))
            continue;

        if (fval(u, UFL_LONGACTION) && u->thisorder == NULL) {
            /* this message was already given in laws.setdefaults
               cmistake(u, u->thisorder, 52, MSG_PRODUCE);
               */
            continue;
        }

        for (ord = u->orders; ord; ord = ord->next) {
            keyword_t kwd = getkeyword(ord);
            if (kwd == K_BUY) {
                buy(u, &buyorders, ord);
                trader = true;
            }
            else if (kwd == K_SELL) {
                /* sell returns true if the sale is not limited
                 * by the region limit */
                limited &= !sell(u, &sellorders, ord);
                trader = true;
            }
        }
        if (trader) {
            attrib *a = a_find(u->attribs, &at_trades);
            if (a && a->data.i) {
                produceexp(u, SK_TRADE, u->number);
            }
            fset(u, UFL_LONGACTION | UFL_NOTMOVING);
            continue;
        }

        todo = getkeyword(u->thisorder);
        if (todo == NOKEYWORD)
            continue;

        if (fval(r->terrain, SEA_REGION) && u_race(u) != get_race(RC_AQUARIAN)
            && !(u_race(u)->flags & RCF_SWIM)
            && todo != K_STEAL && todo != K_SPY && todo != K_SABOTAGE)
            continue;

        switch (todo) {
        case K_ENTERTAIN:
            entertain_cmd(u, u->thisorder);
            break;

        case K_WORK:
            if (!rule_autowork && do_work(u, u->thisorder, nextworker) == 0) {
                assert(nextworker - workers < MAX_WORKERS);
                ++nextworker;
            }
            break;

        case K_TAX:
            tax_cmd(u, u->thisorder, &taxorders);
            break;

        case K_LOOT:
            loot_cmd(u, u->thisorder, &lootorders);
            break;

        case K_STEAL:
            steal_cmd(u, u->thisorder, &stealorders);
            break;

        case K_SPY:
            spy_cmd(u, u->thisorder);
            break;

        case K_SABOTAGE:
            sabotage_cmd(u, u->thisorder);
            break;

        case K_PLANT:
        case K_BREED:
            breed_cmd(u, u->thisorder);
            break;

        case K_RESEARCH:
            research_cmd(u, u->thisorder);
            break;
        default:
            /* not handled here */
            break;
        }
    }

    /* Entertainment (expandentertainment) und Besteuerung (expandtax) vor den
     * Befehlen, die den Bauern mehr Geld geben, damit man aus den Zahlen der
     * letzten Runde berechnen kann, wieviel die Bauern für Unterhaltung
     * auszugeben bereit sind. */
    if (entertaining)
        expandentertainment(r);
    if (!rule_autowork) {
        expandwork(r, workers, nextworker, maxworkingpeasants(r));
    }
    if (taxorders)
        expandtax(r, taxorders);

    if (lootorders)
        expandloot(r, lootorders);

    /* An erster Stelle Kaufen (expandbuying), die Bauern so Geld bekommen, um
     * nachher zu beim Verkaufen (expandselling) den Spielern abkaufen zu
     * können. */

    if (buyorders)
        expandbuying(r, buyorders);

    if (sellorders) {
        int limit = rpeasants(r) / TRADE_FRACTION;
        if (r->terrain == newterrain(T_DESERT)
            && buildingtype_exists(r, bt_find("caravan"), true))
            limit *= 2;
        expandselling(r, sellorders, limited ? limit : INT_MAX);
    }

    /* Die Spieler sollen alles Geld verdienen, bevor sie beklaut werden
     * (expandstealing). */

    if (stealorders)
        expandstealing(r, stealorders);

    assert(rmoney(r) >= 0);
    assert(rpeasants(r) >= 0);
}
