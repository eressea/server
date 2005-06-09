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

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <config.h>
#include "cvector.h"
#include "memory.h"

void
cv_init(cvector * cv)
{
	cv->begin = 0;
	cv->end = 0;
	cv->space = 0;
}

cvector *
cv_kill(cvector * cv)
{
	if (cv->begin) free(cv->begin);
	cv_init(cv);
	return cv;
}

size_t
cv_size(cvector * cv)
{
	return cv->end - cv->begin;
}

void
cv_reserve(cvector * cv, size_t size)
{
	size_t count = cv->end - cv->begin;
	cv->begin = realloc(cv->begin, size * sizeof(void *));

	cv->space = size;
	cv->end = cv->begin + count;
}

void
cv_pushback(cvector * cv, void *u)
{
	if (cv->space == cv_size(cv))
		cv_reserve(cv, cv->space ? cv->space * 2 : 2);
	*(cv->end++) = u;
}

void
v_sort(void **begin, void **end, int (__cdecl * compare) (const void *, const void *))
{
	qsort(begin, end - begin, sizeof(void *), compare);
}

void **
v_find(void **begin, void **end, void *item)
{
	void **it;

	for (it = begin; it != end; ++it)
		if (*it == item)
			return it;
	return it;
}

void **
v_findx(void **begin, void **end, void *item, int (__cdecl * cmp) (void *, void *))
{
	void **it;

	for (it = begin; it != end; ++it)
		if (cmp(*it, item))
			return it;
	return it;
}

int
__cv_scramblecmp(const void *p1, const void *p2)
{
	return *((long *) p1) - *((long *) p2);
}

#define addptr(p,i)         ((void *)(((char *)p) + i))

void
__cv_scramble(void *v1, size_t n, size_t width)
{
	size_t i;
	static size_t s = 0;
	static void *v = 0;

	if (n * (width + 4) > s) {
		s = n * (width + 4);
		v = (void *) realloc(v, s);
	}
	for (i = 0; i != n; i++) {
		*(long *) addptr(v, i * (width + 4)) = rand();
		memcpy(addptr(v, i * (width + 4) + 4), addptr(v1, i * width), width);
	}

	qsort(v, n, width + 4, __cv_scramblecmp);

	for (i = 0; i != n; i++)
		memcpy(addptr(v1, i * width), addptr(v, i * (width + 4) + 4), width);

}

void
v_scramble(void **begin, void **end)
{
	__cv_scramble(begin, end - begin, sizeof(void *));
}

void
cv_mergeunique(cvector * c, const cvector * a, const cvector * b, int (__cdecl * keyfun) (const void *))
{
	void **ai, **bi;

	assert(cv_size(c) == 0);

	for (ai = a->begin, bi = b->begin; ai != a->end || bi != b->end;) {
		int ak = (ai == a->end) ? INT_MAX : keyfun(*ai);
		int bk = (bi == b->end) ? INT_MAX : keyfun(*bi);
		int nk = (ak < bk) ? ak : bk;

		cv_pushback(c, (ak == nk) ? (*ai) : (*bi));
		while (ai != a->end && keyfun(*ai) == nk)
			++ai;
		while (bi != b->end && keyfun(*bi) == nk)
			++bi;
	}
}
