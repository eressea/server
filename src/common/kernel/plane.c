/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "plane.h"

/* kernel includes */
#include "region.h"
#include "faction.h"

/* util includes */
#include <resolve.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>

plane *
getplane(const region *r)
{
	if(r)
		return r->planep;

	return NULL;
}

plane *
getplanebyid(int id)
{
	plane *p;

	for (p=planes; p; p=p->next)
		if (p->id == id)
			return p;
	return NULL;
}

plane *
getplanebyname(const char * name)
{
	plane *p;

	for (p=planes; p; p=p->next)
		if (!strcasecmp(p->name, name))
			return p;
	return NULL;
}

plane *
findplane(int x, int y)
{
	plane *pl;

	for(pl=planes;pl;pl=pl->next) {
		if(x >= pl->minx && x <= pl->maxx
				&& y >= pl->miny && y <= pl->maxy) {
			return pl;
		}
	}
	return NULL;
}

int
getplaneid(region *r)

{
	if(r) {
		plane * pl = getplane(r);
		if (pl) return pl->id;

		for(pl=planes;pl;pl=pl->next) {
			if(r->x >= pl->minx && r->x <= pl->maxx
					&& r->y >= pl->miny && r->y <= pl->maxy) {
				return pl->id;
			}
		}
	}
	return 0;
}

int
ursprung_x(const faction *f, const plane *pl)
{
	ursprung *ur;
	int id = 0;

	if(!f)
		return 0;

	if(pl)
		id = pl->id;

	for(ur = f->ursprung; ur; ur = ur->next) {
		if(ur->id == id)
			return ur->x;
	}

	if (pl) {
		set_ursprung((faction*)f, id, plane_center_x(pl), plane_center_y(pl));
		return plane_center_x(pl);
	}

	return 0;
}

int
ursprung_y(const faction *f, const plane *pl)
{
	ursprung *ur;
	int id = 0;

	if(!f)
		return 0;

	if(pl)
		id = pl->id;

	for(ur = f->ursprung; ur; ur = ur->next) {
		if(ur->id == id)
			return ur->y;
	}

	if (pl) {
		set_ursprung((faction*)f, id, plane_center_x(pl), plane_center_y(pl));
		return plane_center_y(pl);
	}

	return 0;
}

int
plane_center_x(const plane *pl)
{
	if(pl == NULL)
		return 0;

	return(pl->minx + pl->maxx)/2;
}

int
plane_center_y(const plane *pl)
{
	if(pl == NULL)
		return 0;

	return(pl->miny + pl->maxy)/2;
}

int
region_x(const region *r, const faction *f)
{
	plane *pl;

	pl = getplane(r);
	return r->x - ursprung_x(f, pl) - plane_center_x(pl);
}

int
region_y(const region *r, const faction *f)
{
	plane *pl;

	pl = getplane(r);
	return r->y - ursprung_y(f, pl) - plane_center_y(pl);
}

void
set_ursprung(faction *f, int id, int x, int y)
{
	ursprung *ur;

	for(ur=f->ursprung;ur;ur=ur->next) {
		if(ur->id == id) {
			ur->x += x;
			ur->y += y;
			return;
		}
	}

	ur = calloc(1, sizeof(ursprung));
	ur->id   = id;
	ur->x    = x;
	ur->y    = y;

	addlist(&f->ursprung, ur);
}

plane *
create_new_plane(int id, const char *name, int minx, int maxx, int miny, int maxy, int flags)
{
	plane *pl = getplanebyid(id);

	if (pl) return pl;
	pl = calloc(1, sizeof(plane));

	pl->next  = NULL;
	pl->id    = id;
	pl->name  = strdup(name);
	pl->minx = minx;
	pl->maxx = maxx;
	pl->miny = miny;
	pl->maxy = maxy;
	pl->flags = flags;

	addlist(&planes, pl);
	return pl;
}

/* Umrechnung Relative-Absolute-Koordinaten */
int
rel_to_abs(const struct plane *pl, const struct faction * f, int rel, unsigned char index)
{
	assert(index == 0 || index == 1);

	if(index == 0)
		return (rel + ursprung_x(f,pl));

	return (rel + ursprung_y(f,pl));
}


void *
resolve_plane(void * id)
{
   return getplanebyid((int)id);
}

void
write_plane_reference(const plane * u, FILE * F)
{
	fprintf(F, "%d ", u?(u->id):0);
}

void
read_plane_reference(plane ** up, FILE * F)
{
	int i;
	fscanf(F, "%d", &i);
	if (i==0) *up = NULL;
	{
		*up = getplanebyid(i);
		if (*up==NULL) ur_add((void*)i, (void**)&up, resolve_plane);
	}
}
