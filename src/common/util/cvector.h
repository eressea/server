/* vi: set ts=2:
 *
 *	$Id: cvector.h,v 1.1 2001/01/25 09:37:58 enno Exp $
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

#ifndef CVECTOR_H
#define CVECTOR_H

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
void v_sort(void **begin, void **end, int (__cdecl * compare) (const void *, const void *));
extern void **v_find(void **begin, void **end, void *);
extern void **v_findx(void **begin, void **end, void *, int (__cdecl * cmp) (void *, void *));
void v_scramble(void **begin, void **end);
void cv_mergeunique(cvector * c, const cvector * a, const cvector * b, int (__cdecl * keyfun) (const void *));

#define cv_remove(c, i) { void** x = v_find((c)->begin, (c)->end, (i)); if (x) { *x = *(c)->end; (c)->end--; } }
#define for_each(item, vector) \
{	\
	void **iterator;	\
	for (iterator = (vector).begin; iterator<(vector).end; ++iterator)	\
	{	\
		(item) = *iterator;
#define next(item) } }

#endif
