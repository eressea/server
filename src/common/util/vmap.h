/* vi: set ts=2:
 *
 *	$Id: vmap.h,v 1.2 2001/01/26 16:19:41 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef VMAP_H
#define VMAP_H

typedef struct vmapentry vmapentry;
struct vmapentry {
	int key;
	void *value;
};
typedef struct vmap vmap;
struct vmap {
	vmapentry *data;
	unsigned int size;
	unsigned int maxsize;
};

unsigned int vmap_lowerbound(const vmap * vm, const int key);
unsigned int vmap_upperbound(const vmap * vm, const int key);
unsigned int vmap_insert(vmap * vm, const int key, void *data);
unsigned int vmap_find(const vmap * vm, const int key);
unsigned int vmap_get(vmap * vm, const int key);
void vmap_init(vmap * vm);

#endif
