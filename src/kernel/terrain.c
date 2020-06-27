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
#include <kernel/attrib.h>
#include <util/strings.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

const char *terrainnames[MAXTERRAINS] = {
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
static int terrain_changes = 1;

bool terrain_changed(int *cache) {
    assert(cache);
    if (*cache != terrain_changes) {
        *cache = terrain_changes;
        return true;
    }
    return false;
}

void free_terrains(void)
{
    while (registered_terrains) {
        terrain_type * t = registered_terrains;
        registered_terrains = t->next;
        free(t->_name);
        free(t->herbs);
        if (t->production) {
            int n;
            for (n = 0; t->production[n].type; ++n) {
                free(t->production[n].base);
                free(t->production[n].divisor);
                free(t->production[n].startlevel);
            }
            free(t->production);
        }
        free(t);
    }
    ++terrain_changes;
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

static terrain_type *get_terrain_i(const char *name) {
    return terrain_find_i(name);
}

const terrain_type *get_terrain(const char *name) {
    return get_terrain_i(name);
}

terrain_type * get_or_create_terrain(const char *name) {
    terrain_type *terrain = terrain_find_i(name);
    if (!terrain) {
        ++terrain_changes;
        terrain = (terrain_type *)calloc(1, sizeof(terrain_type));
        if (terrain) {
            terrain->_name = str_strdup(name);
            terrain->next = registered_terrains;
            registered_terrains = terrain;
            if (strcmp(terrainnames[T_PLAIN], name) == 0) {
                /* TODO: this is awful, it belongs in config */
                terrain->name = &plain_name;
            }
        }
    }
    return terrain;
}

static terrain_type *newterrains[MAXTERRAINS];

const struct terrain_type *newterrain(terrain_t t)
{
    static int changed;
    const struct terrain_type *result;
    if (t == NOTERRAIN) {
        return NULL;
    }
    assert(t >= 0);
    assert(t < MAXTERRAINS);
    if (terrain_changed(&changed)) {
        memset(newterrains, 0, sizeof(newterrains));
        result = NULL;
    }
    else {
        result = newterrains[t];
    }
    if (!result) {
        result = newterrains[t] = get_terrain_i(terrainnames[t]);
    }
    if (!result) {
        log_error("no such terrain: %s.", terrainnames[t]);
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
        terrain_type *newterrain = newterrains[t];
        if (newterrain != NULL)
            continue;
        if (terrainnames[t] != NULL) {
            newterrain = get_terrain_i(terrainnames[t]);
            if (newterrain != NULL) {
                newterrains[t] = newterrain;
            }
        }
    }
}
