/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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

#include "language.h"
#include <assert.h>


#define RF_CHAOTIC		(1<<0)
#define RF_MALLORN 		(1<<1)
#define RF_BLOCKED		(1<<2)

#define RF_BLOCK_NORTHWEST	(1<<3)
#define RF_BLOCK_NORTHEAST	(1<<4)
#define RF_BLOCK_EAST				(1<<5)
#define RF_BLOCK_SOUTHEAST	(1<<6)
#define RF_BLOCK_SOUTHWEST	(1<<7)
#define RF_BLOCK_WEST				(1<<8)

#define RF_ENCOUNTER				(1<<9)
#define RF_MIGRATION				(1<<10)
#define RF_UNUSED_1					(1<<11)
#define RF_ORCIFIED					(1<<12)
#define RF_DH								(1<<18)

#define RF_ALL 0xFFFFFF

#define RF_SAVEMASK (RF_CHAOTIC|RF_MALLORN|RF_BLOCKED|RF_BLOCK_NORTHWEST|RF_BLOCK_NORTHEAST|RF_BLOCK_EAST|RF_BLOCK_SOUTHEAST|RF_BLOCK_SOUTHWEST|RF_BLOCK_WEST|RF_ENCOUNTER|RF_ORCIFIED)
struct message;
struct message_list;
struct rawmaterial;

typedef struct land_region {
	char *name;
	/* TODO: demand kann nach Konvertierung entfernt werden. */
	struct demand {
		struct demand * next;
		const struct luxury_type * type;
		int value;
	} * demands;
	const struct herb_type * herbtype;
	short herbs;
#if GROWING_TREES
	int trees[3]; /* 0 -> Samen, 1 -> Sprößlinge, 2 -> Bäume */
#else
	int trees;
#endif
	int horses;
	int peasants;
	int newpeasants;
	int money;
#if NEW_RESOURCEGROWTH == 0
	int iron;
#endif
} land_region;

typedef struct region {
	struct region *next;
	struct land_region *land;
	struct unit *units;
	struct ship *ships;
	struct building *buildings;
	int x, y;
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
	struct region *nexthash;
	terrain_t terrain;
#ifdef WEATHER
	weather_t weathertype;
#endif
#if NEW_RESOURCEGROWTH
	struct rawmaterial * resources;
#endif
} region;

extern struct message_list * r_getmessages(const struct region * r, const struct faction * viewer);
extern struct message * r_addmessage(struct region * r, const struct faction * viewer, struct message * msg);

typedef struct {
	int  x;
	int  y;
	int  duration;
	char *desc;
	char *keyword;
} spec_direction;

typedef struct {
	direction_t dir;
} moveblock;

#define region_hashkey(r) (abs((r)->x + 0x100 * (r)->y))
int distance(const struct region*, const struct region*);
int koor_distance(int ax, int ay, int bx, int by) ;
direction_t reldirection(struct region * from, struct region * to);
direction_t koor_reldirection(int ax, int ay, int bx, int by) ;
extern struct region * findregion(int x, int y);

extern attrib_type at_direction;
extern attrib_type at_moveblock;
/* new: */
#if AT_SALARY
extern attrib_type at_salary;
#endif
extern attrib_type at_peasantluck;
extern attrib_type at_horseluck;
extern attrib_type at_chaoscount;
extern attrib_type at_woodcount;
extern attrib_type at_deathcount;
extern attrib_type at_travelunit;
extern attrib_type at_road;
extern attrib_type at_laen;

void initrhash(void);
void rhash(struct region * r);
void runhash(struct region * r);

typedef struct regionlist {
	struct regionlist *next;
	struct region     *region;
} regionlist;

void free_regionlist(regionlist *rl);
void add_regionlist(regionlist **rl, struct region *r);

int woodcount(const struct region * r);
int deathcount(const struct region * r);
int chaoscount(const struct region * r);

void woodcounts(struct region * r, int delta);
void deathcounts(struct region * r, int delta);
void chaoscounts(struct region * r, int delta);

void setluxuries(struct region * r, const struct luxury_type * sale);

int rroad(const struct region * r, direction_t d);
void rsetroad(struct region * r, direction_t d, int value);

int is_coastregion(struct region *r);

#if GROWING_TREES
int rtrees(const struct region * r, int ageclass);
int rsettrees(const struct region *r, int ageclass, int value);
#else
int rtrees(const struct region * r);
int rsettrees(const struct region * r, int value);
#endif

int rpeasants(const struct region * r);
void rsetpeasants(struct region * r, int value);
int rmoney(const struct region * r);
void rsetmoney(struct region * r, int value);
int rhorses(const struct region * r);
void rsethorses(const struct region * r, int value);

#define rbuildings(r) ((r)->buildings)

#if NEW_RESOURCEGROWTH == 0
#define riron(r) ((r)->land?(r)->land->iron:0)
#define rsetiron(r, value) ((r)->land?((r)->land->iron=(value)):(value),0)

int rlaen(const struct region * r);
void rsetlaen(struct region * r, int value);
#endif /* NEW_RESOURCEGROWTH */

#define rherbtype(r) ((r)->land?(r)->land->herbtype:0)
#define rsetherbtype(r, value) ((r)->land?((r)->land->herbtype=(value)):(value),0)

#define rherbs(r) ((r)->land?(r)->land->herbs:0)
#define rsetherbs(r, value) ((r)->land?((r)->land->herbs=(short)(value)):(value),0)

extern boolean r_isforest(const struct region * r);
extern boolean r_issea(const struct region * r);
extern boolean r_isglacier(const struct region * r);

#define rterrain(r) (terrain_t)((r)?(r)->terrain:T_FIREWALL)
#define rsetterrain(r, value) (r->terrain=value)

extern const char * rname(const struct region * r, const struct locale * lang);
#define rsetname(r, str) (set_string(&(r)->land->name, str))

#define rplane(r) getplane(r)

extern void r_setdemand(struct region * r, const struct luxury_type * ltype, int value);
extern int r_demand(const struct region * r, const struct luxury_type * ltype);

extern const char * regionname(const struct region * r, const struct faction * f);
extern void * resolve_region(void * data);
extern struct region * new_region(int x, int y);
extern void terraform(struct region * r, terrain_t terrain);

extern const int delta_x[MAXDIRECTIONS];
extern const int delta_y[MAXDIRECTIONS];
extern const direction_t back[MAXDIRECTIONS];

extern int production(const struct region *r);
extern int read_region_reference(struct region ** r, FILE * F);
extern void write_region_reference(const struct region * r, FILE * F);

extern struct unit * region_owner(const struct region * r);

#ifdef __cplusplus
}
#endif
#endif /* _REGION_H */
