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
#include "race.h"

#include "alchemy.h"
#include "build.h"
#include "building.h"
#include "equipment.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "names.h"
#include "pathfinder.h"
#include "region.h"
#include "ship.h"
#include "skill.h"
#include "spell.h"
#include "terrain.h"
#include "unit.h"

/* util includes */
#include <util/attrib.h>
#include <util/bsdstring.h>
#include <util/functions.h>
#include <util/umlaut.h>
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
static int rc_changes = 1;

static const char *racenames[MAXRACES] = {
    "dwarf", "elf", NULL, "goblin", "human", "troll", "demon", "insect",
    "halfling", "cat", "aquarian", "orc", "snotling", "undead", "illusion",
    "youngdragon", "dragon", "wyrm", "ent", "catdragon", "dracoid",
    NULL, "spell", "irongolem", "stonegolem", "shadowdemon",
    "shadowmaster", "mountainguard", "alp", "toad", "braineater", "peasant",
    "wolf", NULL, NULL, NULL, NULL, "songdragon", NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, "seaserpent",
    "shadowknight", NULL, "skeleton", "skeletonlord", "zombie",
    "juju-zombie", "ghoul", "ghast", NULL, NULL, "template",
    "clone"
};

const struct race *findrace(const char *s, const struct locale *lang)
{
    void **tokens = get_translations(lang, UT_RACES);
    variant token;

    assert(lang);
    if (tokens && findtoken(*tokens, s, &token) == E_TOK_SUCCESS) {
        return (const struct race *)token.v;
    }
    return NULL;
}

const struct race *get_race(race_t rt) {
    const char * name;

    assert(rt < MAXRACES);
    name = racenames[rt];
    if (!name) {
        return NULL;
    }
    return rc_find(name);
}

typedef struct xref {
    race_t id;
    const race *rc;
} rc_xref;

int cmp_xref(const void *a, const void *b)
{
    const rc_xref *l = (const rc_xref *)a;
    const rc_xref *r = (const rc_xref *)b;
    if (l->rc<r->rc) return -1;
    if (l->rc>r->rc) return 1;
    return 0;
}

static rc_xref *xrefs;
race_t old_race(const struct race * rc)
{
    static int cache;
    int i, l, r;
    
    if (rc_changed(&cache)) {
        if (!xrefs) {
            xrefs = malloc(sizeof(rc_xref) * MAXRACES);
        }
        for (i = 0; i != MAXRACES; ++i) {
            xrefs[i].rc = get_race(i);
            xrefs[i].id = (race_t)i;
        }
        qsort(xrefs, MAXRACES, sizeof(rc_xref), cmp_xref);
    }
    l=0; r=MAXRACES-1;
    while (l<=r) {
        int m = (l+r)/2;
        if (rc<xrefs[m].rc) {
            r = m-1;
        } else if (rc>xrefs[m].rc) {
            l = m+1;
        } else {
            return (race_t)xrefs[m].id;
        }
    }
    return NORACE;
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
    race_list *rl2 = (race_list *)malloc(sizeof(race_list));

    rl2->data = r;
    rl2->next = *rl;

    *rl = rl2;
}

void free_races(void) {
    while (races) {
        int i;
        race * rc = races->next;
        
        for (i = 0; races->attack[i].type!=AT_NONE; ++i) {
            spellref_free(races->attack[i].data.sp);
        }
        spellref_free(races->precombatspell);
        free_params(&races->parameters);
        free(xrefs);
        xrefs = 0;
        free(races->_name);
        free(races->def_damage);
        free(races);
        races = rc;
    }
    num_races = 0;
    ++rc_changes;
}

static race *rc_find_i(const char *name)
{
    const char *rname = name;
    race *rc = races;

    while (rc && strcmp(rname, rc->_name) != 0) {
        rc = rc->next;
    }
    if (!rc && strcmp(name, "uruk") == 0) {
        rc = rc_find_i("orc");
        log_warning("a reference was made to the retired race '%s', returning '%s'.", name, rc->_name);
    }
    return rc;
}

const race * rc_find(const char *name) {
    return rc_find_i(name);
}

bool rc_changed(int *cache) {
    assert(cache);
    if (*cache != rc_changes) {
        *cache = rc_changes;
        return true;
    }
    return false;
}

