/* vi: set ts=2:
 *
 *	$Id: pathfinder.c,v 1.1 2001/01/25 09:37:57 enno Exp $
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
#include "pathfinder.h"

#include "region.h"

#include <limits.h>
#include <stdlib.h>

#ifdef NEW_PATH
boolean
allowed_swim(const region * src, const region * target)
{
	if (terrain[target->terrain].flags & SWIM_INTO) return true;
	return false;
}

boolean
allowed_walk(const region * src, const region * target)
{
	if (terrain[target->terrain].flags & WALK_INTO) return true;
	return false;
}

boolean
allowed_fly(const region * src, const region * target)
{
	if (terrain[target->terrain].flags & FLY_INTO) return true;
	return false;
}

#define FAST_PATH
#ifdef FAST_PATH
typedef struct node {
	struct node * next;
	region * r;
	struct node * prev;
	int distance;
} node;

static node * node_garbage;

static node *
new_node(region * r, int distance, node * prev)
{
	node * n;
	if (node_garbage!=NULL) {
		n = node_garbage;
		node_garbage = n->next;
	}
	else n = malloc(sizeof(node));
	n->next = NULL;
	n->prev = prev;
	n->r= r;
	n->distance= distance;
	return n;
}

static node *
free_node(node * n)
{
	node * s = n->next;
	n->next = node_garbage;
	node_garbage = n;
	return s;
}

static region **
internal_path_find(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	static region * path[MAXDEPTH+2];
	direction_t d;
	node * root = new_node(start, 0, NULL);
	node ** end = &root->next;
	node * n = root;
	boolean found = false;
	assert(maxlen<=MAXDEPTH);
	fset(start, FL_MARK);

	while (n!=NULL) {
		region * r = n->r;
		int depth = n->distance+1;
		if (n->distance >= maxlen) break;
		for (d=0;d!=MAXDIRECTIONS; ++d) {
			region * rn = rconnect(r, d);
			if (rn==NULL) continue;
			if (fval(rn, FL_MARK)) continue; /* already been there */
			if (!allowed(r, rn)) continue; /* can't go there */
			if (rn==target) {
				int i = depth;
				path[i+1] = NULL;
				path[i] = rn;
				while (n) {
					path[--i] = n->r;
					n = n->prev;
				}
				found = true;
				break;
			} else {
				fset(rn, FL_MARK);
				*end = new_node(rn, depth, n);
				end = &(*end)->next;
			}
		}
		if (found) break;
		n = n->next;
	}
	while (root!=NULL) {
		region * r = root->r;
		freset(r, FL_MARK);
		root = free_node(root);
	}
	if (found) return path;
	return NULL;
}

boolean
path_exists(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	assert((!fval(start, FL_MARK) && !fval(target, FL_MARK)) || !"Some Algorithm did not clear its FL_MARKs!");
	if (start==target) return true;
	if (internal_path_find(start, target, maxlen, allowed)!=NULL) return true;
	return false;
}

region **
path_find(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	assert((!fval(start, FL_MARK) && !fval(target, FL_MARK)) || !"Did you call path_init()?");
	return internal_path_find(start, target, maxlen, allowed);
}

#else
static boolean
internal_path_exists(region *start, const region *target, int maxlen, int * depth, boolean (*allowed)(const region*, const region*))
{
	int position = *depth;
	direction_t dir;

	assert(maxlen<=MAXDEPTH);
	assert(position<=maxlen);

	*depth = INT_MAX;
	if (start==target) {
		*depth = position;
		return true;
	} else if (position==maxlen) {
		return false; /* not found */
	}
	position++;

	fset(start, FL_MARK);
	for (dir=0; dir != MAXDIRECTIONS; dir++) {
		int deep = position;
		region * rn = rconnect(start, dir);
		if (rn==NULL) continue;

		if (fval(rn, FL_MARK)) continue; /* already been there */
		if (!allowed(start, rn)) continue; /* can't go there */

		if (internal_path_exists(rn, target, maxlen, &deep, allowed)) {
			*depth = deep;
			break;
		}
	}
	freset(start, FL_MARK);
	return (dir!=MAXDIRECTIONS);
}

