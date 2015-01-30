/*
Copyright (c) 1998-2015, Enno Rehling Rehling <enno@eressea.de>
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
#include "phoenixcompass.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/messages.h>

/* util includes */
#include <util/functions.h>
#include <util/rand.h>
#include <util/rng.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <limits.h>

static int
use_phoenixcompass(struct unit *u, const struct item_type *itype,
int amount, struct order *ord)
{
    region *r;
    unit *closest_phoenix = NULL;
    int closest_phoenix_distance = INT_MAX;
    bool confusion = false;
    direction_t direction;
    unit *u2;
    direction_t closest_neighbour_direction = 0;
    static const race *rc_phoenix = NULL;

    if (rc_phoenix == NULL) {
        rc_phoenix = rc_find("phoenix");
        if (rc_phoenix == NULL)
            return 0;
    }

    /* find the closest phoenix. */

    for (r = regions; r; r = r->next) {
        for (u2 = r->units; u2; u2 = u2->next) {
            if (u_race(u2) == rc_phoenix) {
                if (closest_phoenix == NULL) {
                    closest_phoenix = u2;
                    closest_phoenix_distance =
                        distance(u->region, closest_phoenix->region);
                }
                else {
                    int dist = distance(u->region, r);
                    if (dist < closest_phoenix_distance) {
                        closest_phoenix = u2;
                        closest_phoenix_distance = dist;
                        confusion = false;
                    }
                    else if (dist == closest_phoenix_distance) {
                        confusion = true;
                    }
                }
            }
        }
    }

    /* no phoenix found at all.* if confusion == true more than one phoenix
     * at the same distance was found and the device is confused */

    if (closest_phoenix == NULL
        || closest_phoenix->region == u->region || confusion) {
        add_message(&u->faction->msgs, msg_message("phoenixcompass_confusion",
            "unit region command", u, u->region, ord));
        return 0;
    }

    /* else calculate the direction. this is tricky. we calculate the
     * neighbouring region which is closest to the phoenix found. hardcoded
     * for readability. */

    for (direction = 0; direction < MAXDIRECTIONS; ++direction) {
        region *neighbour;
        int closest_neighbour_distance = INT_MAX;

        neighbour = r_connect(u->region, direction);
        if (neighbour != NULL) {
            int dist = distance(neighbour, closest_phoenix->region);
            if (dist < closest_neighbour_distance) {
                closest_neighbour_direction = direction;
                closest_neighbour_distance = dist;
            }
            else if (dist == closest_neighbour_distance && rng_int() % 100 < 50) {
                /* there can never be more than two neighbours with the same
                 * distance (except when you are standing in the same region
                 * as the phoenix, but that case has already been handled).
                 * therefore this simple solution is correct */
                closest_neighbour_direction = direction;
                closest_neighbour_distance = dist;
            }
        }
    }

    add_message(&u->faction->msgs, msg_message("phoenixcompass_success",
        "unit region command dir",
        u, u->region, ord, closest_neighbour_direction));

    return 0;
}

void register_phoenixcompass(void)
{
    register_item_use(use_phoenixcompass, "use_phoenixcompass");
}
