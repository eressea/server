#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <kernel/config.h>
#include "defaults.h"

#include "laws.h"

/* kernel includes */
#include "kernel/alliance.h"
#include "kernel/ally.h"
#include "kernel/calendar.h"
#include "kernel/callbacks.h"
#include "kernel/connection.h"
#include "kernel/curse.h"
#include "kernel/building.h"
#include "kernel/faction.h"
#include "kernel/group.h"
#include "kernel/item.h"
#include "kernel/messages.h"
#include "kernel/order.h"
#include "kernel/plane.h"
#include "kernel/pool.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/resources.h"
#include "kernel/ship.h"
#include "kernel/skills.h"
#include "kernel/spell.h"
#include "kernel/spellbook.h"
#include "kernel/terrain.h"
#include "kernel/terrainid.h"
#include "kernel/unit.h"

/* util includes */
#include <kernel/attrib.h>
#include <util/base36.h>
#include <kernel/event.h>
#include <util/goodies.h>
#include "util/keyword.h"
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/macros.h>
#include <util/message.h>
#include <util/param.h>
#include <util/parser.h>
#include <util/password.h>
#include <util/path.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/umlaut.h>
#include <util/unicode.h>

/* attributes includes */
#include <attributes/otherfaction.h>
#include <attributes/racename.h>
#include <attributes/raceprefix.h>
#include <attributes/seenspell.h>
#include <attributes/stealth.h>

#include <spells/buildingcurse.h>
#include <spells/regioncurse.h>
#include <spells/unitcurse.h>

#include <iniparser.h>
#include <selist.h>
#include <strings.h>

#include <stb_ds.h>

