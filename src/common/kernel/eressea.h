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

#ifndef ERESSEA_H
#define ERESSEA_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Features enabled:
 * If you are lacking the settings.h, create a new file common/settings.h,
 * and write #include <settings-eressea.h> (or whatever settings you want
 * your game to use) in there.
 * !!! DO NOT COMMIT THE SETTINGS.H FILE TO CVS !!!
 */
#include <settings.h>

/* basic types used in the eressea "kernel" */
#ifdef __TINYC__
#define order_t short
#define terrain_t short
#define direction_t short
#define race_t short
#define magic_t short
#define skill_t short
#define typ_t short
#define herb_t short
#define luxury_t short
#define weapon_t short
#define item_t short
#define spellid_t unsigned int
#else
typedef short order_t;
typedef short terrain_t;
typedef short direction_t;
typedef short race_t;
typedef short magic_t;
typedef short skill_t;
typedef short typ_t;
typedef short herb_t;
typedef short potion_t;
typedef short luxury_t;
typedef short weapon_t;
typedef short item_t;
typedef unsigned int spellid_t;
#endif

struct plane;
struct order;
struct spell;
struct region;
struct fighter;
struct region_list;
struct race;
struct ship;
struct terrain_type;
struct building;
struct faction;
struct party;
struct unit;
struct unit_list;
struct item;
/* item */
struct strlist;
struct resource_type;
struct item_type;
struct potion_type;
struct luxury_type;
struct weapon_type;
/* types */
struct ship_type;
struct building_type;

#include <util/attrib.h>
#include <util/cvector.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/variant.h>
#include <util/vmap.h>
#include <util/vset.h>

#define AT_PERSISTENT
#define ALLIED(f1, f2) (f1==f2 || (f1->alliance && f1->alliance==f2->alliance))

/* eressea-defined attribute-type flags */
#define ATF_CURSE  ATF_USER_DEFINED

#define OLD_FAMILIAR_MOD /* conversion required */
/* feature-dis/en-able */
#define NEW_DRIVE     /* Neuer Algorithmus Transportiere/Fahre */
#define PARTIAL_STUDY /* Wenn nicht genug Silber vorhanden, wird ein Talent anteilig gelernt */
#define GOBLINKILL

#define MONSTER_FACTION 0 /* Die Partei, in der die Monster sind. */

/**
 * heaps and heaps of unsupported versions:
	#define RACES_VERSION 91
	#define MAGIEGEBIET_VERSION 92
	#define FATTRIBS_VERSION 94
	#define ATNORD_VERSION 95
	#define NEWMAGIC 100
	#define MEMSAVE_VERSION 101
	#define BORDER_VERSION 102
	#define ATNAME_VERSION 103
	#define ATTRIBFIX_VERSION 104
	#define BORDERID_VERSION 105
	#define NEWROAD_VERSION 106
	#define GUARD_VERSION 107
	#define TIMEOUT_VERSION 108
	#define GUARDFIX_VERSION 109
	#define NEW_FACTIONID_VERSION 110
	#define ACTIONFIX1_VERSION 111
	#define SHIPTYPE_VERSION 112
	#define GROUPS_VERSION 113
	#define MSGLEVEL_VERSION 114
	#define DISABLE_ROADFIX 114
	#define FACTIONFLAGS_VERSION 114
	#define KARMA_VERSION 114
	#define FULL_BASE36_VERSION 115
 **/
