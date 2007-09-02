/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef H_KRNL_UNIT_H
#define H_KRNL_UNIT_H

#include <util/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

struct skill;
struct item;
#define UFL_DEBUG         (1<<0)
#define UFL_ISNEW         (1<<1)	/* 2 */
#define UFL_LONGACTION    (1<<2)	/* 4 */
#define UFL_OWNER         (1<<3)	/* 8 */
#define UFL_PARTEITARNUNG (1<<4)	/* 16 */
#define UFL_DISBELIEVES   (1<<5)	/* 32 */
#define UFL_WARMTH        (1<<6)	/* 64 */
#define UFL_HERO          (1<<7)
#define UFL_MOVED         (1<<8)
#define UFL_NOTMOVING     (1<<9)  /* Die Einheit kann sich wg. langen Kampfes nicht bewegen */
#define UFL_HUNGER        (1<<11) /* kann im Folgemonat keinen langen Befehl außer ARBEITE ausführen */
#define UFL_SIEGE         (1<<12) /* speedup: belagert eine burg, siehe attribut */
#define UFL_TARGET        (1<<13) /* speedup: hat ein target, siehe attribut */
#define UFL_WERE          (1<<14)
#define UFL_ENTER         (1<<15) /* unit has entered a ship/building and will not leave it */

/* warning: von 512/1024 gewechslet, wegen konflikt mit NEW_FOLLOW */
#define UFL_LOCKED        (1<<16) /* Einheit kann keine Personen aufnehmen oder weggeben, nicht rekrutieren. */
#define UFL_STORM         (1<<19) /* Kapitän war in einem Sturm */
#define UFL_FOLLOWING     (1<<20)
#define UFL_FOLLOWED      (1<<21)

#define UFL_NOAID         (1<<22) /* Einheit hat Noaid-Status */
#define UFL_MARK          (1<<23) /* same as FL_MARK */
#define UFL_ORDERS        (1<<24) /* Einheit hat Befehle erhalten */
#define UFL_TAKEALL       (1<<25) /* Einheit nimmt alle Gegenstände an */

/* flags that speed up attribute access: */
#define UFL_STEALTH       (1<<26)
#define UFL_GUARD         (1<<27)
#define UFL_GROUP         (1<<28)

/* Flags, die gespeichert werden sollen: */
#ifndef HEROES
# define UFL_SAVEMASK (UFL_MOVED | UFL_NOAID | UFL_OWNER | UFL_PARTEITARNUNG | UFL_LOCKED | UFL_HUNGER | UFL_TAKEALL | UFL_GUARD | UFL_STEALTH | UFL_GROUP )
#else
# define UFL_SAVEMASK (UFL_MOVED | UFL_NOAID | UFL_OWNER | UFL_PARTEITARNUNG | UFL_LOCKED | UFL_HUNGER | UFL_TAKEALL | UFL_GUARD | UFL_STEALTH | UFL_GROUP | UFL_HERO)
#endif

#define UNIT_MAXSIZE 50000
#ifdef HEROES
extern int maxheroes(const struct faction * f);
extern int countheroes(const struct faction * f);
#endif

typedef struct unit {
	struct unit *next; /* needs to be first entry, for region's unitlist */
	struct unit *nexthash;
	struct unit *nextF; /* nächste Einheit der Partei */
	struct region *region;
  int no;
  int hp;
	char *name;
	char *display;
	struct faction *faction;
	struct building *building;
	struct ship *ship;
  unsigned short number;
  short age;

	/* skill data */
	short skill_size;
	struct skill *skills;
	struct item * items;
	struct reservation {
		struct reservation * next;
		const struct resource_type * type;
		int value;
	} * reservations;

	/* orders */
	struct order * orders;
	struct order * thisorder;
#ifdef LASTORDER
	struct order * lastorder;
#else
  struct order * old_orders;
#endif

	/* race and illusionary race */
	const struct race * race;
	const struct race * irace;

	unsigned int flags;
	struct attrib * attribs;
	status_t status;
	int n; /* enno: attribut? */
	int wants; /* enno: attribut? */
} unit;

