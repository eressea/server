#include <platform.h>
#include <kernel/config.h>
#include "chaos.h"
#include "monsters.h"
#include "move.h"
#include "spy.h"

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

#include <kernel/attrib.h>
#include <util/rng.h>

#include <stdlib.h>
#include <assert.h>

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
            int n = 0;
            types = malloc(sizeof(terrain_type *) * numtypes);
            if (!types) abort();
            for (terrain = terrains(); n != numtypes && terrain != NULL; terrain = terrain->next) {
                if ((terrain->flags & LAND_REGION) && terrain->herbs) {
                    types[n++] = terrain;
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

static void chaos(region * r, faction *monsters)
{
    if (rng_int() % 100 < 8) {
        switch (rng_int() % 3) {
        case 0:                  /* Untote */
            if (!(r->terrain->flags & SEA_REGION)) {
                unit *u = random_unit(r);
                if (u && !undeadrace(u_race(u))) {
                    faction * f = u->faction;
                    if (join_monsters(u, monsters)) {
                        ADDMSG(&f->msgs, msg_message("chaos_disease", "unit", u));
                        u_setrace(u, get_race(RC_GHOUL));
                    }
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
                        create_unit(r, monsters, rng_int() % 8 + 1,
                        get_race(RC_FIREDRAGON), 0, NULL, NULL);
                    break;
                case 1:
                    mfac = 500;
                    u = create_unit(r, monsters, rng_int() % 4 + 1,
                        get_race(RC_DRAGON), 0, NULL, NULL);
                    break;
                default:
                    mfac = 1000;
                    u = create_unit(r, monsters, rng_int() % 2 + 1,
                        get_race(RC_WYRM), 0, NULL, NULL);
                    break;
                }
                if (mfac) {
                    set_money(u, u->number * (rng_int() % mfac));
                }
                u->flags |= (UFL_ISNEW | UFL_MOVED);
            }
            break;
        case 2:                  /* Terrainveraenderung */
            if (!(r->terrain->flags & FORBIDDEN_REGION)) {
                if (!(r->terrain->flags & SEA_REGION)) {
                    direction_t dir;
                    for (dir = 0; dir != MAXDIRECTIONS; ++dir) {
                        region *rn = rconnect(r, dir);
                        if (rn && (rn->terrain->flags & SEA_REGION))
                            break;
                    }
                    if (dir != MAXDIRECTIONS) {
                        ship **slist = &r->ships;
                        unit **up;

                        while (*slist) {
                            ship *sh = *slist;

                            damage_ship(sh, 0.5);
                            if (sh->damage >= sh->size * DAMAGE_SCALE) {
                                sink_ship(sh);
                                remove_ship(slist, sh);
                            }
                            else {
                                slist = &sh->next;
                            }
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
        if ((r->flags & RF_CHAOTIC)) {
            chaos(r, get_monsters());
        }
    }
}
