#include <kernel/config.h>
#include "group.h"

/* kernel includes */
#include "ally.h"
#include "faction.h"
#include "unit.h"
#include "objtypes.h"

/* attrib includes */
#include <attributes/raceprefix.h>

/* util includes */
#include <util/base36.h>
#include <util/lists.h>
#include <util/resolve.h>
#include <util/unicode.h>

#include <kernel/attrib.h>
#include <kernel/gamedata.h>

#include <storage.h>
#include <strings.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#define GMAXHASH 2039
static group *ghash[GMAXHASH];
static int maxgid;

group *group_create(faction *f, int gid)
{
    int index = gid % GMAXHASH;
    group *g = calloc(1, sizeof(group));

    if (!g) abort();
    addlist(&f->groups, g);
    if (gid > maxgid) maxgid = gid;
    g->f = f;
    g->gid = gid;
    g->nexthash = ghash[index];
    return ghash[index] = g;
}

group *create_group(faction * f, const char *name, int gid)
{
    group *g = group_create(f, gid);

    g->name = str_strdup(name);
    return g;

}

static void init_group(faction * f, group * g)
{
    g->allies = allies_clone(f->allies);
}

group *find_groupbyname(faction *f, const char *name)
{
    group* g;
    for (g = f->groups; g; g = g->next)
    {
        if (utf8_strcasecmp(name, g->name) == 0)
        {
            return g;
        }
    }
    return NULL;
}

static group *find_group(int gid)
{
    int index = gid % GMAXHASH;
    group *g = ghash[index];
    while (g && g->gid != gid)
        g = g->nexthash;
    return g;
}

static int read_group(variant *var, void *owner, gamedata *data)
{
    struct storage *store = data->store;
    group *g;
    unit * u = (unit *)owner;
    int gid;

    READ_INT(store, &gid);
    var->v = g = find_group(gid);
    if (g != NULL) {
        if (g->f != u->faction) {
            return AT_READ_FAIL;
        }
        g->members++;
        return AT_READ_OK;
    }
    return AT_READ_FAIL;
}

static void
write_group(const variant *var, const void *owner, struct storage *store)
{
    group *g = (group *)var->v;
    WRITE_INT(store, g->gid);
}

attrib_type at_group = {        /* attribute for units assigned to a group */
    "grp",
    DEFAULT_INIT,
    DEFAULT_FINALIZE, DEFAULT_AGE, write_group, read_group, NULL, ATF_UNIQUE };

void free_group(group * g)
{
    int index;
    group **g_ptr;
    assert(g);
    index = g->gid % GMAXHASH;
    g_ptr = ghash + index;
    while (*g_ptr && (*g_ptr)->gid != g->gid) {
        g_ptr = &(*g_ptr)->nexthash;
    }
    assert(*g_ptr == g);
    *g_ptr = g->nexthash;

    if (g->attribs) {
        a_removeall(&g->attribs, NULL);
    }
    allies_free(g->allies);
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
        g = find_groupbyname(u->faction, name);
        if (g == NULL) {
            g = create_group(u->faction, name, ++maxgid);
            init_group(u->faction, g);
        }
    }

    set_group(u, g);
    return g;
}

void write_groups(struct gamedata *data, const faction * f)
{
    group *g;
    storage *store = data->store;
    for (g = f->groups; g; g = g->next) {
        WRITE_INT(store, g->gid);
        WRITE_STR(store, g->name);
        write_allies(data, g->allies);
        a_write(store, g->attribs, g);
        WRITE_SECTION(store);
    }
    WRITE_INT(store, 0);
}

void read_groups(gamedata *data, faction * f)
{
    struct storage *store = data->store;
    for (;;) {
        group *g;
        int gid;
        char buf[1024];

        READ_INT(store, &gid);
        if (gid == 0)
            break;
        READ_STR(store, buf, sizeof(buf));
        g = create_group(f, buf, gid);
        read_allies(data, &g->allies);
        read_attribs(data, &g->attribs, g);
    }
}
