/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */
#ifndef FACTION_H
#define FACTION_H

struct player;

typedef struct faction {
	struct faction *next;
	struct faction *nexthash;

	struct player *owner;
	struct region *first;
	struct region *last;
	int no;
	int unique_id;
	unsigned int flags;
	char *name;
	char *banner;
	char *email;
	char *passw;
	char *override;
	const struct locale * locale;
	int lastorders;	/* enno: short? */
	int age;	/* enno: short? */
	ursprung *ursprung;
	const struct race * race;
	magic_t magiegebiet;
	int newbies;
	int num_migrants;			/* Anzahl Migranten */
	int num_people;				/* Anzahl Personen */
	int options;
	int no_units;
	int karma;
#ifdef USE_UGROUPS
	struct ugroup *ugroups;
#endif
	struct warning * warnings;
	struct msglevel * msglevels;
	struct ally *allies;
	struct group *groups;
	struct strlist *mistakes; /* enno: das muﬂ irgendwann noch ganz raus */
	boolean alive; /* enno: sollte ein flag werden */
	int nregions;
	int number; /* enno: unterschied zu num_people ? */
	int money;
	int score;
#ifdef VICTORY_DELAY
	unsigned char victory_delay;
#endif
#ifndef FAST_REGION
	vset regions;
#endif
	struct unit * units;
	struct attrib *attribs;
	struct message_list * msgs;
	struct bmsg {
		struct bmsg * next;
		struct region * r;
		struct message_list * msgs;
	} * battles;
} faction;

typedef struct faction_list {
	struct faction_list * next;
	struct faction * data;
} faction_list;

extern const struct unit * random_unit_in_faction(const struct faction *f);
extern const char * factionname(const struct faction * f);
extern void * resolve_faction(void * data);
extern struct unit * addplayer(struct region *r, const char *email, const char* password, const struct race * frace, const struct locale *loc);
extern boolean checkpasswd(const faction * f, const char * passwd);

#endif
