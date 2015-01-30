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

#ifndef H_KRNL_REGION
#define H_KRNL_REGION
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "types.h"
#include "direction.h"

    /* FAST_CONNECT: regions are directly connected to neighbours, saves doing
       a hash-access each time a neighbour is needed, 6 extra pointers per hex */
#define FAST_CONNECT

#define RF_CHAOTIC     (1<<0) /* persistent */
#define RF_MALLORN     (1<<1) /* persistent */
#define RF_BLOCKED     (1<<2) /* persistent */

#define RF_UNUSED_3  (1<<3)
#define RF_UNUSED_4  (1<<4)
#define RF_UNUSED_5  (1<<5)
#define RF_UNUSED_6  (1<<6)
#define RF_UNUSED_7  (1<<7)
#define RF_UNUSED_8  (1<<8)

#define RF_ENCOUNTER   (1<<9) /* persistent */
#define RF_MAPPER_HIGHLIGHT (1<<10)
#define RF_LIGHTHOUSE  (1<<11) /* this region may contain a lighthouse */
#define RF_ORCIFIED    (1<<12) /* persistent */
#define RF_MIGRATION   (1<<13)

#define RF_UNUSED_14 (1<<14)
#define RF_UNUSED_15   (1<<15)
#define RF_UNUSED_16   (1<<16)

#define RF_SELECT      (1<<17)
#define RF_MARK        (1<<18)
#define RF_TRAVELUNIT    (1<<19)
#define RF_GUARDED       (1<<20) /* persistent */

#define RF_ALL 0xFFFFFF

#define RF_SAVEMASK (RF_CHAOTIC|RF_MALLORN|RF_BLOCKED|RF_ENCOUNTER|RF_ORCIFIED|RF_GUARDED)
    struct message;
    struct message_list;
    struct rawmaterial;
    struct donation;
    struct item;

#define MORALE_TAX_FACTOR 0.005 /* 0.5% tax per point of morale */
#define MORALE_MAX 10           /* Maximum morale allowed */
#define MORALE_DEFAULT 1        /* Morale of peasants when they are conquered for the first time */
#define MORALE_TAKEOVER 0       /* Morale of peasants after they lose their lord */
#define MORALE_COOLDOWN 2       /* minimum cooldown before a morale change occurs */
#define MORALE_AVERAGE 6        /* default average time for morale to change */
#define MORALE_TRANSFER 2       /* points of morale lost when GIVE COMMAND */

#define OWNER_MOURNING 0x01
    typedef struct region_owner {
        struct faction *owner;
        struct alliance *alliance;
        int since_turn;             /* turn the region changed owners */
        int morale_turn;            /* turn when morale has changed most recently */
        int flags;
    } region_owner;

    typedef struct demand {
        struct demand *next;
        const struct luxury_type *type;
        int value;
    } demand;

    typedef struct land_region {
        char *name;
        /* TODO: demand kann nach Konvertierung entfernt werden. */
        demand *demands;
        const struct item_type *herbtype;
        short herbs;
        short morale;
        int trees[3];               /* 0 -> seeds, 1 -> shoots, 2 -> trees */
        int horses;
        int peasants;
        int newpeasants;
        int money;
        struct item *items;         /* items that can be claimed */
        struct region_owner *ownership;
    } land_region;

    typedef struct donation {
        struct donation *next;
        struct faction *f1, *f2;
        int amount;
    } donation;

    typedef struct region {
        struct region *next;
        struct land_region *land;
        struct unit *units;
        struct ship *ships;
        struct building *buildings;
        unsigned int index;
        /* an ascending number, to improve the speed of determining the interval in
           which a faction has its units. See the implementations of firstregion
           and lastregion */
        int uid;           /* a unique id */
        int x, y;
        struct plane *_plane;       /* to access, use rplane(r) */
        char *display;
        int flags;
        unsigned short age;
        struct message_list *msgs;
        struct individual_message {
            struct individual_message *next;
            const struct faction *viewer;
            struct message_list *msgs;
        } *individual_messages;
        struct attrib *attribs;
        struct donation *donations;
        const struct terrain_type *terrain;
        struct rawmaterial *resources;
#ifdef FAST_CONNECT
        struct region *connect[MAXDIRECTIONS];      /* use rconnect(r, dir) to access */
#endif
    } region;

    extern struct region *regions;

    typedef struct region_list {
        struct region_list *next;
        struct region *data;
    } region_list;

    struct message_list *r_getmessages(const struct region *r,
        const struct faction *viewer);
    struct message *r_addmessage(struct region *r, const struct faction *viewer,
    struct message *msg);

    typedef struct {
        direction_t dir;
    } moveblock;

