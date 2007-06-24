/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

#ifndef H_KRNL_RACE_H
#define H_KRNL_RACE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "magic.h" /* wegen MAXMAGIETYP */

#define AT_NONE 0
#define AT_STANDARD	1
#define AT_DRAIN_EXP 2
#define AT_DRAIN_ST 3
#define AT_NATURAL 4
#define AT_DAZZLE 5
#define AT_SPELL 6
#define AT_COMBATSPELL 7
#define AT_STRUCTURAL 8

#define GOLEM_IRON   4 /* Anzahl Eisen in einem Eisengolem */
#define GOLEM_STONE  4 /* Anzahl Steine in einem Steingolem */

#define RACESPOILCHANCE 5 /* Chance auf rassentypische Beute */

typedef struct att {
	int type;
	union {
		const char * dice;
		const spell * sp;
	} data;
	int flags;
} att;

typedef struct race {
	const char *_name[4]; /* neu: name[4]völker */
	float magres;
	float maxaura; /* Faktor auf Maximale Aura */
	float regaura; /* Faktor auf Regeneration */
	int recruitcost;
	int maintenance;
	int splitsize;
	int weight;
	int capacity;
	float speed;
  float aggression; /* chance that a monster will attack */
	int hitpoints;
	const char *def_damage;
	char armor;
	char at_default; /* Angriffsskill Unbewaffnet (default: -2)*/
	char df_default; /* Verteidigungsskill Unbewaffnet (default: -2)*/
	char at_bonus;   /* Verändert den Angriffsskill (default: 0)*/
	char df_bonus;   /* Verändert den Verteidigungskill (default: 0)*/
	const spell * precombatspell;
	struct att attack[10];
	char bonus[MAXSKILLS];
	boolean __remove_me_nonplayer;
	int flags;
	int battle_flags;
	int ec_flags;
	race_t oldfamiliars[MAXMAGIETYP];

	const char *(*generate_name) (const struct unit *);
	void (*age)(struct unit *u);
	boolean (*move_allowed)(const struct region *, const struct region *);
	struct item * (*itemdrop)(const struct race *, int size);
	void (*init_familiar)(struct unit *);

	const struct race * familiars[MAXMAGIETYP];
	struct attrib * attribs;
	struct race * next;
} race;

typedef struct race_list {
  struct race_list * next;
  const struct race * data;
} race_list;

extern void racelist_clear(struct race_list **rl);
extern void racelist_insert(struct race_list **rl, const struct race *r);

extern struct race_list * get_familiarraces(void);
extern struct race * races;

extern struct race * rc_find(const char *);
extern const char * rc_name(const struct race *, int);
extern struct race * rc_add(struct race *);
extern struct race * rc_new(const char * zName);
extern int rc_specialdamage(const race *, const race *, const struct weapon_type *);

/* Flags */
#define RCF_PLAYERRACE     (1<<0)	/* can be played by a player. */
#define RCF_KILLPEASANTS   (1<<1)		/* Töten Bauern. Dämonen werden nicht über dieses Flag, sondern in randenc() behandelt. */
#define RCF_SCAREPEASANTS  (1<<2)
#define RCF_CANSTEAL       (1<<3)
#define RCF_MOVERANDOM     (1<<4)
#define RCF_CANNOTMOVE     (1<<5)
#define RCF_LEARN          (1<<6)  /* Lernt automatisch wenn struct faction == 0 */
#define RCF_FLY            (1<<7)  /* kann fliegen */
#define RCF_SWIM           (1<<8)  /* kann schwimmen */
#define RCF_WALK           (1<<9)  /* kann über Land gehen */
#define RCF_NOLEARN        (1<<10) /* kann nicht normal lernen */
#define RCF_NOTEACH        (1<<11) /* kann nicht lehren */
#define RCF_HORSE          (1<<12) /* Einheit ist Pferd, sozusagen */
#define RCF_DESERT         (1<<13) /* 5% Chance, das Einheit desertiert */
#define RCF_ILLUSIONARY    (1<<14) /* (Illusion & Spell) Does not drop items. */
#define RCF_ABSORBPEASANTS (1<<15) /* Tötet und absorbiert Bauern */
#define RCF_NOHEAL         (1<<16) /* Einheit kann nicht geheilt werden */
#define RCF_NOWEAPONS      (1<<17) /* Einheit kann keine Waffen benutzen */
#define RCF_SHAPESHIFT     (1<<18) /* Kann TARNE RASSE benutzen. */
#define RCF_SHAPESHIFTANY  (1<<19) /* Kann TARNE RASSE "string" benutzen. */
#define RCF_UNDEAD         (1<<20) /* Undead. */
#define RCF_DRAGON         (1<<21) /* Drachenart (für Zauber)*/
#define RCF_COASTAL        (1<<22) /* kann in Landregionen an der Küste sein */
#define RCF_UNARMEDGUARD   (1<<23) /* kann ohne Waffen bewachen */
#define RCF_CANSAIL        (1<<24) /* Einheit darf Schiffe betreten */

