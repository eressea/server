/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "group.h"

/* kernel includes */
#include "unit.h"
#include "faction.h"
#include "save.h"
#include "version.h"

/* attrib includes */
#include <attributes/raceprefix.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/resolve.h>
#include <util/storage.h>
#include <util/unicode.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#define GMAXHASH 2039
static group * ghash[GMAXHASH];
static int maxgid;

static group *
new_group(faction * f, const char * name, int gid)
{
	group ** gp = &f->groups;
	int index = gid % GMAXHASH;
	group * g = calloc(sizeof(group), 1);

	while (*gp) gp = &(*gp)->next;
	*gp = g;

	maxgid = max(gid, maxgid);
	g->name = strdup(name);
	g->gid = gid;

	g->nexthash = ghash[index];
	return ghash[index] = g;
}

static void
init_group(faction * f, group * g)
{
	ally * a, ** an;

	an = &g->allies;
	for (a=f->allies;a;a=a->next) if (a->faction) {
		ally * ga = calloc(sizeof(ally), 1);
		*ga = *a;
		*an = ga;
		an = &ga->next;
	}
}

static group *
find_groupbyname(group * g, const char * name)
{
	while (g && unicode_utf8_strcasecmp(name, g->name)!=0) g = g->next;
	return g;
}

static group *
find_group(int gid)
{
	int index = gid % GMAXHASH;
	group * g = ghash[index];
	while (g && g->gid!=gid) g = g->nexthash;
	return g;
}

static int
read_group(attrib * a, struct storage * store)
{
  group * g;
  int gid = store->r_int(store);
  a->data.v = g = find_group(gid);
  if (g!=0) {
    g->members++;
    return AT_READ_OK;
  }
  return AT_READ_FAIL;
}

static void
write_group(const attrib * a, struct storage * store)
{
  group * g = (group*)a->data.v;
  store->w_int(store, g->gid);
}

attrib_type
at_group = { /* attribute for units assigned to a group */
	"grp",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
	write_group,
	read_group,
	ATF_UNIQUE
};

void
free_group(group * g)
{
	while (g->allies) {
		ally * a = g->allies;
		g->allies = a->next;
		free(a);
	}
	free(g->name);
	free(g);
}

void
set_group(struct unit * u, struct group * g)
{
  attrib * a = NULL;

  if (fval(u, UFL_GROUP)) {
    a = a_find(u->attribs, &at_group);
  }

  if (a) {
    group * og = (group *)a->data.v;
    if (og==g) return;
    --og->members;
  }

  if (g) {
    if (!a) {
      a = a_add(&u->attribs, a_new(&at_group));
      fset(u, UFL_GROUP);
    }
    a->data.v = g;
    g->members++;
  } else if (a) {
    a_remove(&u->attribs, a);
    freset(u, UFL_GROUP);
  }
}

boolean
join_group(unit * u, const char * name)
{
  group * g = NULL;

  if (name && name[0]) {
    g = find_groupbyname(u->faction->groups, name);
    if (g==NULL) {
      g = new_group(u->faction, name, ++maxgid);
      init_group(u->faction, g);
    }
  }

  set_group(u, g);
  return true;
}

void
write_groups(struct storage * store, group * g)
{
  while (g) {
    ally * a;
    store->w_int(store, g->gid);
    store->w_str(store, g->name);
    for (a=g->allies;a;a=a->next) {
      if (a->faction) {
        write_faction_reference(a->faction, store);
        store->w_int(store, a->status);
      }
    }
    store->w_id(store, 0);
    a_write(store, g->attribs);
    store->w_brk(store);
    g=g->next;
  }
  store->w_int(store, 0);
}

void
read_groups(struct storage * store, faction * f)
{
  for(;;) {
    ally ** pa;
    group * g;
    int gid;
    char buf[1024];

    gid = store->r_int(store);
    if (gid==0) break;
    store->r_str_buf(store, buf, sizeof(buf));
    g = new_group(f, buf, gid);
    pa = &g->allies;
    for (;;) {
      ally * a;
      variant fid;
      fid.i = store->r_id(store);
      if (fid.i<=0) break;
      if (store->version<STORAGE_VERSION && fid.i==0) break;
      a = malloc(sizeof(ally));
      *pa = a;
      pa = &a->next;
      a->status = store->r_int(store);
      a->faction = findfaction(fid.i);
      if (!a->faction) ur_add(fid, (void**)&a->faction, resolve_faction);
    }
    *pa = 0;
    a_read(store, &g->attribs);
  }
}
