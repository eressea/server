#pragma once
#include <stddef.h>
#include "magic.h"              /* wegen MAXMAGIETYP */
#include "skill.h"

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
#define GOLEM_STONE  4          /* Anzahl Steine in einem Steingolem */

#define RACESPOILCHANCE 5       /* Chance auf rassentypische Beute */

#define RACE_ATTACKS 10         /* maximum number of attacks */

#define PERSON_WEIGHT 1000      /* weight of a "normal" human unit */

struct params;
struct spell;
struct spellref;
struct locale;
struct rcoption;
struct item_type;

typedef enum {
    RC_DWARF,                     /* 0 - Zwerg */
    RC_ELF,
    RC_GOBLIN,
    RC_HUMAN,
    RC_TROLL,
    RC_DAEMON,
    RC_INSECT,
    RC_HALFLING,
    RC_CAT,
    RC_AQUARIAN,
    RC_ORC,
    /* last of the addplayer races */

    RC_SNOTLING,
    RC_UNDEAD,

    RC_FIREDRAGON,
    RC_DRAGON,
    RC_WYRM,
    RC_TREEMAN,
    RC_BIRTHDAYDRAGON,
    RC_DRACOID,

    RC_IRONGOLEM,
    RC_STONEGOLEM,
    RC_SHADOW,
    RC_SHADOWLORD,
    RC_IRONKEEPER,
    RC_ALP,
    RC_TOAD,
    RC_HIRNTOETER,
    RC_PEASANT,
    RC_WOLF,

    RC_SONGDRAGON,

    RC_SEASERPENT,
    RC_SHADOWKNIGHT,

    RC_SKELETON,
    RC_SKELETON_LORD,
    RC_ZOMBIE,
    RC_ZOMBIE_LORD,
    RC_GHOUL,
    RC_GHOUL_LORD,
    RC_TEMPLATE,
    RC_CLONE,
    MAXRACES,
    NORACE
} race_t;

#define MAX_START_RACE RC_ORC

extern struct race *races;
extern int num_races;
extern const char *racenames[MAXRACES];

typedef struct att {
    int type;
    union {
        char *dice;
        struct spellref *sp;
    } data;
    int flags;
    int level;
} att;

typedef void (*race_func) (struct unit *);

typedef struct race {
    char *_name;
    variant magres;
    int healing;
    int maxaura;            /* Faktor auf Maximale Aura */
    double regaura;            /* Faktor auf Regeneration */
    int recruit_multi;      /* Faktor fuer Bauernverbrauch */
    int index;
    int recruitcost;
    int maintenance;
    int splitsize;
    int weight;
    int capacity;
    int income;
    double speed;
    int hitpoints;
    char *def_damage;
    int armor;
    int at_default;             /* Angriffsskill Unbewaffnet (default: -2) */
    int df_default;             /* Verteidigungsskill Unbewaffnet (default: -2) */
    int at_bonus;               /* Veraendert den Angriffsskill (default: 0) */
    int df_bonus;               /* Veraendert den Verteidigungskill (default: 0) */
    signed char *study_speed;   /* study-speed-bonus in points/turn (0=30 Tage) */
    unsigned int flags;
    unsigned int battle_flags;
    unsigned int ec_flags;
    int mask_item;
    struct att attack[RACE_ATTACKS];
    signed char bonus[MAXSKILLS];

    race_func name_unit;
    race_func age_unit;

    struct rcoption *options; /* rarely used properties */

    const struct race *familiars[MAXMAGIETYP];
    struct race *next;
} race;

typedef struct race_list {
    struct race_list *next;
    const struct race *data;
} race_list;

void init_races(struct locale* lang);

void racelist_clear(struct race_list **rl);
void racelist_insert(struct race_list **rl, const struct race *r);

const struct race *findrace(const char *, const struct locale *);

struct race_list *get_familiarraces(void);
const struct race *get_race(race_t rt);
/** TODO: compatibility hacks: **/
race_t old_race(const struct race *);

race *rc_create(const char *zName);
race *rc_get_or_create(const char *name);
bool rc_changed(int *cache);
race *rc_find(const char *);
void free_races(void);

bool rc_can_use(const struct race *rc, const struct item_type *itype);
bool rc_leaves_corpse(const struct race *rc);

const char* race_name(const race* rc);
typedef enum name_t { NAME_SINGULAR, NAME_PLURAL, NAME_DEFINITIVE, NAME_CATEGORY } name_t;
const char * rc_name_s(const race *rc, name_t n);
const char * rc_key(const char *rcname, name_t n, char *name, size_t size);
const char * rc_name(const race *rc, name_t n, char *name, size_t size);

void rc_set_param(struct race *rc, const char *key, const char *value);

int rc_luxury_trade(const struct race *rc);
bool rc_can_learn(const race *rc, skill_t sk);
int rc_herb_trade(const struct race *rc);
variant rc_magres(const struct race *rc);
double rc_maxaura(const struct race *rc);
int rc_armor_bonus(const struct race *rc);
int rc_scare(const struct race *rc);
int rc_skillmod(const struct race *rc, enum skill_t sk);
const char * rc_hungerdamage(const race *rc);
const race *rc_otherrace(const race *rc);

#define MIGRANTS_NONE 0
#define MIGRANTS_LOG10 1
int rc_migrants_formula(const race *rc);
    