static region **
internal_path_find(region *start, const region *target, int maxlen, int * depth, boolean (*allowed)(const region*, const region*))
{
	static region * path[MAXDEPTH+2];
	static region * bestpath[MAXDEPTH+2];
	int position = *depth;
	direction_t dir;

	assert(maxlen<=MAXDEPTH);
	assert(position<=maxlen);

	*depth = INT_MAX;
	path[position] = start;
	if (start==target) {
		*depth = position;
		path[position+1] = NULL;
		return path;
	} else if (position==maxlen) {
		return NULL; /* not found */
	}
	position++;
	bestpath[position] = NULL;

	fset(start, FL_MARK);
	for (dir=0; dir != MAXDIRECTIONS; dir++) {
		int deep = position;
		region * rn = rconnect(start, dir);
		region ** findpath;

		if (rn==NULL) continue;
		if (fval(rn, FL_MARK)) continue;
		if (!allowed(start, rn)) continue;
		findpath = internal_path_find(rn, target, maxlen, &deep, allowed);
		if (deep<=maxlen) {
			assert(findpath);
			maxlen = deep;
			memcpy(bestpath, findpath, sizeof(region*)*(maxlen+2));
		}
	}
	freset(start, FL_MARK);
	if (bestpath[position]!=NULL) return bestpath;
	return NULL;
}

boolean
path_exists(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	int len = 0;
	boolean exist;
	assert((!fval(start, FL_MARK) && !fval(target, FL_MARK)) || !"Did you call path_init()?");
	exist = internal_path_exists(start, target, maxlen, &len, allowed);
	assert(exist == (path_find(start, target, maxlen, allowed)!=NULL));
	return exist;
}

region **
path_find(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	int depth = 0;
	assert((!fval(start, FL_MARK) && !fval(target, FL_MARK)) || !"Did you call path_init()?");
	return internal_path_find(start, target, maxlen, &depth, allowed);
}
#endif

#else

static boolean
step_store(region *r, region *target, int t, int depth)
{
	direction_t dir;

	search[depth][0] = r->x;
	search[depth][1] = r->y;
	fset(r, FL_MARK);

	if(depth > MAXDEPTH) return false;

	for(dir=0; dir < MAXDIRECTIONS; dir++) {
		region * rn = rconnect(r, dir);

		if (rn==NULL) continue;
		if(rn && !fval(rn, FL_MARK) && allowed_terrain(rn->terrain, r->terrain, t)) {
			if(rn == target) {
				search[depth+1][0] = rn->x;
				search[depth+1][1] = rn->y;
				search_len = depth+1;
				return true;
			}
			if(step_store(rn, target, t, depth+1)) return true;
		}
	}
	return false;
}

boolean
path_find(region *start, region *target, int t)
{
	region *r;

	for(r=regions;r;r=r->next) freset(r, FL_MARK);
	return step_store(start, target, t, 0);
}

int search[MAXDEPTH][2];
int search_len;

static boolean
allowed_terrain(terrain_t ter, terrain_t ter_last, int t)
{
	if((t & DRAGON_LIMIT) && ter == T_OCEAN && ter_last == T_GLACIER)
		return false;

	if(t & FLY && terrain[ter].flags & FLY_INTO) return true;
	if(t & SWIM && terrain[ter].flags & SWIM_INTO) return true;
	if(t & WALK && terrain[ter].flags & WALK_INTO) return true;

	/* Alte Variante:
	if(ter == T_FIREWALL) return false;
	if(t & FLY) return true;
	if(t & SWIM && !(t & WALK) && ter == T_OCEAN) return true;
	if(t & SWIM && t & WALK) return true;
	if(t & WALK && ter != T_OCEAN) return true;
	*/

	return false;
}

boolean
step(region *r, region *target, int t, int depth)
{
	direction_t dir;
	region *rn;

	fset(r, FL_MARK);

	if(depth > MAXDEPTH) return false;

	for(dir=0; dir < MAXDIRECTIONS; dir++) {
		rn = rconnect(r, dir);
		if(rn && !fval(rn, FL_MARK) && allowed_terrain(rn->terrain, r->terrain, t)) {
			if(rn == target) return true;
			if(step(rn, target, t, depth+1)) return true;
		}
	}
	return false;
}

boolean
path_exists(region *start, region *target, int t)
{
	region *r;

	for(r=regions;r;r=r->next) freset(r, FL_MARK);
	return step(start, target, t, 0);
}

#endif
