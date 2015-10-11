#include <platform.h>
#include <kernel/config.h>
#include "piracy.h"

#include "direction.h"
#include "keyword.h"
#include "move.h"

#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/attrib.h>
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

static void piracy_init(struct attrib *a)
{
    a->data.v = calloc(1, sizeof(piracy_data));
}

static void piracy_done(struct attrib *a)
{
    free(a->data.v);
}

static attrib_type at_piracy_direction = {
    "piracy_direction",
    piracy_init,
    piracy_done,
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

void piracy_cmd(unit * u, struct order *ord)
{
    region *r = u->region;
    ship *sh = u->ship, *sh2;
    direction_t dir, target_dir = NODIRECTION;
    struct {
        const faction *target;
        int value;
    } aff[MAXDIRECTIONS];
    int saff = 0;
    int *il = NULL;
    const char *s;
    attrib *a;

    if (!sh) {
        cmistake(u, ord, 144, MSG_MOVE);
        return;
    }

    if (!u->ship || u != ship_owner(u->ship)) {
        cmistake(u, ord, 146, MSG_MOVE);
        return;
    }

    /* Feststellen, ob schon ein anderer alliierter Pirat ein
    * Ziel gefunden hat. */

    init_order(ord);
    s = getstrtoken();
    if (s != NULL && *s) {
        il = intlist_init();
        while (s && *s) {
            il = intlist_add(il, atoi36(s));
            s = getstrtoken();
        }
    }

    for (a = a_find(r->attribs, &at_piracy_direction);
    a && a->type == &at_piracy_direction; a = a->next) {
        piracy_data *data = a->data.v;
        const faction *p = data->pirate;
        const faction *t = data->target;

        if (alliedunit(u, p, HELP_FIGHT)) {
            if (il == 0 || (t && intlist_find(il, t->no))) {
                target_dir = data->dir;
                break;
            }
        }
    }

    /* Wenn nicht, sehen wir, ob wir ein Ziel finden. */

    if (target_dir == NODIRECTION) {
        /* Einheit ist also Kapitän. Jetzt gucken, in wievielen
        * Nachbarregionen potentielle Opfer sind. */

        for (dir = 0; dir < MAXDIRECTIONS; dir++) {
            region *rc = rconnect(r, dir);
            aff[dir].value = 0;
            aff[dir].target = 0;
            if (rc && fval(rc->terrain, SWIM_INTO) && can_takeoff(sh, r, rc)) {

                for (sh2 = rc->ships; sh2; sh2 = sh2->next) {
                    unit *cap = ship_owner(sh2);
                    if (cap) {
                        faction *f = visible_faction(cap->faction, cap);
                        if (alliedunit(u, f, HELP_FIGHT))
                            continue;
                        if (il == 0 || intlist_find(il, cap->faction->no)) {
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
                if (saff < aff[dir].value)
                    break;
                saff -= aff[dir].value;
            }
            target_dir = dir;
            a =
                a_add(&r->attribs, mk_piracy(u->faction, aff[dir].target, target_dir));
        }
    }

    free(il);

    /* Wenn kein Ziel gefunden, entsprechende Meldung generieren */
    if (target_dir == NODIRECTION) {
        ADDMSG(&u->faction->msgs, msg_message("piratenovictim",
            "ship region", sh, r));
        return;
    }

    /* Meldung generieren */
    ADDMSG(&u->faction->msgs, msg_message("piratesawvictim",
        "ship region dir", sh, r, target_dir));

    /* Befehl konstruieren */
    set_order(&u->thisorder, create_order(K_MOVE, u->faction->locale, "%s",
        LOC(u->faction->locale, directions[target_dir])));

    /* Bewegung ausführen */
    init_order(u->thisorder);
    move_cmd(u, true);
}

void age_piracy(region *r) {
    a_removeall(&r->attribs, &at_piracy_direction);
}
