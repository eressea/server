/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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
#include "race.h"

#include "alchemy.h"
#include "build.h"
#include "building.h"
#include "equipment.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "magic.h"
#include "names.h"
#include "pathfinder.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "terrain.h"
#include "unit.h"
#include "version.h"

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>
#include <util/rng.h>

#include <storage.h>

/* attrib includes */
#include <attributes/raceprefix.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/** external variables **/
race *races;
int num_races = 0;
static int cache_breaker;

static const char *racenames[MAXRACES] = {
    "dwarf", "elf", NULL, "goblin", "human", "troll", "demon", "insect",
    "halfling", "cat", "aquarian", "orc", "snotling", "undead", "illusion",
    "youngdragon", "dragon", "wyrm", "ent", "catdragon", "dracoid",
    "special", "spell", "irongolem", "stonegolem", "shadowdemon",
    "shadowmaster", "mountainguard", "alp", "toad", "braineater", "peasant",
    "wolf", NULL, NULL, NULL, NULL, "songdragon", NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, "seaserpent",
    "shadowknight", "centaur", "skeleton", "skeletonlord", "zombie",
    "juju-zombie", "ghoul", "ghast", "museumghost", "gnome", "template",
    "clone"
};

static race * race_cache[MAXRACES];

struct race *get_race(race_t rt) {
    static int cache = -1;
    const char * name;
    
    assert(rt < MAXRACES);
    name = racenames[rt];
    if (!name) {
        return 0;
    }
    if (cache_breaker != cache) {
        cache = cache_breaker;
        memset(race_cache, 0, sizeof(race_cache));
        return race_cache[rt] = rc_get_or_create(name);
    } else {
        race * result = race_cache[rt];
        if (!result) {
            result = race_cache[rt] = rc_get_or_create(name);
        }
        return result;
    }
    return 0;
}

race_list *get_familiarraces(void)
{
  static int init = 0;
  static race_list *familiarraces;

  if (!init) {
    race *rc = races;
    for (; rc != NULL; rc = rc->next) {
      if (rc->init_familiar != NULL) {
        racelist_insert(&familiarraces, rc);
      }
    }
    init = false;
  }
  return familiarraces;
}

void racelist_clear(struct race_list **rl)
{
  while (*rl) {
    race_list *rl2 = (*rl)->next;
    free(*rl);
    *rl = rl2;
  }
}

void racelist_insert(struct race_list **rl, const struct race *r)
{
  race_list *rl2 = (race_list *) malloc(sizeof(race_list));

  rl2->data = r;
  rl2->next = *rl;

  *rl = rl2;
}

void free_races(void) {
    while (races) {
        race * rc = races->next;
        free(races);
        races =rc;
    }
}

static race *rc_find_i(const char *name)
{
  const char *rname = name;
  race *rc = races;

  while (rc && !strcmp(rname, rc->_name[0]) == 0) {
      rc = rc->next;
  }
  return rc;
}

const race * rc_find(const char *name) {
    return rc_find_i(name);
}

race *rc_get_or_create(const char *zName)
{
    race *rc;

    assert(zName);
    rc = rc_find_i(zName);
    if (!rc) {
        char zBuffer[80]; 

        rc = (race *)calloc(sizeof(race), 1);
        rc->hitpoints = 1;
        if (strchr(zName, ' ') != NULL) {
            log_error("race '%s' has an invalid name. remove spaces\n", zName);
            assert(strchr(zName, ' ') == NULL);
        }
        strcpy(zBuffer, zName);
        rc->_name[0] = _strdup(zBuffer);
        sprintf(zBuffer, "%s_p", zName);
        rc->_name[1] = _strdup(zBuffer);
        sprintf(zBuffer, "%s_d", zName);
        rc->_name[2] = _strdup(zBuffer);
        sprintf(zBuffer, "%s_x", zName);
        rc->_name[3] = _strdup(zBuffer);
        rc->precombatspell = NULL;

        rc->attack[0].type = AT_COMBATSPELL;
        rc->attack[1].type = AT_NONE;
        rc->index = num_races++;
        ++cache_breaker;
        rc->next = races;
        return races = rc;
    }
    return rc;
}

