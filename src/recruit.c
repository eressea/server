#include "recruit.h"

#include "alchemy.h"
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
#include "kernel/direction.h"
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
#include <util/log.h>
#include "util/message.h"
#include "util/param.h"
#include <util/parser.h>
#include <util/rng.h>

#include "stb_ds.h"

/* libs includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define RECRUIT_MERGE 1
#define ORCS_PER_PEASANT 2

static int rules_recruit = -1;

typedef struct recruit_request {
    struct unit *unit;
    struct order *ord;
    int qty;
} recruit_request;

typedef struct recruitment {
    faction *f;
    recruit_request **requests;
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

void free_requests(recruit_request** requests)
{
    ptrdiff_t i, len = arrlen(requests);
    for (i = 0; i != len; ++i) {
        free(requests[i]);
    }
    arrfree(requests);
}

void free_recruitments(recruitment ** recruits)
{
    ptrdiff_t i, len = arrlen(recruits);
    for (i = 0; i != len; ++i) {
        free(recruits[i]);
    }
    arrfree(recruits);
}

/** Creates a list of recruitment structs, one for each faction. Adds every quantifyable production
 * to the faction's struct and to total.
 */
static recruitment **select_recruitment(recruit_request ** requests,
    int(*quantify) (const struct race *, int), int *total)
{
    recruitment **recruits = NULL;
    ptrdiff_t ui, len = arrlen(requests);

    for (ui = 0; ui != len; ++ui) {
        recruit_request *ro = requests[ui];
        if (ro) {
            unit* u = ro->unit;
            const race* rc = u_race(u);
            int qty = quantify(rc, ro->qty);

            if (qty > 0) {
                recruitment* rec = NULL;
                ptrdiff_t i, len = arrlen(recruits);
                for (i = 0; i != len; ++i) {
                    if (recruits[i]->f == u->faction) {
                        rec = recruits[i];
                        break;
                    }
                }
                if (rec == NULL) {
                    rec = (recruitment*)malloc(sizeof(recruitment));
                    if (!rec) abort();
                    rec->f = u->faction;
                    rec->total = 0;
                    rec->assigned = 0;
                    rec->requests = NULL;
                    if (recruits) {
                        arrput(recruits, rec);
                    } else {
                        arrsetlen(recruits, 1);
                        recruits[0] = rec;
                    }
                }
                else {
                    assert(rec->requests);
                }
                *total += qty;
                rec->total += qty;
                arrput(rec->requests, ro);
                requests[ui] = NULL;
            }
        }
    }
    return recruits;
}

void add_recruits(unit * u, int number, int wanted, int ordered)
{
    region *r = u->region;
    const struct race* rc = u_race(u);
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
            unew = create_unit(r, u->faction, number, rc, 0, NULL, u);
        }

        len = snprintf(equipment, sizeof(equipment), "new_%s", race_name(rc));
        if (len > 0 && (size_t)len < sizeof(equipment)) {
            equip_unit(unew, equipment);
        }
        if (unew != u) {
            transfermen(unew, u, unew->number);
            remove_unit(&r->units, unew);
        }
    }
    if (number < ordered) {
        ADDMSG(&u->faction->msgs, msg_message("recruit",
            "unit region amount want", u, r, number, u->wants));
    }
}

static int any_recruiters(const struct race *rc, int qty)
{
    return (int)(qty * 2 / rc->recruit_multi);
}

