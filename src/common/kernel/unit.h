/* vi: set ts=2:
 *
 *	$Id: unit.h,v 1.2 2001/01/26 16:19:40 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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

#ifndef UNIT_H
#define UNIT_H

#ifdef NEW_UNITS
typedef struct party {
	struct party *next;
	struct party *nexthash;
	struct party *nextF; /* nächste Einheit der Partei */
	struct party *prevF; /* letzte Einheit der Partei */
	int no;
	char *name;
	char *display;
	short age;
	struct region *region;
	struct faction *faction;
	struct building *building;
	struct ship *ship;
	struct strlist *orders;
	struct strlist *botschaften;
	int n;
	int wants;
	race_t race;
	race_t irace;
	char *thisorder;
	char *lastorder;
	struct item * items;
	struct reservation {
		struct reservation * next;
		const struct resource_type * type;
		int value;
	} * reservations;
	struct unit *units; /* new */
	struct attrib * attribs;
} party;

typedef struct unit {
	struct unit *next; /* new */
	struct party *party;
	int no;
	int number;
	int hp;
	int skill_size;
	struct skillvalue *skills;
	struct strlist *orders; /* new */
	struct attrib *attribs; /* new */
	unsigned int flags;
	char effstealth;
	status_t status;
} struct unit;

#else

typedef struct unit {
	struct unit *next;
	struct unit *nexthash;
	struct unit *nextF; /* nächste Einheit der Partei */
	struct unit *prevF; /* letzte Einheit der Partei */
	struct region *region;
	int no;
	char *name;
	char *display;
	int number;
	int hp;
	short age;
	struct faction *faction;
	struct building *building;
	struct ship *ship;
	char *thisorder;
	char *lastorder;
	int skill_size;
	struct skillvalue *skills;
	struct item * items;
	struct reservation {
		struct reservation * next;
		const struct resource_type * type;
		int value;
	} * reservations;
	struct strlist *orders;
	struct strlist *botschaften;
	unsigned int flags;
	struct attrib * attribs;
#ifndef NDEBUG
	int debug_number;			/* Sollte immer == number sein. Wenn nicht, ist
								 * * nicht mit createunit und set_number
								 * * gearbeitet worden. */
#endif
	status_t status;
	race_t race;
	race_t irace;
	int n;	/* enno: attribut? */
	int wants;	/* enno: attribut? */
} unit;
#endif

extern attrib_type at_alias;
extern attrib_type at_siege;
extern attrib_type at_target;
extern attrib_type at_potion;
extern attrib_type at_potionuser;
extern attrib_type at_contact;
extern attrib_type at_effect;
extern attrib_type at_private;

int ualias(const struct unit * u);

extern attrib_type at_stealth;

void u_seteffstealth(struct unit * u, int value);
int u_geteffstealth(const struct unit * u);

struct building * usiege(const struct unit * u);
void usetsiege(struct unit * u, const struct building * b);

struct unit * utarget(const struct unit * u);
void usettarget(struct unit * u, const struct unit * b);

struct unit * utarget(const struct unit * u);
void usettarget(struct unit * u, const struct unit * b);

const char* uprivate(const struct unit * u);
void usetprivate(struct unit * u, const char * c);

const struct potion_type * ugetpotionuse(const struct unit * u); /* benutzt u einein trank? */
void usetpotionuse(struct unit * u, const struct potion_type * p); /* u benutzt trank p (es darf halt nur einer pro runde) */

boolean ucontact(const struct unit * u, const struct unit * u2);
void usetcontact(struct unit * u, const struct unit * c);

struct unit * findnewunit (struct region * r, struct faction *f, int alias);

#define upotions(u) fval(u, FL_POTIONS)
extern const struct unit u_peasants;

int change_skill(struct unit * u, skill_t id, int byvalue);
void set_skill(struct unit * u, skill_t id, int value);
int get_skill(const struct unit * u, skill_t id);
void transfermen(struct unit * u, struct unit * u2, int n);

#undef DESTROY

/* Einheiten werden nicht wirklich zerstört. */
extern void destroy_unit(struct unit * u);

/* see resolve.h */
extern void * resolve_unit(void * data);
extern void write_unit_reference(const unit * u, FILE * F);
extern void read_unit_reference(unit ** up, FILE * F);

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
extern struct faction * dfindhash(int i);
#endif
