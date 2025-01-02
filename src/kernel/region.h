#ifndef H_KRNL_REGION
#define H_KRNL_REGION

#include "../util/resolve.h"

#include "direction.h"

#include <stddef.h>
#include <stdbool.h>

#define TREESIZE 8             /* space used by trees (in #peasants) */
#define MAXTREES 100000000     /* bug 2360: some players are crazy */
#define MAXLUXURIES 16         /* there must be no more than MAXLUXURIES kinds of luxury goods in any game */
#define MAXREGIONS 524287      /* must be prime for hashing. 262139 was a little small */

#define RF_CHAOTIC     (1<<0) /* persistent */
#define RF_MALLORN     (1<<1) /* persistent */
#define RF_BLOCKED     (1<<2) /* persistent */

#define RF_OBSERVER  (1<<3) /* persistent */
#define RF_UNUSED_4  (1<<4)
#define RF_UNUSED_5  (1<<5)
#define RF_UNUSED_6  (1<<6)
#define RF_UNUSED_7  (1<<7)
#define RF_UNUSED_8  (1<<8)

#define RF_MAPPER_HIGHLIGHT (1<<10)
#define RF_LIGHTHOUSE  (1<<11) /* this region may contain a lighthouse */
#define RF_MIGRATION   (1<<13)

#define RF_UNUSED_14 (1<<14)
#define RF_UNUSED_15   (1<<15)
#define RF_UNUSED_16   (1<<16)

#define RF_SELECT      (1<<17)
#define RF_MARK        (1<<18)
#define RF_TRAVELUNIT    (1<<19)
#define RF_GUARDED       (1<<20) /* persistent */

#define RF_ALL 0xFFFFFF

#define RF_SAVEMASK (RF_CHAOTIC|RF_MALLORN|RF_BLOCKED|RF_GUARDED|RF_LIGHTHOUSE)

#define MORALE_TAX_FACTOR 200 /* 0.5% tax per point of morale, 1 silver per 200 */
#define MORALE_MAX 10           /* Maximum morale allowed */
#define MORALE_DEFAULT 1        /* Morale of peasants when they are conquered for the first time */
#define MORALE_TAKEOVER 0       /* Morale of peasants after they lose their lord */
#define MORALE_COOLDOWN 2       /* minimum cooldown before a morale change occurs */
#define MORALE_AVERAGE 6        /* default average time for morale to change */
#define MORALE_TRANSFER 2       /* points of morale lost when GIVE COMMAND */

typedef struct region_owner {
    struct faction *owner;
    struct faction *last_owner;
    int since_turn;             /* turn the region changed owners */
    int morale_turn;            /* turn when morale has changed most recently */
    int flags;
} region_owner;

typedef struct demand {
    const struct luxury_type *type;
    int value;
} demand;

typedef struct land_region {
    char *name;
    char *display;
    demand *_demands;
    const struct item_type *herbtype;
    unsigned short horses;
    unsigned short herbs;
    unsigned short peasants;
    unsigned short morale;
    short newpeasants;
    int trees[3];               /* 0 -> seeds, 1 -> shoots, 2 -> trees */
    int money;
    struct region_owner *ownership;
    int roads[MAXDIRECTIONS];
} land_region;

/* seen_mode: visibility in the report */
typedef enum seen_mode {
    seen_none,
    seen_neighbour,
    seen_lighthouse_land,
    seen_lighthouse,
    seen_travel,
    seen_unit,
    seen_spell,
    seen_battle
} seen_mode;

typedef struct faction_messages {
    const struct faction *viewer;
    struct message_list *msgs;
} faction_messages;

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
    int flags;
    unsigned short age;
    struct message_list *msgs;
    struct faction_messages *individual_messages;
    struct attrib *attribs;
    const struct terrain_type *terrain;
    struct rawmaterial *resources;
    struct region *connect[MAXDIRECTIONS];      /* use rconnect(r, dir) to access */
    struct {
        seen_mode mode;
    } seen;
} region;

typedef struct region_list {
    struct region_list *next;
    struct region *data;
} region_list;

typedef struct {
    enum direction_t dir;
} moveblock;

struct message;
struct message_list;
struct resource_type;
struct item_type;
struct faction;
struct gamedata;

extern struct region* regions;
extern struct attrib_type at_moveblock;
extern struct attrib_type at_peasantluck;
extern struct attrib_type at_horseluck;
extern struct attrib_type at_woodcount;
extern struct attrib_type at_deathcount;
extern const int delta_x[MAXDIRECTIONS];
extern const int delta_y[MAXDIRECTIONS];


