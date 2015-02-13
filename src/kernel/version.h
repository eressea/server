/* 
 +-------------------+  
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2007   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |
 +-------------------+

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */
#define INTPAK_VERSION 329      /* in binary, ints can get packed */
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
#define CASTLE_DAMAGE_VERSION 346 /* New value for building damage */

#define MIN_VERSION INTPAK_VERSION      /* minimal datafile we support */
#define RELEASE_VERSION CASTLE_DAMAGE_VERSION /* current datafile */

#define STREAM_VERSION 2 /* internal encoding of binary files */
