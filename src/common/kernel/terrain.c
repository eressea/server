/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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
#include <kernel/eressea.h>
#include "terrain.h"
#include "terrainid.h"

/* kernel includes */
#include "curse.h"
#include "region.h"
#include "resources.h"

#include <util/log.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAXTERRAINS 20

const char * terraindata[MAXTERRAINS] = {
  "ocean",
  "plain",
  "swamp",
  "desert",
  "highland",
  "mountain",
  "glacier",
  "firewall",
  NULL, /* dungeon module */
  NULL,  /* former grassland */
  "fog",
  "thickfog",
  "volcano",
  "activevolcano",
  "iceberg_sleep",
  "iceberg",

  NULL, /* museum module */
  NULL, /* museum module */
  NULL, /* former magicstorm */
  NULL /* museum module */
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
  log_warning(("%s is not a classic terrain.\n", terrain->_name));
  return NOTERRAIN;
}

const char *
terrain_name(const struct region * r)
{
  if (r->terrain->name!=NULL) {
    return r->terrain->name(r);
  } else if (fval(r->terrain, SEA_REGION)) {
    if (is_cursed(r->attribs, C_MAELSTROM, 0)) {
      return "maelstrom";
    }
  }
  return r->terrain->_name;
}

void
init_terrains(void)
{
  terrain_t t;
  for (t=0;t!=MAXTERRAINS;++t) {
    const terrain_type * newterrain = newterrains[t];
    if (newterrain!=NULL) continue;
    if (terraindata[t]!=NULL) {
      newterrain = get_terrain(terraindata[t]);
      if (newterrain!=NULL) {
        newterrains[t] = newterrain;
      }
    }
  }
}
