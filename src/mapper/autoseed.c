/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "autoseed.h"

/* kernel includes */
#include <region.h>
#include <plane.h>
#include <faction.h>
#include <race.h>
#include <unit.h>

/* libc includes */
#include <limits.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

newfaction * newfactions = NULL;

static int regions_per_faction = 3;

void
get_island(region_list ** rlist)
{
	region_list *rcurrent = *rlist;
	region_list **rnext = &rcurrent->next;
	assert(rcurrent && *rnext==NULL);

	fset(rcurrent->data, FL_MARK);
	while (rcurrent!=NULL) {
		direction_t dir;
		for (dir=0;dir!=MAXDIRECTIONS;++dir) {
			region * r = rconnect(rcurrent->data, dir);
			if (r!=NULL && r->land && !fval(r, FL_MARK)) {
				fset(r, FL_MARK);
				add_regionlist(rnext, r);
			}
		}
		rcurrent = *rnext;
		rnext = &rcurrent->next;
	}
	rnext=rlist;
	while (*rnext) {
		rcurrent = *rnext;
		freset(rcurrent->data, FL_MARK);
		rnext = &rcurrent->next;
	}
}

typedef struct seed_t {
	struct region * region;
	struct newfaction * player;
	struct seed_t * next[MAXDIRECTIONS];
} seed_t;

double
get_influence(struct seed_t * seed, struct seed_t * target)
{
	double q = 0.0;
	direction_t d;
	if (target==0 || seed==0) return q;
	for (d=0;d!=MAXDIRECTIONS;++d) {
		seed_t * s = seed->next[d];
		if (s==target) {
			if (target->player) {
				if (seed->player) {
					/* neighbours bad. */
					q -= 4.0;
					if (seed->player->race==target->player->race) {
						/* don't want similar people next to each other */
						q -= 2.0;
					} else if (seed->player->race==new_race[RC_ELF]) {
						if (target->player->race==new_race[RC_TROLL]) {
							/* elf sitting next to troll: poor troll */
							q -= 5.0;
						}
					}
				}
				else if (target->player) {
					/* empty regions good */
					q += 1.0;
				}
			}
		}
	}
	return q;
}

double
get_quality(struct seed_t * seed)
{
	double q = 0.0;
	int nterrains[MAXTERRAINS];
	direction_t d;

	memset(nterrains, 0, sizeof(nterrains));
	for (d=0;d!=MAXDIRECTIONS;++d) {
		seed_t * ns = seed->next[d];
		if (ns) ++nterrains[rterrain(ns->region)];
	}
	++nterrains[rterrain(seed->region)];

	if (nterrains[T_MOUNTAIN]) {
		/* 5 points for the first, and 2 points for every other mountain */
		q += (3+nterrains[T_MOUNTAIN]*2);
	} else if (seed->player->race==new_race[RC_DWARF]) {
		/* -10 for a dwarf who isn't close to a mountain */
		q -= 10.0;
	}

	if (nterrains[T_PLAIN]) {
		/* 5 points for the first, and 2 points for every other plain */
		q += (3+nterrains[T_PLAIN]*2);
	} else if (seed->player->race==new_race[RC_ELF]) {
		/* -10 for an elf who isn't close to a plain */
		q -= 10.0;
	}

	if (nterrains[T_DESERT]) {
		/* +10 points for the first if we are insects, and -2 points for every
		 * other desert */
		if (seed->player->race==new_race[RC_INSECT]) q += 12;
		q -= (nterrains[T_DESERT]*2);
	}

	switch (rterrain(seed->region)) {
	case T_DESERT:
		q -=8.0;
		break;
	case T_VOLCANO:
		q -=20.0;
		break;
	case T_GLACIER:
		q -=10.0;
		break;
	case T_SWAMP:
		q -= 1.0;
		break;
	case T_PLAIN:
		q += 4.0;
		break;
	case T_MOUNTAIN:
		q += 5.0;
		break;
	}

	return 2.0;
}

