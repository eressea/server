/* vi: set ts=2:
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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

#ifndef ERESSEA_TYPES_H
#define ERESSEA_TYPES_H

/*
 * Features enabled:
 * If you are lacking the settings.h, create a new file common/settings.h,
 * and write #include <settings-eressea.h> (or whatever settings you want
 * your game to use) in there.
 * !!! DO NOT COMMIT THE SETTINGS.H FILE TO CVS !!!
 * settings.h should always be the first thing you include (after config.h).
 */
#include <settings.h>
#include <util/variant.h>

typedef short terrain_t;
typedef short direction_t;
typedef short race_t;
typedef short magic_t;
typedef short skill_t;
typedef short typ_t;
typedef short item_t;
typedef unsigned int spellid_t;

struct attrib;
struct attrib_type;
struct building;
struct building_type;
struct curse;
struct equipment;
struct faction;
struct fighter;
struct item;
struct item_type;
struct locale;
struct luxury_type;
struct order;
struct plane;
struct potion_type;
struct race;
struct region;
struct region_list;
struct resource_type;
struct ship;
struct ship_type;
struct skill;
struct spell;
struct storage;
struct strlist;
struct terrain_type;
struct unit;
struct unit_list;
struct weapon_type;

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
  K_COMBATSPELL,
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
  K_URSPRUNG,
  K_EMAIL,
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
  K_PLANT,
  K_WEREWOLF,
  K_XE,
  K_ALLIANCE,
  K_CLAIM,
  K_PROMOTION,
  MAXKEYWORDS,
  NOKEYWORD = (keyword_t) - 1
};

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
  P_EACH,
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
  P_MONEY,
  P_ROAD,
  P_TEMP,
  P_FLEE,
  P_GEBAEUDE,
  P_GIVE,
  P_FIGHT,
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

typedef enum {          /* Fehler und Meldungen im Report */
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

enum {          /* Message-Level */
  ML_IMPORTANT,        /* Sachen, die IMO erscheinen _muessen_ */
  ML_DEBUG,
  ML_MISTAKE,
  ML_WARN,
  ML_INFO,
  ML_MAX
};

extern const char *parameters[MAXPARAMS];

/* --------------- Reports Typen ------------------------------- */

enum {
  O_REPORT,        /* 1 */
  O_COMPUTER,      /* 2 */
  O_ZUGVORLAGE,    /* 4 */
  O_UNUSED_3,
  O_STATISTICS,    /* 16 */
  O_DEBUG,        /* 32 */
  O_COMPRESS,      /* 64 */
  O_NEWS,          /* 128 */
  O_UNUSED_8,
  O_ADRESSEN,      /* 512 */
  O_BZIP2,        /* 1024 - compress as bzip2 */
  O_SCORE,        /* 2048 - punkte anzeigen? */
  O_SHOWSKCHANGE,  /* 4096 - Skillveränderungen anzeigen? */
  O_XML,          /* 8192 - XML report versenden */
  MAXOPTIONS
};

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
  SK_HORSE_TRAINING,      /* 10 */
  SK_RIDING,
  SK_ARMORER,
  SK_SHIPBUILDING,
  SK_MELEE,
  SK_SAILING,
  SK_SPEAR,
  SK_SPY,
  SK_QUARRYING,
  SK_ROAD_BUILDING,
  SK_TACTICS,          /* 20 */
  SK_STEALTH,
  SK_ENTERTAINMENT,
  SK_WEAPONSMITH,
  SK_CARTMAKER,
  SK_PERCEPTION,
  SK_TAXING,
  SK_STAMINA,
  SK_WEAPONLESS,
  MAXSKILLS,
  NOSKILL = (skill_t) -1
};

/* ------------- Typ von Einheiten ----------------------------- */

enum {
  RC_DWARF,    /* 0 - Zwerg */
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

#define DONT_HELP      0
#define HELP_MONEY     1      /* Mitversorgen von Einheiten */
#define HELP_FIGHT     2      /* Bei Verteidigung mithelfen */
#define HELP_OBSERVE   4      /* Bei Wahrnehmung mithelfen */
#define HELP_GIVE      8      /* Dinge annehmen ohne KONTAKTIERE */
#define HELP_GUARD    16      /* Laesst Steuern eintreiben etc. */
#define HELP_FSTEALTH 32      /* Parteitarnung anzeigen. */
#define HELP_TRAVEL   64      /* Laesst Regionen betreten. */
#define HELP_ALL    (127-HELP_TRAVEL-HELP_OBSERVE)    /* Alle "positiven" HELPs zusammen */
/* HELP_OBSERVE deaktiviert */
/* ------------------------------------------------------------- */
/* Prototypen */


#define ALLIED_TAX     1
#define ALLIED_NOBLOCK 2
#define ALLIED_HELP    4

/* alle vierstelligen zahlen: */
#define MAX_UNIT_NR (36*36*36*36-1)
#define MAX_CONTAINER_NR (36*36*36*36-1)


#endif
