/* vi: set ts=2:
 *
 *	$Id: terrain.c,v 1.2 2001/01/26 16:19:40 enno Exp $
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

#include <config.h>
#include "eressea.h"
#include "terrain.h"

#include "curse.h"
#include "region.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#ifdef NO_FOREST
static const char * plain_herbs[] = {"Flachwurz", "Würziger Wagemut", "Eulenauge", "Grüner Spinnerich", "Würziger Wagemut", "Elfenlieb", NULL};
#else
static const char * plain_herbs[] = {"Flachwurz", "Würziger Wagemut", "Eulenauge", NULL};
static const char * forest_herbs[] = {"Grüner Spinnerich", "Blauer Baumringel", "Elfenlieb", NULL};
#endif
static const char * swamp_herbs[] = {"Gurgelkraut", "Knotiger Saugwurz", "Blasenmorchel", NULL};
static const char * desert_herbs[] = {"Wasserfinder", "Kakteenschwitz", "Sandfäule", NULL};
static const char * highland_herbs[] = {"Windbeutel", "Fjordwuchs", "Alraune", NULL};
static const char * mountain_herbs[] = {"Steinbeißer", "Spaltwachs", "Höhlenglimm", NULL};
static const char * glacier_herbs[] = {"Eisblume", "Weißer Wüterich", "Schneekristall", NULL};

const char *
trailinto(const region * r, const locale * lang)
{
	char ref[32];
	const char * s;
	if (r) {
		terrain_t t = r->terrain;
		if (is_cursed(r->attribs, C_MAELSTROM, 0)) {
			/* das kostet. evtl. wäre ein FL_CURSED gut? */
			s = locale_string(lang, "maelstrom_trail");
		}
		else if (terrain[t].trailname==NULL) {
			strcat(strcpy(ref, terrain[t].name), "_trail");
			s = locale_string(lang, ref);
		}
		else s = locale_string(lang, terrain[t].trailname(r));
		if (s && *s) {
			assert(strstr(s, "%s"));
			return s;
		}
	}
	return "%s";
}

const char *
trailintoplain(const region * r)
{
	assert(r->terrain==T_PLAIN);
	if (r_isforest(r)) return "forest_trail";
	return "plain_trail";
}

