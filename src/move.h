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

#ifndef H_KRNL_MOVEMENT
#define H_KRNL_MOVEMENT

#include "direction.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct region;
    struct region_list;
    struct ship;
    struct building_type;

    extern struct attrib_type at_shiptrail;

    /* die Zahlen sind genau äquivalent zu den race Flags */
#define MV_CANNOTMOVE     (1<<5)
#define MV_FLY            (1<<7)        /* kann fliegen */
#define MV_SWIM           (1<<8)        /* kann schwimmen */
#define MV_WALK           (1<<9)        /* kann über Land gehen */

#define HORSESNEEDED    2
#define STRENGTHMULTIPLIER  50   /* multiplier for trollbelt */

    /* ein mensch wiegt 10, traegt also 5, ein pferd wiegt 50, traegt also 20. ein
    ** wagen wird von zwei pferden gezogen und traegt total 140, davon 40 die
    ** pferde, macht nur noch 100, aber samt eigenem gewicht (40) macht also 140. */

    /* movewhere error codes */
    enum {
        E_MOVE_OK = 0,              /* possible to move */
        E_MOVE_NOREGION,            /* no region exists in this direction */
        E_MOVE_BLOCKED              /* cannot see this region, there is a blocking connection. */
    };
    int movewhere(const struct unit *u, const char *token,
    struct region *r, struct region **resultp);
    direction_t reldirection(const struct region *from, const struct region *to);

    int personcapacity(const struct unit *u);
    void movement(void);
    void run_to(struct unit *u, struct region *to);
    struct unit *is_guarded(struct region *r, struct unit *u, unsigned int mask);
    bool is_guard(const struct unit *u, unsigned int mask);
    int enoughsailors(const struct ship *sh, int sumskill);
    bool canswim(struct unit *u);
    bool canfly(struct unit *u);
    void travelthru(const struct unit *u, struct region *r);
    struct ship *move_ship(struct ship *sh, struct region *from,
    struct region *to, struct region_list *route);
    int walkingcapacity(const struct unit *u);
    void follow_unit(struct unit *u);
    bool buildingtype_exists(const struct region *r,
        const struct building_type *bt, bool working);
    struct unit *owner_buildingtyp(const struct region *r,
        const struct building_type *bt);
    bool move_blocked(const struct unit *u, const struct region *src,
        const struct region *dest);

#define SA_HARBOUR 2
#define SA_COAST 1
#define SA_NO_INSECT -1
#define SA_NO_COAST -2

    int check_ship_allowed(struct ship *sh, const struct region * r);
#ifdef __cplusplus
}
#endif
#endif
