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
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "eressea.h"
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

/* libc includes */
#include <stdlib.h>
#include <string.h>

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
	while (g && strcasecmp(name, g->name)) g = g->next;
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
read_group(attrib * a, FILE * f)
{
	int gid;
	group * g;
	fscanf(f, "%d ", &gid);
	a->data.v = g = find_group(gid);
	if (g!=0) {
		g->members++;
		return AT_READ_OK;
	}
	return AT_READ_FAIL;
}

static void
write_group(const attrib * a, FILE * f)
{
	group * g = (group*)a->data.v;
	fprintf(f, "%d ", g->gid);
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

boolean
join_group(unit * u, const char * name)
{
	attrib * a = NULL;
	group * g;
  if (fval(u, UFL_GROUP)) {
    a = a_find(u->attribs, &at_group);
  }

	if (a) ((group *)(a->data.v))->members--;
	if (!name || !strlen(name)) {
    if (a) {
      a_remove(&u->attribs, a);
      freset(u, UFL_GROUP);
    }
		return true;
	}
	g = find_groupbyname(u->faction->groups, name);
	if (!g) {
		g = new_group(u->faction, name, ++maxgid);
		init_group(u->faction, g);
	}
  if (!a) {
    a = a_add(&u->attribs, a_new(&at_group));
    fset(u, UFL_GROUP);
  }
	a->data.v = g;
	g->members++;
	return true;
}

void
write_groups(FILE * F, group * g)
{
	while (g) {
		ally * a;
		fprintf(F, "%d \"%s\" ", g->gid, g->name);
		for (a=g->allies;a;a=a->next) if (a->faction)
			fprintf(F, "%s %d ", factionid(a->faction), a->status);
		fputs("0 ", F);
		a_write(F, g->attribs);
		fputs("\n", F);
		g=g->next;
	}
	fputs("0\n", F);
}

void
read_groups(FILE * F, faction * f)
{
	for(;;) {
		ally ** pa;
		group * g;
		int gid;
		fscanf(F, "%d ", &gid);
		if (!gid) break;
		rsf(F, buf, 1024);
		g = new_group(f, buf, gid);
		pa = &g->allies;
		for (;;) {
			ally * a;
			variant aid;
			fscanf(F, "%s ", buf);
			aid.i = atoi36(buf);
			if (aid.i==0) break;
			a = calloc(sizeof(ally), 1);
			*pa = a;
			pa = &a->next;
			fscanf(F, "%d ", &a->status);
			a->faction = findfaction(aid.i);
			if (!a->faction) ur_add(aid, (void**)&a->faction, resolve_faction);
		}
		if(global.data_version >= GROUPATTRIB_VERSION) {
			a_read(F, &g->attribs);
		}
	}
}
