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
#ifdef __cplusplus
extern "C" {
#endif

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

    struct param;
    struct spell;

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
        RC_SPECIAL,
        RC_SPELL,
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
        RC_CENTAUR,
        RC_SKELETON,

        RC_SKELETON_LORD,
        RC_ZOMBIE,
        RC_ZOMBIE_LORD,
        RC_GHOUL,
        RC_GHOUL_LORD,

        RC_MUS_SPIRIT,
        RC_GNOME,
        RC_TEMPLATE,
        RC_CLONE,

        MAXRACES,
        NORACE = -1
    } race_t;

    typedef struct att {
        int type;
        union {
            const char *dice;
            const struct spell *sp;
        } data;
        int flags;
        int level;
    } att;

    extern int num_races;

    typedef struct race {
        char *_name;
        float magres;
        double maxaura;            /* Faktor auf Maximale Aura */
        double regaura;            /* Faktor auf Regeneration */
        double recruit_multi;      /* Faktor für Bauernverbrauch */
        int index;
        int recruitcost;
        int maintenance;
        int splitsize;
        int weight;
        int capacity;
        float speed;
        float aggression;           /* chance that a monster will attack */
        int hitpoints;
        char *def_damage;
        int armor;
        int at_default;             /* Angriffsskill Unbewaffnet (default: -2) */
        int df_default;             /* Verteidigungsskill Unbewaffnet (default: -2) */
        int at_bonus;               /* Verändert den Angriffsskill (default: 0) */
        int df_bonus;               /* Verändert den Verteidigungskill (default: 0) */
        struct param *parameters;   // additional properties, for an example see natural_armor
        const struct spell *precombatspell;
        signed char *study_speed;   /* study-speed-bonus in points/turn (0=30 Tage) */
        int flags;
        int battle_flags;
        int ec_flags;
        struct att attack[RACE_ATTACKS];
        signed char bonus[MAXSKILLS];

        const char *(*generate_name) (const struct unit *);
        const char *(*describe) (const struct unit *, const struct locale *);
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

    extern void racelist_clear(struct race_list **rl);
    extern void racelist_insert(struct race_list **rl, const struct race *r);


    struct race_list *get_familiarraces(void);
    struct race *races;
    struct race *get_race(race_t rt);
    /** TODO: compatibility hacks: **/
    race_t old_race(const struct race *);

    extern race *rc_get_or_create(const char *name);
    extern const race *rc_find(const char *);
    void free_races(void);

    typedef enum name_t { NAME_SINGULAR, NAME_PLURAL, NAME_DEFINITIVE, NAME_CATEGORY } name_t;
    const char * rc_name_s(const race *rc, name_t n);
    const char * rc_name(const race *rc, name_t n, char *name, size_t size);

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
#define RCF_WALK           (1<<9)       /* kann über Land gehen */
#define RCF_NOLEARN        (1<<10)      /* kann nicht normal lernen */
#define RCF_NOTEACH        (1<<11)      /* kann nicht lehren */
#define RCF_HORSE          (1<<12)      /* Einheit ist Pferd, sozusagen */
#define RCF_DESERT         (1<<13)      /* 5% Chance, das Einheit desertiert */
#define RCF_ILLUSIONARY    (1<<14)      /* (Illusion & Spell) Does not drop items. */
#define RCF_ABSORBPEASANTS (1<<15)      /* Tötet und absorbiert Bauern */
#define RCF_NOHEAL         (1<<16)      /* Einheit kann nicht geheilt werden */
#define RCF_NOWEAPONS      (1<<17)      /* Einheit kann keine Waffen benutzen */
#define RCF_SHAPESHIFT     (1<<18)      /* Kann TARNE RASSE benutzen. */
#define RCF_SHAPESHIFTANY  (1<<19)      /* Kann TARNE RASSE "string" benutzen. */
#define RCF_UNDEAD         (1<<20)      /* Undead. */
#define RCF_DRAGON         (1<<21)      /* Drachenart (für Zauber) */
#define RCF_COASTAL        (1<<22)      /* kann in Landregionen an der Küste sein */
#define RCF_UNARMEDGUARD   (1<<23)      /* kann ohne Waffen bewachen */
#define RCF_CANSAIL        (1<<24)      /* Einheit darf Schiffe betreten */
#define RCF_INVISIBLE      (1<<25)      /* not visible in any report */
#define RCF_SHIPSPEED      (1<<26)      /* race gets +1 on shipspeed */
#define RCF_STONEGOLEM     (1<<27)      /* race gets stonegolem properties */
#define RCF_IRONGOLEM      (1<<28)      /* race gets irongolem properties */
#define RCF_ATTACK_MOVED   (1<<29)      /* may attack if it has moved */

    /* Economic flags */
#define ECF_KEEP_ITEM       (1<<1)   /* gibt Gegenstände weg */
#define GIVEPERSON     (1<<2)   /* übergibt Personen */
#define GIVEUNIT       (1<<3)   /* Einheiten an andere Partei übergeben */
#define GETITEM        (1<<4)   /* nimmt Gegenstände an */
#define ECF_REC_HORSES     (1<<6)       /* Rekrutiert aus Pferden */
#define ECF_REC_ETHEREAL   (1<<7)       /* Rekrutiert aus dem Nichts */
#define ECF_REC_UNLIMITED  (1<<8)       /* Rekrutiert ohne Limit */

    /* Battle-Flags */
#define BF_EQUIPMENT    (1<<0)  /* Kann Ausrüstung benutzen */
#define BF_NOBLOCK      (1<<1)  /* Wird in die Rückzugsberechnung nicht einbezogen */
#define BF_RES_PIERCE   (1<<2)  /* Halber Schaden durch PIERCE */
#define BF_RES_CUT      (1<<3)  /* Halber Schaden durch CUT */
#define BF_RES_BASH     (1<<4)  /* Halber Schaden durch BASH */
#define BF_INV_NONMAGIC (1<<5)  /* Immun gegen nichtmagischen Schaden */
#define BF_NO_ATTACK    (1<<6)  /* Kann keine ATTACKIERE Befehle ausfuehren */

    int unit_old_max_hp(struct unit *u);
    const char *racename(const struct locale *lang, const struct unit *u,
        const race * rc);

#define omniscient(f) ((f)->race==get_race(RC_ILLUSION) || (f)->race==get_race(RC_TEMPLATE))

#define playerrace(rc) (!fval((rc), RCF_NPC))
#define dragonrace(rc) ((rc) == get_race(RC_FIREDRAGON) || (rc) == get_race(RC_DRAGON) || (rc) == get_race(RC_WYRM) || (rc) == get_race(RC_BIRTHDAYDRAGON))
#define humanoidrace(rc) (fval((rc), RCF_UNDEAD) || (rc)==get_race(RC_DRACOID) || playerrace(rc))
#define illusionaryrace(rc) (fval(rc, RCF_ILLUSIONARY))

    bool allowed_dragon(const struct region *src,
        const struct region *target);

    bool r_insectstalled(const struct region *r);

    void write_race_reference(const struct race *rc,
        struct storage *store);
    variant read_race_reference(struct storage *store);

    const char *raceprefix(const struct unit *u);

    void give_starting_equipment(const struct equipment *eq,
        struct unit *u);
    const char *dbrace(const struct race *rc);

#ifdef __cplusplus
}
#endif
#endif
