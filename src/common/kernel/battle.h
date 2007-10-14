/* vi: set ts=2:
*
*	Eressea PB(E)M host Copyright (C) 1998-2003
*      Christian Schlittchen (corwin@amber.kn-bremen.de)
*      Katja Zedel (katze@felidae.kn-bremen.de)
*      Henning Peters (faroul@beyond.kn-bremen.de)
*      Enno Rehling (enno@eressea.de)
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

#include <util/cvector.h>

#define SHOW_KILLS
#undef SMALL_BATTLE_MESSAGES

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
#define MAXSIDES 128 /* if there are ever more than this, we're fucked. */

  struct message;

  typedef struct bfaction {
    struct bfaction * next;
    struct side * sides;
    struct faction *faction;
#ifndef SIMPLE_COMBAT
    int lastturn; /* last time this struct faction was involved in combat */
#endif
    boolean attacker;
  } bfaction;


  typedef struct battle {
    cvector leaders;
    struct region *region;
    struct plane  *plane;
    bfaction * factions;
    int nfactions;
    int nfighters;
    struct side * sides;
    int nsides;
    cvector meffects;
    int		max_tactics;
    int		turn;
    boolean has_tactics_turn;
    int     keeploot;
    boolean reelarrow;
    int     alive;
#ifdef SMALL_BATTLE_MESSAGES
    boolean small;
#endif
#define FASTROW
#ifdef FASTROW
    struct {
      const struct side * as;
      const struct side * vs;
      int alive;
      int row;
      int result;
    } rowcache;
#endif
#define FASTCOUNT
#ifdef FASTCOUNT
    struct {
      struct side * side;
      int status;
      int alive;
      int minrow, maxrow;
      int enemies[8];
    } fast;
#endif
  } battle;

  typedef struct tactics {
    cvector fighters;
    int value;
  } tactics;

#define SIDE_STEALTH   1<<0
#ifdef SIMPLE_COMBAT
#define SIDE_HASGUARDS  1<<1
#endif
  typedef struct side {
    struct side * next; /* nächstes Heer in der Schlacht */
    struct side * nextF; /* nächstes Heer der gleichen Partei */
    struct battle * battle;
    struct bfaction * bf; /* Die Partei, die hier kämpft */
    const struct group * group;
    struct tactics leader;				/* der beste Taktiker des Heeres */
# define E_ENEMY 1
# define E_FRIEND 2
# define E_ATTACKING 4
    unsigned char relations[MAXSIDES];
    struct side * enemies[MAXSIDES];
    struct fighter * fighters;
    int index;		/* Eintrag der Fraktion in b->matrix/b->enemies */
    int size[NUMROWS];	/* Anzahl Personen in Reihe X. 0 = Summe */
    int nonblockers[NUMROWS]; /* Anzahl nichtblockierender Kämpfer, z.B. Schattenritter. */
    int alive;		/* Die Partei hat den Kampf verlassen */
    int removed;  /* stoned */
    int flee;
    int dead;
    int casualties; /* those dead that were real people, not undead! */
    int healed;
    unsigned int flags;
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
#define FL_COURAGE  8 /* Helden fliehen nie */
#define FL_SLEEPING 16
#define FL_STUNNED	32	/* eine Runde keinen Angriff */
#define FL_HIT    	64	/* the person at attacked */

  typedef struct troop {
    struct fighter *fighter;
    int index;
  } troop;

  typedef struct armor {
    struct armor * next;
    const struct armor_type * atype;
    int count;
  } armor;

  /*** fighter::flags ***/
#define FIG_ATTACKER   1<<0
#define FIG_NOLOOT     1<<1
  typedef struct fighter {
    struct fighter * next;
    struct side *side;
    struct unit *unit;                /* Die Einheit, die hier kämpft */
    struct building *building;        /* Gebäude, in dem die Einheit evtl. steht */
    status_t status;           /* Kampfstatus */
    struct weapon * weapons;
    struct armor *armors;         /* Anzahl Rüstungen jeden Typs */
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
      int hp;    /* Trefferpunkte der Personen */
      int attack      : 8;     /* (Magie) Attackenbonus der Personen */
      int defence     : 8;     /* (Magie) Paradenbonus der Personen */
      int damage      : 8;     /* (Magie) Schadensbonus der Personen im Nahkampf */
      int damage_rear : 8;     /* (Magie) Schadensbonus der Personen im Fernkampf */
      int flags       : 8;     /* (Magie) Diverse Flags auf Kämpfern */
      int speed       : 8;     /* (Magie) Geschwindigkeitsmultiplkator. */
      int reload      : 4;     /* Anzahl Runden, die die Waffe x noch laden muss.
                                * dahinter steckt ein array[RL_MAX] wenn er min. eine hat. */
      int last_action : 4;     /* In welcher Runde haben wir zuletzt etwas getan */
      struct weapon * missile; /* missile weapon */
      struct weapon * melee;   /* melee weapon */
    } * person;
    unsigned int flags;
    struct {
      int number;  /* number of people who fled */
      int hp;      /* accumulated hp of fleeing people */
#ifndef SIMPLE_ESCAPE
      struct region *region;  /* destination of fleeing people */
      struct item * items; /* items they take */
#endif /* SIMPLE_ESCAPE */
    } run;
#ifndef SIMPLE_COMBAT
    int action_counter;	/* number of active actions the struct unit did in the fight */
#endif /* SIMPLE_COMBAT */
#ifdef SHOW_KILLS
    int kills;
    int hits;
#endif
  } fighter;


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

  extern void do_battle(struct region * r);

  /* for combar spells and special attacks */
  enum { SELECT_ADVANCE = 0x1, SELECT_DISTANCE = 0x2, SELECT_FIND = 0x4 };

  extern troop select_enemy(struct fighter * af, int minrow, int maxrow, int select);
  extern int count_enemies(struct battle * b, const struct fighter * af, int minrow, int maxrow, int select);
  extern boolean terminate(troop dt, troop at, int type, const char *damage, boolean missile);
  extern void message_all(battle * b, struct message * m);
  extern int hits(troop at, troop dt, weapon * awp);
  extern void damage_building(struct battle *b, struct building *bldg, int damage_abs);
  extern struct cvector * fighters(struct battle *b, const struct side * vs, int minrow, int maxrow, int mask);
  extern int count_allies(const struct side * as, int minrow, int maxrow, int select);
  extern int get_unitrow(const struct fighter * af, const struct side * vs);
  extern boolean helping(const struct side * as, const struct side * ds);
  extern void rmfighter(fighter *df, int i);
#ifndef SIMPLE_ESCAPE
  extern struct region * fleeregion(const struct unit * u);
#endif
  extern struct fighter * select_corpse(struct battle * b, struct fighter * af);
  extern fighter * make_fighter(struct battle * b, struct unit * u, side * s, boolean attack);
  extern int statusrow(int status);
  extern void drain_exp(struct unit *u, int d);
  extern void kill_troop(troop dt);
  extern void remove_troop(troop dt); /* not the same as the badly named rmtroop */
  extern boolean is_attacker(const fighter * fig);

#ifdef __cplusplus
}
#endif

#endif
