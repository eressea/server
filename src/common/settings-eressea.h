/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2002   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

/*
 * Contains defines for the "free" game (Eressea) .
 * Include this file from settings.h to make eressea work.
 */
#define CATAPULT_AMMUNITION 1	/* Gebaut werden kann sie auch mit 0! */
#define CHANGED_CROSSBOWS 1
#define COMBAT_TURNS 5
#define ENTERTAINBASE 0
#define ENTERTAINPERLEVEL 20
#define ENTERTAINFRACTION 20
#define GAME_ID 0
#define GROWING_TREES 1
#define GUARD_DISABLES_PRODUCTION 1
#define GUARD_DISABLES_RECRUIT 1
#define HUNGER_DISABLES_LONGORDERS 1
#define IMMUN_GEGEN_ANGRIFF 8
#define LARGE_CASTLES 1
#define NEW_MIGRATION 1
#define NEW_RESOURCEGROWTH 1
#define NMRTIMEOUT 4
#define PEASANTS_DO_NOT_STARVE 0
#define PEASANT_ADJUSTMENT 1
#define RACE_ADJUSTMENTS 1
#define RECRUITFRACTION 40		/* 100/RECRUITFRACTION% */
#define REDUCED_PEASANTGROWTH 1
#define REMOVENMRNEWBIE 1
#define RESOURCE_CONVERSION 1
#define RESOURCE_QUANTITY 0.5
#define TEACHDIFFERENCE 2
#define GIVERESTRICTION 3
#define NEWATSROI 0

#define CHECK_OVERLOAD_ON_ENTER
#undef REGIONOWNERS

#define MUSEUM_MODULE
#define ARENA_MODULE

#define MAILITPATH	"/usr/sbin:$HOME/eressea/bin:/bin:/usr/bin:/usr/local/bin"

/* Krücke für die Berechnung der Jahreszeiten in Eressea */

#define FIRST_TURN 184

