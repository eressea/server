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

#ifndef VMAP_H
#define VMAP_H
#ifdef __cplusplus
extern "C" {
#endif

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

size_t vmap_lowerbound(const vmap * vm, const int key);
size_t vmap_upperbound(const vmap * vm, const int key);
size_t vmap_insert(vmap * vm, const int key, void *data);
size_t vmap_find(const vmap * vm, const int key);
size_t vmap_get(vmap * vm, const int key);
void vmap_init(vmap * vm);

#ifdef __cplusplus
}
#endif
#endif
