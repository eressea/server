#ifdef _MSC_VER
#include <platform.h>
#endif

#include "recruit.h"

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

#define RECRUIT_MERGE 1
static int rules_recruit = -1;

typedef struct recruit_request {
    struct recruit_request *next;
    struct unit *unit;
    struct order *ord;
    int qty;
} recruit_request;

typedef struct recruitment {
    struct recruitment *next;
    faction *f;
    recruit_request *requests;
    int total, assigned;
} recruitment;

static void recruit_init(void)
{
    if (rules_recruit < 0) {
        rules_recruit = 0;
        if (config_get_int("recruit.allow_merge", 1)) {
            rules_recruit |= RECRUIT_MERGE;
        }
    }
}

static void free_requests(recruit_request *requests) {
    while (requests) {
        recruit_request *req = requests->next;
        free(requests);
        requests = req;
    }
}

void free_recruitments(recruitment * recruits)
{
    while (recruits) {
        recruitment *rec = recruits;
        recruits = rec->next;
        free_requests(rec->requests);
        free(rec);
    }
}

/** Creates a list of recruitment structs, one for each faction. Adds every quantifyable production
 * to the faction's struct and to total.
 */
static recruitment *select_recruitment(recruit_request ** rop,
    int(*quantify) (const struct race *, int), int *total)
{
    recruitment *recruits = NULL;

    while (*rop) {
        recruitment *rec = recruits;
        recruit_request *ro = *rop;
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
                rec = (recruitment *)malloc(sizeof(recruitment));
                if (!rec) abort();
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
        int len;

        if (u->number == 0) {
            set_number(u, number);
            u->hp = number * unit_max_hp(u);
            unew = u;
        }
        else {
            unew = create_unit(r, u->faction, number, u_race(u), 0, NULL, u);
        }

        len = snprintf(equipment, sizeof(equipment), "new_%s", u_race(u)->_name);
        if (len > 0 && (size_t)len < sizeof(equipment)) {
            equip_unit(unew, equipment);
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

static int do_recruiting(recruitment * recruits, int available)
{
    recruitment *rec;
    int recruited = 0;

    /* try to assign recruits to factions fairly */
    while (available > 0) {
        int n = 0;
        int rest, mintotal = INT_MAX;

        /* find smallest production */
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

        /* assign size of smallest production for everyone if possible; in the end roll dice to assign
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
        recruit_request *req;
        int get = rec->assigned;

        for (req = rec->requests; req; req = req->next) {
            unit *u = req->unit;
            const race *rc = u_race(u); /* race is set in recruit() */
            int number;
            double multi = 2.0 * rc->recruit_multi;

            number = (int)(get / multi);
            if (number > req->qty) number = req->qty;
            if (rc->recruitcost) {
                int afford = get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT,
                    number * rc->recruitcost) / rc->recruitcost;
                if (number > afford) number = afford;
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
                int dec = (int)(number * multi);
                if ((rc->ec_flags & ECF_REC_ETHEREAL) == 0) {
                    recruited += dec;
                }

                get -= dec;
            }
        }
    }
    return recruited;
}

/* Rekrutierung */
static void expandrecruit(region * r, recruit_request * recruitorders)
{
    recruitment *recruits;
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

message *can_recruit(unit *u, const race *rc, order *ord, int now)
{
    region *r = u->region;

    /* this is a very special case because the recruiting unit may be empty
    * at this point and we have to look at the creating unit instead. This
    * is done in cansee, which is called indirectly by is_guarded(). */
    if (is_guarded(r, u)) {
        return msg_error(u, ord, 70);
    }

    if (rc == get_race(RC_INSECT)) {

        /* in Gletschern, Eisbergen gar nicht rekrutieren */
        if (r_insectstalled(r)) {
            return msg_error(u, ord, 97);
        }

        /* in Wüsten ganzjährig rekrutieren, sonst im Winter nur mit Trank */
        if (r->terrain != newterrain(T_DESERT) && calendar_season(now) == SEASON_WINTER) {
            bool usepotion = false;
            unit *u2;

            for (u2 = r->units; u2; u2 = u2->next) {
                if (fval(u2, UFL_WARMTH)) {
                    usepotion = true;
                    break;
                }
            }
            if (!usepotion) {
                return msg_error(u, ord, 98);
            }
        }
    }
    if (is_cursed(r->attribs, &ct_riotzone)) {
        /* Die Region befindet sich in Aufruhr */
        return msg_error(u, ord, 237);
    }

    if (rc && !playerrace(rc)) {
        return msg_error(u, ord, 139);
    }

    if (fval(u, UFL_HERO)) {
        return msg_feedback(u, ord, "error_herorecruit", "");
    }
    if (has_skill(u, SK_MAGIC)) {
        /* error158;de;{unit} in {region}: '{command}' - Magier arbeiten
        * grundsaetzlich nur alleine! */
        return msg_error(u, ord, 158);
    }
    return NULL;
}

static void recruit_cmd(unit * u, struct order *ord, recruit_request ** recruitorders)
{
    region *r = u->region;
    recruit_request *o;
    int recruitcost = -1;
    const faction *f = u->faction;
    const struct race *rc = u_race(u);
    int n;
    message *msg;

    init_order_depr(ord);
    n = getint();
    if (n <= 0) {
        syntax_error(u, ord);
        return;
    }

    if (u->number == 0) {
        char token[128];
        const char *str;

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

    if (recruitcost > 0) {
        int pool;
        plane *pl = getplane(r);

        if (pl && (pl->flags & PFL_NORECRUITS)) {
            ADDMSG(&u->faction->msgs, msg_feedback(u, ord, "error_pflnorecruit", ""));
            return;
        }

        pool = get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, recruitcost * n);
        if (pool < recruitcost) {
            cmistake(u, ord, 142, MSG_EVENT);
            return;
        }
        pool /= recruitcost;
        if (n > pool) n = pool;
    }

    if (!n) {
        cmistake(u, ord, 142, MSG_EVENT);
        return;
    }
    if (has_skill(u, SK_ALCHEMY)) {
        if (faction_count_skill(u->faction, SK_ALCHEMY) + n > faction_skill_limit(u->faction, SK_ALCHEMY)) {
            cmistake(u, ord, 156, MSG_EVENT);
            return;
        }
    }

    assert(rc);
    msg = can_recruit(u, rc, ord, turn);
    if (msg) {
        add_message(&u->faction->msgs, msg);
        msg_release(msg);
        return;
    }

    u_setrace(u, rc);
    u->wants = n;
    o = (recruit_request *)calloc(1, sizeof(recruit_request));
    if (!o) abort();
    o->qty = n;
    o->unit = u;
    o->ord = ord;
    addlist(recruitorders, o);
}

void recruit(region * r)
{
    unit *u;
    recruit_request *recruitorders = NULL;

    if (rules_recruit < 0)
        recruit_init();
    for (u = r->units; u; u = u->next) {
        order *ord;

        if ((rules_recruit & RECRUIT_MERGE) || u->number == 0) {
            for (ord = u->orders; ord; ord = ord->next) {
                if (getkeyword(ord) == K_RECRUIT) {
                    recruit_cmd(u, ord, &recruitorders);
                    break;
                }
            }
        }
    }

    if (recruitorders) {
        expandrecruit(r, recruitorders);
    }
    remove_empty_units_in_region(r);
}
