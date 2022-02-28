#ifndef ERESSEA_TYPES_H
#define ERESSEA_TYPES_H

#include <settings.h>

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
struct selist;
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

/* seen_mode: visibility in the report */
typedef enum seen_mode {
    seen_none,
    seen_neighbour,
    seen_lighthouse_land,
    seen_lighthouse,
    seen_travel,
    seen_unit,
    seen_spell,
    seen_battle
} seen_mode;

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
  O_SHOWSKCHANGE,               /* 4096 - Skillveraenderungen anzeigen? */
  MAXOPTIONS
};

typedef enum magic_t {
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

/* ------------------------------------------------------------- */
/* Prototypen */

#endif
