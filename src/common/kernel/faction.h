/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */
#ifndef H_KRNL_FACTION
#define H_KRNL_FACTION
#ifdef __cplusplus
extern "C" {
#endif

struct player;
struct alliance;
struct item;
  
/* SMART_INTERVALS: define to speed up finding the interval of regions that a 
   faction is in. defining this speeds up the turn by 30-40% */
#define SMART_INTERVALS

#ifdef SHORTPWDS
typedef struct shortpwd {
  struct shortpwd * next;
  char * email;
  char * pwd;
  boolean used;
} shortpwd;
#endif

/* faction flags */
#define FFL_NEWID (1<<0) /* Die Partei hat bereits einmal ihre no gewechselt */  
#define FFL_ISNEW         (1<<1)
#define FFL_RESTART       (1<<2)
#define FFL_QUIT          (1<<3)
#define FFL_SELECT        (1<<18) /* ehemals f->dh, u->dh, r->dh, etc... */
#define FFL_NOAID         (1<<21) /* Hilfsflag Kampf */
#define FFL_MARK          (1<<23) /* für markierende algorithmen, die das 
                                   * hinterher auch wieder löschen müssen! 
                                   * (FL_DH muss man vorher initialisieren, 
                                   * FL_MARK hinterher löschen) */
#define FFL_NOIDLEOUT     (1<<24) /* Partei stirbt nicht an NMRs */
#define FFL_OVERRIDE      (1<<27) /* Override-Passwort wurde benutzt */
#define FFL_DBENTRY       (1<<28) /* Partei ist in Datenbank eingetragen */
#define FFL_NOTIMEOUT     (1<<29) /* ignore MaxAge() */
#define FFL_GM            (1<<30) /* eine Partei mit Sonderrechten */

#define FFL_SAVEMASK (FFL_NEWID|FFL_GM|FFL_NOTIMEOUT|FFL_DBENTRY|FFL_NOTIMEOUT)

typedef struct faction {
	struct faction *next;
	struct faction *nexthash;

	struct player *owner;
#ifdef SMART_INTERVALS
	struct region *first;
	struct region *last;
#endif
	int no;
	int subscription;
	unsigned int flags;
	char *name;
	char *banner;
	char *email;
	char *passw;
	char *override;
#ifdef SHORTPWDS
  struct shortpwd * shortpwds;
#endif
	const struct locale * locale;
	int lastorders;	/* enno: short? */
	int age;	/* enno: short? */
	struct ursprung *ursprung;
	const struct race * race;
	magic_t magiegebiet;
	int newbies;
	int num_people;				/* Anzahl Personen ohne Monster */
  int num_total;        /* Anzahl Personen mit Monstern */
	int options;
	int no_units;
	struct ally *allies;
	struct group *groups;
	boolean alive; /* enno: sollte ein flag werden */
	int nregions;
	int money;
#ifdef SCORE_MODULE
  int score;
#endif
#ifdef KARMA_MODULE
  int karma;
#endif
	struct alliance * alliance;
#ifdef VICTORY_DELAY
	unsigned char victory_delay;
#endif
	struct unit * units;
	struct attrib *attribs;
	struct message_list * msgs;
#ifdef ENEMIES
  struct faction_list * enemies;
#endif
	struct bmsg {
		struct bmsg * next;
		struct region * r;
		struct message_list * msgs;
	} * battles;
  struct item * items; /* items this faction can claim */
} faction;

typedef struct faction_list {
	struct faction_list * next;
	struct faction * data;
} faction_list;

extern const struct unit * random_unit_in_faction(const struct faction *f);
extern const char * factionname(const struct faction * f);
extern void * resolve_faction(variant data);
extern struct unit * addplayer(struct region *r, faction * f);
extern struct faction * addfaction(const char *email, const char* password, 
                                   const struct race * frace, 
                                   const struct locale *loc, int subscription);
extern boolean checkpasswd(const faction * f, const char * passwd, boolean shortp);
extern void destroyfaction(faction * f);

extern void set_alliance(struct faction * a, struct faction * b, int status);
extern int get_alliance(const struct faction * a, const struct faction * b);

#ifdef ENEMIES
extern boolean is_enemy(const struct faction * f, const struct faction * enemy);
extern void add_enemy(struct faction * f, struct faction * enemy);
extern void remove_enemy(struct faction * f, struct faction * enemy);
#endif

extern void write_faction_reference(const struct faction * f, FILE * F);
extern int read_faction_reference(struct faction ** f, FILE * F);


#ifdef SMART_INTERVALS
extern void update_interval(struct faction * f, struct region * r);
#endif

#ifdef __cplusplus
}
#endif
#endif
