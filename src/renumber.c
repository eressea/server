#include <platform.h>
#include "renumber.h"

#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/messages.h>

#include <util/attrib.h>
#include <util/base36.h>
#include <util/parser.h>

#include <assert.h>
#include <stdlib.h>

static attrib_type at_number = {
    "faction_renum",
    NULL, NULL, NULL, NULL, NULL, NULL,
    ATF_UNIQUE
};

void renumber_factions(void)
/* gibt parteien neue nummern */
{
    struct renum {
        struct renum *next;
        int want;
        faction *faction;
        attrib *attrib;
    } *renum = NULL, *rp;
    faction *f;
    for (f = factions; f; f = f->next) {
        attrib *a = a_find(f->attribs, &at_number);
        int want;
        struct renum **rn;

        if (!a)
            continue;
        want = a->data.i;
        if (!faction_id_is_unused(want)) {
            a_remove(&f->attribs, a);
            ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
            continue;
        }
        for (rn = &renum; *rn; rn = &(*rn)->next) {
            if ((*rn)->want >= want)
                break;
        }
        if (*rn && (*rn)->want == want) {
            ADDMSG(&f->msgs, msg_message("renumber_inuse", "id", want));
        }
        else {
            struct renum *r = calloc(sizeof(struct renum), 1);
            r->next = *rn;
            r->attrib = a;
            r->faction = f;
            r->want = want;
            *rn = r;
        }
    }
    for (rp = renum; rp; rp = rp->next) {
        f = rp->faction;
        a_remove(&f->attribs, rp->attrib);
        renumber_faction(f, rp->want);
    }
    while (renum) {
        rp = renum->next;
        free(renum);
        renum = rp;
    }
}

int renumber_cmd(unit * u, order * ord)
{
    char token[128];
    const char *s;
    int i = 0;
    faction *f = u->faction;

    init_order(ord);
    s = gettoken(token, sizeof(token));
    switch (findparam_ex(s, u->faction->locale)) {

    case P_FACTION:
        s = gettoken(token, sizeof(token));
        if (s && *s) {
            int id = atoi36((const char *)s);
            attrib *a = a_find(f->attribs, &at_number);
            if (!a)
                a = a_add(&f->attribs, a_new(&at_number));
            a->data.i = id;
        }
        break;

    case P_UNIT:
        s = gettoken(token, sizeof(token));
        if (s && *s) {
            i = atoi36((const char *)s);
            if (i <= 0 || i > MAX_UNIT_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                break;
            }

            if (forbiddenid(i)) {
                cmistake(u, ord, 116, MSG_EVENT);
                break;
            }

            if (findunitg(i, u->region)) {
                cmistake(u, ord, 115, MSG_EVENT);
                break;
            }
        }
        renumber_unit(u, i);
        break;

    case P_SHIP:
        if (!u->ship) {
            cmistake(u, ord, 144, MSG_EVENT);
            break;
        }
        if (ship_owner(u->ship) != u) {
            cmistake(u, ord, 146, MSG_EVENT);
            break;
        }
        if (u->ship->coast != NODIRECTION) {
            cmistake(u, ord, 116, MSG_EVENT);
            break;
        }
        s = gettoken(token, sizeof(token));
        if (s == NULL || *s == 0) {
            i = newcontainerid();
        }
        else {
            i = atoi36((const char *)s);
            if (i <= 0 || i > MAX_CONTAINER_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                break;
            }
            if (findship(i)) {
                cmistake(u, ord, 115, MSG_EVENT);
                break;
            }
        }
        sunhash(u->ship);
        u->ship->no = i;
        shash(u->ship);
        break;
    case P_BUILDING:
    case P_GEBAEUDE:
        if (!u->building) {
            cmistake(u, ord, 145, MSG_EVENT);
            break;
        }
        if (building_owner(u->building) != u) {
            cmistake(u, ord, 148, MSG_EVENT);
            break;
        }
        s = gettoken(token, sizeof(token));
        if (*s == 0) {
            i = newcontainerid();
        }
        else {
            i = atoi36((const char *)s);
            if (i <= 0 || i > MAX_CONTAINER_NR) {
                cmistake(u, ord, 114, MSG_EVENT);
                break;
            }
            if (findbuilding(i)) {
                cmistake(u, ord, 115, MSG_EVENT);
                break;
            }
        }
        bunhash(u->building);
        u->building->no = i;
        bhash(u->building);
        break;

    default:
        cmistake(u, ord, 239, MSG_EVENT);
    }
    return 0;
}