#define TYPES_VERSION 117
#define ITEMTYPE_VERSION 190
#define NOFOREST_VERSION 191
#define REGIONAGE_VERSION 192
#define CURSE_NO_VERSION 193
#define EFFSTEALTH_VERSION 194
#define MAGE_ATTRIB_VERSION 195
#define GLOBAL_ATTRIB_VERSION 196
#define BASE36IDS_VERSION 197
#define NEWSOURCE_VERSION 197
#define NEWSTATUS_VERSION 198
#define NEWNAMES_VERSION 199
#define LOCALE_VERSION 300
#define GROUPATTRIB_VERSION 301
#define NEWRESOURCE_VERSION 303
#define GROWTREE_VERSION 305
#define RANDOMIZED_RESOURCES_VERSION 306 /* should be the same, but doesn't work */
#define NEWRACE_VERSION 307
#define INTERIM_VERSION 309
#define NEWSKILL_VERSION 309
#define WATCHERS_VERSION 310
#define OVERRIDE_VERSION 311
#define CURSETYPE_VERSION 312
#define ALLIANCES_VERSION 313
#define DBLINK_VERSION 314
#define CURSEVIGOURISFLOAT_VERSION 315
#define SAVEXMLNAME_VERSION 316
#define SAVEALLIANCE_VERSION 317
#define CLAIM_VERSION 318
#define BACTION_VERSION 319 /* building action gets a param string */
#define NOLASTORDER_VERSION 320 /* do not use lastorder */
#define SPELLNAME_VERSION 321 /* reference spells by name */
#define TERRAIN_VERSION 322 /* terrains are a full type and saved by name */
#define REGIONITEMS_VERSION 323 /* regions have items */

#define MIN_VERSION CURSETYPE_VERSION
#define REGIONOWNERS_VERSION 400

#ifdef ENEMIES
# define RELEASE_VERSION ENEMIES_VERSION
#else
# define RELEASE_VERSION REGIONITEMS_VERSION
#endif

#if RESOURCE_CONVERSION
extern void init_resourcefix(void);
extern void read_iron(struct region * r, int iron);
extern void read_laen(struct region * r, int laen);
#endif

#define ECHECK_VERSION "4.01"

/* changes from->to: 72->73: struct unit::lock entfernt.
 * 73->74: struct unit::flags eingeführt.
 * 74->75: parteitarnung als flag.
 * 75->76: #ifdef NEW_HP: hp
 * 76->77: ship->damage
 * 77->78: neue Message-Option "Orkvermehrung" (MAX_MSG +1)
 * 78->79: showdata nicht mehr speichern
 * 79->HEX_VERSION: hex
 * 80->82: ATTRIB_VERSION
 * 90: Ebenen
 * 92: Magiegebiet-Auswahl f->magiegebiet
 * 94: f->attribs wird gespeichert
 * 100: NEWMAGIC, neue Message-Option "Zauber" (MAX_MSG +1)
 * 108: Speichern von Timeouts
 * 193: curse bekommen id aus struct unit-nummernraum
 */

/* NEWMAIL benutzt ein spezielles Skript zum Erstellen der Reportmails */
#define MAIL				"eresseamail"

#define DEFAULT_LOCALE "de"

#define MAXPEASANTS_PER_AREA    10

/* So lange kann die Partei nicht angegriffen werden */
#undef STUDY_IF_NOT_WORKING

/** Attackierende Einheiten können keine langen Befehle ausführen */

#define DISALLOW_ATTACK_AND_WORK

/** Ausbreitung und Vermehrung */

#define MAXDEMAND      25
#define DMRISE         10
#define DMRISEHAFEN    20

/* Vermehrung trotz 90% Auslastung */
#define PEASANTFORCE   0.75

#define PEASANTSWANDER_WEIGHT 5
#define PEASANTSGREED_WEIGHT  5

#define STARVATION_SURVIVAL  10

/* Pferdevermehrung */
#define HORSEGROWTH 4
/* Wanderungschance pro Pferd */
#define HORSEMOVE   3

/* Vermehrungschance pro Baum */
#define FORESTGROWTH 10000					/* In Millionstel */
#define TREESIZE     (MAXPEASANTS_PER_AREA-2)

/* Eisen in Bergregionen bei Erschaffung */
#define IRONSTART     250
/* Zuwachs Eisen in Bergregionen */
#define IRONPERTURN    25
#define MAXLAENPERTURN  6
/* Eisen in Gletschern */
#define GLIRONSTART    20
/* Zuwachs Eisen in Gletschern */
#define GLIRONPERTURN   3
/* Maximales Eisen in Gletschern */
#define MAXGLIRON      80

/* Verrottchance für Kräuter */
#define HERBROTCHANCE 5
#define HERBS_ROT

#define STUDYCOST     200

/* Gebäudegröße = Minimalbelagerer */
#define SIEGEFACTOR     2