#define reg_hashkey(r) (r->index)

    int distance(const struct region *, const struct region *);
    int koor_distance(int ax, int ay, int bx, int by);
    struct region *findregion(int x, int y);
    struct region *findregionbyid(int uid);

    extern struct attrib_type at_moveblock;
    extern struct attrib_type at_peasantluck;
    extern struct attrib_type at_horseluck;
    extern struct attrib_type at_woodcount;
    extern struct attrib_type at_deathcount;
    extern struct attrib_type at_travelunit;

    void initrhash(void);
    void rhash(struct region *r);
    void runhash(struct region *r);

    void free_regionlist(region_list * rl);
    void add_regionlist(region_list ** rl, struct region *r);

    int deathcount(const struct region *r);
    int chaoscount(const struct region *r);

    void deathcounts(struct region *r, int delta);
    void chaoscounts(struct region *r, int delta);

    void setluxuries(struct region *r, const struct luxury_type *sale);
    int get_maxluxuries(void);

    short rroad(const struct region *r, direction_t d);
    void rsetroad(struct region *r, direction_t d, short value);

    bool is_coastregion(struct region *r);

    int rtrees(const struct region *r, int ageclass);
    enum {
        TREE_SEED = 0,
        TREE_SAPLING = 1,
        TREE_TREE = 2
    };

    int rsettrees(const struct region *r, int ageclass, int value);

    int rpeasants(const struct region *r);
    void rsetpeasants(struct region *r, int value);
    int rmoney(const struct region *r);
    void rsetmoney(struct region *r, int value);
    int rhorses(const struct region *r);
    void rsethorses(const struct region *r, int value);

#define rbuildings(r) ((r)->buildings)

#define rherbtype(r) ((r)->land?(r)->land->herbtype:0)
#define rsetherbtype(r, value) if ((r)->land) (r)->land->herbtype=(value)

#define rherbs(r) ((r)->land?(r)->land->herbs:0)
#define rsetherbs(r, value) if ((r)->land) ((r)->land->herbs=(short)(value))

    bool r_isforest(const struct region *r);

#define rterrain(r) (oldterrain((r)->terrain))
#define rsetterrain(r, t) ((r)->terrain = newterrain(t))

    const char *rname(const struct region *r, const struct locale *lang);

#define rplane(r) getplane(r)

    void r_setdemand(struct region *r, const struct luxury_type *ltype,
        int value);
    int r_demand(const struct region *r, const struct luxury_type *ltype);

    const char *write_regionname(const struct region *r, const struct faction *f,
        char *buffer, size_t size);

    struct region *new_region(int x, int y, struct plane *pl, int uid);
    void remove_region(region ** rlist, region * r);
    void terraform_region(struct region *r, const struct terrain_type *terrain);
    bool pnormalize(int *x, int *y, const struct plane *pl);

    extern const int delta_x[MAXDIRECTIONS];
    extern const int delta_y[MAXDIRECTIONS];
    direction_t dir_invert(direction_t dir);
    int production(const struct region *r);

    void region_set_owner(struct region *r, struct faction *owner, int turn);
    struct faction *region_get_owner(const struct region *r);
    struct alliance *region_get_alliance(const struct region *r);

    struct region *r_connect(const struct region *, direction_t dir);
#ifdef FAST_CONNECT
# define rconnect(r, dir) ((r)->connect[dir]?(r)->connect[dir]:r_connect(r, (direction_t)dir))
#else
# define rconnect(r, dir) r_connect(r, (direction_t)dir)
#endif

    void free_regions(void);

    int region_get_morale(const region * r);
    void region_set_morale(region * r, int morale, int turn);

    void write_region_reference(const struct region *r, struct storage *store);
    variant read_region_reference(struct storage *store);
    int resolve_region_coor(variant id, void *address);
    int resolve_region_id(variant id, void *address);
#define RESOLVE_REGION(version) ((version<UIDHASH_VERSION)?resolve_region_coor:resolve_region_id)

    const char *regionname(const struct region *r, const struct faction *f);

    const char *region_getname(const struct region *self);
    void region_setname(struct region *self, const char *name);
    const char *region_getinfo(const struct region *self);
    void region_setinfo(struct region *self, const char *name);
    int region_getresource(const struct region *r,
        const struct resource_type *rtype);
    void region_setresource(struct region *r, const struct resource_type *rtype,
        int value);
    int owner_change(const region * r);
    bool is_mourning(const region * r, int in_turn);
    const struct item_type *r_luxury(struct region *r);
    void get_neighbours(const struct region *r, struct region **list);

    struct faction *update_owners(struct region *r);

#ifdef __cplusplus
}
#endif
#endif                          /* _REGION_H */
