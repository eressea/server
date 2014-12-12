/* vi: set ts=2:
 +-------------------+  
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2007   |  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |
 +-------------------+

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 
 */

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

/*
#define HEX_VERSION 81
#define GROWTREE_VERSION 305
#define RANDOMIZED_RESOURCES_VERSION 306
#define NEWRACE_VERSION 307
#define INTERIM_VERSION 309
#define NEWSKILL_VERSION 309
#define WATCHERS_VERSION 310
#define OVERRIDE_VERSION 311
*/
#define CURSETYPE_VERSION 312   /* turn 287 */
#define ALLIANCES_VERSION 313
#define DBLINK_VERSION 314
#define CURSEVIGOURISFLOAT_VERSION 315
#define SAVEXMLNAME_VERSION 316
#define SAVEALLIANCE_VERSION 317
#define CLAIM_VERSION 318
/* 319 is the HSE4 data version */
#define BACTION_VERSION 319     /* building action gets a param string */
#define NOLASTORDER_VERSION 320 /* do not use lastorder */
#define SPELLNAME_VERSION 321   /* reference spells by name */
#define TERRAIN_VERSION 322     /* terrains are a full type and saved by name */
#define REGIONITEMS_VERSION 323 /* regions have items */
#define ATTRIBREAD_VERSION 324  /* remove a_readint */
#define CURSEFLAGS_VERSION 325  /* remove a_readint */
#define UNICODE_VERSION 326     /* 2007-06-27 everything is stored as UTF8 */
#define UID_VERSION 327         /* regions have a unique id */
#define STORAGE_VERSION 328     /* with storage.h, some things are stored smarter (ids as base36, fractions as float) */
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
#define MIN_VERSION INTPAK_VERSION      /* minimal datafile we support */
#define RELEASE_VERSION AUTO_RACENAME_VERSION /* current datafile */

#define STREAM_VERSION 2 /* internal encoding of binary files */