/** Magic */
#define MAXMAGICIANS    3
#define MAXALCHEMISTS   3

/** Plagues **/
/* Seuchenwahrscheinlichkeit (siehe plagues()) */
#define SEUCHE         10
/* % Betroffene */
#define SEUCHENOPFER   20
extern void plagues(struct region * r, boolean ismagic);

/** Healing **/
/* Wahrscheinlichkeit Heilung */
#define HEILCHANCE     25
/* Heilkosten */
#define HEILKOSTEN     30

/** Monster */

/* Chance für Monsterangriff */
#define MONSTERATTACK  0.4

/** Gebäude */
/* Chance für Einsturz bei unversorgtem Gebaeude */
#define EINSTURZCHANCE     40
/* Soviele % überleben den Einsturz */
#define EINSTURZUEBERLEBEN 50

/** Geographie. Nicht im laufenden Spiel ändern!!! */
#define BLOCKSIZE           9

/* Nur Bewaffnete bewachen */
#undef TROLLSAVE

/* Spezialtrank für Insekten */
#define INSECT_POTION

/* Schiffsbeschädigungen */
#define SHIPDAMAGE

/* regionen im Report der Parteien werden nur einmal berechnet: */
#define FAST_REGION

/* Magiesystem */
#define NIGHT_EYE_TALENT        5

/* gibt es ein attribut at_salary */
#undef AFFECT_SALARY
/* ist noch nicht fertig! */

#define ARENA_PLANE
#undef UNDERWORLD_PLANE

#define ORCIFICATION

/* Bewegungsweiten: */
#define BP_WALKING 4
#define BP_RIDING  6
#define BP_UNICORN 9
#define BP_DRAGON  4

#define BP_NORMAL 3
#define BP_ROAD   2


/**
 * Hier endet der Teil von eressea.h, der die defines für die
 * Spielwelt Eressea enthält, und beginnen die allgemeinen Routinen
 */

#define ENCCHANCE           10	/* %-Chance für einmalige Zufallsbegegnung */
#define SPACE_REPLACEMENT   '~'
#define ESCAPE_CHAR         '\\'

#define DISPLAYSIZE         8191	/* max. Länge einer Beschreibung, ohne trailing 0 */
#define NAMESIZE            127	/* max. Länge eines Namens, ohne trailing 0 */
#define IDSIZE              15	/* max. Länge einer no (als String), ohne trailing 0 */
#define KEYWORDSIZE         15	/* max. Länge eines Keyword, ohne trailing 0 */
#define OBJECTIDSIZE        (NAMESIZE+5+IDSIZE)	/* max. Länge der Strings, die
						 * von struct unitname, etc. zurückgegeben werden. ohne die 0 */
#define CMDSIZE             (DISPLAYSIZE*2+1)
#define STARTMONEY          5000

#define PRODUCEEXP          10
#define TAVERN_MAINTENANCE  14
/* Man gibt in einer Taverne mehr Geld aus! */

#define BAGCAPACITY		20000	/* soviel paßt in einen Bag of Holding */
#define STRENGTHCAPACITY	50000	/* zusätzliche Tragkraft beim Kraftzauber (deprecated) */
#define STRENGTHMULTIPLIER 50   /* multiplier for trollbelt */

typedef struct ursprung {
	struct ursprung *next;
	int id;
	short x, y;
} ursprung;

/* ----------------- Befehle ----------------------------------- */

