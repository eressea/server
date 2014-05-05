/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

  struct _dictionary_;

  /* experimental gameplay features (that don't affect the savefile) */
  /* TODO: move these settings to settings.h or into configuration files */
#define GOBLINKILL              /* Goblin-Spezialklau kann tödlich enden */
#define HERBS_ROT               /* herbs owned by units have a chance to rot. */
#define SHIPDAMAGE              /* Schiffsbeschädigungen */
#define INSECT_POTION           /* Spezialtrank für Insekten */
#define ORCIFICATION            /* giving snotlings to the peasants gets counted */

#define ALLIED(f1, f2) (f1==f2 || (f1->alliance && f1->alliance==f2->alliance))

/* for some good prime numbers, check http://www.math.niu.edu/~rusin/known-math/98/pi_x */
#ifndef MAXREGIONS
# define MAXREGIONS 524287      /* must be prime for hashing. 262139 was a little small */
#endif
#ifndef MAXUNITS
# define MAXUNITS 1048573       /* must be prime for hashing. 524287 was >90% full */
#endif

#define MAXPEASANTS_PER_AREA 10 /* number of peasants per region-size */
#define TREESIZE (MAXPEASANTS_PER_AREA-2)       /* space used by trees (in #peasants) */

#define PEASANTFORCE 0.75       /* Chance einer Vermehrung trotz 90% Auslastung */
#define HERBROTCHANCE 5         /* Verrottchance für Kräuter (ifdef HERBS_ROT) */

/* Gebäudegröße = Minimalbelagerer */
#define SIEGEFACTOR     2

/** Magic */
#define MAXMAGICIANS    3
#define MAXALCHEMISTS   3

/** Plagues **/
#define PLAGUE_CHANCE      0.1F /* Seuchenwahrscheinlichkeit (siehe plagues()) */
#define PLAGUE_VICTIMS     0.2F /* % Betroffene */
#define PLAGUE_HEALCHANCE  0.25F        /* Wahrscheinlichkeit Heilung */
#define PLAGUE_HEALCOST    30   /* Heilkosten */

/* Chance of a monster attack */
#define MONSTERATTACK  0.4F

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
#define NAMESIZE            128 /* max. Länge eines Namens, incl trailing 0 */
#define IDSIZE              16  /* max. Länge einer no (als String), incl trailing 0 */
#define KEYWORDSIZE         16  /* max. Länge eines Keyword, incl trailing 0 */
#define OBJECTIDSIZE        (NAMESIZE+5+IDSIZE) /* max. Länge der Strings, die
                                                 * von struct unitname, etc. zurückgegeben werden. ohne die 0 */

#define BAGCAPACITY		20000   /* soviel paßt in einen Bag of Holding */
#define STRENGTHCAPACITY	50000   /* zusätzliche Tragkraft beim Kraftzauber (deprecated) */
#define STRENGTHMULTIPLIER 50   /* multiplier for trollbelt */

/* ----------------- Befehle ----------------------------------- */

  extern const char *keywords[MAXKEYWORDS];
  extern const char *parameters[MAXPARAMS];

/** report options **/
#define want(option) (1<<option)
  extern const char *options[MAXOPTIONS];

/* ------------------------------------------------------------- */

  extern int shipspeed(const struct ship *sh, const struct unit *u);

#define i2b(i) ((bool)((i)?(true):(false)))

  void remove_empty_units_in_region(struct region *r);
  void remove_empty_units(void);
  void remove_empty_factions(void);

  typedef struct strlist {
    struct strlist *next;
    char *s;
  } strlist;

#define fval(u, i) ((u)->flags & (i))
#define fset(u, i) ((u)->flags |= (i))
#define freset(u, i) ((u)->flags &= ~(i))

  extern int turn;
  extern int verbosity;

/* parteinummern */
  extern bool faction_id_is_unused(int);

/* leuchtturm */
  extern bool check_leuchtturm(struct region *r, struct faction *f);
  extern void update_lighthouse(struct building *lh);
  extern int lighthouse_range(const struct building *b,
    const struct faction *f);

/* skills */
  extern int skill_limit(struct faction *f, skill_t sk);
  extern int count_skill(struct faction *f, skill_t sk);

/* direction, geography */
  extern const char *directions[];
  extern direction_t finddirection(const char *s, const struct locale *);

  extern int findoption(const char *s, const struct locale *lang);

/* special units */
  void make_undead_unit(struct unit *);

  void addstrlist(strlist ** SP, const char *s);

  int armedmen(const struct unit *u, bool siege_weapons);

  unsigned int atoip(const char *s);
  unsigned int getuint(void);
  int getint(void);

  direction_t getdirection(const struct locale *);

  extern const char *igetstrtoken(const char *s);

  extern skill_t findskill(const char *s, const struct locale *lang);

  extern keyword_t findkeyword(const char *s, const struct locale *lang);

  param_t findparam(const char *s, const struct locale *lang);
  param_t findparam_ex(const char *s, const struct locale * lang);
  bool isparam(const char *s, const struct locale * lang, param_t param);
  param_t getparam(const struct locale *lang);

  extern int getid(void);