typedef struct unit_list {
  struct unit_list * next;
  struct unit * data;
} unit_list;

extern void unitlist_clear(struct unit_list **ul);
extern void unitlist_insert(struct unit_list **ul, struct unit *u);

extern struct attrib_type at_alias;
extern struct attrib_type at_siege;
extern struct attrib_type at_target;
extern struct attrib_type at_potionuser;
extern struct attrib_type at_contact;
extern struct attrib_type at_effect;
extern struct attrib_type at_private;
extern struct attrib_type at_showskchange;

int ualias(const struct unit * u);

extern struct attrib_type at_stealth;

void u_seteffstealth(struct unit * u, int value);
int u_geteffstealth(const struct unit * u);

struct building * usiege(const struct unit * u);
void usetsiege(struct unit * u, const struct building * b);

struct unit * utarget(const struct unit * u);
void usettarget(struct unit * u, const struct unit * b);

struct unit * utarget(const struct unit * u);
void usettarget(struct unit * u, const struct unit * b);

extern const struct race * urace(const struct unit * u);

const char * uprivate(const struct unit * u);
void usetprivate(struct unit * u, const char * c);

const struct potion_type * ugetpotionuse(const struct unit * u); /* benutzt u einein trank? */
void usetpotionuse(struct unit * u, const struct potion_type * p); /* u benutzt trank p (es darf halt nur einer pro runde) */

boolean ucontact(const struct unit * u, const struct unit * u2);
void usetcontact(struct unit * u, const struct unit * c);

struct unit * findnewunit (const struct region * r, const struct faction *f, int alias);

extern struct unit * udestroy;
extern const char * u_description(const unit * u, const struct locale * lang);
extern struct skill * add_skill(struct unit * u, skill_t id);
extern void remove_skill(struct unit *u, skill_t sk);
extern struct skill * get_skill(const struct unit * u, skill_t id);
extern boolean has_skill(const unit* u, skill_t sk);

extern void set_level(struct unit * u, skill_t id, int level);
extern int get_level(const struct unit * u, skill_t id);
extern void transfermen(struct unit * u, struct unit * u2, int n);

extern int eff_skill(const struct unit * u, skill_t sk, const struct region * r);
extern int eff_skill_study(const struct unit * u, skill_t sk, const struct region * r);

extern int get_modifier(const struct unit * u, skill_t sk, int lvl, const struct region * r, boolean noitem);

/* Einheiten werden nicht wirklich zerstört. */
extern void destroy_unit(struct unit * u);
extern void remove_unit(struct unit * u);
extern void distribute_items(struct unit * u);

/* see resolve.h */
extern void * resolve_unit(variant data);
extern void write_unit_reference(const struct unit * u, FILE * F);
extern int read_unit_reference(unit ** up, FILE * F);

extern void leave(struct region * r, struct unit * u);
extern void leave_ship(unit * u);
extern void leave_building(unit * u);

extern void set_leftship(struct unit *u, struct ship *sh);
extern struct ship * leftship(const struct unit *);
extern boolean can_survive(const struct unit *u, const struct region *r);
extern void move_unit(struct unit * u, struct region * target, struct unit ** ulist);

extern struct building * inside_building(const struct unit * u);

/* cleanup code for this module */
extern void free_units(void);
extern struct faction * dfindhash(int no);
extern void u_setfaction(struct unit * u, struct faction * f);
extern void set_number(struct unit * u, int count);

extern boolean learn_skill(struct unit * u, skill_t sk, double chance);

extern int invisible(const struct unit *target, const struct unit * viewer);
extern void stripunit(struct unit * u);

extern void name_unit(struct unit *u);
extern struct unit * create_unit(struct region * r1, struct faction * f, int number, const struct race * rc, int id, const char * dname, struct unit *creator);

extern struct attrib_type at_creator;
#ifdef __cplusplus
}
#endif
#endif
