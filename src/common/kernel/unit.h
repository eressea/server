/* vi: set ts=2:
 *
 *	
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

struct skill;

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
	struct skill *skills;
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
	const struct race * race;
	const struct race * irace;
	int n;	/* enno: attribut? */
	int wants;	/* enno: attribut? */
} unit;

extern attrib_type at_alias;
extern attrib_type at_siege;
extern attrib_type at_target;
extern attrib_type at_potion;
extern attrib_type at_potionuser;
extern attrib_type at_contact;
extern attrib_type at_effect;
extern attrib_type at_private;
extern attrib_type at_showskchange;

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

extern const struct race * urace(const struct unit * u);

const char* uprivate(const struct unit * u);
void usetprivate(struct unit * u, const char * c);

const struct potion_type * ugetpotionuse(const struct unit * u); /* benutzt u einein trank? */
void usetpotionuse(struct unit * u, const struct potion_type * p); /* u benutzt trank p (es darf halt nur einer pro runde) */

boolean ucontact(const struct unit * u, const struct unit * u2);
void usetcontact(struct unit * u, const struct unit * c);

struct unit * findnewunit (const struct region * r, const struct faction *f, int alias);

#define upotions(u) fval(u, FL_POTIONS)
extern const struct unit u_peasants;
extern const struct unit u_unknown;

extern struct unit * udestroy;

extern struct skill * add_skill(struct unit * u, skill_t id);
extern void remove_skill(struct unit *u, skill_t sk);
extern struct skill * get_skill(const struct unit * u, skill_t id);
extern boolean has_skill(const unit* u, skill_t sk);

extern void set_level(struct unit * u, skill_t id, int level);
extern int get_level(const struct unit * u, skill_t id);
extern void transfermen(struct unit * u, struct unit * u2, int n);

extern int eff_skill(const struct unit * u, skill_t sk, const struct region * r);
extern int get_modifier(const struct unit * u, skill_t sk, int lvl, const struct region * r);

#undef DESTROY

/* Einheiten werden nicht wirklich zerstört. */
extern void destroy_unit(struct unit * u);

/* see resolve.h */
extern void * resolve_unit(void * data);
extern void write_unit_reference(const unit * u, FILE * F);
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
/* vorsicht Sprüche können u->number == 0 (RS_FARVISION) haben! */
extern void set_number(struct unit * u, int count);

#if !SKILLPOINTS
extern boolean learn_skill(struct unit * u, skill_t sk, double chance);
#endif

#endif
