/* 
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */

#define ENTERTAINFRACTION 20
#define TEACHDIFFERENCE 2
#define GUARD_DISABLES_RECRUIT 1
#define GUARD_DISABLES_PRODUCTION 1
#define RESOURCE_QUANTITY 0.5
#define RECRUITFRACTION 40      /* 100/RECRUITFRACTION% */
#define COMBAT_TURNS 5
#undef NEWATSROI

/* Vermehrungsrate Bauern in 1/10000.
* TODO: Evt. Berechnungsfehler, reale Vermehrungsraten scheinen höher. */
#define PEASANTGROWTH 10
#define PEASANTLUCK 10

#define ROW_FACTOR 3            /* factor for combat row advancement rule */

/* optional game components. TODO: These should either be 
 * configuration variables (XML), script extensions (lua),
 * or both. We don't want separate binaries for different games
 */
#define MUSEUM_MODULE 1
#define ARENA_MODULE 1

#undef REGIONOWNERS             /* (WIP) region-owner uses HELP_TRAVEL to control entry to region */

 /* experimental gameplay features (that don't affect the savefile) */
 /* TODO: move these settings to settings.h or into configuration files */
#define GOBLINKILL              /* Goblin-Spezialklau kann tödlich enden */
#define INSECT_POTION           /* Spezialtrank für Insekten */
#define ORCIFICATION            /* giving snotlings to the peasants gets counted */

#define TREESIZE (8)            /* space used by trees (in #peasants) */

#define PEASANTFORCE 0.75       /* Chance einer Vermehrung trotz 90% Auslastung */

 /* Gebäudegröße = Minimalbelagerer */
#define SIEGEFACTOR     2

 /** Magic */
#define MAXMAGICIANS    3
#define MAXALCHEMISTS   3

#define ENCCHANCE           10  /* %-Chance für einmalige Zufallsbegegnung */
#define BAGCAPACITY         20000   /* soviel paßt in einen Bag of Holding */
#define PERSON_WEIGHT 1000      /* weight of a "normal" human unit */
#define STAMINA_AFFECTS_HP 1<<0
