#pragma once

#ifndef _GAMEDATA_H
#define _GAMEDATA_H

#include <stream.h>

#define INTPAK_VERSION 329      /* in binary, ints can get packed. starting with E2/572 */
#define NOZEROIDS_VERSION 330   /* 2008-05-16 zero is not a valid ID for anything (including factions) */
#define NOBORDERATTRIBS_VERSION 331     /* 2008-05-17 connection::attribs has been moved to userdata */
#define UIDHASH_VERSION 332     /* 2008-05-22 borders use the region.uid to store */
#define REGIONOWNER_VERSION 333 /* 2009-05-14 regions have owners and morale */
#define ALLIANCELEADER_VERSION 333      /* alliances have a leader */
#define CURSEFLOAT_VERSION 334  /* all curse-effects are float */
#define MOURNING_VERSION 335    /* mourning peasants */
#define FOSS_VERSION 336        /* the open source release */
#define OWNER_2_VERSION 337     /* region owners contain an alliance */
#define FIX_WATCHERS_VERSION 338     /* fixed storage of watchers */
#define UNIQUE_SPELLS_VERSION 339    /* turn 775, spell names are now unique globally, not just per school */
#define SPELLBOOK_VERSION 340        /* turn 775, full spellbooks are stored for factions */
#define NOOVERRIDE_VERSION 341        /* turn 775, full spellbooks are stored for factions */
#define INTFLAGS_VERSION 342   /* turn 876, FFL_NPC is now bit 25, flags is an int */
#define SAVEGAMEID_VERSION 343 /* instead of XMLNAME, save the game.id parameter from the config */
#define BUILDNO_VERSION 344 /* storing the build number in the save */
#define AUTO_RACENAME_VERSION 345 /* NPC units with name==NULL will automatically get their race for a name */
#define JSON_REPORT_VERSION 346 /* bit 3 in f->options flags the json report */
#define EXPLICIT_CURSE_ISNEW_VERSION 347 /* CURSE_ISNEW is not reset in read/write, but in age() */
#define SPELL_LEVEL_VERSION 348 /* f->max_spelllevel gets stored, not calculated */
#define OWNER_3_VERSION 349 /* regions store last owner, not last alliance */
#define ATTRIBOWNER_VERSION 351 /* all attrib_type functions know who owns the attribute */
#define BADCRYPT_VERSION 351 /* passwords are broken, 969.dat only. */
#define NOCRYPT_VERSION 352 /* passwords are plaintext again */
#define ATHASH_VERSION 353 /* attribute-type hash, not name */
#define NOWATCH_VERSION 354 /* plane->watchers is gone */
#define KEYVAL_VERSION 355 /* at_keys has values */
#define NOLANDITEM_VERSION 356 /* land_region has no items */
#define NORCSPELL_VERSION 357 /* data contains no RC_SPELL units */
#define SORTKEYS_VERSION 358 /* at_keys is sorted */
#define FAMILIAR_FIX_VERSION 359 /* at_keys is sorted */
/* unfinished: */
#define CRYPT_VERSION 400 /* passwords are encrypted */

#define RELEASE_VERSION FAMILIAR_FIX_VERSION /* current datafile */
#define MIN_VERSION INTPAK_VERSION      /* minimal datafile we support */
#define MAX_VERSION RELEASE_VERSION /* change this if we can need to read the future datafile, and we can do so */

#define STREAM_VERSION 2 /* internal encoding of binary files */

struct storage;

typedef struct gamedata {
    struct storage *store;
    stream strm;
    int version;
} gamedata;

void gamedata_init(gamedata *data, struct storage *store, int version);
void gamedata_done(gamedata *data);

void gamedata_close(gamedata *data);
gamedata *gamedata_open(const char *filename, const char *mode, int version);
int gamedata_openfile(gamedata *data, const char *filename, const char *mode, int version);

#define STREAM_VERSION 2 /* internal encoding of binary files */

#endif
