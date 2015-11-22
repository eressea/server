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

#ifndef ERESSEA_H
#define ERESSEA_H

#ifdef __cplusplus
extern "C" {
#endif

    /* this should always be the first thing included after platform.h */
#include "types.h"

    /* experimental gameplay features (that don't affect the savefile) */
    /* TODO: move these settings to settings.h or into configuration files */
#define GOBLINKILL              /* Goblin-Spezialklau kann tödlich enden */
#define HERBS_ROT               /* herbs owned by units have a chance to rot. */
#define INSECT_POTION           /* Spezialtrank für Insekten */
#define ORCIFICATION            /* giving snotlings to the peasants gets counted */

    /* for some good prime numbers, check http://www.math.niu.edu/~rusin/known-math/98/pi_x */
#ifndef MAXREGIONS
# define MAXREGIONS 524287      /* must be prime for hashing. 262139 was a little small */
#endif
#ifndef MAXUNITS
# define MAXUNITS 1048573       /* must be prime for hashing. 524287 was >90% full */
#endif
#define MAXLUXURIES 16 /* there must be no more than MAXLUXURIES kinds of luxury goods in any game */

#define TREESIZE (8)            /* space used by trees (in #peasants) */

#define PEASANTFORCE 0.75       /* Chance einer Vermehrung trotz 90% Auslastung */
#define HERBROTCHANCE 5         /* Verrottchance für Kräuter (ifdef HERBS_ROT) */

    /* Gebäudegröße = Minimalbelagerer */
#define SIEGEFACTOR     2

    /** Magic */
#define MAXMAGICIANS    3
#define MAXALCHEMISTS   3

    /* getunit results: */
#define GET_UNIT 0
#define GET_NOTFOUND 1
#define GET_PEASANTS 2
    /* Bewegungsweiten: */
#define BP_WALKING 4
#define BP_RIDING  6
#define BP_UNICORN 9
#define BP_DRAGON  4

#define BP_NORMAL 3
#define BP_ROAD   2

#define PERSON_WEIGHT 1000      /* weight of a "normal" human unit */
#define STAMINA_AFFECTS_HP 1<<0

    /**
     * Hier endet der Teil von config.h, der die defines für die
     * Spielwelt Eressea enthält, und beginnen die allgemeinen Routinen
     */

#define ENCCHANCE           10  /* %-Chance für einmalige Zufallsbegegnung */

#define DISPLAYSIZE         8192        /* max. Länge einer Beschreibung, incl trailing 0 */
#define ORDERSIZE           (DISPLAYSIZE*2) /* max. length of an order */
#define NAMESIZE            128 /* max. Länge eines Namens, incl trailing 0 */
#define IDSIZE              16  /* max. Länge einer no (als String), incl trailing 0 */
#define OBJECTIDSIZE        (NAMESIZE+5+IDSIZE) /* max. Länge der Strings, die
     * von struct unitname, etc. zurückgegeben werden. ohne die 0 */

#define BAGCAPACITY         20000   /* soviel paßt in einen Bag of Holding */

    /* ----------------- Befehle ----------------------------------- */

#define want(option) (1<<option)
    /* ------------------------------------------------------------- */

#define i2b(i) ((bool)((i)?(true):(false)))

#define fval(u, i) ((u)->flags & (i))
#define fset(u, i) ((u)->flags |= (i))
#define freset(u, i) ((u)->flags &= ~(i))

    /* parteinummern */
    bool faction_id_is_unused(int);

    int max_magicians(const struct faction * f);
    int findoption(const char *s, const struct locale *lang);

    param_t findparam(const char *s, const struct locale *lang);
    param_t findparam_ex(const char *s, const struct locale * lang);
    bool isparam(const char *s, const struct locale * lang, param_t param);
    param_t getparam(const struct locale *lang);

#define unitid(x) itoa36((x)->no)

#define buildingid(x) itoa36((x)->no)
#define shipid(x) itoa36((x)->no)
#define factionid(x) itoa36((x)->no)
#define curseid(x) itoa36((x)->no)

    const char * game_name(void);
    int game_id(void);
    int lovar(double xpct_x2);
    /* returns a value between [0..xpct_2], generated with two dice */

    int distribute(int old, int new_value, int n);
    void init_locale(struct locale *lang);

    int newunitid(void);
    int forbiddenid(int id);
    int newcontainerid(void);

    int getunit(const struct region * r, const struct faction * f, struct unit **uresult);

    int read_unitid(const struct faction *f, const struct region *r);

    int alliedunit(const struct unit *u, const struct faction *f2,
        int mode);
    int alliedfaction(const struct plane *pl, const struct faction *f,
        const struct faction *f2, int mode);
    int alliedgroup(const struct plane *pl, const struct faction *f,
        const struct faction *f2, const struct ally *sf, int mode);

    struct faction *getfaction(void);

    char *untilde(char *s);

    typedef int(*cmp_building_cb) (const struct building * b,
        const struct building * a);
    struct building *largestbuilding(const struct region *r, cmp_building_cb,
        bool imaginary);
    int cmp_wage(const struct building *b, const struct building *bother);
    int cmp_taxes(const struct building *b, const struct building *bother);
    int cmp_current_owner(const struct building *b,
        const struct building *bother);

    bool rule_region_owners(void);
    bool rule_stealth_other(void); // units can pretend to be another faction, TARNE PARTEI <no>
    bool rule_stealth_anon(void);  // units can anonymize their faction, TARNE PARTEI [NICHT]
    int rule_alliance_limit(void);
    int rule_faction_limit(void);
#define HARVEST_WORK  0x00
#define HARVEST_TAXES 0x01
    int rule_blessed_harvest(void);
#define GIVE_SELF 1
#define GIVE_PEASANTS 2
#define GIVE_LUXURIES 4
#define GIVE_HERBS 8
#define GIVE_GOODS 16
#define GIVE_ONDEATH 32
#define GIVE_ALLITEMS (GIVE_GOODS|GIVE_HERBS|GIVE_LUXURIES)
#define GIVE_DEFAULT (GIVE_SELF|GIVE_PEASANTS|GIVE_LUXURIES|GIVE_HERBS|GIVE_GOODS)
    int rule_give(void);

#define COUNT_MONSTERS 0x01
#define COUNT_MIGRANTS 0x02
#define COUNT_DEFAULT  0x04
#define COUNT_ALL      0x07
#define COUNT_UNITS    0x10

    int count_faction(const struct faction * f, int flags);
    int count_migrants(const struct faction * f);
    int count_maxmigrants(const struct faction * f);
    int count_all(const struct faction * f);
    int count_units(const struct faction * f);

    bool has_limited_skills(const struct unit *u);
    const struct race *findrace(const char *, const struct locale *);

    bool unit_has_cursed_item(const struct unit *u);

    /* grammatik-flags: */
#define GF_NONE 0
    /* singular, ohne was dran */
#define GF_PLURAL 1
    /* Angaben in Mehrzahl */
#define GF_ARTICLE 8
    /* der, die, eine */
#define GF_SPECIFIC 16
    /* der, die, das vs. ein, eine */
#define GF_DETAILED 32
    /* mehr Informationen. z.b. straße zu 50% */
#define GF_PURE 64
    /* untranslated */

#define GUARD_NONE 0
#define GUARD_TAX 1
    /* Verhindert Steuereintreiben */
#define GUARD_MINING 2
    /* Verhindert Bergbau */
#define GUARD_TREES 4
    /* Verhindert Waldarbeiten */
#define GUARD_TRAVELTHRU 8
    /* Blockiert Durchreisende */
#define GUARD_LANDING 16
    /* Verhindert Ausstieg + Weiterreise */
#define GUARD_CREWS 32
    /* Verhindert Unterhaltung auf Schiffen */
#define GUARD_RECRUIT 64
    /* Verhindert Rekrutieren */
#define GUARD_PRODUCE 128
    /* Verhindert Abbau von Resourcen mit RTF_LIMITED */
#define GUARD_ALL 0xFFFF

    void setstatus(struct unit *u, int status);
    /* !< sets combatstatus of a unit */
    int besieged(const struct unit *u);
    int maxworkingpeasants(const struct region *r);
    bool has_horses(const struct unit *u);
    bool markets_module(void);
    int wage(const struct region *r, const struct faction *f,
        const struct race *rc, int in_turn);
    int maintenance_cost(const struct unit *u);

    const char *datapath(void);
    void set_datapath(const char *path);

    const char *basepath(void);
    void set_basepath(const char *);

    const char *reportpath(void);
    void set_reportpath(const char *);

    void kernel_init(void);
    void kernel_done(void);

    /* globale settings des Spieles */
    typedef struct settings {
        const char *gamename;
        struct attrib *attribs;
        unsigned int data_turn;
        struct param *parameters;
        void *vm_state;
        int data_version; /* TODO: eliminate in favor of gamedata.version */
        struct _dictionary_ *inifile;

        struct global_functions {
            int(*wage) (const struct region * r, const struct faction * f,
                const struct race * rc, int in_turn);
            int(*maintenance) (const struct unit * u);
        } functions;
        /* the following are some cached values, because get_param can be slow.
         * you should almost never need to touch them */
        int cookie;
    } settings;

    typedef struct helpmode {
        const char *name;
        int status;
    } helpmode;

    const char *dbrace(const struct race *rc);

    void set_param(struct param **p, const char *key, const char *data);
    const char *get_param(const struct param *p, const char *key);
    int get_param_int(const struct param *p, const char *key, int def);
    int check_param(const struct param *p, const char *key, const char *searchvalue);
    double get_param_flt(const struct param *p, const char *key, double def);
    void free_params(struct param **pp);

    bool ExpensiveMigrants(void);
    int NMRTimeout(void);
    int LongHunger(const struct unit *u);
    int NewbieImmunity(void);
    bool IsImmune(const struct faction *f);
    int AllianceAuto(void);        /* flags that allied factions get automatically */
    int AllianceRestricted(void);  /* flags restricted to allied factions */
    int HelpMask(void);    /* flags restricted to allied factions */

    struct order *default_order(const struct locale *lang);
    void set_default_order(int kwd);

    int entertainmoney(const struct region *r);
    void init_parameters(struct locale *lang);

    void free_gamedata(void);

    extern struct helpmode helpmodes[];
    extern const char *parameters[];
    extern const char *localenames[];
    extern settings global;

    extern bool battledebug;
    extern bool sqlpatch;
    extern bool lomem;         /* save memory */
    extern int turn;
    extern bool getunitpeasants;

    extern const char *options[MAXOPTIONS];    /* report options */

#ifdef __cplusplus
}
#endif
#endif
