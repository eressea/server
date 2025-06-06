#pragma once

#ifndef _GAMEDATA_H
#define _GAMEDATA_H

#include <stream.h>

#define UIDHASH_VERSION 332     /* 2008-05-22 = 572 borders use the region.uid to store */
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
#define FAMILIAR_FIX_VERSION 359 /* familiar links are fixed */
#define SKILLSORT_VERSION 360 /* u->skills is sorted */
#define LANDDISPLAY_VERSION 360 /* r.display is now in r.land.display */
#define FIXATKEYS_VERSION 361 /* remove global.attribs, fix at_keys */
#define FACTION_UID_VERSION 362 /* f->uid contains a database id */
#define CRYPT_VERSION 363 /* passwords are encrypted */
#define FAMILIAR_FIXMAGE_VERSION 364 /* familiar links are fixed */
#define FAMILIAR_FIXSPELLBOOK_VERSION 365 /* familiar spells are fixed */
#define FIX_STARTLEVEL_VERSION 366 /* fixing resource startlevels */
#define FIX_RES_BASE_VERSION 367 /* fixing resource base */
#define FIX_CLONES_VERSION 368 /* dissolve clones */
#define FIX_MIGRANT_AURA_VERSION 369 /* bug 2585, migrants with aura */
#define SHIP_NUMBER_VERSION 370 /* ships have a number */
#define FIX_SHAPESHIFT_VERSION 371 /* shapeshifting demons */
#define FIX_SEAROADS_VERSION 372 /* removing roads in ocean regions */
#define FIX_SHAPESHIFT_SPELL_VERSION 373 /* shapeshift spell, bug 2719 */
#define FIX_SHIP_DAMAGE_VERSION 374 /* ship damage for fleets */
#define FIX_TELEPORT_PLANE_VERSION 375 /* astral space not created, bug 2856 */
#define FIX_RESOURCES_VERSION 376 /* badly made rawmaterials, bug 2898 */
#define REMOVE_FACTION_AGE_VERSION 377 /* save start_turn, not age, bug 2878 */
#define FORBIDDEN_LAND_VERSION 378 /* forbidden regions (walls) do not store land info */
#define FIX_SHADOWS_VERSION 379 /* shadowdemon/-master skills, bug 3011 */
#define FIX_SHAPESHIFT_IRACE_VERSION 380 /* shapeshift spell, bug 2991 */
#define SKILL_DAYS_VERSION 381 /* skills are stored as days, not weeks */
#define BORDER_ID_VERSION 382 /* borders no longer have an id, from turn 1376 */
#define SLAVE_DATA_VERSION 383 /* ct_slavery has curse-data */
#define WALL_DATA_VERSION 384 /* wall_data no longer has mage stored in it */
#define FIX_ROADS_VERSION 385 /* regions did not write the new roads */

#define RELEASE_VERSION FIX_ROADS_VERSION /* use for new datafiles */
#define MIN_VERSION UIDHASH_VERSION      /* minimal datafile we support */
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

#endif
