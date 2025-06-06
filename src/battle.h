#pragma once

#include "kernel/status.h"
#include <stdbool.h>
#include <stdint.h>

struct message;
struct weapon_type;
union variant;

#define TACTICS_BONUS 1         /* when undefined, we have a tactics round. else this is the bonus tactics give */
#define TACTICS_MODIFIER 1      /* modifier for generals in the front/rear */

/** more defines **/
#define FS_ENEMY 1
#define FS_HELP  2

/***** Verteidigungslinien.
* Eressea hat 4 Verteidigungslinien. 1 ist vorn, 5. enthaelt Summen
*/
#define NUMROWS 5
#define SUM_ROW 0
#define FIGHT_ROW 1
#define BEHIND_ROW 2
#define AVOID_ROW 3
#define FLEE_ROW 4
#define FIRST_ROW FIGHT_ROW
#define LAST_ROW FLEE_ROW

/*** fighter::person::flags ***/
#define FL_TIRED      1
#define FL_DAZZLED  2           /* durch Untote oder Daemonen eingeschuechtert */
#define FL_PANICED  4
#define FL_COURAGE  8           /* Helden fliehen nie */
#define FL_SLEEPING 16
#define FL_STUNNED    32      /* eine Runde keinen Angriff */
#define FL_HIT        64      /* the person at attacked */
#define FL_HEALING_USED 128   /* has used a healing potion */

typedef struct tactics {
    struct fighter** fighters;
    int value;
} tactics;

#define SIDE_STEALTH   1<<0
#define SIDE_HASGUARDS 1<<1
#define SIDE_ATTACKER  1<<2

#define E_ENEMY 1
#define E_FRIEND 2
#define E_ATTACKING 4

typedef struct side {
    struct battle* battle;
    struct faction* faction;        /* battle info that goes with the faction */
    const struct group* group;
    struct tactics leader;      /* this army's best tactician */
    struct fighter* fighters;
    unsigned int index;         /* Eintrag der Fraktion in b->matrix/b->enemies */
    int size[NUMROWS];          /* Anzahl Personen in Reihe X. 0 = Summe */
    int nonblockers[NUMROWS];   /* Anzahl nichtblockierender Kaempfer, z.B. Schattenritter. */
    int alive;                  /* Die Partei hat den Kampf verlassen */
    int removed;                /* stoned */
    int flee;
    int dead;
    int casualties;             /* those dead that were real people, not undead! */
    int healed;
    unsigned int flags;
    const struct faction* stealthfaction;
} side;

typedef int32_t relation_key_t;
typedef char relation_value_t;
typedef struct relation {
    relation_key_t key;
    relation_value_t value;
} relation;

typedef struct battle {
    struct region* region;
    struct plane* plane;
    struct faction** factions; /* stb array */
    struct relation *relations;
    int nfactions;
    int nfighters;
    side ** sides;
    struct meffect* meffects;
    int max_tactics;
    unsigned char turn;
    signed char keeploot; /* keep (50 + keeploot) percent of items as loot */
    bool has_tactics_turn;
    bool reelarrow;
} battle;

typedef struct weapon {
    union {
        struct item *ref;
        const struct item_type *type;
    } item;
    int attackskill;
    int defenseskill;
} weapon;

#define WEAPON_TYPE(wp) ((wp && (wp)->item.type) ? (wp)->item.type->rtype->wtype : NULL)

typedef struct troop {
    struct fighter* fighter;
    int index;
} troop;

typedef struct armor {
    struct armor* next; /* TODO: make this an array, not a list, like weapon */
    const struct armor_type* atype;
    int count;
} armor;

/*** fighter::flags ***/
#define FIG_ATTACKER   1<<0
#define FIG_NOLOOT     1<<1

typedef struct fighter {
    struct fighter* next;
    struct side* side;
    struct unit* unit;          /* Die Einheit, die hier kaempft */
    struct building* building;  /* Gebaeude, in dem die Einheit evtl. steht */
    status_t status;            /* Kampfstatus */
    struct weapon* weapons;
    struct armor* armors;       /* Anzahl Ruestungen jeden Typs */
    int alive;                  /* Anzahl der noch nicht Toten in der Einheit */
    int fighting;               /* Anzahl der Kaempfer in der aktuellen Runde */
    int removed;                /* Anzahl Kaempfer, die nicht tot sind, aber
                                   aus dem Kampf raus sind (zB weil sie
                                   versteinert wurden).  Diese werden auch
                                   in alive noch mitgezaehlt! */
    int magic;                  /* Magietalent der Einheit  */
    int horses;                 /* Anzahl brauchbarer Pferde der Einheit */
    int elvenhorses;            /* Anzahl brauchbarer Elfenpferde der Einheit */
    struct item* loot;
    struct {
        int attacks;
        int kills;
    } special;
    struct person {
        int hp;                   /* Trefferpunkte der Personen */
        int flags;
        int attack;  /* weapon skill bonus for attacker */
        int defense; /* weapon skill bonus for defender */
        char damage;  /* bonus damage for melee attacks (e.g. troll belt) */
        unsigned char speed;
        unsigned char reload;
        unsigned char last_action;
        const struct weapon* missile;   /* missile weapon */
        const struct weapon* melee;     /* melee weapon */
    } *person;
    unsigned int flags;
    struct {
        int number;               /* number of people who fled */
        int hp;                   /* accumulated hp of fleeing people */
    } run;
    int kills;
    int hits;
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
    fighter* magician;          /* Der Zauberer, der den Schild gezaubert hat */
    int typ;                    /* Wirkungsweise des Schilds */
    int effect;
    int duration;
} meffect;