typedef unsigned char keyword_t;
enum {
  K_KOMMENTAR,
  K_BANNER,
  K_WORK,
  K_ATTACK,
  K_STEAL,
  K_BESIEGE,
  K_NAME,
  K_USE,
  K_DISPLAY,
  K_ENTER,
  K_GUARD,
  K_MAIL,
  K_END,
  K_DRIVE,
  K_NUMBER,
  K_WAR,
  K_PEACE,
  K_FOLLOW,
  K_RESEARCH,
  K_GIVE,
  K_ALLY,
  K_STATUS,
  K_COMBAT,
  K_BUY,
  K_CONTACT,
  K_TEACH,
  K_STUDY,
  K_LIEFERE,
  K_MAKE,
  K_MOVE,
  K_PASSWORD,
  K_RECRUIT,
  K_RESERVE,
  K_ROUTE,
  K_SABOTAGE,
  K_SEND,
  K_SPY,
  K_QUIT,
  K_SETSTEALTH,
  K_TRANSPORT,
  K_TAX,
  K_ENTERTAIN,
  K_SELL,
  K_LEAVE,
  K_FORGET,
  K_CAST,
  K_RESHOW,
  K_DESTROY,
  K_BREED,
  K_DEFAULT,
  K_REPORT,
  K_URSPRUNG,
  K_EMAIL,
  K_VOTE,
  K_MAGIEGEBIET,
  K_PIRACY,
  K_RESTART,
  K_GROUP,
  K_SACRIFICE,
  K_PRAY,
  K_SORT,
  K_SETJIHAD,
  K_GM,          /* perform GM commands */
  K_INFO,        /* set player-info */
  K_PREFIX,
  K_SYNONYM,
  K_PLANT,
  K_WEREWOLF,
  K_XE,
  K_ALLIANCE,
  K_CLAIM,
#ifdef HEROES
  K_PROMOTION,
#endif
  MAXKEYWORDS,
  NOKEYWORD = (keyword_t) - 1
};

extern const char *keywords[MAXKEYWORDS];

/* ------------------ Status von Einheiten --------------------- */

typedef unsigned char status_t;
enum {
  ST_AGGRO,
  ST_FIGHT,
  ST_BEHIND,
  ST_CHICKEN,
  ST_AVOID,
  ST_FLEE
};

/* ----------------- Parameter --------------------------------- */

typedef unsigned char param_t;
enum {
	P_LOCALE,
	P_ANY,
	P_PEASANT,
	P_BUILDING,
	P_UNIT,
	P_PRIVAT,
	P_BEHIND,
	P_CONTROL,
	P_HERBS,
	P_NOT,
	P_NEXT,
	P_FACTION,
	P_GAMENAME,
	P_PERSON,
	P_REGION,
	P_SHIP,
	P_SILVER,
	P_ROAD,
	P_TEMP,
	P_FLEE,
	P_GEBAEUDE,
	P_GIB,
	P_KAEMPFE,
  P_TRAVEL,
	P_GUARD,
	P_ZAUBER,
	P_PAUSE,
	P_VORNE,
	P_AGGRO,
	P_CHICKEN,
	P_LEVEL,
	P_HELP,
	P_FOREIGN,
	P_AURA,
	P_FOR,
	P_AID,
	P_MERCY,
	P_AFTER,
	P_BEFORE,
	P_NUMBER,
	P_ITEMS,
	P_POTIONS,
	P_GROUP,
	P_FACTIONSTEALTH,
	P_TREES,
	P_XEPOTION,
	P_XEBALLOON,
	P_XELAEN,
	MAXPARAMS,
	NOPARAM = (param_t) - 1
};

typedef enum {					/* Fehler und Meldungen im Report */
	MSG_BATTLE,
	MSG_EVENT,
	MSG_MOVE,
	MSG_INCOME,
	MSG_COMMERCE,
	MSG_PRODUCE,
	MSG_ORCVERMEHRUNG,
	MSG_MESSAGE,
	MSG_COMMENT,
	MSG_MAGIC,
	MAX_MSG
} msg_t;

enum {					/* Message-Level */
	ML_IMPORTANT,				/* Sachen, die IMO erscheinen _muessen_ */
	ML_DEBUG,
	ML_MISTAKE,
	ML_WARN,
	ML_INFO,
	ML_MAX
};

extern const char *parameters[MAXPARAMS];

/* --------------- Reports Typen ------------------------------- */

enum {
	O_REPORT,				/* 1 */
	O_COMPUTER,			/* 2 */
	O_ZUGVORLAGE,		/* 4 */
	O_SILBERPOOL,		/* 8 */
	O_STATISTICS,		/* 16 */
	O_DEBUG,				/* 32 */
	O_COMPRESS,			/* 64 */
	O_NEWS,					/* 128 */
	O_MATERIALPOOL,	/* 256 */
	O_ADRESSEN,			/* 512 */
	O_BZIP2,				/* 1024 - compress as bzip2 */
	O_SCORE,				/* 2048 - punkte anzeigen? */
	O_SHOWSKCHANGE,	/* 4096 - Skillveränderungen anzeigen? */
  O_XML,    			/* 8192 - XML report versenden */
	MAXOPTIONS
};