int rc_mask(const race *rc);
int rc_get_mask(char *list);

/* Flags. Do not reorder these without changing json_race() in jsonconf.c */
#define RCF_PLAYABLE       (1<<0)       /* cannot be the race for a player faction (and other limits?) */
#define RCF_KILLPEASANTS   (1<<1)       /* a monster that eats peasants */
#define RCF_SCAREPEASANTS  (1<<2)       /* a monster that scares peasants out of the hex */
#define RCF_NOSTEAL        (1<<3)       /* this race has high stealth, but is not allowed to steal */
#define RCF_MOVERANDOM     (1<<4)
#define RCF_CANNOTMOVE     (1<<5)
#define RCF_AI_LEARN       (1<<6)       /* Lernt automatisch wenn struct faction == 0 */
#define RCF_FLY            (1<<7)       /* kann fliegen */
#define RCF_SWIM           (1<<8)       /* kann schwimmen */
#define RCF_WALK           (1<<9)       /* kann ueber Land gehen */
#define RCF_NOLEARN        (1<<10)      /* kann nicht normal lernen */
#define RCF_NOTEACH        (1<<11)      /* kann nicht lehren */
#define RCF_HORSE          (1<<12)      /* Einheit ist Pferd, sozusagen */
#define RCF_DESERT         (1<<13)      /* 5% Chance, das Einheit desertiert */
#define RCF_ILLUSIONARY    (1<<14)      /* (Illusion & Spell) Does not drop items. */
#define RCF_ABSORBPEASANTS (1<<15)      /* Toetet und absorbiert Bauern */
#define RCF_NOHEAL         (1<<16)      /* Einheit kann nicht geheilt werden */
#define RCF_NOWEAPONS      (1<<17)      /* Einheit kann keine Waffen benutzen */
#define RCF_SHAPESHIFT     (1<<18)      /* Kann TARNE RASSE benutzen. */
#define RCF_UNDEAD         (1<<19)      /* Undead. */
#define RCF_DRAGON         (1<<20)      /* Drachenart (fuer Zauber) */
#define RCF_COASTAL        (1<<21)      /* kann in Landregionen an der Kueste sein */
#define RCF_UNARMEDGUARD   (1<<22)      /* kann ohne Waffen bewachen */
#define RCF_CANSAIL        (1<<23)      /* Einheit darf Schiffe betreten */
#define RCF_FAMILIAR       (1<<24)      /* may be a familiar */
#define RCF_SHIPSPEED      (1<<25)      /* race gets +1 on shipspeed */
#define RCF_MIGRANTS       (1<<26)      /* may have migrant units (human bonus) */
#define RCF_ATTACK_MOVED   (1<<27)      /* may attack if it has moved */

#define RCF_DEFAULT (RCF_CANSAIL|RCF_AI_LEARN)

/* Economic flags */
#define ECF_GIVEPERSON     (1<<0)      /* Uebergibt Personen */
#define ECF_GIVEUNIT       (1<<1)      /* Einheiten an andere Partei uebergeben */
#define ECF_GETITEM        (1<<2)      /* nimmt Gegenstaende an */
#define ECF_REC_ETHEREAL   (1<<3)      /* Rekrutiert aus dem Nichts */
#define ECF_REC_UNLIMITED  (1<<4)      /* Rekrutiert ohne Limit */
#define ECF_STONEGOLEM     (1<<5)      /* race gets stonegolem properties */
#define ECF_IRONGOLEM      (1<<6)      /* race gets irongolem properties */

/* Battle-Flags */
#define BF_EQUIPMENT    (1<<0)         /* Kann Ausruestung benutzen */
#define BF_NOBLOCK      (1<<1)         /* Wird in die Rueckzugsberechnung nicht einbezogen */
#define BF_RES_PIERCE   (1<<2)         /* Halber Schaden durch PIERCE */
#define BF_RES_CUT      (1<<3)         /* Halber Schaden durch CUT */
#define BF_RES_BASH     (1<<4)         /* Halber Schaden durch BASH */
#define BF_INV_NONMAGIC (1<<5)         /* Immun gegen nichtmagischen Schaden */
#define BF_NO_ATTACK    (1<<6)         /* Kann keine ATTACKIERE Befehle ausfuehren */

const char *racename(const struct locale *lang, const struct unit *u,
    const race * rc);

#define playerrace(rc) ((rc)->flags & RCF_PLAYABLE)
#define stealthrace(rc) (((rc)->flags & (RCF_PLAYABLE|RCF_NOLEARN)) == RCF_PLAYABLE)
#define dragonrace(rc) ((rc)->flags & RCF_DRAGON)
#define humanoidrace(rc) (((rc)->flags & RCF_UNDEAD) || (rc)==get_race(RC_DRACOID) || playerrace(rc))
#define undeadrace(rc) (((rc)->flags & RCF_UNDEAD) || (rc)==get_race(RC_DRACOID))
#define illusionaryrace(rc) ((rc)->flags & RCF_ILLUSIONARY)

bool allowed_dragon(const struct region *src,
    const struct region *target);

bool r_insectstalled(const struct region *r);

void write_race_reference(const struct race *rc,
    struct storage *store);
struct race *read_race_reference(struct storage *store);

const char *raceprefix(const struct unit *u);
void register_race_function(race_func, const char *);

void set_study_speed(struct race *rc, skill_t sk, int modifier);