extern const troop no_troop;

/* BEGIN battle interface */
side* find_side(battle* b, const struct faction* f, const struct group* g, unsigned int flags, const struct faction* stealthfaction);
side* get_side(battle* b, const struct unit* u);
/* END battle interface */

void do_battles(void);

/* for combat spells and special attacks */
enum { SELECT_ADVANCE = 0x1, SELECT_DISTANCE = 0x2, SELECT_FIND = 0x4 };
enum { ALLY_SELF, ALLY_ANY };

int get_unitrow(const fighter* af, const side* vs);

troop select_enemy(struct fighter* af, int minrow, int maxrow,
    int select);
troop select_ally(struct fighter* af, int minrow, int maxrow,
    int allytype);
int get_tactics(const struct side* as, const struct side* ds);

int count_enemies(struct battle* b, const struct fighter* af,
    int minrow, int maxrow, int select);
int natural_armor(struct unit* u);
const struct armor_type* select_armor(struct troop t, bool shield);
const struct weapon* select_weapon(const struct troop t, bool attacking, bool ismissile);
int calculate_armor(troop dt, const struct weapon_type* dwtype, const struct weapon_type* awtype, const struct armor_type* armor, const struct armor_type* shield, bool magic);
int apply_resistance(int damage, struct troop dt, const struct weapon_type* dwtype, const struct armor_type* armor, const struct armor_type* shield, bool magic);
bool terminate(troop dt, troop at, int type, const char* damage,
    bool missile);
void battle_add_effect(fighter *af, int typ, int effect, int duration);
void message_all(struct battle* b, struct message* m);
void set_enemy(struct side* as, struct side* ds, bool attacking);
bool hits(troop at, troop dt, const struct weapon_type *awp);
void damage_building(struct battle* b, struct building* bldg,
    int damage_abs);

typedef bool(*select_fun)(const struct side* vs, const struct fighter* fig, void* cbdata);
struct fighter **select_fighters(struct battle *b, const struct side *vs, int mask, select_fun cb, void *cbdata);
struct fighter **fighters(struct battle* b, const struct side* vs, int minrow, int maxrow, int mask);
void flee_all(struct fighter *fig);

int count_allies(const struct side* as, int minrow, int maxrow,
    int select, int allytype);
bool helping(const struct side* as, const struct side* ds);
void reduce_fighter(fighter* df, int i);
struct fighter* select_corpse(struct battle* b, struct fighter* af);
int statusrow(int status);
void drain_exp(struct unit* u, int d);
void kill_troop(troop dt);
void remove_troop(troop dt);   /* not the same as the badly named rmtroop */

bool is_attacker(const fighter* fig);
struct battle* make_battle(struct region* r);
bool start_battle(struct region* r, struct battle** bp);
void join_allies(struct battle *b);
void free_battle(struct battle* b);
void init_tactics(struct battle* b);
struct fighter* make_fighter(struct battle* b, struct unit* u,
    struct side* s, bool attack);
bool join_battle(struct battle* b, struct unit* u, bool attack, struct fighter** cp);
struct side* make_side(struct battle* b, const struct faction* f,
    const struct group* g, unsigned int flags,
    const struct faction* stealthfaction);
int skilldiff(troop at, troop dt, int dist);
void force_leave(struct region* r, struct battle* b);
bool seematrix(const struct faction* f, const struct side* s);
const char *sidename(const struct side *s, const struct faction *f);
void battle_message_faction(struct battle* b, struct faction* f, struct message* m);

double tactics_chance(const struct unit* u, int skilldiff);
int meffect_apply(struct meffect* me, int damage);

void loot_items(fighter* corpse);
void structural_damage(troop td, int damage_abs, int changce_pct);

void set_relation(struct side *as, const struct side *ds, relation_value_t mask);
relation_value_t get_relation(const struct side *as, const struct side *ds);
