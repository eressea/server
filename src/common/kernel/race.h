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

#ifndef FACTYPES_H
#define FACTYPES_H
#include <config.h>
#include "magic.h"

#define AT_NONE 0
#define AT_STANDARD	1
#define AT_DRAIN_EXP 2
#define AT_DRAIN_ST 3
#define AT_NATURAL 4
#define AT_DAZZLE 5
#define AT_SPELL 6
#define AT_COMBATSPELL 7
#define AT_STRUCTURAL 8

#define GOLEM_IRON   4          /* Anzahl Eisen in einem Eisengolem */
#define GOLEM_STONE  5          /* Anzahl Steine in einem Steingolem */

typedef struct att {
	int type;
	union {
		const char * dice;
		int i;
	} data;
	int flags;
} att;

typedef struct race_type {
	const char *name[4]; /* neu: name[4]völker */
	double magres;
	double maxaura; /* Faktor auf Maximale Aura */
	double regaura; /* Faktor auf Regeneration */
	int rekrutieren;
	int maintenance;
	int splitsize;
	int weight;
	double speed;
	int hitpoints;
	const char *def_damage;
	char armor;
	char at_default; /* Angriffsskill Unbewaffnet (default: -2)*/
	char df_default; /* Verteidigungsskill Unbewaffnet (default: -2)*/
	char at_bonus;   /* Verändert den Angriffsskill (default: 0)*/
	char df_bonus;   /* Verändert den Verteidigungskill (default: 0)*/
	att  attack[6];
	char bonus[MAXSKILLS];
	boolean nonplayer;
	int flags;
	int battle_flags;
	int ec_flags;
	race_t familiars[MAXMAGIETYP];
	const char *(*generate_name) (const struct unit *);
	void (*age_function)(struct unit *u);
	boolean (*move_allowed)(const struct region *, const struct region *);
	struct attrib * attribs;
} racetype;

#define racedata race_type

/* Flags */
#define RCF_KILLPEASANTS   (1<<0)		/* Töten Bauern. Dämonen werden nicht über dieses Flag, sondern in randenc() behandelt. */
#define RCF_SCAREPEASANTS  (1<<1)
#define RCF_ATTACKRANDOM   (1<<2)
#define RCF_MOVERANDOM     (1<<3)
#define RCF_CANNOTMOVE     (1<<4)
#define RCF_SEEKTARGET     (1<<5)		/* sucht ein bestimmtes Opfer */
#define RCF_LEARN          (1<<6) 	/* Lernt automatisch wenn struct faction == 0 */
#define RCF_FLY            (1<<7)   /* kann fliegen */
#define RCF_SWIM           (1<<8)   /* kann schwimmen */
#define RCF_WALK           (1<<9)   /* kann über Land gehen */
#define RCF_NOLEARN        (1<<10) 	/* kann nicht normal lernen */
#define RCF_NOTEACH        (1<<11) 	/* kann nicht lehren */
#define RCF_HORSE          (1<<12)  /* Einheit ist Pferd, sozusagen */
#define RCF_DESERT         (1<<13)  /* 5% Chance, das Einheit desertiert */
#define RCF_DRAGONLIMIT    (1<<14)  /* Kann nicht aus Gletscher in Ozean */
#define RCF_ABSORBPEASANTS (1<<15)  /* Tötet und absorbiert Bauern */
#define RCF_NOHEAL         (1<<16)  /* Einheit kann nicht geheilt werden */
#define RCF_NOWEAPONS      (1<<17)  /* Einheit kann keine Waffen bneutzen */
#define RCF_SHAPESHIFT     (1<<18)	/* Kann TARNE RASSE benutzen. */

/* Economic flags */
#define NOGIVE         (1<<0)   /* gibt niemals nix */
#define GIVEITEM       (1<<1)   /* gibt Gegenstände weg */
#define GIVEPERSON     (1<<2)   /* übergibt Personen */
#define GIVEUNIT       (1<<3)   /* Einheiten an andere Partei übergeben */
#define GETITEM        (1<<4)   /* nimmt Gegenstände an */
#define HOARDMONEY     (1<<5)   /* geben niemals Silber weg */
#define CANGUARD       (1<<6)   /* bewachen auch ohne Waffen */
#define REC_HORSES     (1<<7)   /* Rekrutiert aus Pferden */

/* Battle-Flags */
#define BF_EQUIPMENT				(1<<0)
	/* Kann Ausrüstung benutzen */
#define BF_MAGIC_EQUIPMENT	(1<<1)
	/* Kann magische Gegenstände (keine Waffen und Rüstungen)benutzen */
#define BF_DRAIN_STRENGTH		(1<<2)
	/* Treffer entzieht Attack/Defense */
#define BF_DRAIN_EXP				(1<<3)
	/* Treffer entzieht Talenttage */
#define BF_INV_NONMAGIC			(1<<4)
	/* Immun gegen nichtmagischen Schaden */
#define BF_NOBLOCK					(1<<5)
	/* Wird in die Rückzugsberechnung nicht einbezogen */
#define BF_RES_PIERCE				(1<<6)
	/* Halber Schaden durch PIERCE */
#define BF_RES_CUT					(1<<7)
	/* Halber Schaden durch CUT */
#define BF_RES_BASH					(1<<8)
	/* Halber Schaden durch BASH */

extern struct racedata race[];
void give_starting_equipment(struct region *r, struct unit *u);
void give_latestart_bonus(struct region *r, struct unit *u, int b);
int unit_old_max_hp(struct unit * u);
boolean is_undead(const struct unit *u);

#define dragon(u) ((u)->race == RC_FIREDRAGON || (u)->race == RC_DRAGON || (u)->race == RC_WYRM || (u)->race == RC_BIRTHDAYDRAGON)
#define humanoid(u) (u->race==RC_UNDEAD || u->race==RC_DRACOID || !race[(u)->race].nonplayer)
#define nonplayer(u) (race[(u)->race].nonplayer)
#define nonplayer_race(r) (race[r].nonplayer)
#define illusionary(u) ((u)->race==RC_ILLUSION)
#define omniscient(f) ((f)->race==RC_ILLUSION || (f)->race==RC_TEMPLATE)

extern boolean allowed_dragon(const struct region * src, const struct region * target);

extern void init_races(void);

#endif
