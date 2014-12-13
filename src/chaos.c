/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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
#include "chaos.h"
#include "monster.h"
#include "move.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/rng.h>

#include <stdlib.h>
#include <assert.h>

/*********************/
/*   at_chaoscount   */
/*********************/
attrib_type at_chaoscount = {
    "get_chaoscount",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeint,
    a_readint,
    ATF_UNIQUE
};

int get_chaoscount(const region * r)
{
    attrib *a = a_find(r->attribs, &at_chaoscount);
    if (!a)
        return 0;
    return a->data.i;
}

void add_chaoscount(region * r, int fallen)
{
    attrib *a;

    if (fallen == 0)
        return;

    a = a_find(r->attribs, &at_chaoscount);
    if (!a)
        a = a_add(&r->attribs, a_new(&at_chaoscount));
    a->data.i += fallen;

    if (a->data.i <= 0)
        a_remove(&r->attribs, a);
}

static const terrain_type *chaosterrain(void)
{
    static const terrain_type **types;
    static int numtypes;

    if (numtypes == 0) {
        const terrain_type *terrain;
        for (terrain = terrains(); terrain != NULL; terrain = terrain->next) {
            if (fval(terrain, LAND_REGION) && terrain->herbs) {
                ++numtypes;
            }
        }
        if (numtypes > 0) {
            types = malloc(sizeof(terrain_type *) * numtypes);
            numtypes = 0;
            for (terrain = terrains(); terrain != NULL; terrain = terrain->next) {
                if (fval(terrain, LAND_REGION) && terrain->herbs) {
                    types[numtypes++] = terrain;
                }
            }
        }
    }
    if (numtypes > 0) {
        return types[rng_int() % numtypes];
    }
    return NULL;
}

static unit *random_unit(const region * r)
{
    int c = 0;
    int n;
    unit *u;

    for (u = r->units; u; u = u->next) {
        if (u_race(u) != get_race(RC_SPELL)) {
            c += u->number;
        }
    }

    if (c == 0) {
        return NULL;
    }
    n = rng_int() % c;
    c = 0;
    u = r->units;

    while (u && c < n) {
        if (u_race(u) != get_race(RC_SPELL)) {
            c += u->number;
        }
        u = u->next;
    }

    return u;
}

static void chaos(region * r)
{
    if (rng_int() % 100 < 8) {
        switch (rng_int() % 3) {
        case 0:                  /* Untote */
            if (!fval(r->terrain, SEA_REGION)) {
                unit *u = random_unit(r);
                if (u && playerrace(u_race(u))) {
                    ADDMSG(&u->faction->msgs, msg_message("chaos_disease", "unit", u));
                    u_setfaction(u, get_monsters());
                    u_setrace(u, get_race(RC_GHOUL));
                }
            }
            break;
        case 1:                  /* Drachen */
            if (random_unit(r)) {
                int mfac = 0;
                unit *u;
                switch (rng_int() % 3) {
                case 0:
                    mfac = 100;
                    u =
                        createunit(r, get_monsters(), rng_int() % 8 + 1,
                        get_race(RC_FIREDRAGON));
                    break;
                case 1:
                    mfac = 500;
                    u =
                        createunit(r, get_monsters(), rng_int() % 4 + 1,
                        get_race(RC_DRAGON));
                    break;
                default:
                    mfac = 1000;
                    u =
                        createunit(r, get_monsters(), rng_int() % 2 + 1,
                        get_race(RC_WYRM));
                    break;
                }
                if (mfac)
                    set_money(u, u->number * (rng_int() % mfac));
                fset(u, UFL_ISNEW | UFL_MOVED);
            }
        case 2:                  /* Terrainveränderung */
            if (!fval(r->terrain, FORBIDDEN_REGION)) {
                if (!fval(r->terrain, SEA_REGION)) {
                    direction_t dir;
                    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                        region *rn = rconnect(r, dir);
                        if (rn && fval(rn->terrain, SEA_REGION))
                            break;
                    }
                    if (dir != MAXDIRECTIONS) {
                        ship *sh = r->ships;
                        unit **up;

                        while (sh) {
                            ship *nsh = sh->next;
                            float dmg =
                                get_param_flt(global.parameters, "rules.ship.damage.atlantis",
                                0.50);
                            damage_ship(sh, dmg);
                            if (sh->damage >= sh->size * DAMAGE_SCALE) {
                                remove_ship(&sh->region->ships, sh);
                            }
                            sh = nsh;
                        }

                        for (up = &r->units; *up;) {
                            unit *u = *up;
                            if (u_race(u) != get_race(RC_SPELL) && u->ship == 0 && !canfly(u)) {
                                ADDMSG(&u->faction->msgs, msg_message("tidalwave_kill",
                                    "region unit", r, u));
                                remove_unit(up, u);
                            }
                            if (*up == u)
                                up = &u->next;
                        }
                        ADDMSG(&r->msgs, msg_message("tidalwave", "region", r));

                        while (r->buildings) {
                            remove_building(&r->buildings, r->buildings);
                        }
                        terraform_region(r, newterrain(T_OCEAN));
                    }
                }
                else {
                    direction_t dir;
                    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                        region *rn = rconnect(r, dir);
                        if (rn && fval(rn->terrain, SEA_REGION))
                            break;
                    }
                    if (dir != MAXDIRECTIONS) {
                        terraform_region(r, chaosterrain());
                    }
                }
            }
        }
    }
}

void chaos_update(void) {
    region *r;
    /* Chaos */
    for (r = regions; r; r = r->next) {
        int i;

        if (fval(r, RF_CHAOTIC)) {
            chaos(r);
        }
        i = get_chaoscount(r);
        if (i) {
            add_chaoscount(r, -(int)(i * ((double)(rng_int() % 10)) / 100.0));
        }
    }
}

void chaos_init(void) {
    at_register(&at_chaoscount);
}
