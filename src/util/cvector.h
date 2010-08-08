/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

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
