/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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
#include "monsters.h"
#include "move.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <kernel/race.h>
#include <kernel/region.h>
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
    "chaoscount",
    DEFAULT_INIT,
    DEFAULT_FINALIZE,
    DEFAULT_AGE,
    a_writeint,
    a_readint,
    NULL,
    ATF_UNIQUE
};

void set_chaoscount(struct region *r, int deaths)
{
    if (deaths==0) {
        a_removeall(&r->attribs, &at_chaoscount);
    } else {
        attrib *a = a_find(r->attribs, &at_chaoscount);
        if (!a) {
            a = a_add(&r->attribs, a_new(&at_chaoscount));
        }
        a->data.i = deaths;
    }
}

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
            if ((terrain->flags & LAND_REGION) && terrain->herbs) {
                ++numtypes;
            }
        }
        if (numtypes > 0) {
            types = malloc(sizeof(terrain_type *) * numtypes);
            numtypes = 0;
            for (terrain = terrains(); terrain != NULL; terrain = terrain->next) {
                if ((terrain->flags & LAND_REGION) && terrain->herbs) {
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
        c += u->number;
    }

    if (c == 0) {
        return NULL;
    }
    n = rng_int() % c;
    c = 0;
    u = r->units;

    while (u && c < n) {
        c += u->number;
        u = u->next;
    }

    return u;
}

static void chaos(region * r)
{
    if (rng_int() % 100 < 8) {
        switch (rng_int() % 3) {
        case 0:                  /* Untote */
            if (!(r->terrain->flags & SEA_REGION)) {
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
                        create_unit(r, get_monsters(), rng_int() % 8 + 1,
                        get_race(RC_FIREDRAGON), 0, NULL, NULL);
                    break;
                case 1:
                    mfac = 500;
                    u = create_unit(r, get_monsters(), rng_int() % 4 + 1,
                        get_race(RC_DRAGON), 0, NULL, NULL);
                    break;
                default:
                    mfac = 1000;
                    u = create_unit(r, get_monsters(), rng_int() % 2 + 1,
                        get_race(RC_WYRM), 0, NULL, NULL);
                    break;
                }
                if (mfac) {
                    set_money(u, u->number * (rng_int() % mfac));
                }
                u->flags |= (UFL_ISNEW | UFL_MOVED);
            }
            break;
        case 2:                  /* Terrainver�nderung */
            if (!(r->terrain->flags & FORBIDDEN_REGION)) {
                if (!(r->terrain->flags & SEA_REGION)) {
                    direction_t dir;
                    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                        region *rn = rconnect(r, dir);
                        if (rn && (rn->terrain->flags & SEA_REGION))
                            break;
                    }
                    if (dir != MAXDIRECTIONS) {
                        ship *sh = r->ships;
                        unit **up;

                        while (sh) {
                            ship *nsh = sh->next;
                            double dmg =
                                config_get_flt("rules.ship.damage.atlantis",
                                0.50);
                            damage_ship(sh, dmg);
                            if (sh->damage >= sh->size * DAMAGE_SCALE) {
                                remove_ship(&sh->region->ships, sh);
                            }
                            sh = nsh;
                        }

                        for (up = &r->units; *up;) {
                            unit *u = *up;
                            if (u->ship == 0 && !canfly(u)) {
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
                        if (rn && (rn->terrain->flags & SEA_REGION))
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

        if ((r->flags & RF_CHAOTIC)) {
            chaos(r);
        }
        i = get_chaoscount(r);
        if (i) {
            add_chaoscount(r, -(int)(i * ((double)(rng_int() % 10)) / 100.0));
        }
    }
}

void chaos_register(void) {
    at_register(&at_chaoscount);
}