race *rc_create(const char *zName)
{
    race *rc;
    int i;

    assert(zName);
    rc = (race *)calloc(sizeof(race), 1);
    rc->hitpoints = 1;
    rc->weight = PERSON_WEIGHT;
    rc->capacity = 540;
    rc->income = 20;
    rc->recruit_multi = 1.0F;
    rc->regaura = 1.0F;
    rc->speed = 1.0F;
    rc->battle_flags = 0;
    if (strchr(zName, ' ') != NULL) {
        log_error("race '%s' has an invalid name. remove spaces\n", zName);
        assert(strchr(zName, ' ') == NULL);
    }
    rc->_name = strdup(zName);
    rc->precombatspell = NULL;

    rc->attack[0].type = AT_COMBATSPELL;
    for (i = 1; i < RACE_ATTACKS; ++i)
        rc->attack[i].type = AT_NONE;
    rc->index = num_races++;
    ++rc_changes;
    rc->next = races;
    return races = rc;
}

race *rc_get_or_create(const char *zName)
{
    race *rc;

    assert(zName);
    rc = rc_find_i(zName);
    return rc ? rc : rc_create(zName);
}

/** dragon movement **/
bool allowed_dragon(const region * src, const region * target)
{
    if (fval(src->terrain, ARCTIC_REGION) && fval(target->terrain, SEA_REGION))
        return false;
    return allowed_fly(src, target);
}

bool r_insectstalled(const region * r)
{
    return fval(r->terrain, ARCTIC_REGION);
}

double rc_magres(const race *rc) {
    return rc->magres / 100.0;
}

double rc_maxaura(const race *rc) {
    return rc->maxaura / 100.0;
}

const char* rc_name(const race * rc, name_t n, char *name, size_t size) {
    const char * postfix = 0;
    if (!rc) {
        return NULL;
    }
    switch (n) {
    case NAME_SINGULAR: postfix = ""; break;
    case NAME_PLURAL: postfix = "_p"; break;
    case NAME_DEFINITIVE: postfix = "_d"; break;
    case NAME_CATEGORY: postfix = "_x"; break;
    default: assert(!"invalid name_t enum in rc_name_s");
    }
    if (postfix) {
        snprintf(name, size, "race::%s%s", rc->_name, postfix);
        return name;
    }
    return NULL;
}

const char *rc_name_s(const race * rc, name_t n)
{
    static char name[64];  // FIXME: static return value
    return rc_name(rc, n, name, sizeof(name));
}

const char *raceprefix(const unit * u)
{
    attrib *asource = u->faction->attribs;

    if (fval(u, UFL_GROUP)) {
        attrib *agroup = a_find(u->attribs, &at_group);
        if (agroup != NULL)
            asource = ((const group *)(agroup->data.v))->attribs;
    }
    return get_prefix(asource);
}

const char *racename(const struct locale *loc, const unit * u, const race * rc)
{
    const char *str, *prefix = raceprefix(u);

    if (prefix != NULL) {
        static char lbuf[80]; // FIXME: static return value
        char *bufp = lbuf;
        size_t size = sizeof(lbuf) - 1;
        int ch, bytes;

        bytes = (int)strlcpy(bufp, LOC(loc, mkname("prefix", prefix)), size);
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();

        bytes = (int)strlcpy(bufp, LOC(loc, rc_name_s(rc, u->number != 1)), size);
        assert(~bufp[0] & 0x80 || !"unicode/not implemented");
        ch = tolower(*(unsigned char *)bufp);
        bufp[0] = (char)ch;
        if (wrptr(&bufp, &size, bytes) != 0)
            WARN_STATIC_BUFFER();
        *bufp = 0;

        return lbuf;
    }
    str = LOC(loc, rc_name_s(rc, (u->number == 1) ? NAME_SINGULAR : NAME_PLURAL));
    return str ? str : rc->_name;
}

void write_race_reference(const race * rc, struct storage *store)
{
    WRITE_TOK(store, rc ? rc->_name : "none");
}

variant read_race_reference(struct storage *store)
{
    variant result;
    char zName[20];
    READ_TOK(store, zName, sizeof(zName));

    if (strcmp(zName, "none") == 0) {
        result.v = NULL;
        return result;
    }
    else {
        result.v = rc_find_i(zName);
    }
    assert(result.v != NULL);
    return result;
}

void register_race_description_function(race_desc_func func, const char *name) {
    register_function((pf_generic)func, name);
}

void register_race_name_function(race_name_func func, const char *name) {
    register_function((pf_generic)func, name);
}