#define reg_hashkey(r) (r->index)

struct message_list* r_getmessages(const struct region* r,
    const struct faction* viewer);
struct message* r_addmessage(struct region* r, const struct faction* viewer,
    struct message* msg);
void r_add_warning(struct region *r, struct message *msg);

int fix_demand(struct region *r);
int distance(const struct region *, const struct region *);
int koor_distance(int ax, int ay, int bx, int by);
struct region *findregion(int x, int y);
struct region *findregionbyid(int uid);

void rhash(struct region *r);
void runhash(struct region *r);

void free_regionlist(region_list * rl);
void add_regionlist(region_list ** rl, struct region *r);

int deathcount(const struct region *r);
void deathcounts(struct region *r, int delta);

void setluxuries(struct region *r, const struct luxury_type *sale);
int get_maxluxuries(void);

int rroad(const struct region *r, direction_t d);
void rsetroad(struct region *r, direction_t d, int value);

bool is_coastregion(struct region *r);

int rtrees(const struct region *r, int ageclass);
enum {
    TREE_SEED = 0,
    TREE_SAPLING = 1,
    TREE_TREE = 2
};

int rsettrees(const struct region *r, int ageclass, int value);

int rpeasants(const struct region *r);
int rsetpeasants(struct region *r, int value);
int rmoney(const struct region *r);
void rsetmoney(struct region *r, int value);
int rhorses(const struct region *r);
void rsethorses(const struct region *r, int value);

int rherbs(const struct region *r);
void rsetherbs(struct region *r, int value);
void rsetherbtype(struct region *r, const struct item_type *itype);

#define rbuildings(r) ((r)->buildings)
#define rherbtype(r) ((r)->land?(r)->land->herbtype:NULL)
#define rterrain(r) (oldterrain((r)->terrain))
#define rsetterrain(r, t) ((r)->terrain = newterrain(t))

const char *rname(const struct region *r, const struct locale *lang);

#define rplane(r) getplane(r)

void r_setdemand(struct region *r, const struct luxury_type *ltype,
    int value);
int r_demand(const struct region *r, const struct luxury_type *ltype);
void r_foreach_demand(const struct region *r, void (*callback)(struct demand *, int, void *), void *data);
bool r_has_demands(const struct region *r);
bool r_isforest(const struct region *r);

const char *write_regionname(const struct region *r, const struct faction *f,
    char *buffer, size_t size);

struct region *region_create(int uid);
void add_region(region *r, int x, int y);
struct region *new_region(int x, int y, struct plane *pl, int uid);
void remove_region(region ** rlist, region * r);
void terraform_region(struct region *r, const struct terrain_type *terrain);
void init_region(struct region *r);
void pnormalize(int *x, int *y, const struct plane *pl);
void destroy_all_roads(struct region * r);

direction_t dir_invert(direction_t dir);
int max_production(const struct region *r);

void region_set_owner(struct region *r, struct faction *owner, int turn);
struct faction *region_get_owner(const struct region *r);
struct alliance *region_get_alliance(const struct region *r);

struct region *rconnect(const struct region *, direction_t dir);

void free_regions(void);
void free_land(struct land_region * lr);

int region_get_morale(const region * r);
void region_set_morale(region * r, int morale, int turn);

#define RESOLVE_REGION (TYP_REGION << 24)
void resolve_region(region *r);
void write_region_reference(const struct region *r, struct storage *store);
int read_region_reference(struct gamedata *data, region **rp);

const char *regionname(const struct region *r, const struct faction *f);

int region_production(const struct region *r);
int region_maxworkers(const struct region* r, int size);
const char *region_getname(const struct region *self);
void region_setname(struct region *self, const char *name);
const char *region_getinfo(const struct region *self);
void region_setinfo(struct region *self, const char *name);
int region_getresource_level(const struct region * r,
    const struct resource_type * rtype);
int region_getresource(const struct region *r,
    const struct resource_type *rtype);
void region_setresource(struct region *r, const struct resource_type *rtype,
    int value);
int owner_change(const region * r);
bool is_mourning(const region * r, int in_turn);
const struct item_type *r_luxury(const struct region *r);
void get_neighbours(const struct region *r, struct region **list);

struct faction *update_region_owners(struct region *r);
void reorder_units(struct region* r);

#endif                          /* _REGION_H */