#define unitid(x) itoa36((x)->no)

#define getshipid() getid()
#define getfactionid() getid()

#define buildingid(x) itoa36((x)->no)
#define shipid(x) itoa36((x)->no)
#define factionid(x) itoa36((x)->no)
#define curseid(x) itoa36((x)->no)

  extern bool cansee(const struct faction *f, const struct region *r,
    const struct unit *u, int modifier);
  bool cansee_durchgezogen(const struct faction *f, const struct region *r,
    const struct unit *u, int modifier);
  extern bool cansee_unit(const struct unit *u, const struct unit *target,
    int modifier);
  bool seefaction(const struct faction *f, const struct region *r,
    const struct unit *u, int modifier);
  extern int effskill(const struct unit *u, skill_t sk);

  extern int lovar(double xpct_x2);
  /* returns a value between [0..xpct_2], generated with two dice */

  int distribute(int old, int new_value, int n);

  int newunitid(void);
  int forbiddenid(int id);
  int newcontainerid(void);

  extern struct unit *createunit(struct region *r, struct faction *f,
    int number, const struct race *rc);
  extern void create_unitid(struct unit *u, int id);
  extern bool getunitpeasants;
  extern struct unit *getunitg(const struct region *r, const struct faction *f);
  extern struct unit *getunit(const struct region *r, const struct faction *f);

  extern int read_unitid(const struct faction *f, const struct region *r);

  extern int alliedunit(const struct unit *u, const struct faction *f2,
    int mode);
  extern int alliedfaction(const struct plane *pl, const struct faction *f,
    const struct faction *f2, int mode);
  extern int alliedgroup(const struct plane *pl, const struct faction *f,
    const struct faction *f2, const struct ally *sf, int mode);

  struct faction *findfaction(int n);
  struct faction *getfaction(void);

  struct unit *findunitg(int n, const struct region *hint);
  struct unit *findunit(int n);

  struct unit *findunitr(const struct region *r, int n);
  struct region *findunitregion(const struct unit *su);

  extern char *estring(const char *s);
  extern char *estring_i(char *s);
  extern char *cstring(const char *s);
  extern char *cstring_i(char *s);
  extern const char *unitname(const struct unit *u);
  extern char *write_unitname(const struct unit *u, char *buffer, size_t size);

  typedef int (*cmp_building_cb) (const struct building * b,
    const struct building * a);
  struct building *largestbuilding(const struct region *r, cmp_building_cb,
    bool imaginary);
  int cmp_wage(const struct building *b, const struct building *bother);
  int cmp_taxes(const struct building *b, const struct building *bother);
  int cmp_current_owner(const struct building *b,
    const struct building *bother);

#define TAX_ORDER 0x00
#define TAX_OWNER 0x01
  int rule_auto_taxation(void);
  int rule_transfermen(void);
  int rule_region_owners(void);
  int rule_stealth_faction(void);
#define HARVEST_WORK  0x00
#define HARVEST_TAXES 0x01
  int rule_blessed_harvest(void);
  extern int rule_give(void);
  extern int rule_alliance_limit(void);
  extern int rule_faction_limit(void);

  int count_units(const struct faction * f);
  int count_all(const struct faction *f);
  int count_migrants(const struct faction *f);
  int count_maxmigrants(const struct faction *f);

  extern bool has_limited_skills(const struct unit *u);
  extern const struct race *findrace(const char *, const struct locale *);

  int eff_stealth(const struct unit *u, const struct region *r);
  int ispresent(const struct faction *f, const struct region *r);

  int check_option(struct faction *f, int option);
  extern void parse_kwd(keyword_t kword, int (*dofun) (struct unit *,
      struct order *), bool thisorder);

/* Anzahl Personen in einer Einheit festlegen. NUR (!) mit dieser Routine,
 * sonst großes Unglück. Durch asserts an ein paar Stellen abgesichert. */
  void verify_data(void);

  void freestrlist(strlist * s);

  int change_hitpoints(struct unit *u, int value);

  int weight(const struct unit *u);
  void changeblockchaos(void);

/* intervall, in dem die regionen der partei zu finden sind */
  extern struct region *firstregion(struct faction *f);
  extern struct region *lastregion(struct faction *f);

  void fhash(struct faction *f);
  void funhash(struct faction *f);

  bool idle(struct faction *f);
  bool unit_has_cursed_item(struct unit *u);

