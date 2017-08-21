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
#include "donations.h"
#include "guard.h"
#include "give.h"
#include "laws.h"
#include "randenc.h"
#include "spy.h"
#include "study.h"
#include "move.h"
#include "monsters.h"
#include "morale.h"
#include "reports.h"
#include "calendar.h"

#include <attributes/reduceproduction.h>
#include <attributes/racename.h>
#include <spells/buildingcurse.h>
#include <spells/regioncurse.h>

/* kernel includes */
#include <kernel/ally.h>
#include <kernel/building.h>
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

static unsigned int norders;
static request *g_requests;

#define RECRUIT_MERGE 1
static int rules_recruit = -1;

static void recruit_init(void)
{
    if (rules_recruit < 0) {
        rules_recruit = 0;
        if (config_get_int("recruit.allow_merge", 1)) {
            rules_recruit |= RECRUIT_MERGE;
        }
    }
}

int entertainmoney(const region * r)
{
    double n;

    if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
        return 0;
    }

    n = rmoney(r) / (double)ENTERTAINFRACTION;

    if (is_cursed(r->attribs, C_GENEROUS, 0)) {
        n *= get_curseeffect(r->attribs, C_GENEROUS, 0);
    }

    return (int)n;
}

int income(const unit * u)
{
    const race *rc = u_race(u);
    return rc->income * u->number;
}

static void scramble(void *data, unsigned int n, size_t width)
{
    unsigned int j;
    char temp[64];
    assert(width <= sizeof(temp));
    for (j = 0; j != n; ++j) {
        unsigned int k = rng_uint() % n;
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
        g_requests = (request *)calloc(norders, sizeof(request));
        for (o = requests; o; o = o->next) {
            if (o->qty > 0) {
                unsigned int j;
                for (j = o->qty; j; j--) {
                    g_requests[i] = *o;
                    g_requests[i].unit->n = 0;
                    i++;
                }
            }
        }
        scramble(g_requests, norders, sizeof(request));
    }
    else {
        g_requests = NULL;
    }
    while (requests) {
        request *o = requests->next;
        free_order(requests->ord);
        free(requests);
        requests = o;
    }
}

/* ------------------------------------------------------------- */

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

void add_recruits(unit * u, int number, int wanted)
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
            double multi = 2.0 * rc->recruit_multi;

            number = MIN(req->qty, (int)(get / multi));
            if (rc->recruitcost) {
                int afford = get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT,
                    number * rc->recruitcost) / rc->recruitcost;
                number = MIN(number, afford);
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
            add_recruits(u, number, req->qty);
            if (number > 0) {
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
            free_order(req->ord);
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
    if (is_monsters(f) || valid_race(f, rc)) {
        return rc->recruitcost;
    }
    return -1;
}

