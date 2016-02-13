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
#include "group.h"

/* kernel includes */
#include "ally.h"
#include "faction.h"
#include "save.h"
#include "unit.h"
#include "version.h"

/* attrib includes */
#include <attributes/raceprefix.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/gamedata.h>
#include <util/resolve.h>
#include <util/unicode.h>

#include <storage.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#define GMAXHASH 2039
static group *ghash[GMAXHASH];
static int maxgid;

group *new_group(faction * f, const char *name, int gid)
{
    group **gp = &f->groups;
    int index = gid % GMAXHASH;
    group *g = calloc(sizeof(group), 1);

    while (*gp)
        gp = &(*gp)->next;
    *gp = g;

    maxgid = _max(gid, maxgid);
    g->name = _strdup(name);
    g->gid = gid;

    g->nexthash = ghash[index];
    return ghash[index] = g;
}

static void init_group(faction * f, group * g)
{
    ally *a, **an;

    an = &g->allies;
    for (a = f->allies; a; a = a->next)
        if (a->faction) {
            ally *ga = ally_add(an, a->faction);
            ga->status = a->status;
            an = &ga->next;
        }
}

static group *find_groupbyname(group * g, const char *name)
{
    while (g && unicode_utf8_strcasecmp(name, g->name) != 0)
        g = g->next;
    return g;
}

static group *find_group(int gid)
{
    int index = gid % GMAXHASH;
    group *g = ghash[index];
    while (g && g->gid != gid)
        g = g->nexthash;
    return g;
}

static int read_group(attrib * a, void *owner, gamedata *data)
{
    struct storage *store = data->store;
    group *g;
    int gid;

    READ_INT(store, &gid);
    a->data.v = g = find_group(gid);
    if (g != 0) {
        g->members++;
        return AT_READ_OK;
    }
    return AT_READ_FAIL;
}

static void
write_group(const attrib * a, const void *owner, struct storage *store)
{
    group *g = (group *)a->data.v;
    WRITE_INT(store, g->gid);
}

attrib_type at_group = {        /* attribute for units assigned to a group */
    "grp",
    DEFAULT_INIT,
    DEFAULT_FINALIZE, DEFAULT_AGE, write_group, read_group, NULL, ATF_UNIQUE };

void free_group(group * g)
{
    int index = g->gid % GMAXHASH;
    group **g_ptr = ghash + index;
    while (*g_ptr && (*g_ptr)->gid != g->gid)
        g_ptr = &(*g_ptr)->nexthash;
    assert(*g_ptr == g);
    *g_ptr = g->nexthash;

    while (g->allies) {
        ally *a = g->allies;
        g->allies = a->next;
        free(a);
    }
    free(g->name);
    free(g);
}

group * get_group(const struct unit *u)
{
    if (fval(u, UFL_GROUP)) {
        attrib * a = a_find(u->attribs, &at_group);
        if (a) {
            return (group *)a->data.v;
        }
    }
    return 0;
}

void set_group(struct unit *u, struct group *g)
{
    attrib *a = NULL;

    if (fval(u, UFL_GROUP)) {
        a = a_find(u->attribs, &at_group);
    }

    if (a) {
        group *og = (group *)a->data.v;
        if (og == g)
            return;
        --og->members;
    }

    if (g) {
        if (!a) {
            a = a_add(&u->attribs, a_new(&at_group));
            fset(u, UFL_GROUP);
        }
        a->data.v = g;
        g->members++;
    }
    else if (a) {
        a_remove(&u->attribs, a);
        freset(u, UFL_GROUP);
    }
}

group *join_group(unit * u, const char *name)
{
    group *g = NULL;

    if (name && name[0]) {
        g = find_groupbyname(u->faction->groups, name);
        if (g == NULL) {
            g = new_group(u->faction, name, ++maxgid);
            init_group(u->faction, g);
        }
    }

    set_group(u, g);
    return g;
}

void write_groups(struct storage *store, const faction * f)
{
    group *g;
    for (g = f->groups; g; g = g->next) {
        ally *a;
        WRITE_INT(store, g->gid);
        WRITE_STR(store, g->name);
        for (a = g->allies; a; a = a->next) {
            if (a->faction && a->faction->_alive) {
                write_faction_reference(a->faction, store);
                WRITE_INT(store, a->status);
            }
        }
        write_faction_reference(NULL, store);
        a_write(store, g->attribs, g);
        WRITE_SECTION(store);
    }
    WRITE_INT(store, 0);
}

void read_groups(gamedata *data, faction * f)
{
    struct storage *store = data->store;
    for (;;) {
        ally **pa;
        group *g;
        int gid;
        char buf[1024];

        READ_INT(store, &gid);
        if (gid == 0)
            break;
        READ_STR(store, buf, sizeof(buf));
        g = new_group(f, buf, gid);
        pa = &g->allies;
        for (;;) {
            ally *a;
            variant fid;

            fid = read_faction_reference(store);
            if (fid.i <= 0)
                break;
            a = ally_add(pa, findfaction(fid.i));
            READ_INT(store, &a->status);
            if (!a->faction)
                ur_add(fid, &a->faction, resolve_faction);
        }
        read_attribs(data, &g->attribs, g);
    }
}
