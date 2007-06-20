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
#include "terrain.h"

#include <limits.h>
#include <stdlib.h>

boolean
allowed_swim(const region * src, const region * r)
{
	if (fval(r->terrain, SWIM_INTO)) return true;
	return false;
}

boolean
allowed_walk(const region * src, const region * r)
{
	if (fval(r->terrain, WALK_INTO)) return true;
	return false;
}

boolean
allowed_fly(const region * src, const region * r)
{
	if (fval(r->terrain, FLY_INTO)) return true;
	return false;
}

typedef struct node {
	struct node * next;
	region * r;
	struct node * prev;
	int distance;
} node;

static node * node_garbage;

void
pathfinder_cleanup(void)
{
  while (node_garbage) {
    node * n = node_garbage;
    node_garbage = n->next;
    free(n);
  }
}

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

static void
free_nodes(node * root)
{
  while (root!=NULL) {
    region * r = root->r;
    freset(r, RF_MARK);
    root = free_node(root);
  }
}

struct region_list * 
regions_in_range(struct region * start, int maxdist, boolean (*allowed)(const struct region*, const struct region*))
{
  region_list * rlist = NULL;
  node * root = new_node(start, 0, NULL);
  node ** end = &root->next;
  node * n = root;

  while (n!=NULL) {
    region * r = n->r;
    int depth = n->distance+1;
    direction_t d;

    if (n->distance >= maxdist) break;
    for (d=0;d!=MAXDIRECTIONS; ++d) {
      region * rn = rconnect(r, d);
      if (rn==NULL) continue;
      if (fval(rn, RF_MARK)) continue; /* already been there */
      if (allowed && !allowed(r, rn)) continue; /* can't go there */

      /* add the region to the list of available ones. */
      add_regionlist(&rlist, rn);

      /* make sure we don't go here again, and put the region into the set for
         further BFS'ing */
      fset(rn, RF_MARK);
      *end = new_node(rn, depth, n);
      end = &(*end)->next;
    }
    n = n->next;
  }
  free_nodes(root);

  return rlist;
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
	fset(start, RF_MARK);

	while (n!=NULL) {
		region * r = n->r;
		int depth = n->distance+1;
		if (n->distance >= maxlen) break;
		for (d=0;d!=MAXDIRECTIONS; ++d) {
			region * rn = rconnect(r, d);
			if (rn==NULL) continue;
			if (fval(rn, RF_MARK)) continue; /* already been there */
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
				fset(rn, RF_MARK);
				*end = new_node(rn, depth, n);
				end = &(*end)->next;
			}
		}
		if (found) break;
		n = n->next;
	}
  free_nodes(root);
	if (found) return path;
	return NULL;
}

boolean
path_exists(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	assert((!fval(start, RF_MARK) && !fval(target, RF_MARK)) || !"Some Algorithm did not clear its RF_MARKs!");
	if (start==target) return true;
	if (internal_path_find(start, target, maxlen, allowed)!=NULL) return true;
	return false;
}

region **
path_find(region *start, const region *target, int maxlen, boolean (*allowed)(const region*, const region*))
{
	assert((!fval(start, RF_MARK) && !fval(target, RF_MARK)) || !"Did you call path_init()?");
	return internal_path_find(start, target, maxlen, allowed);
}
