/* vi: set ts=2:
 *
 *  
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef H_KRNL_REGION
#define H_KRNL_REGION
#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

/* FAST_CONNECT: regions are directly connected to neighbours, saves doing
   a hash-access each time a neighbour is needed */
#define FAST_CONNECT

#define RF_CHAOTIC     (1<<0)
#define RF_MALLORN     (1<<1)
#define RF_BLOCKED     (1<<2)

#define RF_BLOCK_NORTHWEST  (1<<3)
#define RF_BLOCK_NORTHEAST  (1<<4)
#define RF_BLOCK_EAST       (1<<5)
#define RF_BLOCK_SOUTHEAST  (1<<6)
#define RF_BLOCK_SOUTHWEST  (1<<7)
#define RF_BLOCK_WEST       (1<<8)

#define RF_ENCOUNTER   (1<<9)
#define RF_MIGRATION   (1<<10)
#define RF_UNUSED_1    (1<<11)
#define RF_ORCIFIED    (1<<12)
#define RF_CURSED      (1<<13)

  /* debug flags */
#define RF_COMBATDEBUG (1<<14)
#define RF_MAPPER_HIGHLIGHT (1<<14) /* only used by mapper, not stored */
#define RF_LIGHTHOUSE (1<<15) /* this region may contain a lighthouse */

#define RF_SELECT      (1<<17)
#define RF_MARK        (1<<18)

/* flags that speed up attribute access: */
#define RF_TRAVELUNIT    (1<<19)
#define RF_GUARDED       (1<<20)

#define RF_ALL 0xFFFFFF

#define RF_SAVEMASK (RF_CHAOTIC|RF_MALLORN|RF_BLOCKED|RF_BLOCK_NORTHWEST|RF_BLOCK_NORTHEAST|RF_BLOCK_EAST|RF_BLOCK_SOUTHEAST|RF_BLOCK_SOUTHWEST|RF_BLOCK_WEST|RF_ENCOUNTER|RF_ORCIFIED)
struct message;
struct message_list;
struct rawmaterial;
struct donation;
struct item;

typedef struct land_region {
  char *name;
  /* TODO: demand kann nach Konvertierung entfernt werden. */
  struct demand {
    struct demand * next;
    const struct luxury_type * type;
    int value;
  } * demands;
  const struct item_type * herbtype;
  short herbs;
  int trees[3]; /* 0 -> seeds, 1 -> shoots, 2 -> trees */
  int horses;
  int peasants;
  int newpeasants;
  int money;
  struct item * items; /* items that can be claimed */
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
  unsigned int uid; /* a unique id */
  short x, y;
  struct plane *planep;
  char *display;
  unsigned int flags;
  unsigned short age;
  struct message_list *msgs;
  struct individual_message {
    struct individual_message * next;
    const struct faction * viewer;
    struct message_list *msgs;
  } * individual_messages;
  struct attrib *attribs;
  struct donation * donations;
  const struct terrain_type * terrain;
  struct rawmaterial * resources;
#ifdef REGIONOWNERS
  struct faction * owner;
#endif
#ifdef FAST_CONNECT
  struct region * connect[MAXDIRECTIONS]; /* use rconnect(r, dir) to access */
#endif
} region;

typedef struct region_list {
  struct region_list * next;
  struct region * data;
} region_list;

struct message_list * r_getmessages(const struct region * r, const struct faction * viewer);
struct message * r_addmessage(struct region * r, const struct faction * viewer, struct message * msg);

typedef struct spec_direction {
  short x, y;
  int  duration;
  boolean active;
  char *desc;
  char *keyword;
} spec_direction;

typedef struct {
  direction_t dir;
} moveblock;

#define reg_hashkey(r) (r->index)

int distance(const struct region*, const struct region*);
int koor_distance(int ax, int ay, int bx, int by) ;
direction_t reldirection(const struct region * from, const struct region * to);
struct region * findregion(short x, short y);
struct region * findregionbyid(unsigned int uid);