/* simple garbage collection: */
  void *gc_add(void *p);

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

  extern void setstatus(struct unit *u, int status);
/* !< sets combatstatus of a unit */
  extern void setguard(struct unit *u, unsigned int flags);
/* !< setzt die guard-flags der Einheit */
  extern unsigned int getguard(const struct unit *u);
  /* liest die guard-flags der Einheit */
  extern void guard(struct unit *u, unsigned int mask);
  /* Einheit setzt "BEWACHE", rassenspezifzisch.
   * 'mask' kann einzelne flags zusätzlich und-maskieren.
   */
  unsigned int guard_flags(const struct unit *u);

  extern bool hunger(int number, struct unit *u);
  extern int lifestyle(const struct unit *);
  extern int besieged(const struct unit *u);
  extern int maxworkingpeasants(const struct region *r);
  extern bool has_horses(const struct unit *u);
  extern int markets_module(void);
  extern int wage(const struct region *r, const struct faction *f,
    const struct race *rc, int in_turn);
  extern int maintenance_cost(const struct unit *u);
  extern struct message *movement_error(struct unit *u, const char *token,
    struct order *ord, int error_code);
  extern bool move_blocked(const struct unit *u, const struct region *src,
    const struct region *dest);
  extern void add_income(struct unit *u, int type, int want, int qty);

/* movewhere error codes */
  enum {
    E_MOVE_OK = 0,              /* possible to move */
    E_MOVE_NOREGION,            /* no region exists in this direction */
    E_MOVE_BLOCKED              /* cannot see this region, there is a blocking connection. */
  };
  extern int movewhere(const struct unit *u, const char *token,
    struct region *r, struct region **resultp);

  extern const char *datapath(void);
  extern void set_datapath(const char *path);

  extern const char *basepath(void);
  extern void set_basepath(const char *);
  void load_inifile(struct _dictionary_ *d);

  extern const char *reportpath(void);
  extern void set_reportpath(const char *);

  extern void kernel_init(void);
  extern void kernel_done(void);

  extern const char *localenames[];

/** compatibility: **/
  extern race_t old_race(const struct race *);
  extern const struct race *new_race[];

/* globale settings des Spieles */
  typedef struct settings {
    const char *gamename;
    struct attrib *attribs;
    unsigned int data_turn;
    bool disabled[MAXKEYWORDS];
    struct param *parameters;
    void *vm_state;
    float producexpchance;
    int cookie;
    int game_id;
    int data_version; /* TODO: eliminate in favor of gamedata.version */
    struct _dictionary_ *inifile;

    struct global_functions {
      int (*wage) (const struct region * r, const struct faction * f,
        const struct race * rc, int in_turn);
      int (*maintenance) (const struct unit * u);
    } functions;
  } settings;
  extern settings global;

  extern int produceexp(struct unit *u, skill_t sk, int n);

  extern bool battledebug;
  extern bool sqlpatch;
  extern bool lomem;         /* save memory */

  extern const char *dbrace(const struct race *rc);

  extern void set_param(struct param **p, const char *name, const char *data);
  extern const char *get_param(const struct param *p, const char *name);
  extern int get_param_int(const struct param *p, const char *name, int def);
  extern float get_param_flt(const struct param *p, const char *name,
    float def);

  extern bool ExpensiveMigrants(void);
  extern int NMRTimeout(void);
  extern int LongHunger(const struct unit *u);
  extern int SkillCap(skill_t sk);
  extern int NewbieImmunity(void);
  extern bool IsImmune(const struct faction *f);
  extern int AllianceAuto(void);        /* flags that allied factions get automatically */
  extern int AllianceRestricted(void);  /* flags restricted to allied factions */
  extern int HelpMask(void);    /* flags restricted to allied factions */
  extern struct order *default_order(const struct locale *lang);
  extern int entertainmoney(const struct region *r);

  extern void plagues(struct region *r, bool ismagic);
  typedef struct helpmode {
    const char *name;
    int status;
  } helpmode;

  extern struct helpmode helpmodes[];

#define GIVE_SELF 1
#define GIVE_PEASANTS 2
#define GIVE_LUXURIES 4
#define GIVE_HERBS 8
#define GIVE_GOODS 16
#define GIVE_ONDEATH 32
#define GIVE_ALLITEMS (GIVE_GOODS|GIVE_HERBS|GIVE_LUXURIES)
#define GIVE_DEFAULT (GIVE_SELF|GIVE_PEASANTS|GIVE_LUXURIES|GIVE_HERBS|GIVE_GOODS)

  extern struct attrib_type at_guard;
  extern void free_gamedata(void);
#if 1                           /* disable to count all units */
# define count_unit(u) (u->number>0 && playerrace(u_race(u)))
#else
# define count_unit(u) 1
#endif

#ifdef __cplusplus
}
#endif
#endif
