/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef TERRAIN_H
#define TERRAIN_H

#define landregion(t) (t!=T_OCEAN && t!=T_FIREWALL && t!=T_ASTRAL && t!=T_ASTRALB && t!=T_FIREWALL && t!=T_HELL && t!=T_MAGICSTORM)
enum {
	T_OCEAN,
	T_PLAIN,
#ifndef NO_FOREST
	T_FOREST,					/* wird zu T_PLAIN konvertiert */
#endif
	T_SWAMP,
	T_DESERT,					/* kann aus T_PLAIN entstehen */
	T_HIGHLAND,
	T_MOUNTAIN,
	T_GLACIER,		/* kann aus T_MOUNTAIN entstehen */
	T_FIREWALL,		/* Unpassierbar */
	T_HELL,				/* Hölle */
	T_GRASSLAND,
	T_ASTRAL,
	T_ASTRALB,
	T_VOLCANO,
	T_VOLCANO_SMOKING,
	T_ICEBERG_SLEEP,
	T_ICEBERG,
	T_HALL1,
	T_CORRIDOR1,
	T_MAGICSTORM,
	MAXTERRAINS,
	NOTERRAIN = (terrain_t) - 1
};

/* diverse Flags */
#define BUILD_BUILDINGS	(1<<0)		/* man darf Gebäude bauen */
#define BUILD_SHIPS		  (1<<1)		/* man darf Schiffe bauen */
/* Strassen können gebaut werden, wenn roadreq > 0 */
#define WALK_INTO		    (1<<2)		/* man darf hierhin laufen */
#define SAIL_INTO		    (1<<3)		/* man darf hierhin segeln */
#define FLY_INTO		    (1<<4)		/* man darf hierhin fliegen */
#define SWIM_INTO		    (1<<5)		/* man darf hierhin schwimmen */
#define LARGE_SHIPS		  (1<<6)		/* grosse Schiffe dürfen hinfahren */
#define FORBIDDEN_LAND	(1<<7)		/* unpassierbare Blockade-struct region */
#define LAND_REGION     (1<<8)		/* Standard-Land-struct region */

#define NORMAL_TERRAIN	(BUILD_BUILDINGS|BUILD_SHIPS|WALK_INTO|SAIL_INTO|FLY_INTO)

typedef struct terraindata_t {
	const char *name;              /* "Ozean", "Ebene" etc */
	char symbol;             /* Merian-Zeichen: '.', 'E', etc */
	const char *(*trailname)(const struct region * r); /* (im Norden liegt) "der Wald von %s" */
	const char *roadinto;          /* (eine Strasse fuehrt) "in den Wald von %s" */
	                         /* ist NULL, wenn kein Strassenbau möglich */

	int quarries;			/* abbaubare Steine pro Runde */
	int roadreq;			/* Steine fuer Strasse (-1 = nicht machbar) */
	int production_max;		/* bebaubare Parzellen (10 Leute pro Parzelle) */
	/* Achtung: Produktion wird durch Flueche (zB Duerre) beeinflusst.
	 * Die Funktion production(struct region *r) in economic.c ermittelt den
	 * aktuellen Stand fuer eine bestimmte struct region.
	 */
	unsigned int flags;
	const char ** herbs;
} terraindata_t;

extern const char * trailinto(const struct region * r, const struct locale * lang);

extern const terraindata_t terrain[];

#endif /* TERRAIN_H */
