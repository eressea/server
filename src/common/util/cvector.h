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

#ifndef CVECTOR_H
#define CVECTOR_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
#ifndef __cdecl
#define __cdecl
#endif
#endif

#include <stddef.h>
typedef struct cvector cvector;

struct cvector {
	void **begin;
	void **end;
	size_t space;
};

typedef int (__cdecl * v_sort_fun) (const void *, const void *);

void cv_init(cvector * cv);
cvector *cv_kill(cvector * cv);
size_t cv_size(cvector * cv);
void cv_reserve(cvector * cv, size_t size);
void cv_pushback(cvector * cv, void *u);
void v_scramble(void **begin, void **end);

#define cv_remove(c, i) { void** x = v_find((c)->begin, (c)->end, (i)); if (x) { *x = *(c)->end; (c)->end--; } }
#define cv_foreach(item, vector) \
{	\
	void **iterator;	\
	for (iterator = (vector).begin; iterator<(vector).end; ++iterator)	\
	{	\
		(item) = *iterator;
#define cv_next(item) } }

#ifdef __cplusplus
}
#endif
#endif