static void recruit(unit * u, struct order *ord, request ** recruitorders)
{
    region *r = u->region;
    plane *pl;
    request *o;
    int recruitcost = -1;
    const faction *f = u->faction;
    const struct race *rc = u_race(u);
    const char *str;
    int n;

    init_order(ord);
    n = getint();
    if (n <= 0) {
        syntax_error(u, ord);
        return;
    }

    if (u->number == 0) {
        char token[128];
        str = gettoken(token, sizeof(token));
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

    /* this is a very special case because the recruiting unit may be empty
     * at this point and we have to look at the creating unit instead. This
     * is done in cansee, which is called indirectly by is_guarded(). */
    if (is_guarded(r, u)) {
        cmistake(u, ord, 70, MSG_EVENT);
        return;
    }

    if (rc == get_race(RC_INSECT)) {
        gamedate date;
        get_gamedate(turn, &date);
        if (date.season == 0 && r->terrain != newterrain(T_DESERT)) {
            bool usepotion = false;
            unit *u2;

            for (u2 = r->units; u2; u2 = u2->next)
                if (fval(u2, UFL_WARMTH)) {
                    usepotion = true;
                    break;
                }
            if (!usepotion)
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
    if (!playerrace(rc)) {
        cmistake(u, ord, 139, MSG_EVENT);
        return;
    }

    if (fval(u, UFL_HERO)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_herorecruit", ""));
        return;
    }
    if (has_skill(u, SK_MAGIC)) {
        /* error158;de;{unit} in {region}: '{command}' - Magier arbeiten
         * grunds�tzlich nur alleine! */
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
        n = MIN(n, pooled / recruitcost);
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
                building *b = largestbuilding(r, cmp_current_owner, false);
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
    char token[128];
    region *r = u->region;
    unit *u2;
    const char *s;

    init_order(ord);
    getunit(r, u->faction, &u2);

    s = gettoken(token, sizeof(token));
    if (s && isparam(s, u->faction->locale, P_CONTROL)) {
        bool okay = false;
        if (!can_give_to(u, u2)) {
            ADDMSG(&u->faction->msgs,
                msg_feedback(u, ord, "feedback_unit_not_found", ""));
        }
        else if (!u->building && !u->ship) {
            cmistake(u, ord, 140, MSG_EVENT);
        }
        else if (u->building) {
            if (u != building_owner(u->building)) {
                cmistake(u, ord, 49, MSG_EVENT);
            }
            else if (u2->building != u->building) {
                cmistake(u, ord, 33, MSG_EVENT);
            }
            else {
                okay = true;
            }
        }
        else if (u->ship) {
            if (u != ship_owner(u->ship)) {
                cmistake(u, ord, 49, MSG_EVENT);
            }
            else if (u2->ship != u->ship) {
                cmistake(u, ord, 32, MSG_EVENT);
            }
            else {
                okay = true;
            }
        }
        if (okay) {
            message *msg = msg_message("givecommand", "unit recipient", u, u2);
            add_message(&u->faction->msgs, msg);
            if (u->faction != u2->faction) {
                add_message(&u2->faction->msgs, msg);
            }
            msg_release(msg);
            give_control(u, u2);
        }
    }
    return 0;
}

static int forget_cmd(unit * u, order * ord)
{
    char token[128];
    skill_t sk;
    const char *s;

    if (is_cursed(u->attribs, C_SLAVE, 0)) {
        /* charmed units shouldn't be losing their skills */
        return 0;
    }

    init_order(ord);
    s = gettoken(token, sizeof(token));

    if ((sk = get_skill(s, u->faction->locale)) != NOSKILL) {
        ADDMSG(&u->faction->msgs, msg_message("forget", "unit skill", u, sk));
        set_level(u, sk, 0);
    }
    return 0;
}

static int maintain(building * b)
{
    int c;
    region *r = b->region;
    bool paid = true;
    unit *u;

    if (fval(b, BLD_MAINTAINED) || b->type == NULL || b->type->maintenance == NULL) {
        return BLD_MAINTAINED;
    }
    if (fval(b, BLD_DONTPAY)) {
        return 0;
    }
    u = building_owner(b);
    if (u == NULL) {
        /* no owner - send a message to the entire region */
        ADDMSG(&r->msgs, msg_message("maintenance_noowner", "building", b));
        return 0;
    }
    /* If the owner is the region owner, check if dontpay flag is set for the building where he is in */
    if (config_token("rules.region_owner_pay_building", b->type->_name)) {
        if (fval(u->building, BLD_DONTPAY)) {
            return 0;
        }
    }
    for (c = 0; b->type->maintenance[c].number && paid; ++c) {
        const maintenance *m = b->type->maintenance + c;
        int need = m->number;

        if (fval(m, MTF_VARIABLE))
            need = need * b->size;
        need -= get_pooled(u, m->rtype, GET_DEFAULT, need);
        if (need > 0) {
            paid = false;
        }
    }
    if (fval(b, BLD_DONTPAY)) {
        ADDMSG(&r->msgs, msg_message("maintenance_nowork", "building", b));
        return 0;
    }
    if (!paid) {
        ADDMSG(&u->faction->msgs, msg_message("maintenancefail", "unit building", u, b));
        ADDMSG(&r->msgs, msg_message("maintenance_nowork", "building", b));
        return 0;
    }
    for (c = 0; b->type->maintenance[c].number; ++c) {
        const maintenance *m = b->type->maintenance + c;
        int cost = m->number;

        if (fval(m, MTF_VARIABLE)) {
            cost = cost * b->size;
        }
        cost -=
            use_pooled(u, m->rtype, GET_SLACK | GET_RESERVE | GET_POOLED_SLACK,
                cost);
        assert(cost == 0);
    }
    ADDMSG(&u->faction->msgs, msg_message("maintenance", "unit building", u, b));
    return BLD_MAINTAINED;
}

void maintain_buildings(region * r)
{
    building **bp = &r->buildings;
    while (*bp) {
        building *b = *bp;
        int flags = BLD_MAINTAINED;

        if (!curse_active(get_curse(b->attribs, &ct_nocostbuilding))) {
            flags = maintain(b);
        }
        fset(b, flags);
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

    if (recruitorders) {
        expandrecruit(r, recruitorders);
    }
    remove_empty_units_in_region(r);

    for (u = r->units; u; u = u->next) {
        order *ord = u->thisorder;
        keyword_t kwd = getkeyword(ord);
        if (kwd == K_DESTROY) {
            if (destroy_cmd(u, ord) == 0) {
                fset(u, UFL_LONGACTION | UFL_NOTMOVING);
            }
        }
    }

}

static void mod_skill(const resource_mod *mod, skill_t sk, int *skill) {
    skill_t msk;
    assert(mod->type == RMT_PROD_SKILL);
    msk = (skill_t)mod->value.sa[0];
    if (msk == NOSKILL || msk == sk) {
        *skill += mod->value.sa[1];
    }
}

static struct message * get_modifiers(unit *u, skill_t sk, const resource_type *rtype, variant *savep, int *skillp) {
    struct building *b = inside_building(u);
    const struct building_type *btype = building_is_active(b) ? b->type : NULL;
    int save_n = 1, save_d = 1;
    int skill = 0;
    int need_race = 0, need_bldg = 0;
    resource_mod *mod;
    const struct building_type *btype_needed = NULL;

    if (btype && btype->modifiers) {
        for (mod = btype->modifiers; mod && mod->type != RMT_END; ++mod) {
            if (mod->type == RMT_PROD_SKILL) {
                mod_skill(mod, sk, &skill);
            }
        }
    }

    for (mod = rtype->modifiers; mod && mod->type != RMT_END; ++mod) {
        if (mod->btype == NULL || mod->btype == btype) {
            if (mod->race == NULL || mod->race == u_race(u)) {
                switch (mod->type) {
                case RMT_PROD_SAVE:
                    if (savep) {
                        save_n *= mod->value.sa[0];
                        save_d *= mod->value.sa[1];
                    }
                    break;
                case RMT_PROD_SKILL:
                    mod_skill(mod, sk, &skill);
                    break;
                case RMT_PROD_REQUIRE:
                    if (mod->race) need_race |= 1;
                    if (mod->btype) {
                        need_bldg |= 1;
                    }
                    break;
                default:
                    /* is not a production modifier, ignore it */
                    break;
                }
            }
        }
        if (mod->type == RMT_PROD_REQUIRE) {
            if (mod->race) need_race |= 2;
            if (mod->btype) {
                btype_needed = mod->btype;
                need_bldg |= 2;
            }
        }
    }
    if (need_race == 2) {
        return msg_error(u, u->thisorder, 117);
    }
    if (btype_needed && need_bldg == 2) {
        return msg_feedback(u, u->thisorder, "building_needed", "building", btype_needed->_name);
    }
    *skillp = skill;
    if (savep) *savep = frac_make(save_n, save_d);
    return NULL;
}

static void manufacture(unit * u, const item_type * itype, int want)
{
    int n;
    int minskill = itype->construction->minskill;
    skill_t sk = itype->construction->skill;
    message *msg;
    int skill_mod;

    msg = get_modifiers(u, sk, itype->rtype, NULL, &skill_mod);
    if (msg) {
        ADDMSG(&u->faction->msgs, msg);
        return;
    }

    if (want == 0) {
        want = maxbuild(u, itype->construction);
    }
    n = build(u, itype->construction, 0, want, skill_mod);
    switch (n) {
    case ENEEDSKILL:
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "skill_needed", "skill", sk));
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
        ADDMSG(&u->faction->msgs, msg_message("produce",
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
    variant save;
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
    const resource_type *rring;
    int amount, skill, skill_mod = 0;
    variant save_mod;
    skill_t sk;

    /* momentan kann man keine ressourcen abbauen, wenn man daf�r
     * Materialverbrauch hat: */
    assert(itype != NULL && (itype->construction == NULL
        || itype->construction->materials == NULL));

    sk = itype->construction->skill;
    if (!rtype->raw) {
        int avail = limit_resource(r, rtype);
        if (avail <= 0) {
            cmistake(u, u->thisorder, 121, MSG_PRODUCE);
            return;
        }
    }

    if (besieged(u)) {
        cmistake(u, u->thisorder, 60, MSG_PRODUCE);
        return;
    }

    if (rtype->modifiers) {
        message *msg = get_modifiers(u, sk, rtype, &save_mod, &skill_mod);
        if (msg) {
            ADDMSG(&u->faction->msgs, msg);
            return;
        }
    }
    else {
        save_mod.sa[0] = 1;
        save_mod.sa[1] = 1;
    }

    /* Bergw�chter k�nnen Abbau von Eisen/Laen durch Bewachen verhindern.
     * Als magische Wesen 'sehen' Bergw�chter alles und werden durch
     * Belagerung nicht aufgehalten.  (Ansonsten wie oben bei Elfen anpassen).
     */
    if (itype->rtype && (itype->rtype == get_resourcetype(R_IRON) || itype->rtype == rt_find("laen"))) {
        unit *u2;
        for (u2 = r->units; u2; u2 = u2->next) {
            if (is_guard(u)
                && !fval(u2, UFL_ISNEW)
                && u2->number && !alliedunit(u2, u->faction, HELP_GUARD)) {
                ADDMSG(&u->faction->msgs,
                    msg_feedback(u, u->thisorder, "region_guarded", "guard", u2));
                return;
            }
        }
    }

    assert(sk != NOSKILL || "limited resource needs a required skill for making it");
    skill = effskill(u, sk, 0);
    if (skill == 0) {
        add_message(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "skill_needed", "skill", sk));
        return;
    }
    if (skill < itype->construction->minskill) {
        add_message(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "manufacture_skills",
                "skill minskill product", sk, itype->construction->minskill,
                itype->rtype));
        return;
    }
    skill += skill_mod;
    amount = skill * u->number;
    /* nun ist amount die Gesamtproduktion der Einheit (in punkten) */

    /* mit Flinkfingerring verzehnfacht sich die Produktion */
    rring = get_resourcetype(R_RING_OF_NIMBLEFINGER);
    if (rring) {
        int dm = i_get(u->items, rring->itype);
        amount += skill * MIN(u->number, dm) * (roqf_factor() - 1);
    }

    /* Schaffenstrunk: */
    if ((dm = get_effect(u, oldpotiontype[P_DOMORE])) != 0) {
        dm = MIN(dm, u->number);
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
    al->save = save_mod;
    al->next = alist->data;
    al->unit = u;
    alist->data = al;
}

static int required(int want, variant save)
{
    int req = (int)(want * save.sa[0] / save.sa[1]);
    int r = want * save.sa[0] % save.sa[1];
    if (r>0) ++req;
    return req;
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
            int nreq = 0;
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
                    if (effskill(al->unit, itype->construction->skill, 0)
                        >= rm->level + itype->construction->minskill - 1) {
                        if (req) {
                            nreq += req;
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
            need = nreq;

            avail = MIN(avail, nreq);
            if (need > 0) {
                int use = 0;
                for (al = alist; al; al = al->next) {
                    if (!fval(al, AFL_DONE)) {
                        if (avail > 0) {
                            int want = required(al->want - al->get, al->save);
                            int x = avail * want / nreq;
                            int r = (avail * want) % nreq;
                            /* Wenn Rest, dann wuerfeln, ob ich etwas bekomme: */
                            if (r > 0 && rng_int() % nreq < r) ++x;
                            avail -= x;
                            use += x;
                            nreq -= want;
                            need -= x;
                            al->get = MIN(al->want, al->get + x * al->save.sa[1] / al->save.sa[0]);
                        }
                    }
                }
                if (use) {
                    rawmaterial_type *raw = rmt_get(rm->rtype);
                    if (raw && raw->use) {
                        assert(use <= rm->amount);
                        raw->use(rm, r, use);
                    }
                }
                assert(avail == 0 || nreq == 0);
            }
            first = false;
        } while (need > 0);
    }
}

static void
attrib_allocation(const resource_type * rtype, region * r, allocation * alist)
{
    allocation *al;
    int nreq = 0;
    int avail = INT_MAX;

    for (al = alist; al; al = al->next) {
        nreq += required(al->want, al->save);
    }

    if (!rtype->raw) {
        avail = limit_resource(r, rtype);
        if (avail < 0) {
            avail = 0;
        }
    }

    avail = MIN(avail, nreq);
    for (al = alist; al; al = al->next) {
        if (avail > 0) {
            int want = required(al->want, al->save);
            int x = avail * want / nreq;
            int rx = (avail * want) % nreq;
            /* Wenn Rest, dann wuerfeln, ob ich was bekomme: */
            if (rx>0 && rng_int() % nreq < rx) ++x;
            avail -= x;
            nreq -= want;
            al->get = x * al->save.sa[1] / al->save.sa[0];
            al->get = MIN(al->want, al->get);
            if (!rtype->raw) {
                int use = required(al->get, al->save);
                if (use) {
                    produce_resource(r, rtype, use);
                }
            }
        }
    }
    assert(avail == 0 || nreq == 0);
}

typedef void(*allocate_function) (const resource_type *, struct region *,
    struct allocation *);

static allocate_function get_allocator(const struct resource_type *rtype)
{
    if (rtype->raw) {
        return leveled_allocation;
    }
    return attrib_allocation;
}

void split_allocations(region * r)
{
    allocation_list **p_alist = &allocations;
    while (*p_alist) {
        allocation_list *alist = *p_alist;
        const resource_type *rtype = alist->type;
        allocate_function alloc = get_allocator(rtype);
        const item_type *itype = resource2item(rtype);
        allocation **p_al = &alist->data;

        alloc(rtype, r, alist->data);

        while (*p_al) {
            allocation *al = *p_al;
            if (al->get) {
                assert(itype || !"not implemented for non-items");
                i_change(&al->unit->items, itype, al->get);
                produceexp(al->unit, itype->construction->skill, al->unit->number);
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
    built = build(u, ptype->itype->construction, 0, want, 0);
    switch (built) {
    case ELOWSKILL:
    case ENEEDSKILL:
        /* no skill, or not enough skill points to build */
        cmistake(u, u->thisorder, 50, MSG_PRODUCE);
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
        ADDMSG(&u->faction->msgs, msg_message("produce",
            "unit region amount wanted resource", u, u->region, built, want,
            ptype->itype->rtype));
        break;
    }
}

void make_item(unit * u, const item_type * itype, int want)
{
    if (itype->construction && fval(itype->rtype, RTF_LIMITED)) {
        if (is_guarded(u->region, u)) {
            cmistake(u, u->thisorder, 70, MSG_EVENT);
            return;
        }
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
    s = gettoken(token, sizeof(token));

    if (s) {
        m = atoip(s);
        sprintf(ibuf, "%d", m);
        if (!strcmp(ibuf, (const char *)s)) {
            /* a quantity was given */
            s = gettoken(token, sizeof(token));
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
            const char * s = gettoken(token, sizeof(token));
            direction_t d = s ? get_direction(s, u->faction->locale) : NODIRECTION;
            if (d != NODIRECTION) {
                build_road(u, m, d);
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
            continue_ship(u, m);
        }
        return 0;
    }
    else if (p == P_HERBS) {
        herbsearch(u, m);
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
            create_ship(u, stype, m, ord);
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
        make_item(u, itype, m);
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
    } trades[MAXLUXURIES], *trade;
    static int ntrades = 0;
    int i;
    const luxury_type *ltype;

    if (ntrades == 0) {
        for (ntrades = 0, ltype = luxurytypes; ltype; ltype = ltype->next) {
            assert(ntrades < MAXLUXURIES);
            trades[ntrades++].type = ltype;
        }
    }
    for (i = 0; i != ntrades; ++i) {
        trades[i].number = 0;
        trades[i].multi = 1;
    }

    if (!buyorders)
        return;

    /* Initialisation. multiplier ist der Multiplikator auf den
     * Verkaufspreis. F�r max_products Produkte kauft man das Produkt zum
     * einfachen Verkaufspreis, danach erh�ht sich der Multiplikator um 1.
     * counter ist ein Z�hler, der die gekauften Produkte z�hlt. money
     * wird f�r die debug message gebraucht. */

    max_products = rpeasants(r) / TRADE_FRACTION;

    /* Kauf - auch so programmiert, da� er leicht erweiterbar auf mehrere
     * G�ter pro Monat ist. j sind die Befehle, i der Index des
     * gehandelten Produktes. */
    if (max_products > 0) {
        unsigned int j;
        expandorders(r, buyorders);
        if (!norders)
            return;

        for (j = 0; j != norders; j++) {
            int price, multi;
            ltype = g_requests[j].type.ltype;
            trade = trades;
            while (trade->type != ltype)
                ++trade;
            multi = trade->multi;
            price = ltype->price * multi;

            if (get_pooled(g_requests[j].unit, rsilver, GET_DEFAULT,
                price) >= price) {
                unit *u = g_requests[j].unit;
                item *items;

                /* litems z�hlt die G�ter, die verkauft wurden, u->n das Geld, das
                 * verdient wurde. Dies mu� gemacht werden, weil der Preis st�ndig sinkt,
                 * man sich also das verdiente Geld und die verkauften Produkte separat
                 * merken mu�. */
                attrib *a = a_find(u->attribs, &at_luxuries);
                if (a == NULL)
                    a = a_add(&u->attribs, a_new(&at_luxuries));

                items = a->data.v;
                i_change(&items, ltype->itype, 1);
                a->data.v = items;
                i_change(&g_requests[j].unit->items, ltype->itype, 1);
                use_pooled(u, rsilver, GET_DEFAULT, price);
                if (u->n < 0)
                    u->n = 0;
                u->n += price;

                rsetmoney(r, rmoney(r) + price);

                /* Falls mehr als max_products Bauern ein Produkt verkauft haben, steigt
                 * der Preis Multiplikator f�r das Produkt um den Faktor 1. Der Z�hler
                 * wird wieder auf 0 gesetzt. */
                if (++trade->number == max_products) {
                    trade->number = 0;
                    ++trade->multi;
                }
                fset(u, UFL_LONGACTION | UFL_NOTMOVING);
            }
        }
        free(g_requests);

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
    char token[128];
    region *r = u->region;
    int n, k;
    request *o;
    attrib *a;
    const item_type *itype = NULL;
    const luxury_type *ltype = NULL;
    keyword_t kwd;
    const char *s;

    if (u->ship && is_guarded(r, u)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return;
    }
    if (u->ship && is_guarded(r, u)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return;
    }
    /* Im Augenblick kann man nur 1 Produkt kaufen. expandbuying ist aber
     * schon daf�r ausger�stet, mehrere Produkte zu kaufen. */

    kwd = init_order(ord);
    assert(kwd == K_BUY);
    n = getint();
    if (n <= 0) {
        cmistake(u, ord, 26, MSG_COMMERCE);
        return;
    }
    if (besieged(u)) {
        /* Belagerte Einheiten k�nnen nichts kaufen. */
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
        /* ...oder in der Region mu� es eine Burg geben. */
        building *b = 0;
        if (r->buildings) {
            static int cache;
            static const struct building_type *bt_castle;
            if (bt_changed(&cache)) {
                bt_castle = bt_find("castle");
            }

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

    /* Ein H�ndler kann nur 10 G�ter pro Talentpunkt handeln. */
    k = u->number * 10 * effskill(u, SK_TRADE, 0);

    /* hat der H�ndler bereits gehandelt, muss die Menge der bereits
     * verkauften/gekauften G�ter abgezogen werden */
    a = a_find(u->attribs, &at_trades);
    if (!a) {
        a = a_add(&u->attribs, a_new(&at_trades));
    }
    else {
        k -= a->data.i;
    }

    n = MIN(n, k);

    if (!n) {
        cmistake(u, ord, 102, MSG_COMMERCE);
        return;
    }

    assert(n >= 0);
    /* die Menge der verkauften G�ter merken */
    a->data.i += n;

    s = gettoken(token, sizeof(token));
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

/* Steuers�tze in % bei Burggr��e */
static int tax_per_size[7] = { 0, 6, 12, 18, 24, 30, 36 };

static void expandselling(region * r, request * sellorders, int limit)
{
    int money, price, max_products;
    unsigned int j;
    /* int m, n = 0; */
    int maxsize = 0, maxeffsize = 0;
    int taxcollected = 0;
    int hafencollected = 0;
    unit *maxowner = (unit *)NULL;
    building *maxb = (building *)NULL;
    building *b;
    unit *u;
    unit *hafenowner;
    static int counter[MAXLUXURIES];
    static int ncounter = 0;
    static int bt_cache;
    static const struct building_type *castle_bt, *harbour_bt, *caravan_bt;

    if (bt_changed(&bt_cache)) {
        castle_bt = bt_find("castle");
        harbour_bt = bt_find("harbour");
        caravan_bt = bt_find("caravan");
    }
    if (ncounter == 0) {
        const luxury_type *ltype;
        for (ltype = luxurytypes; ltype; ltype = ltype->next) {
            assert(ncounter < MAXLUXURIES);
            ++ncounter;
        }
    }
    memset(counter, 0, sizeof(int) * ncounter);

    if (!sellorders) {            /* NEIN, denn Insekten k�nnen in   || !r->buildings) */
        return;                     /* S�mpfen und W�sten auch so handeln */
    }
    /* Stelle Eigent�mer der gr��ten Burg fest. Bekommt Steuern aus jedem
     * Verkauf. Wenn zwei Burgen gleicher Gr��e bekommt gar keiner etwas. */
    for (b = rbuildings(r); b; b = b->next) {
        if (b->size > maxsize && building_owner(b) != NULL
            && b->type == castle_bt) {
            maxb = b;
            maxsize = b->size;
            maxowner = building_owner(b);
        }
        else if (b->size == maxsize && b->type == castle_bt) {
            maxb = (building *)NULL;
            maxowner = (unit *)NULL;
        }
    }

    hafenowner = owner_buildingtyp(r, harbour_bt);

    if (maxb != (building *)NULL && maxowner != (unit *)NULL) {
        maxeffsize = buildingeffsize(maxb, false);
        if (maxeffsize == 0) {
            maxb = (building *)NULL;
            maxowner = (unit *)NULL;
        }
    }
    /* Die Region muss genug Geld haben, um die Produkte kaufen zu k�nnen. */

    money = rmoney(r);

    /* max_products sind 1/100 der Bev�lkerung, falls soviele Produkte
     * verkauft werden - counter[] - sinkt die Nachfrage um 1 Punkt.
     * multiplier speichert r->demand f�r die debug message ab. */

    max_products = rpeasants(r) / TRADE_FRACTION;
    if (max_products <= 0)
        return;

    if (r->terrain == newterrain(T_DESERT)
        && buildingtype_exists(r, caravan_bt, true)) {
        max_products = rpeasants(r) * 2 / TRADE_FRACTION;
    }
    /* Verkauf: so programmiert, dass er leicht auf mehrere Gueter pro
     * Runde erweitert werden kann. */

    expandorders(r, sellorders);
    if (!norders)
        return;

    for (j = 0; j != norders; j++) {
        const luxury_type *search = NULL;
        const luxury_type *ltype = g_requests[j].type.ltype;
        int multi = r_demand(r, ltype);
        int i;
        int use = 0;
        for (i = 0, search = luxurytypes; search != ltype; search = search->next) {
            /* TODO: this is slow and lame! */
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
            unit *u = g_requests[j].unit;
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

            /* r->money -= price; --- dies wird eben nicht ausgef�hrt, denn die
             * Produkte k�nnen auch als Steuern eingetrieben werden. In der Region
             * wurden Silberst�cke gegen Luxusg�ter des selben Wertes eingetauscht!
             * Falls mehr als max_products Kunden ein Produkt gekauft haben, sinkt
             * die Nachfrage f�r das Produkt um 1. Der Z�hler wird wieder auf 0
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
            use_pooled(g_requests[j].unit, ltype->itype->rtype, GET_DEFAULT, use);
        }
    }
    free(g_requests);

    /* Steuern. Hier werden die Steuern dem Besitzer der gr��ten Burg gegeben. */
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
    char token[128];
    bool unlimited = true;
    const item_type *itype;
    const luxury_type *ltype;
    int n;
    region *r = u->region;
    const char *s;
    keyword_t kwd;
    static int bt_cache;
    static const struct building_type *castle_bt, *caravan_bt;

    if (bt_changed(&bt_cache)) {
        castle_bt = bt_find("castle");
        caravan_bt = bt_find("caravan");
    }

    if (u->ship && is_guarded(r, u)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return false;
    }
    /* sellorders sind KEIN array, weil f�r alle items DIE SELBE resource
     * (das geld der region) aufgebraucht wird. */

    kwd = init_order(ord);
    assert(kwd == K_SELL);
    s = gettoken(token, sizeof(token));

    if (findparam(s, u->faction->locale) == P_ANY) {
        unlimited = false;
        n = rpeasants(r) / TRADE_FRACTION;
        if (r->terrain == newterrain(T_DESERT)
            && buildingtype_exists(r, caravan_bt, true))
            n *= 2;
        if (n == 0) {
            cmistake(u, ord, 303, MSG_COMMERCE);
            return false;
        }
    }
    else {
        n = s ? atoip(s) : 0;
        if (n == 0) {
            cmistake(u, ord, 27, MSG_COMMERCE);
            return false;
        }
    }
    /* Belagerte Einheiten k�nnen nichts verkaufen. */

    if (besieged(u)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error60", ""));
        return false;
    }
    /* In der Region mu� es eine Burg geben. */

    if (u_race(u) == get_race(RC_INSECT)) {
        if (r->terrain != newterrain(T_SWAMP) && r->terrain != newterrain(T_DESERT)
            && !rbuildings(r)) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return false;
        }
    }
    else {
        /* ...oder in der Region mu� es eine Burg geben. */
        building *b = 0;
        if (r->buildings) {
            for (b = r->buildings; b; b = b->next) {
                if (b->type == castle_bt && b->size >= 2) break;
            }
        }
        if (!b) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return false;
        }
    }

    /* Ein H�ndler kann nur 10 G�ter pro Talentpunkt verkaufen. */

    n = MIN(n, u->number * 10 * effskill(u, SK_TRADE, 0));

    if (!n) {
        cmistake(u, ord, 54, MSG_COMMERCE);
        return false;
    }
    s = gettoken(token, sizeof(token));
    itype = s ? finditemtype(s, u->faction->locale) : 0;
    ltype = itype ? resource2luxury(itype->rtype) : 0;
    if (ltype == NULL || itype == NULL) {
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

        /* Wenn andere Einheiten das selbe verkaufen, mu� ihr Zeug abgezogen
         * werden damit es nicht zweimal verkauft wird: */
        for (o = *sellorders; o; o = o->next) {
            if (o->type.ltype == ltype && o->unit->faction == u->faction) {
                int fpool =
                    o->qty - get_pooled(o->unit, itype->rtype, GET_RESERVE, INT_MAX);
                available -= MAX(0, fpool);
            }
        }

        n = MIN(n, available);

        if (n <= 0) {
            cmistake(u, ord, 264, MSG_COMMERCE);
            return false;
        }
        /* Hier wird request->type verwendet, weil die obere limit durch
         * das silber gegeben wird (region->money), welches f�r alle
         * (!) produkte als summe gilt, als nicht wie bei der
         * produktion, wo f�r jedes produkt einzeln eine obere limite
         * existiert, so dass man arrays von orders machen kann. */

         /* Ein H�ndler kann nur 10 G�ter pro Talentpunkt handeln. */
        k = u->number * 10 * effskill(u, SK_TRADE, 0);

        /* hat der H�ndler bereits gehandelt, muss die Menge der bereits
         * verkauften/gekauften G�ter abgezogen werden */
        a = a_find(u->attribs, &at_trades);
        if (!a) {
            a = a_add(&u->attribs, a_new(&at_trades));
        }
        else {
            k -= a->data.i;
        }

        n = MIN(n, k);
        assert(n >= 0);
        /* die Menge der verkauften G�ter merken */
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
    unsigned int j;

    assert(rsilver);

    expandorders(r, stealorders);
    if (!norders) return;

    /* F�r jede unit in der Region wird Geld geklaut, wenn sie Opfer eines
     * Beklauen-Orders ist. Jedes Opfer mu� einzeln behandelt werden.
     *
     * u ist die beklaute unit. oa.unit ist die klauende unit.
     */

    for (j = 0; j != norders && g_requests[j].unit->n <= g_requests[j].unit->wants; j++) {
        unit *u = findunitg(g_requests[j].no, r);
        int n = 0;
        if (u && u->region == r) {
            n = get_pooled(u, rsilver, GET_ALL, INT_MAX);
        }
        if (n > 10 && rplane(r) && (rplane(r)->flags & PFL_NOALLIANCES)) {
            /* In Questen nur reduziertes Klauen */
            n = 10;
        }
        if (n > 0) {
            n = MIN(n, g_requests[j].unit->wants);
            use_pooled(u, rsilver, GET_ALL, n);
            g_requests[j].unit->n = n;
            change_money(g_requests[j].unit, n);
            ADDMSG(&u->faction->msgs, msg_message("stealeffect", "unit region amount",
                u, u->region, n));
        }
        add_income(g_requests[j].unit, IC_STEAL, g_requests[j].unit->wants, g_requests[j].unit->n);
        fset(g_requests[j].unit, UFL_LONGACTION | UFL_NOTMOVING);
    }
    free(g_requests);
}

/* ------------------------------------------------------------- */
static void plant(unit * u, int raw)
{
    int n, i, skill, planted = 0;
    const item_type *itype;
    const resource_type *rt_water = get_resourcetype(R_WATER_OF_LIFE);
    region *r = u->region;

    assert(rt_water != NULL);
    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }
    itype = rherbtype(r);
    if (itype == NULL) {
        cmistake(u, u->thisorder, 108, MSG_PRODUCE);
        return;
    }

    /* Skill pr�fen */
    skill = effskill(u, SK_HERBALISM, 0);
    if (skill < 6) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "plant_skills",
                "skill minskill product", SK_HERBALISM, 6, itype->rtype, 1));
        return;
    }
    /* Wasser des Lebens pr�fen */
    if (get_pooled(u, rt_water, GET_DEFAULT, 1) == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rt_water));
        return;
    }
    n = get_pooled(u, itype->rtype, GET_DEFAULT, skill * u->number);
    /* Kr�uter pr�fen */
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing",
                itype->rtype));
        return;
    }

    n = MIN(skill * u->number, n);
    n = MIN(raw, n);
    /* F�r jedes Kraut Talent*10% Erfolgschance. */
    for (i = n; i > 0; i--) {
        if (rng_int() % 10 < skill)
            planted++;
    }
    produceexp(u, SK_HERBALISM, u->number);

    /* Alles ok. Abziehen. */
    use_pooled(u, rt_water, GET_DEFAULT, 1);
    use_pooled(u, itype->rtype, GET_DEFAULT, n);
    rsetherbs(r, (short)(rherbs(r) + planted));
    ADDMSG(&u->faction->msgs, msg_message("plant", "unit region amount herb",
        u, r, planted, itype->rtype));
}

static void planttrees(unit * u, int raw)
{
    int n, i, skill, planted = 0;
    const resource_type *rtype;
    region * r = u->region;

    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }

    /* Mallornb�ume kann man nur in Mallornregionen z�chten */
    rtype = get_resourcetype(fval(r, RF_MALLORN) ? R_MALLORN_SEED : R_SEED);

    /* Skill pr�fen */
    skill = effskill(u, SK_HERBALISM, 0);
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
    raw = MIN(raw, skill * u->number);
    n = get_pooled(u, rtype, GET_DEFAULT, raw);
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
        return;
    }
    n = MIN(raw, n);

    /* F�r jeden Samen Talent*10% Erfolgschance. */
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

/* z�chte b�ume */
static void breedtrees(unit * u, int raw)
{
    int n, i, skill, planted = 0;
    const resource_type *rtype;
    int current_season;
    region *r = u->region;
    gamedate date;

    get_gamedate(turn, &date);
    current_season = date.season;

    /* B�ume z�chten geht nur im Fr�hling */
    if (current_season != SEASON_SPRING) {
        planttrees(u, raw);
        return;
    }

    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }

    /* Mallornb�ume kann man nur in Mallornregionen z�chten */
    rtype = get_resourcetype(fval(r, RF_MALLORN) ? R_MALLORN_SEED : R_SEED);

    /* Skill pr�fen */
    skill = effskill(u, SK_HERBALISM, 0);
    if (skill < 12) {
        planttrees(u, raw);
        return;
    }

    /* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
    raw = MIN(skill * u->number, raw);
    n = get_pooled(u, rtype, GET_DEFAULT, raw);
    /* Samen pr�fen */
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
        return;
    }
    n = MIN(raw, n);

    /* F�r jeden Samen Talent*5% Erfolgschance. */
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

/* z�chte pferde */
static void breedhorses(unit * u)
{
    int n, c, breed = 0;
    const struct resource_type *rhorse = get_resourcetype(R_HORSE);
    int horses, effsk;
    static int bt_cache;
    static const struct building_type *stables_bt;

    if (bt_changed(&bt_cache)) {
        stables_bt = bt_find("stables");
    }

    assert(rhorse && rhorse->itype);
    if (!active_building(u, stables_bt)) {
        cmistake(u, u->thisorder, 122, MSG_PRODUCE);
        return;
    }
    horses = i_get(u->items, rhorse->itype);
    if (horses < 2) {
        cmistake(u, u->thisorder, 107, MSG_PRODUCE);
        return;
    }
    effsk = effskill(u, SK_HORSE_TRAINING, 0);
    n = u->number * effsk;
    n = MIN(n, horses);

    for (c = 0; c < n; c++) {
        if (rng_int() % 100 < effsk) {
            i_change(&u->items, rhorse->itype, 1);
            ++breed;
        }
    }

    produceexp(u, SK_HORSE_TRAINING, u->number);

    ADDMSG(&u->faction->msgs, msg_message("raised", "unit amount", u, breed));
}

static void breed_cmd(unit * u, struct order *ord)
{
    char token[128];
    int m;
    const char *s;
    param_t p;
    region *r = u->region;
    const resource_type *rtype = NULL;

    if (r->land == NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_onlandonly", ""));
        return;
    }

    /* z�chte [<anzahl>] <parameter> */
    (void)init_order(ord);
    s = gettoken(token, sizeof(token));

    m = s ? atoip(s) : 0;
    if (m != 0) {
        /* first came a want-paramter */
        s = gettoken(token, sizeof(token));
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
        plant(u, m);
        break;
    case P_TREES:
        breedtrees(u, m);
        break;
    default:
        if (p != P_ANY) {
            rtype = findresourcetype(s, u->faction->locale);
            if (rtype == get_resourcetype(R_SEED) || rtype == get_resourcetype(R_MALLORN_SEED)) {
                breedtrees(u, m);
                break;
            }
            else if (rtype != get_resourcetype(R_HORSE)) {
                ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_cannotmake", ""));
                break;
            }
        }
        breedhorses(u);
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

    if (effskill(u, SK_HERBALISM, 0) < 7) {
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
            int effsk = effskill(u, sk, 0);
            if (effsk > w) {
                w = effsk;
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
    int n, i, id, effsk;
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

    o = (request *)calloc(1, sizeof(request));
    o->unit = u;
    o->qty = 1;                   /* Betrag steht in u->wants */
    o->no = u2->no;
    o->type.goblin = goblin;      /* Merken, wenn Goblin-Spezialklau */
    addlist(stealorders, o);

    /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */

    produceexp(u, SK_STEALTH, MIN(n, u->number));
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

        /* Nur soviel PRODUCEEXP wie auch tats�chlich gemacht wurde */
        produceexp(u, SK_ENTERTAINMENT, MIN(u->n, u->number));
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
        const char *str = config_get("entertain.base");
        entertainbase = str ? atoi(str) : 0;
    }
    if (!entertainperlevel) {
        const char *str = config_get("entertain.perlevel");
        entertainperlevel = str ? atoi(str) : 0;
    }
    if (fval(u, UFL_WERE)) {
        cmistake(u, ord, 58, MSG_INCOME);
        return;
    }
    if (!effskill(u, SK_ENTERTAINMENT, 0)) {
        cmistake(u, ord, 58, MSG_INCOME);
        return;
    }
    if (besieged(u)) {
        cmistake(u, ord, 60, MSG_INCOME);
        return;
    }
    if (u->ship && is_guarded(r, u)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return;
    }
    if (is_cursed(r->attribs, C_DEPRESSION, 0)) {
        cmistake(u, ord, 28, MSG_INCOME);
        return;
    }

    u->wants = u->number * (entertainbase + effskill(u, SK_ENTERTAINMENT, 0)
        * entertainperlevel);

    max_e = getuint();
    if (max_e != 0) {
        u->wants = MIN(u->wants, max_e);
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
            int r = (u->number * jobs) % working;
            workers = u->number * jobs / working;
            if (r > 0 && rng_int() % working < r)
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
    if (r->attribs && rule_blessed_harvest() == HARVEST_TAXES) {
        /* E3 rules */
        int happy =
            (int)(jobs * curse_geteffect(get_curse(r->attribs, &ct_blessedharvest)));
        earnings += happy;
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
        if (u->ship && is_guarded(r, u)) {
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
    unsigned int i;
    int m, looted = 0;
    int startmoney = rmoney(r);

    expandorders(r, lootorders);
    if (!norders)
        return;

    for (i = 0; i != norders && startmoney > looted + TAXFRACTION * 2; i++) {
        change_money(g_requests[i].unit, TAXFRACTION);
        g_requests[i].unit->n += TAXFRACTION;
        /*Looting destroys double the money*/
        looted += TAXFRACTION * 2;
    }
    rsetmoney(r, startmoney - looted);
    free(g_requests);

    /* Lowering morale by 1 depending on the looted money (+20%) */
    m = region_get_morale(r);
    if (m && startmoney>0) {
        if (rng_int() % 100 < 20 + (looted * 80) / startmoney) {
            /*Nur Moral -1, turns is not changed, so the first time nothing happens if the morale is good*/
            region_set_morale(r, m - 1, -1);
        }
    }

    for (u = r->units; u; u = u->next) {
        if (u->n >= 0) {
            add_income(u, IC_LOOT, u->wants, u->n);
            fset(u, UFL_LONGACTION | UFL_NOTMOVING);
        }
    }
}

void expandtax(region * r, request * taxorders)
{
    unit *u;
    unsigned int i;

    expandorders(r, taxorders);
    if (!norders)
        return;

    for (i = 0; i != norders && rmoney(r) > TAXFRACTION; i++) {
        change_money(g_requests[i].unit, TAXFRACTION);
        g_requests[i].unit->n += TAXFRACTION;
        rsetmoney(r, rmoney(r) - TAXFRACTION);
    }
    free(g_requests);

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
    static int taxperlevel = 0;

    if (!taxperlevel) {
        taxperlevel = config_get_int("taxing.perlevel", 0);
    }

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

    if (effskill(u, SK_TAXING, 0) <= 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "error_no_tax_skill", ""));
        return;
    }

    max = getint();

    if (max <= 0) {
        max = INT_MAX;
    }
    if (!playerrace(u_race(u))) {
        u->wants = MIN(income(u), max);
    }
    else {
        u->wants = MIN(n * effskill(u, SK_TAXING, 0) * taxperlevel, max);
    }

    u2 = is_guarded(r, u);
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

    if (config_get_int("rules.enable_loot", 0) == 0 && !is_monsters(u->faction)) {
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

    u2 = is_guarded(r, u);
    if (u2) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "region_guarded", "guard", u2));
        return;
    }

    max = getint();

    if (max <= 0) {
        max = INT_MAX;
    }
    if (!playerrace(u_race(u))) {
        u->wants = MIN(income(u), max);
    }
    else {
        /* For player start with 20 Silver +10 every 5 level of close combat skill*/
        int skbonus = (MAX(effskill(u, SK_MELEE, 0), effskill(u, SK_SPEAR, 0)) * 2 / 10) + 2;
        u->wants = MIN(n * skbonus * 10, max);
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
        expandwork(r, workers, nextworker, region_maxworkers(r));
    }
}

static void peasant_taxes(region * r)
{
    faction *f;
    unit *u;
    building *b;
    int money;
    int level;

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

    level = buildingeffsize(b, false);
    if (level > 0) {
        double taxfactor = (double)money * level / building_taxes(b);
        double morale = (double)money * region_get_morale(r) / MORALE_TAX_FACTOR;
        if (taxfactor > morale) {
            taxfactor = morale;
        }
        if (taxfactor > 0) {
            int taxmoney = (int)taxfactor;
            change_money(u, taxmoney);
            rsetmoney(r, money - taxmoney);
            ADDMSG(&u->faction->msgs, msg_message("income_tax",
                "unit region amount", u, r, taxmoney));
        }
    }
}

static bool rule_auto_taxation(void)
{
    return config_get_int("rules.economy.taxation", 0) != 0;
}

static bool rule_autowork(void) {
    return config_get_int("work.auto", 0) != 0;
}

void produce(struct region *r)
{
    request workers[MAX_WORKERS];
    request *taxorders, *lootorders, *sellorders, *stealorders, *buyorders;
    unit *u;
    bool limited = true;
    request *nextworker = workers;
    static int bt_cache;
    static const struct building_type *caravan_bt;
    static int rc_cache;
    static const race *rc_spell, *rc_insect, *rc_aquarian;
    
    if (bt_changed(&bt_cache)) {
        caravan_bt = bt_find("caravan");
    }
    if (rc_changed(&rc_cache)) {
        rc_spell = get_race(RC_SPELL);
        rc_insect = get_race(RC_INSECT);
        rc_aquarian = get_race(RC_AQUARIAN);
    }
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

        if (u_race(u) == rc_spell || fval(u, UFL_LONGACTION))
            continue;

        if (u_race(u) == rc_insect && r_insectstalled(r) &&
            !is_cursed(u->attribs, C_KAELTESCHUTZ, 0))
            continue;

        if (fval(u, UFL_LONGACTION) && u->thisorder == NULL) {
            /* this message was already given in laws.c:update_long_order
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

        if (fval(r->terrain, SEA_REGION) && u_race(u) != rc_aquarian
            && !(u_race(u)->flags & RCF_SWIM)
            && todo != K_STEAL && todo != K_SPY && todo != K_SABOTAGE)
            continue;

        switch (todo) {
        case K_ENTERTAIN:
            entertain_cmd(u, u->thisorder);
            break;

        case K_WORK:
            if (!rule_autowork() && do_work(u, u->thisorder, nextworker) == 0) {
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
     * letzten Runde berechnen kann, wieviel die Bauern f�r Unterhaltung
     * auszugeben bereit sind. */
    if (entertaining)
        expandentertainment(r);
    if (!rule_autowork()) {
        expandwork(r, workers, nextworker, region_maxworkers(r));
    }
    if (taxorders)
        expandtax(r, taxorders);

    if (lootorders)
        expandloot(r, lootorders);

    /* An erster Stelle Kaufen (expandbuying), die Bauern so Geld bekommen, um
     * nachher zu beim Verkaufen (expandselling) den Spielern abkaufen zu
     * k�nnen. */

    if (buyorders)
        expandbuying(r, buyorders);

    if (sellorders) {
        int limit = rpeasants(r) / TRADE_FRACTION;
        if (r->terrain == newterrain(T_DESERT)
            && buildingtype_exists(r, caravan_bt, true))
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