#define want(i) (1<<i)
#define Pow(i) (1<<i)

extern const char *options[MAXOPTIONS];

/* ------------------ Talente ---------------------------------- */

enum {
	SK_ALCHEMY,
	SK_CROSSBOW,
	SK_MINING,
	SK_LONGBOW,
	SK_BUILDING,
	SK_TRADE,
	SK_LUMBERJACK,
	SK_CATAPULT,
	SK_HERBALISM,
	SK_MAGIC,
	SK_HORSE_TRAINING,			/* 10 */
	SK_RIDING,
	SK_ARMORER,
	SK_SHIPBUILDING,
	SK_MELEE,
	SK_SAILING,
	SK_SPEAR,
	SK_SPY,
	SK_QUARRYING,
	SK_ROAD_BUILDING,
	SK_TACTICS,					/* 20 */
	SK_STEALTH,
	SK_ENTERTAINMENT,
	SK_WEAPONSMITH,
	SK_CARTMAKER,
	SK_OBSERVATION,
	SK_TAXING,
	SK_AUSDAUER,
	SK_WEAPONLESS,
	MAXSKILLS,
	NOSKILL = (skill_t) -1
};

/* ------------- Typ von Einheiten ----------------------------- */

enum {
	RC_DWARF,		/* 0 - Zwerg */
	RC_ELF,
	RC_ORC,
	RC_GOBLIN,
	RC_HUMAN,

  RC_TROLL,
  RC_DAEMON,
	RC_INSECT,
	RC_HALFLING,
	RC_CAT,

  RC_AQUARIAN,
  RC_URUK,
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
	NORACE = (race_t) - 1
};

/* movewhere error codes */
enum {
  E_MOVE_OK = 0,   /* possible to move */
  E_MOVE_NOREGION, /* no region exists in this direction */
  E_MOVE_BLOCKED   /* cannot see this region, there is a blocking border. */
};

/* Richtungen */
enum {
	D_NORTHWEST,
	D_NORTHEAST,
	D_EAST,
	D_SOUTHEAST,
	D_SOUTHWEST,
	D_WEST,
	MAXDIRECTIONS,
	D_PAUSE,
  D_SPECIAL,
	NODIRECTION = (direction_t) - 1
};

/* Jahreszeiten, siehe auch res/timestrings */
enum {
	SEASON_WINTER,
	SEASON_SPRING,
	SEASON_SUMMER,
	SEASON_AUTUMN
};

/* Siegbedingungen */

#define VICTORY_NONE   0
#define VICTORY_MURDER 1

/* ------------------------------------------------------------- */

extern int shipspeed(const struct ship * sh, const struct unit * u);
extern int init_data(const char * filename);

/* MAXSPEED setzt die groesse fuer den Array, der die Kuesten Beschreibungen
 * enthaelt. MAXSPEED muss 2x +3 so gross wie die groesste Geschw. sein
 * (Zauberspruch) */

/* ------------------------------------------------------------- */

/* ------------------------------------------------------------- */
/* TODO
 * werden diese hier eigendlich noch für irgendwas gebraucht?*/


/* Attribute: auf Einheiten */

/* Klassen */

enum {
	A_KL_GEGENSTAND,
	A_KL_EIGENSCHAFT
};
/* Eigenschaften (in i[0]) */

#define ZEIGE_PARTEI	1		/* Zeigt s[0] in eigenem Report an */
#define ZEIGE_ANDEREN 2			/* Zeigt s[1] in fremden Reports an */
#define TALENTMOD			4	/* Gegenstand verändert Talent
						 * i[2] um i[3] */

/* A_KL_EIGENSCHAFT */

