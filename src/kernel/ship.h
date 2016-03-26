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

#ifndef H_KRNL_SHIP
#define H_KRNL_SHIP
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "direction.h"

#define DAMAGE_SCALE 100        /* multiplier for sh->damage */

    /* ship_type::flags */
#define SFL_OPENSEA 0x01
#define SFL_FLY     0x02
#define SFL_NOCOAST 0x04

    typedef struct ship_type {
        char *_name;

        int range;                  /* range in regions */
        int range_max;
        int flags;                  /* flags */
        int combat;                 /* modifier for combat */
        int fishing;                /* weekly income from fishing */

        double storm;               /* multiplier for chance to drift in storm */
        double damage;              /* multiplier for damage taken by the ship */

        int cabins;                 /* max. cabins (weight) */
        int cargo;                  /* max. cargo (weight) */

        int cptskill;               /* min. skill of captain */
        int minskill;               /* min. skill to sail this (crew) */
        int sumskill;               /* min. sum of crew+captain */

        int at_bonus;               /* Verändert den Angriffsskill (default: 0) */
        int df_bonus;               /* Verändert den Verteidigungskill (default: 0) */
        float tac_bonus;

        struct terrain_type ** coasts; /* coast that this ship can land on */

        struct construction *construction;  /* how to build a ship */
    } ship_type;

    extern struct quicklist *shiptypes;

    /* Alte Schiffstypen: */

    const ship_type *st_find(const char *name);
    ship_type *st_get_or_create(const char *name);
    void free_shiptypes(void);

#define NOSHIP NULL

#define SF_DRIFTED 1<<0
#define SF_MOVED   1<<1
#define SF_DAMAGED 1<<2         /* for use in combat */
#define SF_SELECT  1<<3         /* previously FL_DH */
#define SF_FISHING 1<<4         /* was on an ocean, can fish */
#define SF_FLYING  1<<5         /* the ship can fly */

#define SFL_SAVEMASK (SF_FLYING)
#define INCOME_FISHING 10

    typedef struct ship {
        struct ship *next;
        struct ship *nexthash;
        struct unit * _owner; /* never use directly, always use ship_owner() */
        int no;
        struct region *region;
        char *name;
        char *display;
        struct attrib *attribs;
        int size;
        int damage;                 /* damage in 100th of a point of size */
        int flags;
        const struct ship_type *type;
        direction_t coast;
        struct ship *fleet;
    } ship;

    void damage_ship(struct ship * sh, double percent);
    bool ship_isdamaged(const struct ship *sh);
    bool ship_isdestroyed(const struct ship *sh);
    void ship_set_owner(struct unit * u);
    struct unit *ship_owner(const struct ship *sh);
    void ship_update_owner(struct ship * sh);

    extern const char *shipname(const struct ship *self);
    extern int ship_capacity(const struct ship *sh);
    extern int ship_cabins(const struct ship *sh);
    extern void ship_weight(const struct ship *sh, int *weight, int *cabins);

    extern ship *new_ship(const struct ship_type *stype, struct region *r,
        const struct locale *lang);
    extern const char *write_shipname(const struct ship *sh, char *buffer,
        size_t size);
    extern struct ship *findship(int n);
    extern struct ship *findshipr(const struct region *r, int n);

    extern const struct ship_type *findshiptype(const char *s,
        const struct locale *lang);

    extern void write_ship_reference(const struct ship *sh,
    struct storage *store);

    extern void remove_ship(struct ship **slist, struct ship *s);
    extern void free_ship(struct ship *s);
    extern void free_ships(void);

    const char *ship_name(const struct ship *self);
    void ship_setname(struct ship *self, const char *name);
    int ship_speed(const struct ship *sh, const struct unit *u);
    int ship_crew_skill(const struct ship *sh);

    int ship_damage_percent(const struct ship *ship);

    bool ship_isfleet(const struct ship *sh);
    struct ship *fleet_add_ship(struct ship *sh, struct ship *fleet, struct unit *cpt);
    struct ship *fleet_remove_ship(struct ship *sh, struct unit *new_cpt);
    void fleet_cmd(struct region * r);

    int ship_type_cpt_skill(const struct ship *sh);
    int ship_type_crew_skill(const struct ship *sh);
    bool ship_iscomplete(const struct ship *sh);

    int aggregate_max(int i1, int i2);
    int aggregate_min(int i1, int i2);
    int aggregate_sum(int i1, int i2);

    typedef int (*int_visitor) (const ship *fleet, const ship *sh, void *state);
    typedef bool (*bool_visitor) (struct ship *fleet, struct ship *sh, void *state);
    typedef bool (*const_bool_visitor) (const struct ship *fleet, const struct ship *sh, void *state);
    typedef int (*aggregator) (int v1, int v2);

    int fleet_const_int_aggregate(const struct ship *sh, int_visitor getvalue, aggregator aggr, int init_value, void *state);
    void fleet_visit(struct ship *sh, bool_visitor visit_ship, void *state);
    void fleet_const_visit(const struct ship *sh, const_bool_visitor visit_ship, void *state);

#ifdef __cplusplus
}
#endif
#endif
