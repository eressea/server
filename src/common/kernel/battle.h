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

#ifndef H_KRNL_BATTLE
#define H_KRNL_BATTLE
#ifdef __cplusplus
extern "C" {
#endif

/** new code defines **/
#define FAST_GETUNITROW

/** more defines **/
#define FS_ENEMY 1
#define FS_HELP  2

/***** Verteidigungslinien.
 * Eressea hat 4 Verteidigungslinien. 1 ist vorn, 5. enthält Summen 
 */
#define NUMROWS 5
#define SUM_ROW 0
#define FIGHT_ROW 1
#define BEHIND_ROW 2
#define AVOID_ROW 3
#define FLEE_ROW 4
#define LAST_ROW (NUMROWS-1)
#define FIRST_ROW FIGHT_ROW

typedef struct bfaction {
	struct bfaction * next;
	struct side * sides;
	struct faction *faction;
	int lastturn; /* last time this struct faction was involved in combat */
	boolean attacker;
} bfaction;

typedef struct battle {
	cvector leaders;
	struct region *region;
	struct plane  *plane;
	bfaction * factions;
	int nfactions;
	cvector fighters;
	cvector sides;
	cvector meffects;
	int		max_tactics;
	int		turn;
	boolean has_tactics_turn;
	int     keeploot;
	boolean reelarrow;
	int     dh;
	int     alive;
	boolean small;
#ifdef FAST_GETUNITROW
	boolean nonblockers;
#endif
} battle;

typedef struct tactics {
	cvector fighters;
	int value;
} tactics;

typedef struct side {
	struct tactics leader;				/* der beste Taktiker des Heeres */
	struct side * nextF; /* nächstes Heer der gleichen Partei */
	struct battle * battle;
	struct bfaction * bf; /* Die Partei, die hier kämpft */
	const struct group * group;
# define E_ENEMY 1
# define E_ATTACKING 2
	int enemy[128];
	cvector fighters;	/* vector der Einheiten dieser Fraktion */
	int index;		/* Eintrag der Fraktion in b->matrix/b->enemies */
	int size[NUMROWS];	/* Anzahl Personen in Reihe X. 0 = Summe */
	int nonblockers[NUMROWS]; /* Anzahl nichtblockierender Kämpfer, z.B. Schattenritter. */
	int alive;		/* Die Partei hat den Kampf verlassen */
	int removed;  /* stoned */
	int flee;
	int dead;
	int casualties;
	int healed;
	boolean dh;
	boolean stealth;	/* Die Einheiten sind getarnt */
	const struct faction *stealthfaction;
} side;

typedef struct weapon {
	int count, used;
	const struct weapon_type * type;
	int attackskill : 8; 
	int defenseskill : 8;
} weapon;

/*** fighter::person::flags ***/
#define FL_TIRED	  1
#define FL_DAZZLED  2 /* durch Untote oder Dämonen eingeschüchtert */
#define FL_PANICED  4
#define FL_HERO     8 /* Helden fliehen nie */
#define FL_SLEEPING 16
#define FL_STUNNED	32	/* eine Runde keinen Angriff */

/*** fighter::flags ***/
#define FIG_ATTACKED   1
#define FIG_NOLOOT     2

typedef unsigned char armor_t;
enum {
#ifdef COMPATIBILITY
	AR_MAGICAL,
#endif
	AR_PLATE,
	AR_CHAIN,
	AR_RUSTY_CHAIN,
	AR_SHIELD,
	AR_RUSTY_SHIELD,
	AR_EOGSHIELD,
	AR_EOGCHAIN,
	AR_MAX,
	AR_NONE
};

typedef struct fighter {
	struct side *side;
	struct unit *unit;                /* Die Einheit, die hier kämpft */
	struct building *building;        /* Gebäude, in dem die Einheit evtl. steht */
	status_t status;           /* Kampfstatus */
	struct weapon * weapons;
	int armor[AR_MAX];         /* Anzahl Rüstungen jeden Typs */
	int alive;                 /* Anzahl der noch nicht Toten in der Einheit */
	int fighting;              /* Anzahl der Kämpfer in der aktuellen Runde */
	int removed;               /* Anzahl Kaempfer, die nicht tot sind, aber
								   aus dem Kampf raus sind (zB weil sie
								   versteinert wurden).  Diese werden auch
								   in alive noch mitgezählt! */
	int magic;					/* Magietalent der Einheit  */
	int horses;					/* Anzahl brauchbarer Pferde der Einheit */
	int elvenhorses;			/* Anzahl brauchbarer Elfenpferde der Einheit */
	struct item * loot;
	int catmsg;					/* Merkt sich, ob Katapultmessage schon generiert. */
	struct person {
		int attack      : 8;    /* (Magie) Attackenbonus der Personen */
		int defence     : 8;    /* (Magie) Paradenbonus der Personen */
		int damage      : 8;    /* (Magie) Schadensbonus der Personen im Nahkampf */
		int damage_rear : 8;    /* (Magie) Schadensbonus der Personen im Fernkampf */
		int hp          : 16;   /* Trefferpunkte der Personen */
		int flags       : 8;    /* (Magie) Diverse Flags auf Kämpfern */
		int speed       : 8;    /* (Magie) Geschwindigkeitsmultiplkator. */
		int reload      : 4;    /* Anzahl Runden, die die Waffe x noch laden muss.
		                         * dahinter steckt ein array[RL_MAX] wenn er min. eine hat. */
		int last_action : 8;		/* In welcher Runde haben wir zuletzt etwas getan */
		struct weapon * missile;   /* missile weapon */
		struct weapon * melee;     /* melee weapon */
	} * person;
	int flags;
	struct {
		int number;  /* number of people who have flown */
		int hp;      /* accumulated hp of fleeing people */
#ifndef NO_RUNNING
		struct region *region;  /* destination of fleeing people */
		struct item * items; /* items they take */
#endif
	} run;
	int action_counter;	/* number of active actions the struct unit did in the fight */
#ifdef SHOW_KILLS
	int kills;
	int hits;
#endif
#ifdef FAST_GETUNITROW
	struct {
		int alive;
		int cached;
	} row;
#endif
} fighter;

typedef struct troop {
	struct fighter *fighter;
	int index;
} troop;


/* schilde */

enum {
	SHIELD_REDUCE,
	SHIELD_ARMOR,
	SHIELD_WIND,
	SHIELD_BLOCK,
	SHIELD_MAX
};

typedef struct meffect {
	fighter *magician;  /* Der Zauberer, der den Schild gezaubert hat */
	int typ;            /* Wirkungsweise des Schilds */
	int effect;
	int duration;
} meffect;

extern const troop no_troop;

extern void do_battle(void);

/* for combar spells and special attacks */
extern int damage_unit(struct unit *u, const char *dam, boolean armor, boolean magic);
extern troop select_enemy(struct battle * b, struct fighter * af, int minrow, int maxrow);
extern int count_enemies(struct battle * b, struct side * as, int mask, int minrow, int maxrow);
extern boolean terminate(troop dt, troop at, int type, const char *damage, boolean missile);
extern void battlemsg(battle * b, struct unit * u, const char * s);
extern void battlerecord(battle * b, const char *s);
extern int hits(troop at, troop dt, weapon * awp);
extern void damage_building(struct battle *b, struct building *bldg, int damage_abs);
extern struct cvector * fighters(struct battle *b, struct fighter *af, int minrow, int maxrow, int mask);
extern int countallies(struct side * as);
extern int get_unitrow(struct fighter * af);
extern boolean helping(struct side * as, struct side * ds);
extern void rmfighter(fighter *df, int i);
extern struct region * fleeregion(const struct unit * u);
extern struct troop select_corpse(struct battle * b, struct fighter * af);
extern fighter * make_fighter(struct battle * b, struct unit * u, side * s, boolean attack);
extern int statusrow(int status);
extern void drain_exp(struct unit *u, int d);
extern void rmtroop(troop dt);

#ifdef __cplusplus
}
#endif

#endif
