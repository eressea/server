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

#ifndef VOIDPTR_SETS
#define VOIDPTR_SETS
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
typedef struct vset vset;
struct vset {
	void **data;
	size_t size;
	size_t maxsize;
};
extern void vset_init(vset * s);
extern void vset_destroy(vset * s);
extern size_t vset_add(vset * s, void *);
extern int vset_erase(vset * s, void *);
extern int vset_count(vset *s, void * i);
extern void *vset_pop(vset *s);
extern void vset_concat(vset *to, vset *from);

#ifdef __cplusplus
}
#endif
#endif
