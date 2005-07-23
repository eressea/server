/* vi: set ts=2:
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

#ifndef H_KRNL_BUILDING
#define H_KRNL_BUILDING

#include <util/variant.h>
#ifdef __cplusplus
extern "C" {
#endif

/* maintenance::flags */
#define MTF_NONE     0x00
#define MTF_VARIABLE 0x01	/* resource usage scales with size */
#define MTF_VITAL    0x02	/* if resource missing, building may crash */

typedef struct maintenance {
  const struct resource_type * rtype; /* type of resource required */
  int number;         /* amount of resources */
  unsigned int flags; /* misc. flags */
} maintenance;

/* building_type::flags */
#define BTF_NONE           0x00
#define BTF_INDESTRUCTIBLE 0x01 /* cannot be torm down */
#define BTF_NOBUILD        0x02 /* special, can't be built */
#define BTF_UNIQUE         0x04 /* only one per struct region (harbour) */
#define BTF_DECAY          0x08 /* decays when not occupied */
#define BTF_DYNAMIC        0x10 /* dynamic type, needs bt_write */
#define BTF_PROTECTION     0x20 /* protection in combat */
#define BTF_MAGIC          0x40 /* magical effect */

typedef struct building_type {
	const char * _name;

	int flags;  /* flags */
	int capacity; /* Kapazität pro Größenpunkt */
	int maxcapacity;  /* Max. Kapazität */
	int maxsize;      /* how big can it get, with all the extensions? */
	int magres;       /* how well it resists against spells */
	int magresbonus;  /* bonus it gives the target against spells */
	int fumblebonus;  /* bonus that reduces fumbling */
	double auraregen; /* modifier for aura regeneration inside building */
	struct maintenance * maintenance; /* array of requirements */
	struct construction * construction; /* construction of 1 building-level */

	const char * (*name)(int size);
	void (*init)(struct building_type*);
	struct attrib * attribs;
} building_type;

extern const building_type * bt_find(const char* name);
extern void register_buildings(void);
extern void bt_register(building_type * type);

typedef struct building_typelist {
	struct building_typelist * next;
	const building_type * type;
} building_typelist;

extern struct building_typelist *buildingtypes;
/* buildingt => building_type
 * Name => locale_string(name)
 * MaxGroesse => levels
 * MinBauTalent => construction->minskill
 * Kapazitaet => capacity, maxcapacity
 * Materialien => construction->materials
 * UnterSilber, UnterSpezialTyp, UnterSpezial => maintenance
 * per_size => !maintenance->fixed
 */
#define BFL_NONE           0x00
#define BLD_MAINTAINED     0x01 /* vital maintenance paid for */
#define BLD_WORKING        0x02 /* full maintenance paid, it works */
#define BLD_UNGUARDED      0x04 /* you can enter this building anytime */

#define BLD_SAVEMASK       0x00 /* mask for persistent flags */

typedef struct building {
	struct building *next;
	struct building *nexthash;

	const struct building_type * type;
	struct region *region;
	char *name;
	char *display;
	struct attrib * attribs;
	int no;
	int size;
	int sizeleft;	/* is only used during battle. should be a temporary attribute */
	int besieged;	/* should be an attribute */
	unsigned int flags;
} building;

typedef struct building_list {
  struct building_list * next;
  building * data;
} building_list;

extern void free_buildinglist(building_list *bl);
extern void add_buildinglist(building_list **bl, struct building *b);

extern struct attrib_type at_building_generic_type;
extern const char * buildingtype(const building_type * btype, const struct building * b, int bsize);
extern const char * buildingname(const struct building * b);
extern int buildingcapacity(const struct building * b);
extern struct building *new_building(const struct building_type * typ, struct region * r, const struct locale * lang);
void build_building(struct unit * u, const struct building_type * typ, int size, struct order * ord);

/* Alte Gebäudetypen: */

/* old functions, still in build.c: */
int buildingeffsize(const building * b, boolean img);
void bhash(struct building * b);
void bunhash(struct building * b);
int buildingcapacity(const struct building * b);
void destroy_building(struct building * b);

const struct building_type * findbuildingtype(const char * name, const struct locale * lang);

extern struct building_type * bt_make(const char * name, int flags, int capacity, int maxcapacity, int maxsize);

#include "build.h"
#define NOBUILDING NULL

extern void * resolve_building(variant data);
extern void write_building_reference(const struct building * b, FILE * F);
extern int read_building_reference(struct building ** b, FILE * F);

extern struct building *findbuilding(int n);

extern struct unit * buildingowner(const struct region * r, const struct building * b);

extern attrib_type at_nodestroy;
extern attrib_type at_building_action;

typedef struct building_action {
  building * b;
  char * fname;
  char * param;
} building_action;

#ifdef __cplusplus
}
#endif
#endif