enum {
	A_TY_NOT_PARTEI_GETARNT,	/* In i[0] die Pseudopartei?! */
	A_TY_NOT_FLAGS,		/* Verschiedene Eigenschaften in i[0] */
	A_TY_EX_TYPNAME,		/* Was soll als Typname erscheinen? s[0] =
				 * Singular, s[1] = Plural */
	A_TY_EX_ZIELREGION,
	A_TY_EX_BAUERNBLUT		/* In i[0] die Zahl der max. geschützten
				 * Dämonen. */
};

/* A_TY_GESEHEN:    fuer cansee() in i[0] steht drin, welche Partei uns
 * sah, in i[1], wie gut (0,1 oder 2) */

/* Auf Regionen */

/* Klassen */

enum {
	A_KL_MERKMAL,
	A_KL_ZAUBER
};

/* A_KL_MERKMAL */

enum {
	A_TY_MITHRIL,
	A_TY_ADAMANTIUM,
	A_TY_DIRECTION		/* In s[0] describe_txt, in s[1] das command,
											 in i[0]-i[2] die Koordinaten des Ziels. */
};

/* Auf Schiffen */

/* Klassen */

enum {
	A_KL_MERKMAL_SCHIFF
};

/* A_KL_MERKMAL */

enum {
	A_TY_EX_KUESTE		/* In i[0] steht die Richtung */
};

/* Auf Borders */

/* Klassen */

enum {
	A_KL_BLOCK
};

enum {
	A_TY_LOCK
};

#define DONT_HELP      0
#define HELP_MONEY     1			/* Mitversorgen von Einheiten */
#define HELP_FIGHT     2			/* Bei Verteidigung mithelfen */
#define HELP_OBSERVE   4			/* Bei Wahrnehmung mithelfen */
#define HELP_GIVE      8			/* Dinge annehmen ohne KONTAKTIERE */
#define HELP_GUARD    16			/* Laesst Steuern eintreiben etc. */
#define HELP_FSTEALTH 32			/* Parteitarnung anzeigen. */
#define HELP_TRAVEL   64			/* Laesst Regionen betreten. */
#define HELP_ALL    (127-HELP_TRAVEL-HELP_OBSERVE)		/* Alle "positiven" HELPs zusammen */
/* HELP_OBSERVE deaktiviert */
/* ------------------------------------------------------------- */
/* Prototypen */


#define ALLIED_TAX     1
#define ALLIED_NOBLOCK 2
#define ALLIED_HELP    4

extern vmap region_map;

#define i2b(i) ((boolean)((i)?(true):(false)))

typedef struct ally {
	struct ally *next;
	struct faction *faction;
	int status;
} ally;

#define FF_NEWID (1<<0) /* Die Partei hat bereits einmal ihre no gewechselt */

void remove_empty_units_in_region(struct region *r);
void remove_empty_units(void);
void remove_empty_factions(boolean writedropouts);

typedef struct strlist {
	struct strlist *next;
	char * s;
} strlist;

#define FL_DH             (1<<18) /* ehemals f->dh, u->dh, r->dh, etc... */
#define FL_MARK           (1<<23) /* für markierende algorithmen, die das 
                                   * hinterher auch wieder löschen müssen! 
                                   * (FL_DH muss man vorher initialisieren, 
                                   * FL_MARK hinterher löschen) */

/* alle vierstelligen zahlen: */
#define MAX_UNIT_NR (36*36*36*36-1)
#define MAX_CONTAINER_NR (36*36*36*36-1)

#define fval(u, i) ((u)->flags & (i))
#define fset(u, i) ((u)->flags |= (i))
#define freset(u, i) ((u)->flags &= ~(i))

extern int turn;
extern int quiet;

/* parteinummern */
extern int *used_faction_ids;
extern int no_used_faction_ids;
extern void register_faction_id(int id);
extern boolean faction_id_is_unused(int);

/* leuchtturm */
extern boolean check_leuchtturm(struct region * r, struct faction * f);
extern void update_lighthouse(struct building * lh);
extern struct unit_list * get_lighthouses(const struct region * r);
extern int lighthouse_range(const struct building * b, const struct faction * f);

/* skills */
extern int max_skill(struct faction * f, skill_t sk);
extern int count_skill(struct faction * f, skill_t sk);

/* direction, geography */
extern const char *directions[];
extern direction_t finddirection(const char *s, const struct locale *);

