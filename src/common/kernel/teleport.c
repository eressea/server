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
#include "teleport.h"

/* kernel includes */
#include "unit.h"
#include "region.h"
#include "race.h"
#include "faction.h"
#include "plane.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>

static int
real2tp(int rk) {
	/* in C:
	 * -4 / 5 = 0;
	 * +4 / 5 = 0;
	 * !!!!!!!!!!;
	 */
	return (rk + (TP_RADIUS*5000)) / TP_RADIUS - 5000;
}

static region *
tpregion(const region *r) {
	return findregion(TE_CENTER_X+real2tp(r->x), TE_CENTER_Y+real2tp(r->y));
}

region *
r_standard_to_astral(const region *r)
{
	region *r2;
#if 0
	int x, y;

	x = TE_CENTER_X + (r->x/TP_RADIUS);
	y = TE_CENTER_Y + (r->y/TP_RADIUS);

	r2 = findregion(x,y);
#endif

	r2 = tpregion(r);

	if(getplaneid(r2) != 1 || rterrain(r2) != T_ASTRAL)
		return NULL;

	return r2;
}

region *
r_astral_to_standard(const region *r)
{
	int x, y;
	region *r2;

	assert(getplaneid(r) == 1);
	x = (r->x-TE_CENTER_X)*TP_RADIUS;
	y = (r->y-TE_CENTER_Y)*TP_RADIUS;

	r2 = findregion(x,y);
	if(!r2 || getplaneid(r2) != 0)
		return NULL;

	return r2;
}

regionlist *
all_in_range(region *r, int n)
{
	int x,y;
	regionlist *rlist = NULL;
	region *r2;

	if(r == NULL) return NULL;	/* Um Probleme abzufangen,
																 wenn zu einer Astralregion
																 noch kein Realitätsregion existiert.
															   Nicht gut, aber nicht zu ändern. */

	for(x = r->x-n; x <= r->x+n; x++) {
		for(y = r->y-n; y <= r->y+n; y++) {
			if(koor_distance(r->x, r->y, x, y) <= n) {
				r2 = findregion(x,y);
				if(r2) rlist = add_regionlist(rlist, findregion(x,y));
			}
		}
	}

	return rlist;
}

void
random_in_teleport_plane(void)
{
	region *r;
	unit *u;
	faction *f0 = findfaction(MONSTER_FACTION);

	if(!f0) return;

	for(r=regions; r; r=r->next) {
		if(getplaneid(r) != 1 || rterrain(r) != T_ASTRAL) continue;

		/* Neues Monster ? */
		if(rand()%100 == 0) {
			switch(rand()%1) {
			case 0:
				u = createunit(r, f0, 1+rand()%10+rand()%10, new_race[RC_HIRNTOETER]);
				set_string(&u->name, "Hirntöter");
				set_string(&u->display, "Wabernde grüne Schwaden treiben durch den Nebel und verdichten sich zu einer unheimlichen Kreatur, die nur aus einem langen Ruderschwanz und einem riesigen runden Maul zu bestehen scheint.");
				set_skill(u, SK_STEALTH, 30);
				set_skill(u, SK_OBSERVATION, 30);
				break;
			}
		}
	}
}

plane * astral_plane;

void
create_teleport_plane(void)
{
	region *r;
	int i;

	if (!getplanebyid(1)) {
		astral_plane = create_new_plane(1, "Astralraum",
			TE_CENTER_X-500, TE_CENTER_X+500,
			TE_CENTER_Y-500, TE_CENTER_Y+500,
			PFL_NOCOORDS);
	}

	/* Regionsbereich aufbauen. */
	/* wichtig: das muß auch für neue regionen gemacht werden.
	 * Evtl. bringt man es besser in new_region() unter, und
	 * übergibt an new_region die plane mit, in der die
	 * Region gemacht wird.
	 */

	for (r=regions;r;r=r->next) if(r->planep == NULL) {
		region *ra = tpregion(r);
		if (!ra) {
			ra = new_region(TE_CENTER_X+real2tp(r->x), TE_CENTER_Y+real2tp(r->y));
			rsetterrain(ra, T_ASTRAL);
		}
		ra->planep  = getplanebyid(1);
		if (terrain[rterrain(r)].flags & FORBIDDEN_LAND) rsetterrain(ra, T_ASTRALB);
	}

	for(i=0;i<4;i++) {
		random_in_teleport_plane();
	}
}

regionlist *
allinhab_in_range(const region *r, int n)
{
	int x,y;
	regionlist *rlist = NULL;
	region *r2;

	if(r == NULL) return NULL;	/* Um Probleme abzufangen,
								 wenn zu einer Astralregion
								 noch kein Realitätsregion existiert.
								 Nicht gut, aber nicht zu ändern. */

	for(x = r->x-n; x <= r->x+n; x++) {
		for(y = r->y-n; y <= r->y+n; y++) {
			if(koor_distance(r->x, r->y, x, y) <= n) {
				r2 = findregion(x,y);
				if (r2 && landregion(rterrain(r2)))
					rlist = add_regionlist(rlist, findregion(x,y));
			}
		}
	}

	return rlist;
}
