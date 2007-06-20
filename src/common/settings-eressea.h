/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

/*
 * Contains defines for the "free" game (Eressea) .
 * Include this file from settings.h to make eressea work.
 */
#define ENTERTAINFRACTION 20
#define TEACHDIFFERENCE 2
#define GUARD_DISABLES_RECRUIT 1
#define GUARD_DISABLES_PRODUCTION 1
#define RESOURCE_QUANTITY 0.5
#define RECRUITFRACTION 40    /* 100/RECRUITFRACTION% */
#define COMBAT_TURNS 5
#define NEWATSROI 0

/* Vermehrungsrate Bauern in 1/10000.
* Evt. Berechnungsfehler, reale Vermehrungsraten scheinen höher. */
#define PEASANTGROWTH		10
#define BATTLE_KILLS_PEASANTS 20
#define PEASANTLUCK			10

#define HUNGER_REDUCES_SKILL /* Hunger reduziert den Talentwert
                                auf die Hälfte */

#define ASTRAL_ITEM_RESTRICTIONS /* keine grossen dinge im astralraum */
#define NEW_DAEMONHUNGER_RULE
#define NEW_COMBATSKILLS_RULE
#define ROW_FACTOR 3 /* factor for combat row advancement rule */
#define HEROES

#define SCORE_MODULE
#define MUSEUM_MODULE
#define ARENA_MODULE
#define WORMHOLE_MODULE
#define XECMD_MODULE

#undef FUZZY_BASE36
#define SIMPLE_COMBAT
#define SIMPLE_ESCAPE

#define RESOURCE_CONVERSION /* support for data files < NEWRESOURCE_VERSION */
