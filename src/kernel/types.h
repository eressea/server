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

#ifndef ERESSEA_TYPES_H
#define ERESSEA_TYPES_H

/*
 * Features enabled:
 * If you are lacking the settings.h, create a new file common/settings.h,
 * and write #include <settings-config.h> (or whatever settings you want
 * your game to use) in there.
 * !!! DO NOT COMMIT THE SETTINGS.H FILE TO CVS !!!
 * settings.h should always be the first thing you include (after platform.h).
 */
#include <settings.h>
#include <util/variant.h>

typedef short terrain_t;
typedef short item_t;

struct attrib;
struct attrib_type;
struct ally;
struct building;
struct building_type;
struct curse;
struct curse_type;
struct castorder;
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
struct quicklist;
struct race;
struct region;
struct region_list;
struct resource_type;
struct ship;
struct ship_type;
struct skill;
struct spell;
struct spellbook;
struct storage;
struct strlist;
struct terrain_type;
struct unit;
struct weapon_type;

typedef struct ursprung {
  struct ursprung *next;
  int id;
  int x, y;
} ursprung;

/* seen_mode: visibility in the report */
typedef enum {
    seen_none,
    seen_neighbour,
    seen_lighthouse,
    seen_travel,
    seen_far,
    seen_unit,
    seen_battle
} seen_mode;

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

typedef enum {
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
  P_AFTER,
  P_BEFORE,
  P_NUMBER,
  P_ITEMS,
  P_POTIONS,
  P_GROUP,
  P_FACTIONSTEALTH,
  P_TREES,
  P_ALLIANCE,
  MAXPARAMS,
  NOPARAM 
} param_t;

typedef enum {                  /* Fehler und Meldungen im Report */
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

enum {                          /* Message-Level */
  ML_IMPORTANT,                 /* Sachen, die IMO erscheinen _muessen_ */
  ML_DEBUG,
  ML_MISTAKE,
  ML_WARN,
  ML_INFO,
  ML_MAX
};

extern const char *parameters[MAXPARAMS];

/* --------------- Reports Typen ------------------------------- */

enum {
  O_REPORT,                     /* 1 */
  O_COMPUTER,                   /* 2 */
  O_ZUGVORLAGE,                 /* 4 */
  O_JSON,                       /* 8 */
  O_STATISTICS,                 /* 16 */
  O_DEBUG,                      /* 32 */
  O_COMPRESS,                   /* 64 */
  O_NEWS,                       /* 128 */
  O_UNUSED_8,
  O_ADRESSEN,                   /* 512 */
  O_BZIP2,                      /* 1024 - compress as bzip2 */
  O_SCORE,                      /* 2048 - punkte anzeigen? */
  O_SHOWSKCHANGE,               /* 4096 - Skillveränderungen anzeigen? */
  MAXOPTIONS
};

typedef enum {
  M_GRAY = 0,                 /* Gray */
  M_ILLAUN = 1,               /* Illaun */
  M_TYBIED = 2,               /* Tybied */
  M_CERDDOR = 3,              /* Cerddor */
  M_GWYRRD = 4,               /* Gwyrrd */
  M_DRAIG = 5,                /* Draig */
  M_COMMON = 6,               /* common spells */
  MAXMAGIETYP,
  /* this enum is stored in the datafile, so do not change the numbers around */
  M_NONE = -1
} magic_t;

#define DONT_HELP      0
#define HELP_MONEY     1        /* Mitversorgen von Einheiten */
#define HELP_FIGHT     2        /* Bei Verteidigung mithelfen */
#define HELP_OBSERVE   4        /* Bei Wahrnehmung mithelfen */
#define HELP_GIVE      8        /* Dinge annehmen ohne KONTAKTIERE */
#define HELP_GUARD    16        /* Laesst Steuern eintreiben etc. */
#define HELP_FSTEALTH 32        /* Parteitarnung anzeigen. */
#define HELP_TRAVEL   64        /* Laesst Regionen betreten. */
#define HELP_ALL    (127-HELP_TRAVEL-HELP_OBSERVE)      /* Alle "positiven" HELPs zusammen */
/* HELP_OBSERVE deaktiviert */
/* ------------------------------------------------------------- */
/* Prototypen */

/* alle vierstelligen zahlen: */
#define MAX_UNIT_NR (36*36*36*36-1)
#define MAX_CONTAINER_NR (36*36*36*36-1)

#endif
