/* vi: set ts=2:
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

#ifndef BUILD_H
#define BUILD_H

/* Die enums fuer Gebauede werden nie gebraucht, nur bei der Bestimmung
 * des Schutzes durch eine Burg wird die Reihenfolge und MAXBUILDINGS
 * wichtig
 */

#ifndef NEW_BUILDINGS

enum {
	B_SITE,
	B_FORTIFICATION,
	B_TOWER,
	B_CASTLE,
	B_FORTRESS,
	B_CITADEL,
	MAXBUILDINGS
};

enum {
	BT_BURG,
	BT_LEUCHTTURM,
	BT_BERGWERK,
	BT_STEINBRUCH,
	BT_HAFEN,
	BT_UNIVERSITAET,
	BT_MAGIERTURM,
	BT_SCHMIEDE,
	BT_SAEGEWERK,
	BT_PFERDEZUCHT,
	BT_MONUMENT,
	BT_DAMM,
	BT_KARAWANSEREI,
	BT_TUNNEL,
	BT_TAVERNE,
	BT_STONECIRCLE,
	BT_BLESSEDSTONECIRCLE,
	BT_ICASTLE,
	MAXBUILDINGTYPES,
	NOBUILDING = -1
};
#endif


extern char *buildingnames[MAXBUILDINGS];

void destroy(struct region * r, struct unit * u, const char * cmd);

boolean can_contact(struct region *r, struct unit *u, struct unit *u2);

void do_siege(void);
void build_road(struct region * r, struct unit * u, int size, direction_t d);
void create_ship(struct region * r, struct unit * u, const struct ship_type * newtype, int size);
void continue_ship(struct region * r, struct unit * u, int size);

struct building * getbuilding(const struct region * r);
ship *getship(const struct region * r);

void remove_contacts(void);
void do_leave(void);
void do_misc(char try);

void reportevent(struct region * r, char *s);

void shash(ship * sh);
void sunhash(ship * sh);

void destroy_ship(ship * s, struct region * r);

/* ** ** ** ** ** ** *
 *  new build rules  *
 * ** ** ** ** ** ** */

typedef struct requirement {
#ifdef NO_OLD_ITEMS
	resource_type * rtype;
#else
	resource_t type;
#endif
	int number;
	double recycle; /* recycling quota */
} requirement;

typedef struct construction {
	skill_t skill; /* skill req'd per point of size */
	int minskill;  /* skill req'd per point of size */

	int maxsize;   /* maximum size of this type */
	int reqsize;   /* size of object using up 1 set of requirement. */
	requirement * materials; /* material req'd to build one object */

	const struct construction * improvement;
		/* next level, if upgradable. if more than one of these items
		 * can be built (weapons, armout) per turn, must not be NULL,
		 * but point to the same type again:
		 *   const_sword.improvement = &const_sword
		 * last level of a building points to NULL, as do objects of
		 * an unlimited size.
		 */
	attrib * attribs;
		/* stores skill modifiers and other attributes */
} construction;

int build(struct unit * u, const construction * ctype, int completed, int want);
int maxbuild(const struct unit *u, const construction *cons);

/** error messages that build may return: */
#define ELOWSKILL -1
#define ENEEDSKILL -2
#define ECOMPLETE -3
#define ENOMATERIALS -4

#endif