/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/**
	- Die alten Befehle nicht beim Befehle laden löschen, sondern in u->defaults befördern
	- Dann (hier) beim setzen des langen Befehls wegschmeissen, es sei denn es ist ein K_MOVE,
	- im Falle von K_MOVE eine Variante von update_defaults machen, nur lange Befehle behalten.
*/
static void update_long_order(unit * u)
{
    bool hunger = LongHunger(u);

    freset(u, UFL_MOVED);
    freset(u, UFL_LONGACTION);

    if (hunger) {
        /* Hungernde Einheiten fuehren NUR den default-Befehl aus */
        set_order(&u->thisorder, default_order(u->faction->locale));
    }
    else {
        order* ord;
        keyword_t thiskwd = NOKEYWORD;
        bool exclusive = true;
        /* check all orders for a potential new long order this round: */
        for (ord = u->orders; ord; ord = ord->next) {
            keyword_t kwd = getkeyword(ord);
            if (kwd == NOKEYWORD) continue;

            if (is_long(kwd)) {
                if (thiskwd == NOKEYWORD) {
                    /* we have found the (first) long order
                     * some long orders can have multiple instances: */
                    switch (kwd) {
                        /* Wenn gehandelt wird, darf kein langer Befehl ausgefuehrt
                         * werden. Da Handel erst nach anderen langen Befehlen kommt,
                         * muss das vorher abgefangen werden. Wir merken uns also
                         * hier, ob die Einheit handelt. */
                    case K_BUY:
                    case K_SELL:
                    case K_CAST:
                        /* non-exclusive orders can be used with others. BUY can be paired with SELL,
                         * CAST with other CAST orders. compatibility is checked once the second
                         * long order is analyzed (below). */
                        exclusive = false;
                        set_order(&u->thisorder, NULL);
                        break;

                    default:
                        if (exclusive) {
                            set_order(&u->thisorder, copy_order(ord));
                            if (u->defaults) {
                                if (is_persistent(ord)) {
                                    /* this long order replaces last week's orders (not K_MOVE) */
                                    free_orders(&u->defaults);
                                }
                                else {
                                    order** ordp = &u->defaults;
                                    while (*ordp) {
                                        order* o = *ordp;
                                        if (!is_long(getkeyword(o))) {
                                            *ordp = o->next;
                                            o->next = NULL;
                                            free_order(o);
                                        }
                                        else {
                                            ordp = &o->next;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                    thiskwd = kwd;
                }
                else {
                    /* we have found a second long order. this is okay for some, but not all commands.
                     * u->thisorder is already set, and should not have to be updated. */
                    switch (kwd) {
                    case K_CAST:
                        if (thiskwd != K_CAST) {
                            cmistake(u, ord, 52, MSG_EVENT);
                        }
                        break;
                    case K_SELL:
                        if (thiskwd != K_SELL && thiskwd != K_BUY) {
                            cmistake(u, ord, 52, MSG_EVENT);
                        }
                        break;
                    case K_BUY:
                        if (thiskwd != K_SELL) {
                            cmistake(u, ord, 52, MSG_EVENT);
                        }
                        else {
                            thiskwd = K_BUY;
                        }
                        break;
                    default:
                        cmistake(u, ord, 52, MSG_EVENT);
                        break;
                    }
                }
            }
        }
        if (!u->thisorder) {
            free_orders(&u->defaults);
        }
    }
}

static void remove_long(order** ordp)
{
    while (*ordp) {
        order* ord = *ordp;
        if (is_long(getkeyword(ord))) {
            *ordp = ord->next;
            ord->next = NULL;
            free_order(ord);
        }
        else {
            ordp = &ord->next;
        }
    }
}

void defaultorders(void)
{
    region *r;

    assert(!keyword_disabled(K_DEFAULT));
    for (r = regions; r; r = r->next) {
        unit *u;
        for (u = r->units; u; u = u->next) {
            order **ordp = &u->orders;
            bool neworders = false;
            while (*ordp != NULL) {
                order *ord = *ordp;
                if (getkeyword(ord) == K_DEFAULT) {
                    char lbuf[8192];
                    const char *s;
                    init_order(ord, NULL);
                    s = gettoken(lbuf, sizeof(lbuf));
                    if (s) {
                        order* new_order = parse_order(s, u->faction->locale);
                        if (new_order) {
                            if (!neworders && is_long(getkeyword(new_order)))
                            {
                                remove_long(&u->defaults);
                                neworders = true;
                            }
                            addlist(&u->defaults, new_order);
                        }
                    }
                    /* The DEFAULT order itself must not be repeated */
                    *ordp = ord->next;
                    ord->next = NULL;
                    free_order(ord);
                }
                else {
                    ordp = &ord->next;
                }
            }
        }
    }
}

void update_defaults(void)
{
    faction* f;
    for (f = factions; f; f = f->next)
    {
        unit* u;
        for (u = f->units; u != NULL; u = u->nextF) {
            /* u->defaults contains new orders that were added by K_DEFAULT */
            order** ordi = &u->defaults;
            bool new_long = false;
            while (*ordi) {
                order* ord = *ordi;
                if (is_long(getkeyword(ord))) {
                    new_long = true;
                }
                ordi = &ord->next;
            }
            /* we add persistent new orders to the end of these new defaults: */
            if (u->orders) {
                bool repeated = u->defaults != NULL;
                order** ordp = &u->orders;
                while (*ordp) {
                    order* ord = *ordp;
                    keyword_t kwd = getkeyword(ord);
                    bool keep = !(repeated && is_repeated(kwd));
                    if (!keep && !new_long && is_long(kwd)) {
                        keep = true;
                    }
                    if (keep) {
                        if (is_persistent(ord)) {
                            *ordp = ord->next;
                            *ordi = ord;
                            ord->next = NULL;
                            ordi = &ord->next;
                            continue;
                        }
                    }
                    else {
                        /* replace any old long orders */
                        *ordp = ord->next;
                        ord->next = NULL;
                        free_order(ord);
                        continue;
                    }
                    ordp = &ord->next;
                }
                free_orders(&u->orders);
            }
            u->orders = u->defaults;
            u->defaults = NULL;
        }
    }
}

void update_long_orders(void)
{
    region* r;
    for (r = regions; r; r = r->next) {
        unit* u;
        for (u = r->units; u; u = u->next) {
            update_long_order(u);
        }
    }
}