static int do_recruiting(recruitment ** recruits, int available)
{
    int recruited = 0, tipjar = 0;
    ptrdiff_t i, len = arrlen(recruits);

    /* try to assign recruits to factions fairly */
    while (available > 0) {
        int n = 0;
        int rest, mintotal = INT_MAX;

        /* find smallest production */
        for (i = 0; i != len; ++i) {
            recruitment* rec = recruits[i];
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
        for (i = 0; i != len; ++i) {
            recruitment* rec = recruits[i];
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
    for (i = 0; i != len; ++i) {
        recruitment* rec = recruits[i];
        int get = rec->assigned;
        ptrdiff_t ui, len = arrlen(rec->requests);
        for (ui = 0; ui != len; ++ui) {
            recruit_request* req = rec->requests[ui];
            unit *u = req->unit;
            const race *rc = u_race(u); /* race is set in recruit() */
            int multi = ORCS_PER_PEASANT / rc->recruit_multi;
            int number;

            if (tipjar) {
                get += tipjar;
                tipjar = 0;
            }

            if (multi > 1 && (get % multi)) {
                tipjar = get % multi;
                get -= tipjar;
            }

            number = get / multi;
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
            add_recruits(u, number, req->qty, u->wants);
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
static void expandrecruit(region * r, recruit_request ** recruitorders)
{
    recruitment **recruits;
    int orc_total = 0;

    /* peasant limited: */
    recruits = select_recruitment(recruitorders, any_recruiters, &orc_total);
    if (recruits) {
        int peasants = rpeasants(r);
        int orc_recruited, orc_peasants = peasants * ORCS_PER_PEASANT;
        int orc_frac = peasants / RECRUIT_FRACTION * ORCS_PER_PEASANT;      /* anzahl orks. 2 ork = 1 bauer */
        if (orc_total < orc_frac)
            orc_frac = orc_total;
        orc_recruited = do_recruiting(recruits, orc_frac);
        assert(orc_recruited <= orc_frac);
        rsetpeasants(r, (orc_peasants - orc_recruited) / ORCS_PER_PEASANT);
        free_recruitments(recruits);
    }

    /* no limit: */
    recruits = select_recruitment(recruitorders, any_recruiters, &orc_total);
    if (recruits) {
        int recruited, peasants = rpeasants(r) * ORCS_PER_PEASANT;
        recruited = do_recruiting(recruits, INT_MAX);
        if (recruited > 0) {
            rsetpeasants(r, (peasants - recruited) / ORCS_PER_PEASANT);
        }
        free_recruitments(recruits);
    }
}

static int recruit_cost(const faction * f, const race * rc)
{
    if (valid_race(f, rc)) {
        return rc->recruitcost;
    }
    return -1;
}

int max_recruits(const struct region *r)
{
    if (is_cursed(r->attribs, &ct_riotzone)) {
        return 0;
    }
    return rpeasants(r) / RECRUIT_FRACTION;
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

static recruit_request* recruit_cmd(unit * u, struct order *ord)
{
    region *r = u->region;
    recruit_request *o;
    int recruitcost = -1;
    const faction *f = u->faction;
    const struct race *rc = u_race(u);
    int n;
    message *msg;

    init_order(ord, NULL);
    n = getint();
    if (n <= 0) {
        syntax_error(u, ord);
        return NULL;
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

    u->wants = n;
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
            return NULL;
        }

        pool = get_pooled(u, get_resourcetype(R_SILVER), GET_DEFAULT, recruitcost * n);
        if (pool < recruitcost) {
            cmistake(u, ord, 142, MSG_EVENT);
            return NULL;
        }
        pool /= recruitcost;
        if (n > pool) n = pool;
    }

    if (!n) {
        cmistake(u, ord, 142, MSG_EVENT);
        return NULL;
    }
    if (has_skill(u, SK_ALCHEMY)) {
        if (faction_count_skill(u->faction, SK_ALCHEMY) + n > faction_skill_limit(u->faction, SK_ALCHEMY)) {
            cmistake(u, ord, 156, MSG_EVENT);
            return NULL;
        }
    }

    assert(rc);
    msg = can_recruit(u, rc, ord, turn);
    if (msg) {
        add_message(&u->faction->msgs, msg);
        msg_release(msg);
        return NULL;
    }

    u_setrace(u, rc);
    o = (recruit_request *)calloc(1, sizeof(recruit_request));
    if (!o) abort();
    o->qty = n;
    o->unit = u;
    o->ord = ord;
    return o;
}

void recruit(region * r)
{
    unit *u;
    recruit_request **recruitorders = NULL;

    if (rules_recruit < 0)
        recruit_init();
    for (u = r->units; u; u = u->next) {
        order *ord;

        if (IS_PAUSED(u->faction)) continue;

        if ((rules_recruit & RECRUIT_MERGE) || u->number == 0) {
            for (ord = u->orders; ord; ord = ord->next) {
                if (getkeyword(ord) == K_RECRUIT) {
                    recruit_request *o = recruit_cmd(u, ord);
                    if (o) {
                        arrput(recruitorders, o);
                    }
                    break;
                }
            }
        }
    }

    if (recruitorders) {
        scramble_array(recruitorders, arrlen(recruitorders), sizeof(recruit_request *));
        expandrecruit(r, recruitorders);
        free_requests(recruitorders);
    }
    remove_empty_units_in_region(r);
}