extern int findoption(const char *s, const struct locale * lang);

/* shared character-buffer */
#define BUFSIZE 32765
extern char buf[BUFSIZE + 1];

/* special units */
void make_undead_unit(struct unit *);

extern struct region *regions;
extern struct faction *factions;

void addstrlist(strlist ** SP, const char *s);

int armedmen(const struct unit * u);

void scat(const char *s);
void icat(int n);

int atoip(const char *s);
int geti(void);

extern int findstr(const char **v, const char *s, unsigned char n);

extern const char *igetstrtoken(const char *s);
extern const char *getstrtoken(void);

extern void init_tokens_str(const char * initstr); /* initialize token parsing */
extern void init_tokens(const struct order * ord); /* initialize token parsing */
extern void skip_token(void);
extern const char * parse_token(const char ** str);
extern void parser_pushstate(void);
extern void parser_popstate(void);
extern boolean parser_end(void);

extern skill_t findskill(const char *s, const struct locale * lang);

extern keyword_t findkeyword(const char *s, const struct locale * lang);

extern param_t findparam(const char *s, const struct locale * lang);
extern param_t getparam(const struct locale * lang);

extern int atoi36(const char * s);
#define getid() atoi36(getstrtoken())
#define getstruct unitid() getid()
#define unitid(x) itoa36((x)->no)

#define getshipid() getid()
#define getfactionid() getid()

#define buildingid(x) itoa36((x)->no)
#define shipid(x) itoa36((x)->no)
#define factionid(x) itoa36((x)->no)
#define curseid(x) itoa36((x)->no)

extern boolean cansee(const struct faction * f, const struct region * r, const struct unit * u, int modifier);
boolean cansee_durchgezogen(const struct faction * f, const struct region * r, const struct unit * u, int modifier);
boolean seefaction(const struct faction * f, const struct region * r, const struct unit * u, int modifier);
extern int effskill(const struct unit * u, skill_t sk);

extern int lovar(double xpct_x2); 
  /* returns a value between [0..xpct_2], generated with two dice */

int distribute(int old, int new_value, int n);

int newunitid(void);
int forbiddenid(int id);
int newcontainerid(void);

extern struct unit *createunit(struct region * r, struct faction * f, int number, const struct race * rc);
extern struct unit *create_unit(struct region * r1, struct faction * f, int number, const struct race * rc, int id, const char * dname, struct unit *creator);
extern void create_unitid(struct unit *u, int id);
extern boolean getunitpeasants;
extern struct unit *getunitg(const struct region * r, const struct faction * f);
extern struct unit *getunit(const struct region * r, const struct faction * f);

extern int read_unitid(const struct faction * f, const struct region * r);

extern int alliedunit(const struct unit * u, const struct faction * f2, int mode);
extern int alliedfaction(const struct plane * pl, const struct faction * f,
                         const struct faction * f2, int mode);
extern int alliedgroup(const struct plane * pl, const struct faction * f,
                       const struct faction * f2, const struct ally * sf,
                       int mode);

struct faction *findfaction(int n);
struct faction *getfaction(void);

struct unit *findunitg(int n, const struct region * hint);
struct unit *findunit(int n);

struct unit *findunitr(const struct region * r, int n);
struct region *findunitregion(const struct unit * su);

char *estring(const char *s);
char *cstring(const char *s);
const char *unitname(const struct unit * u);

struct building *largestbuilding(const struct region * r, boolean img);

extern int count_all(const struct faction * f);
extern int count_migrants (const struct faction * f);
extern int count_maxmigrants(const struct faction * f);

extern boolean teure_talente(const struct unit * u);
extern const struct race * findrace(const char *, const struct locale *);

int eff_stealth(const struct unit * u, const struct region * r);
void scale_number(struct unit * u, int n);
int unit_max_hp(const struct unit * u);
int ispresent(const struct faction * f, const struct region * r);

char * set_string(char **s, const char *neu);

int check_option(struct faction * f, int option);
extern void parse(keyword_t kword, int (*dofun)(struct unit *, struct order *), boolean thisorder);

