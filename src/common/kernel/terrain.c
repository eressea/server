/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "terrainid.h"

/* kernel includes */
#include "curse.h"
#include "region.h"
#include "resources.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const char * plain_herbs[] = {"Flachwurz", "Würziger Wagemut", "Eulenauge", "Grüner Spinnerich", "Blauer Baumringel", "Elfenlieb", NULL};
static const char * swamp_herbs[] = {"Gurgelkraut", "Knotiger Saugwurz", "Blasenmorchel", NULL};
static const char * desert_herbs[] = {"Wasserfinder", "Kakteenschwitz", "Sandfäule", NULL};
static const char * highland_herbs[] = {"Windbeutel", "Fjordwuchs", "Alraune", NULL};
static const char * mountain_herbs[] = {"Steinbeißer", "Spaltwachs", "Höhlenglimm", NULL};
static const char * glacier_herbs[] = {"Eisblume", "Weißer Wüterich", "Schneekristall", NULL};

#define MAXTERRAINS 20

const char * terraindata[] = {
	"ocean",
	"plain",
	"swamp",
	"desert",
	"highland",
	"mountain",
	"glacier",
	"firewall",
	"hell", /* dungeon module */
  "plain",  /* former grassland */
  "fog",
  "thickfog",
  "volcano",
	"activevolcano",
  "iceberg_sleep",
  "iceberg",
  
  "hall1", /* museum module */
  "corridor1", /* museum module */
  "plain", /* former magicstorm */
  "wall1", /* museum module */
  NULL
};

static terrain_type * registered_terrains;

const terrain_type * 
terrains(void)
{
  return registered_terrains;
}

static const char *
plain_name(const struct region * r)
{
  /* TODO: xml defined */
  if (r_isforest(r)) return "forest";
  return r->terrain->_name;
}

void
register_terrain(struct terrain_type * terrain)
{
  assert(terrain->next==NULL),
  terrain->next = registered_terrains;
  registered_terrains = terrain;
  if (strcmp("plain", terrain->_name)==0) 
    terrain->name = &plain_name;
}

const struct terrain_type *
get_terrain(const char * name)
{
  const struct terrain_type * terrain;
  for (terrain=registered_terrains;terrain;terrain=terrain->next) {
    if (strcmp(terrain->_name, name)==0) break;
  }
  return terrain;
}

static const terrain_type * newterrains[MAXTERRAINS];

const struct terrain_type *
newterrain(terrain_t t)
{
  if (t==NOTERRAIN) return NULL;
  assert(t>=0);
  assert(t<MAXTERRAINS);
  return newterrains[t];
}

terrain_t
oldterrain(const struct terrain_type * terrain)
{
  terrain_t t;
  if (terrain==NULL) return NOTERRAIN;
  for (t=0;t!=MAXTERRAINS;++t) {
    if (newterrains[t]==terrain) return t;
  }
  log_warning(("%s is not a classic terrain", terrain->_name));
  return NOTERRAIN;
}

const char *
terrain_name(const struct region * r)
{
  if (r->terrain->name!=NULL) return r->terrain->name(r);
  return r->terrain->_name;
}

void
init_terrains(void)
{
  terrain_t t;
  for (t=0;t!=MAXTERRAINS;++t) {
    const terrain_type * newterrain = newterrains[t];
    if (newterrain!=NULL) continue;
    newterrain = get_terrain(terraindata[t]);
    if (newterrain!=NULL) {
      newterrains[t] = newterrain;
    } else {
      log_error(("missing classic terrain %s\n", terraindata[t]));
    }
  }
}
