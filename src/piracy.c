#include <platform.h>
#include "piracy.h"

#include "direction.h"
#include "keyword.h"
#include "move.h"

#include <kernel/ally.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <kernel/attrib.h>
#include <util/base36.h>
#include <util/goodies.h>
#include <util/language.h>
#include <util/parser.h>
#include <util/rng.h>
#include <util/message.h>

#include <attributes/otherfaction.h>

#include <assert.h>
#include <stdlib.h>

typedef struct piracy_data {
    const struct faction *pirate;
    const struct faction *target;
    direction_t dir;
} piracy_data;

static void piracy_init(variant *var)
{
    var->v = calloc(1, sizeof(piracy_data));
}

static attrib_type at_piracy_direction = {
    "piracy_direction",
    piracy_init,
    a_free_voidptr,
    DEFAULT_AGE,
    NO_WRITE,
    NO_READ
};

static attrib *mk_piracy(const faction * pirate, const faction * target,
    direction_t target_dir)
{
    attrib *a = a_new(&at_piracy_direction);
    piracy_data *data = a->data.v;
    data->pirate = pirate;
    data->target = target;
    data->dir = target_dir;
    return a;
}

static bool validate_pirate(unit *u, order *ord) {
    assert(u);
    assert(ord);
    if (u_race(u)->flags & (RCF_SWIM | RCF_FLY)) {
        return true;
    }
    if (!u->ship) {
        cmistake(u, ord, 144, MSG_MOVE);
        return false;
    }

    if (!u->ship || u != ship_owner(u->ship)) {
        cmistake(u, ord, 146, MSG_MOVE);
        return false;
    }
    return true;
}

int *parse_ids(const order *ord) {
    const char *s;
    int *il = NULL;

    init_order_depr(ord);
    s = getstrtoken();
    if (s != NULL && *s) {
        il = intlist_init();
        while (s && *s) {
            il = intlist_add(il, atoi36(s));
            s = getstrtoken();
        }
    }
    return il;
}

direction_t find_piracy_target(unit *u, int *il) {
    attrib *a;
    region *r = u->region;

    for (a = a_find(r->attribs, &at_piracy_direction);
    a && a->type == &at_piracy_direction; a = a->next) {
        piracy_data *data = a->data.v;
        const faction *p = data->pirate;
        const faction *t = data->target;

        if (alliedunit(u, p, HELP_FIGHT)) {
            if (il == 0 || (t && intlist_find(il, t->no))) {
                return data->dir;
            }
        }
    }
    return NODIRECTION;
}

void piracy_cmd(unit * u)
{
    order *ord;
    region *r = u->region;
    ship *sh = u->ship, *sh2;
    direction_t target_dir;
    struct {
        const faction *target;
        int value;
    } aff[MAXDIRECTIONS];
    int *il;

    assert(u->thisorder);
    if (!validate_pirate(u, u->thisorder)) {
        return;
    }

    il = parse_ids(u->thisorder);
    /* Feststellen, ob schon ein anderer alliierter Pirat ein
    * Ziel gefunden hat. */

    target_dir = find_piracy_target(u, il);

    /* Wenn nicht, sehen wir, ob wir ein Ziel finden. */

    if (target_dir == NODIRECTION) {
        int saff = 0;
        direction_t dir;
        /* Einheit ist also Kapitän. Jetzt gucken, in wievielen
        * Nachbarregionen potentielle Opfer sind. */

        for (dir = 0; dir < MAXDIRECTIONS; dir++) {
            region *rc = rconnect(r, dir);
            aff[dir].value = 0;
            aff[dir].target = 0;
            /* TODO this could still result in an illegal movement order (through a wall or whatever)
             * which will be prevented by move_cmd below */
            if (rc &&
                ((sh && !(rc->terrain->flags & FORBIDDEN_REGION) && can_takeoff(sh, r, rc))
                    || (canswim(u) && ((rc->terrain->flags & (SWIM_INTO|SEA_REGION)) == (SWIM_INTO | SEA_REGION))))) {

                for (sh2 = rc->ships; sh2; sh2 = sh2->next) {
                    unit *cap = ship_owner(sh2);
                    if (cap) {
                        faction *f = visible_faction(cap->faction, cap);
                        if (alliedunit(u, f, HELP_FIGHT))
                            continue;
                        if (!il || intlist_find(il, cap->faction->no)) {
                            /* TODO: shouldn't this be f->no? */
                            ++aff[dir].value;
                            if (rng_int() % aff[dir].value == 0) {
                                aff[dir].target = f;
                            }
                        }
                    }
                }

                /* Und aufaddieren. */
                saff += aff[dir].value;
            }
        }

        if (saff != 0) {
            saff = rng_int() % saff;
            for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                if (saff < aff[dir].value) {
                    target_dir = dir;
                    a_add(&r->attribs, mk_piracy(u->faction, aff[dir].target, target_dir));
                    break;
                }
                saff -= aff[dir].value;
            }
        }
    }

    free(il);

    /* Wenn kein Ziel gefunden, entsprechende Meldung generieren */
    if (target_dir == NODIRECTION) {
        ADDMSG(&u->faction->msgs, msg_message("piratenovictim",
            "ship unit region", sh, u, r));
        return;
    }

    /* Meldung generieren */
    ADDMSG(&u->faction->msgs, msg_message("piratesawvictim",
        "ship unit region dir", sh, u, r, target_dir));

    /* Befehl konstruieren */
    /* TODO: why change u->thisorder? */
    /* FIXME: when u->thisorder == ord, set_order calls free, destroys both. */
    ord = create_order(K_MOVE, u->faction->locale, "%s", LOC(u->faction->locale, directions[target_dir]));

    /* Bewegung ausführen */
    init_order_depr(ord);
    move_cmd(u, ord);
    free_order(ord);
}

void age_piracy(region *r) {
    a_removeall(&r->attribs, &at_piracy_direction);
}
