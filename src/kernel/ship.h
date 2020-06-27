#ifndef H_KRNL_SHIP
#define H_KRNL_SHIP

#include "types.h"
#include "direction.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DAMAGE_SCALE 100        /* multiplier for sh->damage */

    /* ship_type::flags */
#define SFL_OPENSEA 0x01
#define SFL_FLY     0x02
#define SFL_NOCOAST 0x04
#define SFL_SPEEDY  0x08

#define SFL_DEFAULT 0

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

        int at_bonus;               /* Veraendert den Angriffsskill (default: 0) */
        int df_bonus;               /* Veraendert den Verteidigungskill (default: 0) */
        double tac_bonus;

        struct terrain_type ** coasts; /* coast that this ship can land on */

        struct construction *construction;  /* how to build a ship */
    } ship_type;

    extern struct selist *shiptypes;

    /* Alte Schiffstypen: */

    const ship_type *st_find(const char *name);
    ship_type *st_get_or_create(const char *name);
    void free_shiptypes(void);

#define NOSHIP NULL

#define SF_DRIFTED (1<<0)
#define SF_MOVED   (1<<1)
#define SF_DAMAGED (1<<2)         /* for use in combat */
#define SF_SELECT  (1<<3)         /* previously FL_DH */
#define SF_FISHING (1<<4)         /* was on an ocean, can fish */
#define SF_FLYING  (1<<5)         /* the ship can fly */

#define SFL_SAVEMASK (SF_FLYING)
#define INCOME_FISHING 10

    typedef struct ship {
        struct ship *next;
        struct ship *nexthash;
        struct unit * _owner; /* never use directly, always use ship_owner() */
        int no;
        int number;
        struct region *region;
        char *name;
        char *display;
        struct attrib *attribs;
        int size;
        int damage;                 /* damage in 100th of a point of size */
        int flags;
        const struct ship_type *type;
        direction_t coast;
    } ship;

    void damage_ship(struct ship * sh, double percent);
    void ship_set_owner(struct unit * u);
    struct unit *ship_owner(const struct ship *sh);
    void ship_update_owner(struct ship * sh);

    const char *shipname(const struct ship *self);
    int ship_capacity(const struct ship *sh);
    int ship_cabins(const struct ship *sh);
    int ship_maxsize(const struct ship *sh);
    bool ship_finished(const struct ship *sh);
    void getshipweight(const struct ship *sh, int *weight, int *cabins);

    ship *new_ship(const struct ship_type *stype, struct region *r,
        const struct locale *lang);
    const char *write_shipname(const struct ship *sh, char *buffer,
        size_t size);
    struct ship *findship(int n);

    const struct ship_type *findshiptype(const char *s,
        const struct locale *lang);

    void write_ship_reference(const struct ship *sh,
    struct storage *store);

    void remove_ship(struct ship **slist, struct ship *s);
    void free_ship(struct ship *s);
    void free_ships(void);

    const char *ship_getname(const struct ship *sh);
    void ship_setname(struct ship *self, const char *name);
    int shipspeed(const struct ship *sh, const struct unit *u);

    bool ship_crewed(const struct ship *sh);
    int crew_skill(const struct ship *sh);
    int ship_captain_minskill(const struct ship *sh);

    int ship_damage_percent(const struct ship *sh);
    void scale_ship(struct ship *sh, int n);
#ifdef __cplusplus
}
#endif
#endif
