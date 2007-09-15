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
/* 319 is the HSE4 data version */
#define BACTION_VERSION 319 /* building action gets a param string */
#define NOLASTORDER_VERSION 320 /* do not use lastorder */
#define SPELLNAME_VERSION 321 /* reference spells by name */
#define TERRAIN_VERSION 322 /* terrains are a full type and saved by name */
#define REGIONITEMS_VERSION 323 /* regions have items */
#define ATTRIBREAD_VERSION 324 /* remove a_readint */
#define CURSEFLAGS_VERSION 325 /* remove a_readint */
#define UNICODE_VERSION 326 /* everything is stored as UTF8 */

#define MIN_VERSION CURSETYPE_VERSION
#define REGIONOWNERS_VERSION 400

#ifdef ENEMIES
# define RELEASE_VERSION ENEMIES_VERSION
#else
# define RELEASE_VERSION CURSEFLAGS_VERSION
#endif