const terraindata_t terrain[] = {
	/* T_OCEAN */
	{
		"ocean", '.',
		NULL,
		NULL,
		0, /* Steine pro Runde */
		-1,	/* Steine fuer Strasse */
		10, /* bewirtschaftbare Parzellen */
		SAIL_INTO|FLY_INTO|LARGE_SHIPS|SWIM_INTO,	/* Flags */
		NULL
	},
	/* T_PLAIN */
	{
		"plain", 'P',
		trailintoplain,
		"in die Ebene von %s",
		0, /* Steine pro Runde */
		50,	/* Steine fuer Strasse */
		1000, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LARGE_SHIPS|LAND_REGION,	/* Flags */
		plain_herbs
	},
#ifndef NO_FOREST
	/* T_FOREST */
	{
		"forest", 'F',
		NULL,
		"in den Wald von %s",
		0, /* Steine pro Runde */
		50,	/* Steine fuer Strasse */
		1000, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LARGE_SHIPS|LAND_REGION,	/* Flags */
		forest_herbs
	},
#endif
	/* T_SWAMP */
	{
		"swamp", 'S',
		NULL,
		"in den Sumpf von %s",
		0, /* Steine pro Runde */
		75,	/* Steine fuer Strasse - nur mit Damm */
		200, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		swamp_herbs
	},
	/* T_DESERT */
	{
		"desert", 'D',
		NULL,
		"durch die Wüste von %s",
		0, /* Steine pro Runde */
		100,	/* Steine fuer Strasse - nur mit Karawanserei */
		50, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		desert_herbs
	},
	/* T_HIGHLAND */
	{
		"highland", 'H',
		NULL,
		"auf das Hochland von %s",
		0, /* Steine pro Runde */
		100,	/* Steine fuer Strasse */
		400, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		highland_herbs
	},
	/* T_MOUNTAIN */
	{
		"mountain", 'M',
		NULL,
		"in die Berge von %s",
		100, /* Steine pro Runde */
		250,	/* Steine fuer Strasse */
		100, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		mountain_herbs
	},
	/* T_GLACIER */
	{
		"glacier", 'G',
		NULL,
		"auf den Gletscher von %s",
		1, /* Steine pro Runde */
		250,	/* Steine fuer Strasse - nur mit Tunnel */
		10, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		glacier_herbs
	},
	/* T_FIREWALL */
	{
		"firewall", '@',
		NULL,
		NULL,
		0, /* Steine pro Runde */
		-1,	/* Steine fuer Strasse */
		0, /* bewirtschaftbare Parzellen */
		FORBIDDEN_LAND,	/* Flags */
		NULL
	},
	/* T_HELL */
	{
		"hell", '&',
		NULL,
		NULL,
		0, /* Steine pro Runde */
		-1,	/* Steine fuer Strasse */
		0, /* bewirtschaftbare Parzellen */
		WALK_INTO,	/* Flags */
		NULL
	},
	/* T_GRASSLAND */
	{
		"grassland", 'L',
		NULL,
		"in die Steppe von %s",
		0, /* Steine pro Runde */
		125,	/* Steine fuer Strasse */
		500, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		highland_herbs
	},
	/* T_ASTRAL */
	{
		"fog", '%',
		NULL,
		NULL,
		0, /* Steine pro Runde */
		-1,	/* Steine fuer Strasse */
		0, /* bewirtschaftbare Parzellen */
		WALK_INTO|SWIM_INTO|FLY_INTO,	/* Flags */
		NULL
	},
	/* T_ASTRALB */
	{
		"thickfog", '*',
		NULL,
		NULL,
		0, /* Steine pro Runde */
		-1,	/* Steine fuer Strasse */
		0, /* bewirtschaftbare Parzellen */
		FORBIDDEN_LAND,	/* Flags */
		NULL
	},
	{
		"volcano", 'v',
		NULL,
		"zum Vulkan von %s",
		50,		/* Steine pro Runde */
		250,	/* Steine fuer Strasse */
		50, 	/* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		NULL
	},
	{
		"activevolcano", 'V',
		NULL,
		"zum aktiven Vulkan von %s",
		50,		/* Steine pro Runde */
		250,	/* Steine fuer Strasse */
		50, 	/* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		NULL
	},
	/* T_ICEBERG_SLEEP */
	{
		"glacier", 'i',
		NULL,
		"auf den Gletscher von %s",
		1, /* Steine pro Runde */
		250,	/* Steine fuer Strasse - nur mit Tunnel */
		10, /* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		glacier_herbs
	},
	/* T_ICEBERG */
	{
		"iceberg", 'I',
		NULL,
		"auf einen Eisberg",
		1,		/* Steine pro Runde */
		250,	/* Steine fuer Strasse */
		10, 	/* bewirtschaftbare Parzellen */
		NORMAL_TERRAIN|LAND_REGION,	/* Flags */
		glacier_herbs
	},
	/* T_HALL1 */
	{
		"hall1", '%',
		NULL,
		"in eine Halle",
		0,		/* Steine pro Runde */
		0,		/* Steine fuer Strasse */
		0, 		/* bewirtschaftbare Parzellen */
		WALK_INTO|LAND_REGION,	/* Flags */
		NULL
	},
	/* T_CORRIDOR1 */
	{
		"corridor1", '%',
		NULL,
		"in einen Gang",
		0,		/* Steine pro Runde */
		0,		/* Steine fuer Strasse */
		0, 		/* bewirtschaftbare Parzellen */
		WALK_INTO|LAND_REGION,	/* Flags */
		NULL
	},

	/*** sentinel - must be last ***/
	{
		NULL, 'X',
		NULL,
		NULL,
		0, /* Steine pro Runde */
		-1,	/* Steine fuer Strasse */
		0, /* bewirtschaftbare Parzellen */
		0,	/* Flags */
	}
};
