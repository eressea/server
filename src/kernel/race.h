/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef H_KRNL_RACE_H
#define H_KRNL_RACE_H
#include <stddef.h>
#include "magic.h"              /* wegen MAXMAGIETYP */
#include "skill.h"

#ifdef __cplusplus
extern "C" {
#endif

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

    struct param;
    struct spell;
    struct spellref;
    struct locale;

    extern int num_races;

    typedef enum {
        RC_DWARF,                     /* 0 - Zwerg */
        RC_ELF,
        RC_GOBLIN = 3,
        RC_HUMAN,
        RC_TROLL,
        RC_DAEMON,
        RC_INSECT,
        RC_HALFLING,
        RC_CAT,
        RC_AQUARIAN,
        RC_ORC,
        RC_SNOTLING,
        RC_UNDEAD,
        RC_ILLUSION,
        RC_FIREDRAGON,
        RC_DRAGON,
        RC_WYRM,
        RC_TREEMAN,
        RC_BIRTHDAYDRAGON,
        RC_DRACOID,

        RC_SPELL = 22,
        RC_IRONGOLEM,
        RC_STONEGOLEM,
        RC_SHADOW,
        RC_SHADOWLORD,
        RC_IRONKEEPER,
        RC_ALP,
        RC_TOAD,
        RC_HIRNTOETER,
        RC_PEASANT,
        RC_WOLF = 32,

        RC_SONGDRAGON = 37,

        RC_SEASERPENT = 51,
        RC_SHADOWKNIGHT,

        RC_SKELETON = 54,
        RC_SKELETON_LORD,
        RC_ZOMBIE,
        RC_ZOMBIE_LORD,
        RC_GHOUL,
        RC_GHOUL_LORD,
        RC_TEMPLATE = 62,
        RC_CLONE,
        MAXRACES,
        NORACE = -1
    } race_t;

    typedef struct att {
        int type;
        union {
            const char *dice;
            struct spellref *sp;
        } data;
        int flags;
        int level;
    } att;

    typedef const char *(*race_desc_func)(const struct race *rc, const struct locale *lang);
    typedef void (*race_name_func)(struct unit *);

    typedef struct race {
        char *_name;
        int magres;
        int healing;
        int maxaura;            /* Faktor auf Maximale Aura */
        double regaura;            /* Faktor auf Regeneration */
        double recruit_multi;      /* Faktor f�r Bauernverbrauch */
        int index;
        int recruitcost;
        int maintenance;
        int splitsize;
        int weight;
        int capacity;
        int income;
        float speed;
        float aggression;           /* chance that a monster will attack */
        int hitpoints;
        char *def_damage;
        int armor;
        int at_default;             /* Angriffsskill Unbewaffnet (default: -2) */
        int df_default;             /* Verteidigungsskill Unbewaffnet (default: -2) */
        int at_bonus;               /* Ver�ndert den Angriffsskill (default: 0) */
        int df_bonus;               /* Ver�ndert den Verteidigungskill (default: 0) */
        struct param *parameters;   // additional properties, for an example see natural_armor
        struct spellref *precombatspell;
        signed char *study_speed;   /* study-speed-bonus in points/turn (0=30 Tage) */
        int flags;
        int battle_flags;
        int ec_flags;
        struct att attack[RACE_ATTACKS];
        signed char bonus[MAXSKILLS];

        race_name_func generate_name;
        race_desc_func describe;
        void(*age) (struct unit * u);
        bool(*move_allowed) (const struct region *, const struct region *);
        struct item *(*itemdrop) (const struct race *, int size);
        void(*init_familiar) (struct unit *);

        const struct race *familiars[MAXMAGIETYP];
        struct attrib *attribs;
        struct race *next;
    } race;

    typedef struct race_list {
        struct race_list *next;
        const struct race *data;
    } race_list;

    void racelist_clear(struct race_list **rl);
    void racelist_insert(struct race_list **rl, const struct race *r);

    const struct race *findrace(const char *, const struct locale *);

    struct race_list *get_familiarraces(void);
    struct race *races;
    const struct race *get_race(race_t rt);
    /** TODO: compatibility hacks: **/
    race_t old_race(const struct race *);

    race *rc_create(const char *zName);
    race *rc_get_or_create(const char *name);
    bool rc_changed(int *cache);
    const race *rc_find(const char *);
    void free_races(void);

    typedef enum name_t { NAME_SINGULAR, NAME_PLURAL, NAME_DEFINITIVE, NAME_CATEGORY } name_t;
    const char * rc_name_s(const race *rc, name_t n);
    const char * rc_name(const race *rc, name_t n, char *name, size_t size);

    void rc_set_param(struct race *rc, const char *key, const char *value);

    double rc_magres(const struct race *rc);
    double rc_maxaura(const struct race *rc);
    int rc_armor_bonus(const struct race *rc);
    int rc_scare(const struct race *rc);

#define MIGRANTS_NONE 0
#define MIGRANTS_LOG10 1
    int rc_migrants_formula(const race *rc);

    /* Flags. Do not reorder these without changing json_race() in jsonconf.c */
#define RCF_NPC            (1<<0)   /* cannot be the race for a player faction (and other limits?) */
#define RCF_KILLPEASANTS   (1<<1)   /* a monster that eats peasants */
#define RCF_SCAREPEASANTS  (1<<2)   /* a monster that scares peasants out of the hex */
#define RCF_NOSTEAL        (1<<3)   /* this race has high stealth, but is not allowed to steal */
#define RCF_MOVERANDOM     (1<<4)
#define RCF_CANNOTMOVE     (1<<5)
#define RCF_LEARN          (1<<6)       /* Lernt automatisch wenn struct faction == 0 */
#define RCF_FLY            (1<<7)       /* kann fliegen */
#define RCF_SWIM           (1<<8)       /* kann schwimmen */
#define RCF_WALK           (1<<9)       /* kann �ber Land gehen */
#define RCF_NOLEARN        (1<<10)      /* kann nicht normal lernen */
#define RCF_NOTEACH        (1<<11)      /* kann nicht lehren */
#define RCF_HORSE          (1<<12)      /* Einheit ist Pferd, sozusagen */
#define RCF_DESERT         (1<<13)      /* 5% Chance, das Einheit desertiert */
#define RCF_ILLUSIONARY    (1<<14)      /* (Illusion & Spell) Does not drop items. */
#define RCF_ABSORBPEASANTS (1<<15)      /* T�tet und absorbiert Bauern */
#define RCF_NOHEAL         (1<<16)      /* Einheit kann nicht geheilt werden */
#define RCF_NOWEAPONS      (1<<17)      /* Einheit kann keine Waffen benutzen */
#define RCF_SHAPESHIFT     (1<<18)      /* Kann TARNE RASSE benutzen. */
#define RCF_SHAPESHIFTANY  (1<<19)      /* Kann TARNE RASSE "string" benutzen. */
#define RCF_UNDEAD         (1<<20)      /* Undead. */
#define RCF_DRAGON         (1<<21)      /* Drachenart (f�r Zauber) */
#define RCF_COASTAL        (1<<22)      /* kann in Landregionen an der K�ste sein */
#define RCF_UNARMEDGUARD   (1<<23)      /* kann ohne Waffen bewachen */
#define RCF_CANSAIL        (1<<24)      /* Einheit darf Schiffe betreten */
#define RCF_INVISIBLE      (1<<25)      /* not visible in any report */
#define RCF_SHIPSPEED      (1<<26)      /* race gets +1 on shipspeed */
#define RCF_STONEGOLEM     (1<<27)      /* race gets stonegolem properties */
#define RCF_IRONGOLEM      (1<<28)      /* race gets irongolem properties */
#define RCF_ATTACK_MOVED   (1<<29)      /* may attack if it has moved */
#define RCF_MIGRANTS       (1<<30)      /* may have migrant units (human bonus) */

    /* Economic flags */
#define ECF_KEEP_ITEM       (1<<1)   /* gibt Gegenst�nde weg */
#define GIVEPERSON     (1<<2)   /* �bergibt Personen */
#define GIVEUNIT       (1<<3)   /* Einheiten an andere Partei �bergeben */
#define GETITEM        (1<<4)   /* nimmt Gegenst�nde an */
#define ECF_REC_ETHEREAL   (1<<7)       /* Rekrutiert aus dem Nichts */
#define ECF_REC_UNLIMITED  (1<<8)       /* Rekrutiert ohne Limit */

    /* Battle-Flags */
#define BF_EQUIPMENT    (1<<0)  /* Kann Ausr�stung benutzen */
#define BF_NOBLOCK      (1<<1)  /* Wird in die R�ckzugsberechnung nicht einbezogen */
#define BF_RES_PIERCE   (1<<2)  /* Halber Schaden durch PIERCE */
#define BF_RES_CUT      (1<<3)  /* Halber Schaden durch CUT */
#define BF_RES_BASH     (1<<4)  /* Halber Schaden durch BASH */
#define BF_INV_NONMAGIC (1<<5)  /* Immun gegen nichtmagischen Schaden */
#define BF_NO_ATTACK    (1<<6)  /* Kann keine ATTACKIERE Befehle ausfuehren */

    const char *racename(const struct locale *lang, const struct unit *u,
        const race * rc);

#define playerrace(rc) (!((rc)->flags & RCF_NPC))
#define dragonrace(rc) ((rc)->flags & RCF_DRAGON)
#define humanoidrace(rc) (((rc)->flags & RCF_UNDEAD) || (rc)==get_race(RC_DRACOID) || playerrace(rc))
#define illusionaryrace(rc) ((rc)->flags & RCF_ILLUSIONARY)

    bool allowed_dragon(const struct region *src,
        const struct region *target);

    bool r_insectstalled(const struct region *r);

    void write_race_reference(const struct race *rc,
        struct storage *store);
    variant read_race_reference(struct storage *store);

    const char *raceprefix(const struct unit *u);
    void register_race_name_function(race_name_func, const char *);
    void register_race_description_function(race_desc_func, const char *);

#ifdef __cplusplus
}
#endif
#endif