extern struct attrib_type at_direction;
extern struct attrib_type at_moveblock;
extern struct attrib_type at_peasantluck;
extern struct attrib_type at_horseluck;
extern struct attrib_type at_chaoscount;
extern struct attrib_type at_woodcount;
extern struct attrib_type at_deathcount;
extern struct attrib_type at_travelunit;

void initrhash(void);
void rhash(struct region * r);
void runhash(struct region * r);

void free_regionlist(region_list *rl);
void add_regionlist(region_list **rl, struct region *r);

struct region * find_special_direction(const struct region *r, const char *token, const struct locale * lang);
void register_special_direction(const char * name);
struct spec_direction * special_direction(const region * from, const region * to);
struct attrib *create_special_direction(struct region *r, struct region *rt,
                                               int duration, const char *desc,
                                               const char *keyword);

int deathcount(const struct region * r);
int chaoscount(const struct region * r);

void deathcounts(struct region * r, int delta);
void chaoscounts(struct region * r, int delta);

void setluxuries(struct region * r, const struct luxury_type * sale);
int get_maxluxuries(void);

short rroad(const struct region * r, direction_t d);
void rsetroad(struct region * r, direction_t d, short value);

int is_coastregion(struct region *r);

int rtrees(const struct region * r, int ageclass);
int rsettrees(const struct region *r, int ageclass, int value);

int rpeasants(const struct region * r);
void rsetpeasants(struct region * r, int value);
int rmoney(const struct region * r);
void rsetmoney(struct region * r, int value);
int rhorses(const struct region * r);
void rsethorses(const struct region * r, int value);

#define rbuildings(r) ((r)->buildings)

#define rherbtype(r) ((r)->land?(r)->land->herbtype:0)
#define rsetherbtype(r, value) if ((r)->land) (r)->land->herbtype=(value)

#define rherbs(r) ((r)->land?(r)->land->herbs:0)
#define rsetherbs(r, value) if ((r)->land) ((r)->land->herbs=(short)(value))

boolean r_isforest(const struct region * r);

#define rterrain(r) (oldterrain((r)->terrain))
#define rsetterrain(r, t) ((r)->terrain = newterrain(t))

const char * rname(const struct region * r, const struct locale * lang);

#define rplane(r) getplane(r)

void r_setdemand(struct region * r, const struct luxury_type * ltype, int value);
int r_demand(const struct region * r, const struct luxury_type * ltype);

const char * write_regionname(const struct region * r, const struct faction * f, char * buffer, size_t size);

struct region * new_region(short x, short y, unsigned int uid);
void remove_region(region ** rlist, region * r);
void terraform_region(struct region * r, const struct terrain_type * terrain);

extern const short delta_x[MAXDIRECTIONS];
extern const short delta_y[MAXDIRECTIONS];
direction_t dir_invert(direction_t dir);
int production(const struct region *r);

void region_setowner(struct region * r, struct faction * owner);
struct faction * region_owner(const struct region * r);

struct region * r_connect(const struct region *, direction_t dir);
#ifdef FAST_CONNECT
# define rconnect(r, dir) ((r)->connect[dir]?(r)->connect[dir]:r_connect(r, dir))
#else
# define rconnect(r, dir) r_connect(r, dir)
#endif

void free_regions(void);

void write_region_reference(const struct region * r, struct storage * store);
variant read_region_reference(struct storage * store);
int resolve_region_coor(variant id, void * address);
int resolve_region_id(variant id, void * address);
#define RESOLVE_REGION(version) ((version<UIDHASH_VERSION)?resolve_region_coor:resolve_region_id)

const char * regionname(const struct region * r, const struct faction * f);

const char * region_getname(const struct region * self);
void region_setname(struct region * self, const char * name);
int region_getresource(const struct region * r, const struct resource_type * rtype);
void region_setresource(struct region * r, const struct resource_type * rtype, int value);

#ifdef __cplusplus
}
#endif
#endif /* _REGION_H */