/* Economic flags */
#define NOGIVE         (1<<0)   /* gibt niemals nix */
#define GIVEITEM       (1<<1)   /* gibt Gegenstände weg */
#define GIVEPERSON     (1<<2)   /* übergibt Personen */
#define GIVEUNIT       (1<<3)   /* Einheiten an andere Partei übergeben */
#define GETITEM        (1<<4)   /* nimmt Gegenstände an */
#define CANGUARD       (1<<5)   /* bewachen auch ohne Waffen */
#define ECF_REC_HORSES     (1<<6)   /* Rekrutiert aus Pferden */
#define ECF_REC_ETHEREAL   (1<<7)   /* Rekrutiert aus dem Nichts */
#define ECF_REC_UNLIMITED  (1<<8)   /* Rekrutiert ohne Limit */

/* Battle-Flags */
#define BF_EQUIPMENT    (1<<0) /* Kann Ausrüstung benutzen */
#define BF_NOBLOCK      (1<<1) /* Wird in die Rückzugsberechnung nicht einbezogen */
#define BF_RES_PIERCE   (1<<2) /* Halber Schaden durch PIERCE */
#define BF_RES_CUT      (1<<3) /* Halber Schaden durch CUT */
#define BF_RES_BASH     (1<<4) /* Halber Schaden durch BASH */
#define BF_INV_NONMAGIC (1<<5) /* Immun gegen nichtmagischen Schaden */
#define BF_CANATTACK    (1<<6) /* Kann keine ATTACKIERE Befehle ausfuehren */

extern int unit_old_max_hp(struct unit * u);
extern const char * racename(const struct locale *lang, const struct unit *u, const race * rc);

#define omniscient(f) (((f)->race)==new_race[RC_ILLUSION] || ((f)->race)==new_race[RC_TEMPLATE])

#define playerrace(rc) (fval((rc), RCF_PLAYERRACE))
#define dragonrace(rc) ((rc) == new_race[RC_FIREDRAGON] || (rc) == new_race[RC_DRAGON] || (rc) == new_race[RC_WYRM] || (rc) == new_race[RC_BIRTHDAYDRAGON])
#define humanoidrace(rc) (fval((rc), RCF_UNDEAD) || (rc)==new_race[RC_DRACOID] || playerrace(rc))
#define illusionaryrace(rc) (fval(rc, RCF_ILLUSIONARY))

extern boolean allowed_dragon(const struct region * src, const struct region * target);

extern void register_races(void);
extern void init_races(void);
extern boolean r_insectstalled(const struct region *r);

extern void add_raceprefix(const char *);
extern char ** race_prefixes;

extern void write_race_reference(const struct race * rc, FILE * F);
extern int read_race_reference(const struct race ** rp, FILE * F);

extern const char * raceprefix(const struct unit *u);

extern void give_starting_equipment(const struct equipment * eq, struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
