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
 * Contains defines for the "alliance" vinyambar game (wdw).
 * Include this file from settings.h to make eressea work.
 */
#define ENTERTAINFRACTION 20
#define IMMUN_GEGEN_ANGRIFF 8
#define RESOURCE_CONVERSION 1
#define NEW_RESOURCEGROWTH 1
#define LARGE_CASTLES 1
#define GROWING_TREES 1
#define REDUCED_PEASANTGROWTH 1
#define RACE_ADJUSTMENTS 1
#define TEACHDIFFERENCE 2
#define PEASANT_ADJUSTMENT 1
#define GUARD_DISABLES_RECRUIT 1
#define GUARD_DISABLES_PRODUCTION 1
#define RESOURCE_QUANTITY 0.5
#define RECRUITFRACTION 40		/* 100/RECRUITFRACTION% */
#define CATAPULT_AMMUNITION 1	/* Gebaut werden kann sie auch mit 0! */
#define CHANGED_CROSSBOWS 1
#define COMBAT_TURNS 5
#define PEASANTS_DO_NOT_STARVE 0
#define NEW_MIGRATION 1
#define ASTRAL_HUNGER

#define NEWATSROI 1
#if NEWATSROI == 1
#define ATSBONUS 2
#define ROIBONUS 4
#endif

#undef  ALLIANCEJOIN

#undef DUNGEON_MODULE
#undef MUSEUM_MODULE
#undef ARENA_MODULE
#undef WORMHOLE_MODULE
#undef XECMD_MODULE
#define WDW_PHOENIX
#define WDW_PYRAMIDSPELL
#define NEW_STARTEQUIPMENT
