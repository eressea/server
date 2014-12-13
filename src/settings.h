/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
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
#define RECRUITFRACTION 40      /* 100/RECRUITFRACTION% */
#define COMBAT_TURNS 5
#define NEWATSROI 0

/* Vermehrungsrate Bauern in 1/10000.
* Evt. Berechnungsfehler, reale Vermehrungsraten scheinen höher. */
#define PEASANTGROWTH		10
#define BATTLE_KILLS_PEASANTS 20
#define PEASANTLUCK			10

#define ASTRAL_ITEM_RESTRICTIONS        /* keine grossen dinge im astralraum */
#define NEW_DAEMONHUNGER_RULE
#define NEW_COMBATSKILLS_RULE
#define ROW_FACTOR 3            /* factor for combat row advancement rule */

/* optional game components. TODO: These should either be 
 * configuration variables (XML), script extensions (lua),
 * or both. We don't want separate binaries for different games
 */
#define SCORE_MODULE 1
#define MUSEUM_MODULE 1
#define ARENA_MODULE 1
#define CHANGED_CROSSBOWS 0     /* use the WTF_ARMORPIERCING flag */
#undef GLOBAL_WARMING           /* number of turns before global warming sets in */

#if defined(BINDINGS_LUABIND)
# undef BINDINGS_TOLUA
#elif defined(BINDINGS_TOLUA)
# undef BINDINGS_LUABIND
#else
# define BINDINGS_TOLUA         /* new default */
#endif

#undef REGIONOWNERS             /* (WIP) region-owner uses HELP_TRAVEL to control entry to region */