double
recalc(seed_t * seeds, int nseeds, int nplayers)
{
	int i, p = 0;
	double quality = 0.0, q = 0.0, *qarr = malloc(sizeof(int) * nplayers);
	for (i=0;i!=nseeds;++i) {
		if (seeds[i].player) {
			double quality = get_quality(seeds+i);
			direction_t d;
			for (d=0;d!=MAXDIRECTIONS;++d) {
				quality += get_influence(seeds[i].next[d], seeds+i);
			}
			qarr[p++] = quality;
			q += quality;
		}
	}
/*
	q = q / nplayers;
	for (i=0;i!=nplayers;++i) {
		quality -= (qarr[i]-q)*(qarr[i]-q);
	}
 */
	return quality + q;
}

extern int numnewbies;

void
autoseed(struct region_list * rlist)
{
	int nregions = listlen(rlist);
	int nseeds = nregions;
	double lastq;
	seed_t * seeds = calloc(sizeof(seed_t), nseeds);
	int i, nfactions = 0;
	newfaction * nf = newfactions;

	while (nf) {
		if (nf->bonus==0) ++nfactions;
		nf = nf->next;
	}
	nfactions = min(nfactions, nregions/regions_per_faction);

	nf = newfactions;
	for (i=0;i!=nseeds;++i) {
		seeds[i].region = rlist->data;
		rlist = rlist->next;
		if (i<nfactions) {
			while (nf->bonus!=0) nf=nf->next;
			seeds[i].player = nf;
			nf = nf->next;
		}
	}
	for (i=0;i!=nseeds;++i) {
		direction_t d;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * r = rconnect(seeds[i].region, d);
			if (r && r->land) {
				int k;
				for (k=i+1;k!=nseeds;++k) if (seeds[k].region==r) {
					seeds[k].next[dir_invert(d)] = seeds+i;
					seeds[i].next[d] = seeds+k;
				}
			}
		}
	}

	lastq = recalc(seeds, nseeds, nfactions);
	for (;;) {
		double quality = lastq;
		int i;
		for (i=0;i!=nseeds;++i) {
			int k;
			for (k=i+1;k!=nseeds;++k) {
				double q;
				newfaction * player = seeds[k].player;
				if (player==NULL && seeds[i].player==NULL) continue;
				seeds[k].player = seeds[i].player;
				seeds[i].player = player;
				q = recalc(seeds, nseeds, nfactions);
				if (q>quality) {
					quality = q;
				} else {
					/* undo */
					seeds[i].player = seeds[k].player;
					seeds[k].player = player;
				}
			}
		}
		if (lastq==quality) break;
		else lastq = quality;
	}
	for (i=0;i!=nseeds;++i) {
		newfaction * nf = seeds[i].player;
		if (nf) {
			newfaction ** nfp = &newfactions;
			unit * u;
			while (*nfp!=nf) nfp=&(*nfp)->next;
			u = addplayer(seeds[i].region, addfaction(nf->email, nf->password, nf->race,
        nf->lang, nf->subscription));
			u->faction->alliance = nf->allies;
			++numnewbies;
			*nfp = nf->next;
			free(nf);
		}
	}
}

static terrain_t
preferred_terrain(const struct race * rc)
{
	if (rc==rc_find("dwarf")) return T_MOUNTAIN;
	if (rc==rc_find("insect")) return T_DESERT;
	if (rc==rc_find("halfling")) return T_SWAMP;
	if (rc==rc_find("troll")) return T_MOUNTAIN;
	return T_PLAIN;
}

#define REGIONS_PER_FACTION 2
#define MINFACTIONS 1
#define MAXAGEDIFF 5
#define VOLCANO_CHANCE 100