/* Anzahl Personen in einer Einheit festlegen. NUR (!) mit dieser Routine,
 * sonst großes Unglück. Durch asserts an ein paar Stellen abgesichert. */
void verify_data(void);

void stripfaction(struct faction * f);
void freestrlist(strlist * s);

int change_hitpoints(struct unit *u, int value);

int weight(const struct unit * u);
void changeblockchaos(void);

/* intervall, in dem die regionen der partei zu finden sind */
extern void update_intervals(void);
extern struct region *firstregion(struct faction * f);
extern struct region *lastregion(struct faction * f);

void inituhash(void);
void uhash(struct unit * u);
void uunhash(struct unit * u);
struct unit *ufindhash(int i);

void fhash(struct faction * f);
void funhash(struct faction * f);

#ifndef NDEBUG
const char *strcheck(const char *s, size_t maxlen);

#else
#define strcheck(s, ml) (s)
#endif

#define attacked(u) (fval(u, UFL_LONGACTION))
boolean idle(struct faction * f);
boolean unit_has_cursed_item(struct unit *u);

/* simple garbage collection: */
void * gc_add(void * p);
void addmessage(struct region * r, struct faction * f, const char *s, msg_t mtype, int level);

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

extern void setguard(struct unit * u, unsigned int flags);
	/* setzt die guard-flags der Einheit */
extern unsigned int getguard(const struct unit * u);
	/* liest die guard-flags der Einheit */
extern void guard(struct unit * u, unsigned int mask);
	/* Einheit setzt "BEWACHE", rassenspezifzisch.
	 * 'mask' kann einzelne flags zusätzlich und-maskieren.
	 */

extern boolean hunger(int number, struct unit * u);
extern int lifestyle(const struct unit*);
extern int besieged(const struct unit * u);
extern int maxworkingpeasants(const struct region * r);

extern int wage(const struct region *r, const struct faction *f, const struct race * rc);
extern int maintenance_cost(const struct unit * u);
extern int movewhere(const struct unit *u, const char * token, struct region * r, struct region** resultp);
extern struct message * movement_error(struct unit * u, const char * token, struct order * ord, int error_code);
extern boolean move_blocked(const struct unit * u, const struct region *src, const struct region *dest);
extern void add_income(struct unit * u, int type, int want, int qty);

extern const char * basepath(void);
extern const char * resourcepath(void);
extern void kernel_init(void);
extern void kernel_done(void);

extern void reorder_owners(struct region * r);

extern const char *localenames[];

#include <log.h>

#ifdef _MSC_VER
#include <stdafx.h>
#endif

/** compatibility: **/
extern race_t old_race(const struct race *);
extern const struct race * new_race[];

/* globale settings des Spieles */
typedef struct settings {
  const char    *gamename;
  const char    *welcomepath;
	boolean				 unitsperalliance;
  unsigned int   maxunits;
  struct attrib *attribs;
  unsigned int   data_version;
  unsigned int   data_turn;
  boolean disabled[MAXKEYWORDS];
  struct param * parameters;
  void * vm_state;

  struct global_functions {
    int (*wage)(const struct region *r, const struct faction * f, const struct race * rc);
    int (*maintenance)(const struct unit * u);
  } functions;
} settings;
extern settings global;

extern int produceexp(struct unit * u, skill_t sk, int n);

extern boolean sqlpatch;
extern boolean lomem; /* save memory */

extern const char * dbrace(const struct race * rc);

extern void set_param(struct param ** p, const char * name, const char * data);
extern const char* get_param(const struct param * p, const char * name);

extern boolean ExpensiveMigrants(void);
extern int NMRTimeout(void);
extern int LongHunger(const struct unit * u);
extern boolean TradeDisabled(void);
extern int SkillCap(skill_t sk);
extern int NewbieImmunity(void);
extern int AllianceAuto(void); /* flags that allied factions get automatically */
extern int AllianceRestricted(void); /* flags restricted to allied factions */
extern struct order * default_order(const struct locale * lang);
extern int entertainmoney(const struct region * r);

extern int freadstr(FILE * F, char * str, size_t size);
extern int fwritestr(FILE * F, const char * str);

extern attrib_type at_guard;
#ifdef __cplusplus
}
#endif
#endif
