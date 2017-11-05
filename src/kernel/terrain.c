/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#include <platform.h>
#include <kernel/config.h>
#include "terrain.h"

#include <attributes/racename.h>
#include <spells/regioncurse.h>

/* kernel includes */
#include "curse.h"
#include "region.h"
#include "resources.h"
#include "terrainid.h"

#include <util/log.h>
#include <util/attrib.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAXTERRAINS 14

const char *terraindata[MAXTERRAINS] = {
    "ocean",
    "plain",
    "swamp",
    "desert",
    "highland",
    "mountain",
    "glacier",
    "firewall",
    "fog",
    "thickfog",
    "volcano",
    "activevolcano",
    "iceberg_sleep",
    "iceberg"
};

static terrain_type *registered_terrains;

void free_terrains(void)
{
    while (registered_terrains) {
        int n;
        terrain_type * t = registered_terrains;
        registered_terrains = t->next;
        free(t->_name);
        free(t->herbs);
        if (t->production) {
            for (n = 0; t->production[n].type; ++n) {
                free(t->production[n].base);
                free(t->production[n].divisor);
                free(t->production[n].startlevel);
            }
            free(t->production);
        }
        free(t);
    }
}

const terrain_type *terrains(void)
{
    return registered_terrains;
}

static const char *plain_name(const struct region *r)
{
    /* TODO: xml defined */
    if (r_isforest(r)) {
        return "forest";
    }
    return r->terrain->_name;
}

static terrain_type *terrain_find_i(const char *name)
{
    terrain_type *terrain;
    for (terrain = registered_terrains; terrain; terrain = terrain->next) {
        if (strcmp(terrain->_name, name) == 0) {
            break;
        }
    }
    return terrain;
}

const terrain_type *get_terrain(const char *name) {
    return terrain_find_i(name);
}

terrain_type * get_or_create_terrain(const char *name) {
    terrain_type *terrain = terrain_find_i(name);
    if (!terrain) {
        terrain = (terrain_type *)calloc(sizeof(terrain_type), 1);
        if (terrain) {
            terrain->_name = strdup(name);
            terrain->next = registered_terrains;
            registered_terrains = terrain;
            if (strcmp("plain", name) == 0) {
                /* TODO: this is awful, it belongs in config */
                terrain->name = &plain_name;
            }
        }
    }
    return terrain;
}

static const terrain_type *newterrains[MAXTERRAINS];

const struct terrain_type *newterrain(terrain_t t)
{
    const struct terrain_type *result;
    if (t == NOTERRAIN) {
        return NULL;
    }
    assert(t >= 0);
    assert(t < MAXTERRAINS);
    result = newterrains[t];
    if (!result) {
        result = newterrains[t] = get_terrain(terraindata[t]);
    }
    if (!result) {
        log_error("no such terrain: %s.", terraindata[t]);
    }
    return result;
}

terrain_t oldterrain(const struct terrain_type * terrain)
{
    terrain_t t;
    assert(terrain);
    for (t = 0; t != MAXTERRAINS; ++t) {
        if (newterrains[t] == terrain)
            return t;
    }
    log_debug("%s is not a classic terrain.\n", terrain->_name);
    return NOTERRAIN;
}

const char *terrain_name(const struct region *r)
{
    if (r->attribs) {
        attrib *a = a_find(r->attribs, &at_racename);
        if (a) {
            const char *str = get_racename(a);
            if (str)
                return str;
        }
    }

    if (r->terrain->name != NULL) {
        return r->terrain->name(r);
    }
    else if (fval(r->terrain, SEA_REGION)) {
        if (curse_active(get_curse(r->attribs, &ct_maelstrom))) {
            return "maelstrom";
        }
    }
    return r->terrain->_name;
}

void init_terrains(void)
{
    terrain_t t;
    for (t = 0; t != MAXTERRAINS; ++t) {
        const terrain_type *newterrain = newterrains[t];
        if (newterrain != NULL)
            continue;
        if (terraindata[t] != NULL) {
            newterrain = get_terrain(terraindata[t]);
            if (newterrain != NULL) {
                newterrains[t] = newterrain;
            }
        }
    }
}