int
mkisland(int nsize)
{
	int x = 0, y = 0;
	region * r;
	region_list * rlist = NULL;
	int rsize, isize=0;
	region * rmin = NULL;
	direction_t d;
	int dist;

	for (r=regions;r;r=r->next) {
		struct plane * p = rplane(r);
		if (r->terrain==T_OCEAN && p==NULL && (rmin==NULL || r->age<=MAXAGEDIFF)) {
			direction_t d;
			for (d=0;d!=MAXDIRECTIONS;++d) {
				if (rconnect(r, d)==NULL) break;
			}
			if (d!=MAXDIRECTIONS) rmin=r;
		}
	}
	r = NULL;
	for (dist=1;r!=rmin;++dist) {
		for (d=0;r!=rmin && d!=MAXDIRECTIONS;++d) {
			int i;
			region * rn = rmin;
			for (i=0;i!=dist;++i) rn=rconnect(rn, d);
			if (rn==NULL) {
				r = rmin;
				x = rmin->x + delta_x[d] * dist;
				y = rmin->y + delta_y[d] * dist;
			}
		}
	}

  if (listlen(newfactions)<MINFACTIONS) return 0;
	r = new_region(x, y);
	terraform(r, T_OCEAN);
	add_regionlist(&rlist, r);
	rsize = 1;
	while (nsize && rsize) {
		int i = rand() % rsize;
		region_list ** rnext = &rlist;
		direction_t d;
		while (i--) rnext=&(*rnext)->next;
		r = (*rnext)->data;
		*rnext = (*rnext)->next;
		if (rsize==0) {
			log_error(("Could not place all factions on the same island as requested\n"));
			break;
		}
		--rsize;
		assert(r->terrain==T_OCEAN);
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * rn = rconnect(r, d);
			if (rn==NULL) {
				rn = new_region(r->x + delta_x[d], r->y + delta_y[d]);
				terraform(rn, T_OCEAN);
				add_regionlist(&rlist, rn);
				++rsize;
			}
		}
    if (rand() % VOLCANO_CHANCE == 0) {
      terraform(r, T_VOLCANO);
    } else if (rand() % REGIONS_PER_FACTION == 0 || rsize==0) {
			newfaction ** nfp, * nextf = newfactions;
			unit * u;
			terraform(r, preferred_terrain(nextf->race));
			++isize;
			u = addplayer(r, addfaction(nextf->email, nextf->password, nextf->race, nextf->lang,
				nextf->subscription));
			u->faction->alliance = nextf->allies;

			/* remove duplicate email addresses */
			nfp=&newfactions;
			while (*nfp) {
				newfaction * nf = *nfp;
				if (strcmp(nextf->email, nf->email)==0) {
					*nfp = nf->next;
					if (nextf!=nf) free(nf);
				}
				else nfp = &nf->next;
			}
			--nsize;
		}
		else {
			terraform(r, (terrain_t)((rand() % T_GLACIER)+1));
		}
	}
	if (rlist) {
#define MINOCEANDIST 3
#define MAXFILLDIST 10
#define SPECIALCHANCE 80
		region_list ** rbegin = &rlist;
		int i;
    int special = 1;

    for (i=0;i!=MINOCEANDIST;++i) {
			region_list ** rend = rbegin;
			while (*rend) rend=&(*rend)->next;
			while (rbegin!=rend) {
				direction_t d;
				region * r = (*rbegin)->data;
				rbegin=&(*rbegin)->next;
				for (d=0;d!=MAXDIRECTIONS;++d) {
					region * rn = rconnect(r, d);
					if (rn==NULL) {
            terrain_t terrain = T_OCEAN;
						rn = new_region(r->x + delta_x[d], r->y + delta_y[d]);
            if (rand() % SPECIALCHANCE < special) {
              terrain = (terrain_t)(1 + rand() % T_GLACIER);
              special = SPECIALCHANCE / 3; /* 33% chance auf noch eines */
            } else {
              special = 1;
            }
						terraform(rn, terrain);
            /* the new region has an extra 15% chance to have laen */
            if (rand() % 100 < 15) rsetlaen(r, 5 + rand() % 5);
            /* the new region has an extra 20% chance to have mallorn */
            if (rand() % 100 < 20) fset(r, RF_MALLORN);
						add_regionlist(rend, rn);
					}
				}
			}

		}
		while (*rbegin) {
			region * r = (*rbegin)->data;
			direction_t d;
			rbegin=&(*rbegin)->next;
			for (d=0;d!=MAXDIRECTIONS;++d) if (rconnect(r, d)==NULL) {
				int i;
				for (i=1;i!=MAXFILLDIST;++i) {
					if (findregion(r->x + i*delta_x[d], r->y + i*delta_y[d]))
						break;

				}
				if (i!=MAXFILLDIST) {
					while (--i) {
						region * rn = new_region(r->x + i*delta_x[d], r->y + i*delta_y[d]);
						terraform(rn, T_OCEAN);
					}
				}
			}
		}
	}
	return isize;
}