/** dragon movement **/
bool allowed_dragon(const region * src, const region * target)
{
  if (fval(src->terrain, ARCTIC_REGION) && fval(target->terrain, SEA_REGION))
    return false;
  return allowed_fly(src, target);
}

char **race_prefixes = NULL;

extern void add_raceprefix(const char *prefix)
{
  static size_t size = 4;
  static unsigned int next = 0;
  if (race_prefixes == NULL)
    race_prefixes = malloc(size * sizeof(char *));
  if (next + 1 == size) {
    size *= 2;
    race_prefixes = realloc(race_prefixes, size * sizeof(char *));
  }
  race_prefixes[next++] = _strdup(prefix);
  race_prefixes[next] = NULL;
}

/* Die Bezeichnungen dürfen wegen der Art des Speicherns keine
 * Leerzeichen enthalten! */

/*                      "den Zwergen", "Halblingsparteien" */

bool r_insectstalled(const region * r)
{
  return fval(r->terrain, ARCTIC_REGION);
}

const char *rc_name(const race * rc, int n)
{
  return rc ? mkname("race", rc->_name[n]) : NULL;
}

const char *raceprefix(const unit * u)
{
  const attrib *asource = u->faction->attribs;

  if (fval(u, UFL_GROUP)) {
    const attrib *agroup = agroup = a_findc(u->attribs, &at_group);
    if (agroup != NULL)
      asource = ((const group *)(agroup->data.v))->attribs;
  }
  return get_prefix(asource);
}

const char *racename(const struct locale *loc, const unit * u, const race * rc)
{
  const char *str, *prefix = raceprefix(u);

  if (prefix != NULL) {
    static char lbuf[80];
    char *bufp = lbuf;
    size_t size = sizeof(lbuf) - 1;
    int ch, bytes;

    bytes = (int)strlcpy(bufp, LOC(loc, mkname("prefix", prefix)), size);
    if (wrptr(&bufp, &size, bytes) != 0)
      WARN_STATIC_BUFFER();

    bytes = (int)strlcpy(bufp, LOC(loc, rc_name(rc, u->number != 1)), size);
    assert(~bufp[0] & 0x80 || !"unicode/not implemented");
    ch = tolower(*(unsigned char *)bufp);
    bufp[0] = (char)ch;
    if (wrptr(&bufp, &size, bytes) != 0)
      WARN_STATIC_BUFFER();
    *bufp = 0;

    return lbuf;
  }
  str = LOC(loc, rc_name(rc, u->number != 1));
  return str ? str : rc->_name[0];
}

int
rc_specialdamage(const race * ar, const race * dr,
  const struct weapon_type *wtype)
{
  race_t art = old_race(ar);
  int m, modifier = 0;

  if (wtype != NULL && wtype->modifiers != NULL)
    for (m = 0; wtype->modifiers[m].value; ++m) {
      /* weapon damage for this weapon, possibly by race */
      if (wtype->modifiers[m].flags & WMF_DAMAGE) {
        race_list *rlist = wtype->modifiers[m].races;
        if (rlist != NULL) {
          while (rlist) {
            if (rlist->data == ar)
              break;
            rlist = rlist->next;
          }
          if (rlist == NULL)
            continue;
        }
        modifier += wtype->modifiers[m].value;
      }
    }
  switch (art) {
    case RC_HALFLING:
      if (wtype != NULL && dragonrace(dr)) {
        modifier += 5;
      }
      break;
    default:
      break;
  }
  return modifier;
}

void write_race_reference(const race * rc, struct storage *store)
{
  WRITE_TOK(store, rc ? rc->_name[0] : "none");
}

variant read_race_reference(struct storage *store)
{
  variant result;
  char zName[20];
  READ_TOK(store, zName, sizeof(zName));

  if (strcmp(zName, "none") == 0) {
    result.v = NULL;
    return result;
  } else {
    result.v = rc_find_i(zName);
  }
  assert(result.v != NULL);
  return result;
}
