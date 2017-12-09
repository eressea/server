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
#define RESOURCE_QUANTITY 0.5
#define RECRUITFRACTION 40      /* 100/RECRUITFRACTION% */
#define COMBAT_TURNS 5
#undef NEWATSROI

/* Vermehrungsrate Bauern in 1/10000.
* TODO: Evt. Berechnungsfehler, reale Vermehrungsraten scheinen hoeher. */
#define PEASANTGROWTH 10
#define PEASANTLUCK 10

#define ROW_FACTOR 3            /* factor for combat row advancement rule */

 /* TODO: move these settings to settings.h or into configuration files */
#define TREESIZE (8)            /* space used by trees (in #peasants) */
#define PEASANTFORCE 0.75       /* Chance einer Vermehrung trotz 90% Auslastung */

/* Gebaeudegroesse = Minimalbelagerer */
#define SIEGEFACTOR     2

#define ENCCHANCE           10  /* %-Chance fuer einmalige Zufallsbegegnung */
#define BAGCAPACITY         20000   /* soviel passt in einen Bag of Holding */
#define PERSON_WEIGHT 1000      /* weight of a "normal" human unit */
