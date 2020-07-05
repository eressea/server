#ifdef _MSC_VER
#include <platform.h>
#endif

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

#include <attributes/reduceproduction.h>
#include <attributes/racename.h>
#include <spells/buildingcurse.h>
#include <spells/regioncurse.h>
#include <spells/unitcurse.h>

/* kernel includes */
#include "kernel/ally.h"
#include "kernel/attrib.h"
#include "kernel/building.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/curse.h"
#include "kernel/equipment.h"
#include "kernel/event.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/pool.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include <util/base36.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include "util/param.h"
#include <util/parser.h>
#include <util/rng.h>

/* libs includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define MAX_REQUESTS 1024
static struct econ_request econ_requests[MAX_REQUESTS];

static econ_request **g_requests; /* TODO: no need for this to be module-global */

#define ENTERTAINFRACTION 20

static void add_request(econ_request * req, enum econ_type type, unit *u, order *ord, int want) {
    req->next = NULL;
    req->unit = u;
    req->qty = u->wants = want;
    req->type = type;
}

static bool rule_auto_taxation(void)
{
    return config_get_int("rules.economy.taxation", 0) != 0;
}

static bool rule_autowork(void) {
    return config_get_int("work.auto", 0) != 0;
}

int entertainmoney(const region * r)
{
    double n;

    if (is_cursed(r->attribs, &ct_depression)) {
        return 0;
    }

    n = rmoney(r) / (double)ENTERTAINFRACTION;

    if (is_cursed(r->attribs, &ct_generous)) {
        n *= get_curseeffect(r->attribs, &ct_generous);
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

int expand_production(region * r, econ_request * requests, econ_request ***results)
{
    unit *u;
    econ_request *o;
    int norders = 0;

    /* Alle Units ohne production haben ein -1, alle units mit orders haben ein
     * 0 hier stehen */

    for (u = r->units; u; u = u->next)
        u->n = -1;

    for (o = requests; o; o = o->next) {
        if (o->qty > 0) {
            norders += o->qty;
        }
    }

    if (norders > 0) {
        int i = 0;
        econ_request **split;
        split = (econ_request **)calloc(norders, sizeof(econ_request *));
        if (!split) abort();
        for (o = requests; o; o = o->next) {
            if (o->qty > 0) {
                unsigned int j;
                for (j = o->qty; j; j--) {
                    split[i] = o;
                    o->unit->n = 0;
                    i++;
                }
            }
        }
        scramble(split, norders, sizeof(econ_request *));
        *results = split;
    }
    else {
        *results = NULL;
    }
    return norders;
}

static int expandorders(region * r, econ_request * requests) {
    return expand_production(r, requests, &g_requests);
}

static void free_requests(econ_request *requests) {
    while (requests) {
        econ_request *req = requests->next;
        free(requests);
        requests = req;
    }
}

/* ------------------------------------------------------------- */

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

    init_order_depr(ord);
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

    if (is_cursed(u->attribs, &ct_slavery)) {
        /* charmed units shouldn't be losing their skills */
        return 0;
    }

    init_order_depr(ord);
    s = gettoken(token, sizeof(token));

    sk = get_skill(s, u->faction->locale);
    if (sk != NOSKILL) {
        if (sk == SK_MAGIC) {
            if (is_familiar(u)) {
                /* some units cannot forget their innate magical abilities */
                return 0;
            }
            else {
                unit *ufam = get_familiar(u);
                if (ufam) {
                    if (join_monsters(ufam, NULL)) {
                        a_removeall(&ufam->attribs, NULL);
                        unit_convert_race(ufam, NULL, "ghost");
                    }
                }
                a_removeall(&u->attribs, &at_mage);
                a_removeall(&u->attribs, &at_familiar);
            }
        }
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
}

void destroy(region *r) {
    unit *u;
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

static void mod_skill(const resource_mod *mod, skill_t sk, int *value) {
    skill_t msk;
    assert(mod->type == RMT_PROD_SKILL);
    msk = (skill_t)mod->value.sa[0];
    if (msk == NOSKILL || msk == sk) {
        *value += mod->value.sa[1];
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
            const race * rc = u_race(u);
            if (mod->race_mask == 0 || (mod->race_mask & rc->mask_item)) {
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
                    if (mod->race_mask) need_race |= 1;
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
            if (mod->race_mask) need_race |= 2;
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
    n = build(u, 1, itype->construction, 0, want, skill_mod);
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
#define new_allocation() (allocation *)calloc(1, sizeof(allocation))
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
    static const struct race *rc_mountainguard;
    static int config;

    if (rc_changed(&config)) {
        rc_mountainguard = rc_find("mountainguard");
    }

    /* momentan kann man keine ressourcen abbauen, wenn man dafuer
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

    /* Bergwaechter koennen Abbau von Eisen/Laen durch Bewachen verhindern.
     * Als magische Wesen 'sehen' Bergwaechter alles und werden durch
     * Belagerung nicht aufgehalten.  (Ansonsten wie oben bei Elfen anpassen).
     */
    if (rc_mountainguard) {
        if (itype->rtype && (itype->rtype == get_resourcetype(R_IRON) || itype->rtype == rt_find("laen"))) {
            unit *u2;
            for (u2 = r->units; u2; u2 = u2->next) {
                if (rc_mountainguard == u_race(u2) && !fval(u2, UFL_ISNEW) && u2->number > 0
                    && is_guard(u2) && !alliedunit(u2, u->faction, HELP_GUARD)) {
                    ADDMSG(&u->faction->msgs,
                        msg_feedback(u, u->thisorder, "region_guarded", "guard", u2));
                    return;
                }
            }
        }
    }

    assert(sk != NOSKILL || !"limited resource needs a required skill for making it");
    skill = effskill(u, sk, NULL);
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
        int more = i_get(u->items, rring->itype);
        if (more > u->number) more = u->number;
        amount += skill * more * (roqf_factor() - 1);
    }

    /* Schaffenstrunk: */
    if ((dm = get_effect(u, oldpotiontype[P_DOMORE])) != 0) {
        if (dm > u->number) dm = u->number;
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
        alist = calloc(1, sizeof(struct allocation_list));
        if (!alist) abort();
        alist->next = allocations;
        alist->type = rtype;
        allocations = alist;
    }
    al = new_allocation();
    if (!al) abort();
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
    if (r > 0) ++req;
    return req;
}

static void
leveled_allocation(const resource_type * rtype, region * r, allocation * alist)
{
    const item_type *itype = resource2item(rtype);
    rawmaterial *rm = rm_get(r, rtype);

    if (rm != NULL) {
        int need;
        bool first = true;
        do {
            int avail = rm->amount, nreq = 0;
            allocation *al;

            if (avail <= 0) {
                for (al = alist; al; al = al->next) {
                    al->get = 0;
                }
                break;
            }

            assert(avail > 0);

            for (al = alist; al; al = al->next) {
                if (!fval(al, AFL_DONE)) {
                    int req = required(al->want - al->get, al->save);
                    assert(al->get <= al->want && al->get >= 0);
                    if (effskill(al->unit, itype->construction->skill, NULL)
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
            }
            need = nreq;

            if (avail > nreq) avail = nreq;
            if (need > 0) {
                int use = 0;
                for (al = alist; al; al = al->next) {
                    if (!fval(al, AFL_DONE)) {
                        if (avail > 0) {
                            int want = required(al->want - al->get, al->save);
                            int x = avail * want / nreq;
                            int req = (avail * want) % nreq;
                            /* Wenn Rest, dann wuerfeln, ob ich etwas bekomme: */
                            if (req > 0 && rng_int() % nreq < req) ++x;
                            avail -= x;
                            use += x;
                            nreq -= want;
                            need -= x;
                            al->get = al->get + x * al->save.sa[1] / al->save.sa[0];
                            if (al->get > al->want) al->get = al->want;
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

    if (avail > nreq) avail = nreq;

    for (al = alist; al; al = al->next) {
        if (avail > 0) {
            int want = required(al->want, al->save);
            int x = avail * want / nreq;
            int rx = (avail * want) % nreq;
            /* Wenn Rest, dann wuerfeln, ob ich was bekomme: */
            if (rx > 0 && rng_int() % nreq < rx) ++x;
            avail -= x;
            nreq -= want;
            al->get = x * al->save.sa[1] / al->save.sa[0];
            if (al->want < al->get) al->get = al->want;
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

static void allocate(const resource_type *rtype, region *r, allocation *data) {
    if (rtype->raw) {
        leveled_allocation(rtype, r, data);
    }
    else {
        attrib_allocation(rtype, r, data);
    }
}

void split_allocations(region * r)
{
    allocation_list **p_alist = &allocations;
    while (*p_alist) {
        allocation_list *alist = *p_alist;
        const resource_type *rtype = alist->type;
        const item_type *itype = resource2item(rtype);
        allocation **p_al = &alist->data;

        allocate(rtype, r, alist->data);

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

static void create_potion(unit * u, const item_type * itype, int want)
{
    int built;

    if (want == 0) {
        want = maxbuild(u, itype->construction);
    }
    built = build(u, 1, itype->construction, 0, want, 0);
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
            itype->construction, want));
        break;
    default:
        i_change(&u->items, itype, built);
        if (want == INT_MAX)
            want = built;
        ADDMSG(&u->faction->msgs, msg_message("produce",
            "unit region amount wanted resource", u, u->region, built, want,
            itype->rtype));
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
        if (itype->flags & ITF_POTION) {
            create_potion(u, itype, want);
        }
        else if (itype->construction && itype->construction->materials) {
            manufacture(u, itype, want);
        }
        else {
            ADDMSG(&u->faction->msgs, msg_feedback(u, u->thisorder,
                "error_cannotmake", ""));
        }
    }
}

int make_cmd(unit * u, struct order *ord)
{
    char token[32];
    region *r = u->region;
    const building_type *btype = 0;
    const ship_type *stype = 0;
    const item_type *itype = 0;
    param_t p = NOPARAM;
    int m = INT_MAX;
    const char *s;
    const struct locale *lang = u->faction->locale;
    keyword_t kwd;

    kwd = init_order_depr(ord);
    assert(kwd == K_MAKE);
    s = gettoken(token, sizeof(token));

    if (s) {
        char ibuf[16];
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
            s = gettoken(token, sizeof(token));
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
        else if (btype->stages && btype->stages->construction) {
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

static void free_luxuries(variant *var)
{
    item *itm = (item *)var->v;
    var->v = NULL;
    i_freeall(&itm);
}

const attrib_type at_luxuries = {
    "luxuries", NULL, free_luxuries, NULL, NULL, NULL
};

static void expandbuying(region * r, econ_request * buyorders)
{
    const resource_type *rsilver = get_resourcetype(R_SILVER);
    int max_products;
    unit *u;
    struct trade {
        const luxury_type *type;
        int number;
        int multi;
    } trades[MAXLUXURIES], *trade;
    int ntrades = 0;
    const luxury_type *ltype;

    for (ntrades = 0, ltype = luxurytypes; ltype; ltype = ltype->next) {
        assert(ntrades < MAXLUXURIES);
        trades[ntrades].number = 0;
        trades[ntrades].multi = 1;
        trades[ntrades++].type = ltype;
    }

    if (!buyorders)
        return;

    /* Initialisation. multiplier ist der Multiplikator auf den
     * Verkaufspreis. Fuer max_products Produkte kauft man das Produkt zum
     * einfachen Verkaufspreis, danach erhoeht sich der Multiplikator um 1.
     * counter ist ein Zaehler, der die gekauften Produkte zaehlt. money
     * wird fuer die debug message gebraucht. */

    max_products = rpeasants(r) / TRADE_FRACTION;

    /* Kauf - auch so programmiert, dass er leicht erweiterbar auf mehrere
     * Gueter pro Monat ist. j sind die Befehle, i der Index des
     * gehandelten Produktes. */
    if (max_products > 0) {
        unsigned int norders = expandorders(r, buyorders);

        if (norders) {
            unsigned int j;
            for (j = 0; j != norders; j++) {
                int price, multi;
                ltype = g_requests[j]->data.trade.ltype;
                trade = trades;
                while (trade->type && trade->type != ltype)
                    ++trade;
                multi = trade->multi;
                price = ltype->price * multi;

                if (get_pooled(g_requests[j]->unit, rsilver, GET_DEFAULT,
                    price) >= price) {
                    item *items;
                    /* litems zaehlt die Gueter, die verkauft wurden, u->n das Geld, das
                     * verdient wurde. Dies muss gemacht werden, weil der Preis staendig sinkt,
                     * man sich also das verdiente Geld und die verkauften Produkte separat
                     * merken muss. */
                    attrib *a;

                    u = g_requests[j]->unit;
                    a = a_find(u->attribs, &at_luxuries);
                    if (a == NULL) {
                        a = a_add(&u->attribs, a_new(&at_luxuries));
                    }
                    items = a->data.v;

                    i_change(&items, ltype->itype, 1);
                    a->data.v = items;
                    i_change(&g_requests[j]->unit->items, ltype->itype, 1);
                    use_pooled(u, rsilver, GET_DEFAULT, price);
                    if (u->n < 0)
                        u->n = 0;
                    u->n += price;

                    rsetmoney(r, rmoney(r) + price);

                    /* Falls mehr als max_products Bauern ein Produkt verkauft haben, steigt
                     * der Preis Multiplikator fuer das Produkt um den Faktor 1. Der Zaehler
                     * wird wieder auf 0 gesetzt. */
                    if (++trade->number == max_products) {
                        trade->number = 0;
                        ++trade->multi;
                    }
                    fset(u, UFL_LONGACTION | UFL_NOTMOVING);
                }
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

bool trade_needs_castle(const terrain_type *terrain, const race *rc)
{
    static int rc_change, terrain_change;
    static const race *rc_insect;
    static const terrain_type *t_desert, *t_swamp;
    if (rc_changed(&rc_change)) {
        rc_insect = get_race(RC_INSECT);
    }
    if (rc != rc_insect) {
        return true;
    }
    if (terrain_changed(&terrain_change)) {
        t_swamp = newterrain(T_SWAMP);
        t_desert = newterrain(T_DESERT);
    }
    return (terrain != t_swamp && terrain != t_desert);
}

static building * first_building(region *r, const struct building_type *btype, int minsize) {
    building *b = NULL;
    if (r->buildings) {
        for (b = r->buildings; b; b = b->next) {
            if (b->type == btype && b->size >= minsize) {
                return b;
            }
        }
    }
    return NULL;
}

static void buy(unit * u, econ_request ** buyorders, struct order *ord)
{
    char token[128];
    region *r = u->region;
    int n, k;
    econ_request *o;
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
     * schon dafuer ausgeruestet, mehrere Produkte zu kaufen. */

    kwd = init_order_depr(ord);
    assert(kwd == K_BUY);
    n = getint();
    if (n <= 0) {
        cmistake(u, ord, 26, MSG_COMMERCE);
        return;
    }

    /* Entweder man ist Insekt in Sumpf/Wueste, oder es muss
     * einen Handelsposten in der Region geben: */
    if (trade_needs_castle(r->terrain, u_race(u))) {
        static int cache;
        static const struct building_type *castle_bt;
        if (bt_changed(&cache)) {
            castle_bt = bt_find("castle");
        }
        if (first_building(r, castle_bt, 2) == NULL) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return;
        }
    }

    /* Ein Haendler kann nur 10 Gueter pro Talentpunkt handeln. */
    k = u->number * 10 * effskill(u, SK_TRADE, NULL);

    /* hat der Haendler bereits gehandelt, muss die Menge der bereits
     * verkauften/gekauften Gueter abgezogen werden */
    a = a_find(u->attribs, &at_trades);
    if (!a) {
        a = a_add(&u->attribs, a_new(&at_trades));
    }
    else {
        k -= a->data.i;
    }

    if (n > k) n = k;

    if (!n) {
        cmistake(u, ord, 102, MSG_COMMERCE);
        return;
    }

    assert(n >= 0);
    /* die Menge der verkauften Gueter merken */
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
    if (!r->land || r_demand(r, ltype)) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "luxury_notsold", ""));
        return;
    }
    o = (econ_request *)calloc(1, sizeof(econ_request));
    if (!o) abort();
    o->data.trade.ltype = ltype;        /* sollte immer gleich sein */

    o->unit = u;
    o->qty = n;
    o->type = ECON_BUY;
    addlist(buyorders, o);
}

/* ------------------------------------------------------------- */
void add_income(unit * u, income_t type, int want, int qty)
{
    if (want == INT_MAX)
        want = qty;
    ADDMSG(&u->faction->msgs, msg_message("income",
        "unit region mode wanted amount", u, u->region, (int)type, want, qty));
}

/* Steuersaetze in % bei Burggroesse */
static int tax_per_size[7] = { 0, 6, 12, 18, 24, 30, 36 };

static void expandselling(region * r, econ_request * sellorders, int limit)
{
    int money, max_products;
    int norders;
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

    assert(r->land);
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

    if (!sellorders) {            /* NEIN, denn Insekten koennen in   || !r->buildings) */
        return;                     /* Suempfen und Wuesten auch so handeln */
    }
    /* Stelle Eigentuemer der groessten Burg fest. Bekommt Steuern aus jedem
     * Verkauf. Wenn zwei Burgen gleicher Groesse bekommt gar keiner etwas. */
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
    /* Die Region muss genug Geld haben, um die Produkte kaufen zu koennen. */

    money = rmoney(r);

    /* max_products sind 1/100 der Bevoelkerung, falls soviele Produkte
     * verkauft werden - counter[] - sinkt die Nachfrage um 1 Punkt.
     * multiplier speichert r->demand fuer die debug message ab. */

    max_products = rpeasants(r) / TRADE_FRACTION;
    if (max_products <= 0)
        return;

    if (r->terrain == newterrain(T_DESERT)
        && buildingtype_exists(r, caravan_bt, true)) {
        max_products = rpeasants(r) * 2 / TRADE_FRACTION;
    }
    /* Verkauf: so programmiert, dass er leicht auf mehrere Gueter pro
     * Runde erweitert werden kann. */

    norders = expandorders(r, sellorders);
    if (norders > 0) {
        int j;
        for (j = 0; j != norders; j++) {
            const luxury_type *search = NULL;
            const luxury_type *ltype = g_requests[j]->data.trade.ltype;
            int multi = r_demand(r, ltype);
            int i, price;
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
                item *itm;
                attrib *a;
                u = g_requests[j]->unit;
                a = a_find(u->attribs, &at_luxuries);
                if (!a) {
                    a = a_add(&u->attribs, a_new(&at_luxuries));
                }
                itm = (item *)a->data.v;
                i_change(&itm, ltype->itype, 1);
                a->data.v = itm;
                ++use;
                if (u->n < 0) {
                    u->n = 0;
                }

                if (hafenowner) {
                    if (hafenowner->faction != u->faction) {
                        int abgezogenhafen = price / 10;
                        hafencollected += abgezogenhafen;
                        price -= abgezogenhafen;
                        money -= abgezogenhafen;
                    }
                }
                if (maxb) {
                    if (maxowner->faction != u->faction) {
                        int abgezogensteuer = price * tax_per_size[maxeffsize] / 100;
                        taxcollected += abgezogensteuer;
                        price -= abgezogensteuer;
                        money -= abgezogensteuer;
                    }
                }
                u->n += price;
                change_money(u, price);
                fset(u, UFL_LONGACTION | UFL_NOTMOVING);

                /* r->money -= price; --- dies wird eben nicht ausgefuehrt, denn die
                 * Produkte koennen auch als Steuern eingetrieben werden. In der Region
                 * wurden Silberstuecke gegen Luxusgueter des selben Wertes eingetauscht!
                 * Falls mehr als max_products Kunden ein Produkt gekauft haben, sinkt
                 * die Nachfrage fuer das Produkt um 1. Der Zaehler wird wieder auf 0
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
                use_pooled(g_requests[j]->unit, ltype->itype->rtype, GET_DEFAULT, use);
            }
        }
    }
    free(g_requests);

    /* Steuern. Hier werden die Steuern dem Besitzer der groessten Burg gegeben. */
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

static bool sell(unit * u, econ_request ** sellorders, struct order *ord)
{
    char token[128];
    bool unlimited = true;
    const item_type *itype;
    const luxury_type *ltype;
    int n, k;
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
    /* sellorders sind KEIN array, weil fuer alle items DIE SELBE resource
     * (das geld der region) aufgebraucht wird. */

    kwd = init_order_depr(ord);
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

    if (trade_needs_castle(r->terrain, u_race(u))) {
        /* In der Region muss es eine Burg geben. */
        if (first_building(r, castle_bt, 2) == NULL) {
            cmistake(u, ord, 119, MSG_COMMERCE);
            return false;
        }
    }
    /* Ein Haendler kann nur 10 Gueter pro Talentpunkt verkaufen. */

    /* Ein Haendler kann nur 10 Gueter pro Talentpunkt handeln. */
    k = u->number * 10 * effskill(u, SK_TRADE, NULL);
    if (n > k) n = k;

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
        econ_request *o;
        int available;

        if (!r->land || !r_demand(r, ltype)) {
            cmistake(u, ord, 263, MSG_COMMERCE);
            return false;
        }
        available = get_pooled(u, itype->rtype, GET_DEFAULT, INT_MAX);

        /* Wenn andere Einheiten das selbe verkaufen, muss ihr Zeug abgezogen
         * werden damit es nicht zweimal verkauft wird: */
        for (o = *sellorders; o; o = o->next) {
            if (o->data.trade.ltype == ltype && o->unit->faction == u->faction) {
                int fpool =
                    o->qty - get_pooled(o->unit, itype->rtype, GET_RESERVE, INT_MAX);
                if (fpool < 0) fpool = 0;
                available -= fpool;
            }
        }

        if (n > available) n = available;

        if (n <= 0) {
            cmistake(u, ord, 264, MSG_COMMERCE);
            return false;
        }
        /* Hier wird production->type verwendet, weil die obere limit durch
         * das silber gegeben wird (region->money), welches fuer alle
         * (!) produkte als summe gilt, als nicht wie bei der
         * produktion, wo fuer jedes produkt einzeln eine obere limite
         * existiert, so dass man arrays von orders machen kann. */

        /* hat der Haendler bereits gehandelt, muss die Menge der bereits
         * verkauften/gekauften Gueter abgezogen werden */
        a = a_find(u->attribs, &at_trades);
        if (!a) {
            a = a_add(&u->attribs, a_new(&at_trades));
        }
        else {
            k -= a->data.i;
        }

        if (n > k) n = k;
        assert(n >= 0);
        /* die Menge der verkauften Gueter merken */
        a->data.i += n;
        o = (econ_request *)calloc(1, sizeof(econ_request));
        if (!o) abort();
        o->unit = u;
        o->qty = n;
        o->type = ECON_SELL;
        o->data.trade.ltype = ltype;
        addlist(sellorders, o);

        return unlimited;
    }
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

    /* Skill pruefen */
    skill = effskill(u, SK_HERBALISM, NULL);
    if (skill < 6) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "plant_skills",
                "skill minskill product", SK_HERBALISM, 6, itype->rtype, 1));
        return;
    }
    /* Wasser des Lebens pruefen */
    if (get_pooled(u, rt_water, GET_DEFAULT, 1) == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rt_water));
        return;
    }
    n = get_pooled(u, itype->rtype, GET_DEFAULT, skill * u->number);
    /* Kraeuter pruefen */
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing",
                itype->rtype));
        return;
    }

    i = skill * u->number;
    if (i > raw) i = raw;
    if (n > i) n = i;
    /* Fuer jedes Kraut Talent*10% Erfolgschance. */
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

static void planttrees(region * r, int type, int n)
{
    if (n > 0) {
        rsettrees(r, type, rtrees(r, type) + n);
    }
}

/* zuechte baeume */
static void breedtrees(unit * u, int raw)
{
    int n, i, skill;
    const resource_type *rtype;
    season_t current_season = calendar_season(turn);
    region *r = u->region;
    int minskill = 6;

    if (!fval(r->terrain, LAND_REGION)) {
        return;
    }

    /* Mallornbaeume kann man nur in Mallornregionen zuechten */
    if (fval(r, RF_MALLORN)) {
        ++minskill;
        rtype = get_resourcetype(R_MALLORN_SEED);
    }
    else {
        rtype = get_resourcetype(R_SEED);
    }

    /* Skill pruefen */
    skill = effskill(u, SK_HERBALISM, NULL);
    if (skill < minskill) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "plant_skills",
                "skill minskill product", SK_HERBALISM, 6, rtype, 1));
        return;
    }

    /* wenn eine Anzahl angegeben wurde, nur soviel verbrauchen */
    i = skill * u->number;
    if (raw > i) raw = i;
    n = get_pooled(u, rtype, GET_DEFAULT, raw);
    /* Samen pruefen */
    if (n == 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, u->thisorder, "resource_missing", "missing", rtype));
        return;
    }
    if (n > raw) n = raw;

    if (skill >= 12 && current_season == SEASON_SPRING) {
        // Plant saplings for every 10 seeds
        planttrees(r, 0, n % 10);
        planttrees(r, 1, n / 10);
    }
    else {
        planttrees(r, 0, n);
    }

    /* Alles ok. Samen abziehen. */
    produceexp(u, SK_HERBALISM, u->number);
    use_pooled(u, rtype, GET_DEFAULT, n);

    ADDMSG(&u->faction->msgs, msg_message("plant",
        "unit region amount herb", u, r, n, rtype));
}

/* zuechte pferde */
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
    effsk = effskill(u, SK_HORSE_TRAINING, NULL);
    n = u->number * effsk;
    if (n > horses) n = horses;

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

    if (r->land == NULL) {
        ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_onlandonly", ""));
        return;
    }

    /* zuechte [<anzahl>] <parameter> */
    (void)init_order_depr(ord);
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
            const resource_type *rtype = findresourcetype(s, u->faction->locale);
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

    kwd = init_order_depr(ord);
    assert(kwd == K_RESEARCH);

    if (effskill(u, SK_HERBALISM, NULL) < 7) {
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

static void expandentertainment(region * r, econ_request *ecbegin, econ_request *ecend, long total)
{
    int m = entertainmoney(r);
    econ_request *o;

    for (o = ecbegin; o != ecend; ++o) {
        if (o->type == ECON_ENTERTAIN) {
            double part = m / (double)total;
            unit *u = o->unit;

            if (total <= m)
                u->n = o->qty;
            else
                u->n = (int)(o->qty * part);
            change_money(u, u->n);
            rsetmoney(r, rmoney(r) - u->n);
            m -= u->n;
            total -= o->qty;

            /* Nur soviel PRODUCEEXP wie auch tatsaechlich gemacht wurde */
            produceexp(u, SK_ENTERTAINMENT, (u->n < u->number) ? u->n : u->number);
            add_income(u, IC_ENTERTAIN, o->qty, u->n);
            fset(u, UFL_LONGACTION | UFL_NOTMOVING);
        }
    }
    assert(total == 0);
}

static int entertain_cmd(unit * u, struct order *ord, econ_request **io_req)
{
    region *r = u->region;
    int wants, max_e;
    econ_request *req = *io_req;
    static int entertainbase = 0;
    static int entertainperlevel = 0;
    keyword_t kwd;

    kwd = init_order_depr(ord);
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
        return 0;
    }
    if (!effskill(u, SK_ENTERTAINMENT, NULL)) {
        cmistake(u, ord, 58, MSG_INCOME);
        return 0;
    }
    if (u->ship && is_guarded(r, u)) {
        cmistake(u, ord, 69, MSG_INCOME);
        return 0;
    }
    if (is_cursed(r->attribs, &ct_depression)) {
        cmistake(u, ord, 28, MSG_INCOME);
        return 0;
    }

    wants = getuint();
    max_e = u->number * (entertainbase + effskill(u, SK_ENTERTAINMENT, NULL) * entertainperlevel);
    if (wants > 0 && wants < max_e) {
        max_e = wants;
    }
    if (max_e > 0) {
        add_request(req++, ECON_ENTERTAIN, u, ord, max_e);
        *io_req = req;
    }
    return max_e;
}

/**
 * \return number of working spaces taken by players
 */
static void
expandwork(region * r, econ_request * work_begin, econ_request * work_end, int maxwork, long total)
{
    int earnings;
    /* n: verbleibende Einnahmen */
    /* fishes: maximale Arbeiter */
    int jobs = maxwork;
    int p_wage = wage(r, NULL, NULL, turn);
    int money = rmoney(r);
    if (total > 0 && !rule_autowork()) {
        econ_request *o;

        for (o = work_begin; o != work_end; ++o) {
            if (o->type == ECON_WORK) {
                unit *u = o->unit;
                int workers;

                if (u->number == 0)
                    continue;

                if (jobs >= total)
                    workers = u->number;
                else {
                    int req = (u->number * jobs) % total;
                    workers = u->number * jobs / total;
                    if (req > 0 && rng_int() % total < req)
                        workers++;
                }

                assert(workers >= 0);

                u->n = workers * wage(u->region, u->faction, u_race(u), turn);

                jobs -= workers;
                assert(jobs >= 0);

                change_money(u, u->n);
                total -= o->unit->number;
                add_income(u, IC_WORK, o->qty, u->n);
                fset(u, UFL_LONGACTION | UFL_NOTMOVING);
            }
        }
        assert(total == 0);
    }
    if (jobs > rpeasants(r)) {
        jobs = rpeasants(r);
    }
    earnings = jobs * p_wage;
    if (jobs > 0 && r->attribs && rule_blessed_harvest() == HARVEST_TAXES) {
        /* E3 rules */
        int happy = harvest_effect(r);
        earnings += happy * jobs;
    }
    rsetmoney(r, money + earnings);
}

static int work_cmd(unit * u, order * ord, econ_request ** io_req)
{
    if (playerrace(u_race(u))) {
        econ_request *req = *io_req;
        region *r = u->region;
        int w;

        if (fval(u, UFL_WERE)) {
            if (ord) {
                cmistake(u, ord, 313, MSG_INCOME);
            }
            return 0;
        }
        if (u->ship && is_guarded(r, u)) {
            if (ord) {
                cmistake(u, ord, 69, MSG_INCOME);
            }
            return 0;
        }
        w = wage(r, u->faction, u_race(u), turn);
        add_request(req++, ECON_WORK, u, ord, w * u->number);
        *io_req = req;
        return u->number;
    }
    else if (ord && !is_monsters(u->faction)) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "race_cantwork", "race", u_race(u)));
    }
    return 0;
}

static void expandloot(region * r, econ_request * lootorders)
{
    unsigned int norders;

    norders = expandorders(r, lootorders);
    if (norders > 0) {
        unit *u;
        unsigned int i;
        int m, looted = 0;
        int startmoney = rmoney(r);

        for (i = 0; i != norders && startmoney > looted + TAXFRACTION * 2; i++) {
            change_money(g_requests[i]->unit, TAXFRACTION);
            g_requests[i]->unit->n += TAXFRACTION;
            /*Looting destroys double the money*/
            looted += TAXFRACTION * 2;
        }
        rsetmoney(r, startmoney - looted);
        free(g_requests);

        /* Lowering morale by 1 depending on the looted money (+20%) */
        m = region_get_morale(r);
        if (m && startmoney > 0) {
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
}

void expandtax(region * r, econ_request * taxorders)
{
    unit *u;
    unsigned int norders;

    norders = expandorders(r, taxorders);
    if (norders > 0) {
        unsigned int i;
        for (i = 0; i != norders && rmoney(r) > TAXFRACTION; i++) {
            change_money(g_requests[i]->unit, TAXFRACTION);
            g_requests[i]->unit->n += TAXFRACTION;
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
}

void tax_cmd(unit * u, struct order *ord, econ_request ** taxorders)
{
    /* Steuern werden noch vor der Forschung eingetrieben */
    region *r = u->region;
    unit *u2;
    int n;
    econ_request *o;
    int max;
    keyword_t kwd;
    static int taxperlevel = 0;

    if (!taxperlevel) {
        taxperlevel = config_get_int("taxing.perlevel", 0);
    }

    kwd = init_order_depr(ord);
    assert(kwd == K_TAX);

    if (!humanoidrace(u_race(u)) && !is_monsters(u->faction)) {
        cmistake(u, ord, 228, MSG_INCOME);
        return;
    }

    if (fval(u, UFL_WERE)) {
        cmistake(u, ord, 228, MSG_INCOME);
        return;
    }

    n = armedmen(u, false);

    if (!n) {
        cmistake(u, ord, 48, MSG_INCOME);
        return;
    }

    if (effskill(u, SK_TAXING, NULL) <= 0) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "error_no_tax_skill", ""));
        return;
    }

    max = getint();

    if (max <= 0) {
        max = INT_MAX;
    }
    if (!playerrace(u_race(u))) {
        u->wants = income(u);
    }
    else {
        u->wants = n * effskill(u, SK_TAXING, NULL) * taxperlevel;
    }
    if (u->wants > max) u->wants = max;

    u2 = is_guarded(r, u);
    if (u2) {
        ADDMSG(&u->faction->msgs,
            msg_feedback(u, ord, "region_guarded", "guard", u2));
        return;
    }

    /* die einnahmen werden in fraktionen von 10 silber eingeteilt: diese
     * fraktionen werden dann bei eintreiben unter allen eintreibenden
     * einheiten aufgeteilt. */

    o = (econ_request *)calloc(1, sizeof(econ_request));
    if (!o) abort();
    o->qty = u->wants / TAXFRACTION;
    o->type = ECON_TAX;
    o->unit = u;
    addlist(taxorders, o);
    return;
}

void loot_cmd(unit * u, struct order *ord, econ_request ** lootorders)
{
    region *r = u->region;
    unit *u2;
    int n;
    int max;
    econ_request *o;
    keyword_t kwd;

    kwd = init_order_depr(ord);
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
        u->wants = income(u);
        if (u->wants > max) u->wants = max;
    }
    else {
        /* For player start with 20 Silver +10 every 5 level of close combat skill*/
        int skm = effskill(u, SK_MELEE, NULL);
        int sks = effskill(u, SK_SPEAR, NULL);
        int skbonus = ((skm > sks ? skm : sks) * 2 / 10) + 2;
        u->wants = n * skbonus * 10;
        if (u->wants > max) u->wants = max;
    }

    o = (econ_request *)calloc(1, sizeof(econ_request));
    if (!o) abort();
    o->qty = u->wants / TAXFRACTION;
    o->type = ECON_LOOT;
    o->unit = u;
    addlist(lootorders, o);

    return;
}

void auto_work(region * r)
{
    econ_request *nextrequest = econ_requests;
    unit *u;
    long total = 0;

    for (u = r->units; u; u = u->next) {
        if (!(u->flags & UFL_LONGACTION) && !is_monsters(u->faction)) {
            int work = work_cmd(u, NULL, &nextrequest);
            if (work) {
                total += work;
                assert(nextrequest - econ_requests <= MAX_REQUESTS);
            }
        }
    }
    if (nextrequest != econ_requests) {
        expandwork(r, econ_requests, nextrequest, region_maxworkers(r), total);
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

void produce(struct region *r)
{
    econ_request *taxorders, *lootorders, *sellorders, *stealorders, *buyorders;
    unit *u;
    bool limited = true;
    long entertaining = 0, working = 0;
    econ_request *nextrequest = econ_requests;
    static int bt_cache;
    static const struct building_type *caravan_bt;
    static int rc_cache;
    static const race *rc_insect, *rc_aquarian;

    if (bt_changed(&bt_cache)) {
        caravan_bt = bt_find("caravan");
    }
    if (rc_changed(&rc_cache)) {
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
    taxorders = 0;
    lootorders = 0;
    stealorders = 0;

    for (u = r->units; u; u = u->next) {
        order *ord;
        bool trader = false;
        keyword_t todo;

        if (fval(u, UFL_LONGACTION))
            continue;

        if (u_race(u) == rc_insect && r_insectstalled(r) &&
            !is_cursed(u->attribs, &ct_insectfur))
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
            entertaining += entertain_cmd(u, u->thisorder, &nextrequest);
            assert(nextrequest - econ_requests <= MAX_REQUESTS);
            break;

        case K_WORK:
            if (!rule_autowork()) {
                int work = work_cmd(u, u->thisorder, &nextrequest);
                if (work != 0) {
                    working += work;
                    assert(nextrequest - econ_requests <= MAX_REQUESTS);
                }
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
        case K_GROW:
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
     * letzten Runde berechnen kann, wieviel die Bauern fuer Unterhaltung
     * auszugeben bereit sind. */
    if (entertaining > 0) {
        expandentertainment(r, econ_requests, nextrequest, entertaining);
    }
    expandwork(r, econ_requests, nextrequest, region_maxworkers(r), working);

    if (taxorders) {
        expandtax(r, taxorders);
        free_requests(taxorders);
    }

    if (lootorders) {
        expandloot(r, lootorders);
        free_requests(lootorders);
    }
    /* An erster Stelle Kaufen (expandbuying), die Bauern so Geld bekommen, um
     * nachher zu beim Verkaufen (expandselling) den Spielern abkaufen zu
     * koennen. */

    if (buyorders) {
        expandbuying(r, buyorders);
        free_requests(buyorders);
    }

    if (sellorders) {
        int limit = rpeasants(r) / TRADE_FRACTION;
        if (r->terrain == newterrain(T_DESERT)
            && buildingtype_exists(r, caravan_bt, true))
            limit *= 2;
        expandselling(r, sellorders, limited ? limit : INT_MAX);
        free_requests(sellorders);
    }

    /* Die Spieler sollen alles Geld verdienen, bevor sie beklaut werden
     * (expandstealing). */

    if (stealorders) {
        expandstealing(r, stealorders);
        free_requests(stealorders);
    }

    assert(rmoney(r) >= 0);
    assert(rpeasants(r) >= 0);
}
