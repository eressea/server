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

#include <config.h>
#include <stdlib.h>
#include "vset.h"

void
vset_init(vset * s)
{
	s->data = 0;
	s->size = 0;
	s->maxsize = 0;
}

void
vset_destroy(vset * s)
{
	if (s->data)
		free(s->data);
}

int
vset_erase(vset * s, void *item)
{
	size_t i;

	for (i = 0; i != s->size; ++i)
		if (s->data[i] == item) {
			s->size--;
			s->data[i] = s->data[s->size];
			return 1;
		}
	return 0;
}

size_t
vset_add(vset * s, void *item)
{
	size_t i;

	if (!s->data) {
		s->size = 0;
		s->maxsize = 4;
		s->data = calloc(4, sizeof(void *));
	}
	for (i = 0; i != s->size; ++i)
		if (s->data[i] == item)
			return i;
	if (s->size == s->maxsize) {
		s->maxsize *= 2;
		s->data = realloc(s->data, s->maxsize * sizeof(void *));
	}
	s->data[s->size] = item;
	++s->size;
	return s->size - 1;
}

void *
vset_pop(vset *s)
{
	if(s->size == 0) return NULL;
	s->size--;
	return s->data[s->size+1];
}

int
vset_count(vset *s, void *item)
{
	size_t i;
	int c = 0;
	
	for(i = 0; i != s->size; ++i) {
		if(s->data[i] == item) c++;
	}

	return c;
}

void
vset_concat(vset *to, vset *from)
{
	size_t i;

	for(i=0; i != from->size; ++i) {
		vset_add(to, from->data[i]);
	}
}

