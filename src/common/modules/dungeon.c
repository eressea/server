/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "dungeon.h"
#include "gmcmd.h"

#include <triggers/gate.h>
#include <triggers/unguard.h>

/* kernel includes */
#include <plane.h>
#include <race.h>
#include <region.h>
#include <building.h>

/* util includes */
#include <event.h>

typedef struct treasure {
	const struct item_type * itype;
	int amount;
	struct treasure * next;
} treasure;

typedef struct monster {
	const struct race * race;
	double chance;
	int maxunits;
	int avgsize;
	struct treasure * treasures;
	struct monster * next;
	struct itemtype_list * weapons;
} monster;

typedef struct dungeon {
	int level;
	int radius;
	int size;
	double connect;
	struct monster * boss;
	struct monster * monsters;
} dungeon;

static dungeon * dungeonstyles;

void
make_dungeon(int radius, int level, plane **pp, region **rp)
{
	int nb[2][3][2] = { 
		{ { -1, 0 }, { 0, 1 }, { 1, -1 } }, 
		{ { 1, 0 }, { -1, 1 }, { 0, -1 } }
	};
	const struct race * bossrace = rc_find("wyrm");
	char name[128];
	int size = level*level;
	double connect = 0.50;
	unsigned int flags = PFL_NORECRUITS;
	int n = 0;
	plane * p;
	region *r, *center;
	region * rnext;

	sprintf(name, "Die Höhlen von %s", bossrace->generate_name(NULL));
	p = gm_addplane(radius, flags, name);

	center = findregion(p->minx+(p->maxx-p->minx)/2, p->miny+(p->maxy-p->miny)/2);
	assert(center);
	terraform(center, T_HELL);
	rnext = r = center;
	while (size>0) {
		int d, o = rand() % 3;
		for (d=0;d!=3;++d) {
			int index = (d+o) % 3;
			region * rn = findregion(r->x+nb[n][index][0], r->y+nb[n][index][1]);
			assert(r->terrain==T_HELL);
			if (rn) {
				switch (rn->terrain) {
				case T_OCEAN:
					if (rand() % 100 < connect*100) {
						terraform(rn, T_HELL);
						--size;
						rnext = rn;
					}
					else terraform(rn, T_FIREWALL);
					break;
				case T_HELL:
					rnext = rn;
					break;
				}
				if (size == 0) break;
			}
			rn = findregion(r->x+nb[(n+1)%2][index][0], r->y+nb[(n+1)%2][index][1]);
			if (rn && rn->terrain==T_OCEAN) terraform(rn, T_FIREWALL);
		}
		if (size==0) break;
		assert(r!=rnext);
		r = rnext;
		n = (n+1) % 2;
	}

	if (pp) *pp=p;
	if (rp) *rp=center;
}

void
make_dungeongate(region * source, region * target, int level)
{
	building *bsource, *btarget;
	
	bsource = new_building(bt_find("castle"), source, default_locale);
	set_string(&bsource->name, "Pforte zur Hölle");
	bsource->size = 50;
	add_trigger(&bsource->attribs, "timer", trigger_gate(bsource, target));
	add_trigger(&bsource->attribs, "create", trigger_unguard(bsource));
	fset(bsource, BLD_UNGUARDED);

	btarget = new_building(bt_find("castle"), target, default_locale);
	set_string(&btarget->name, "Pforte zur Außenwelt");
	btarget->size = 50;
	add_trigger(&btarget->attribs, "timer", trigger_gate(btarget, source));
	add_trigger(&btarget->attribs, "create", trigger_unguard(btarget));
	fset(btarget, BLD_UNGUARDED);
}
