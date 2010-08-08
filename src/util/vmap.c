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

#ifdef _MSC_VER
#pragma warning (disable: 4711)
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <config.h>
#include "vmap.h"

size_t
vmap_lowerbound(const vmap * vm, const int key)
/* returns the index of the entry which has the greatest key  that is less or
 * equal to 'key' */
{
	vmapentry *first = vm->data, *middle;
	unsigned int half, len = vm->size;
	while (len > 0) {
		half = len / 2;
		middle = first;
		middle += half;
		if (middle->key < key) {
			first = middle + 1;
			len = len - half - 1;
		} else
			len = half;
	}
	return first - vm->data;
}

void
vmap_init(vmap * map)
{
	map->size = 0;				/* !! */
	map->maxsize = 4;
	map->data = calloc(4, sizeof(vmapentry));
}

size_t
vmap_upperbound(const vmap * vm, const int key)
/* returns the index of the entry which has the smallest key  that is greater
 * or equal to 'key' */
{
	unsigned int half, len = vm->size;
	vmapentry *first = vm->data, *middle;
	while (len > 0) {
		half = len / 2;
		middle = first;
		middle += half;
		if (key < middle->key)
			len = half;
		else {
			first = middle + 1;
			len = len - half - 1;
		}
	}
	return first - vm->data;
}

size_t
vmap_get(vmap * vm, const int key)
{
	size_t insert = vmap_lowerbound(vm, key);
	vmapentry *at;

	/* make sure it's a unique key: */
	if (insert != vm->size && vm->data[insert].key == key)
		return insert;
	/* insert at this position: */
	if (vm->size == vm->maxsize) {
		/* need more space */
		vm->maxsize *= 2;
		vm->data = realloc(vm->data, vm->maxsize * sizeof(vmapentry));
	}
	at = vm->data + insert;
	memmove(at + 1, at, (vm->size - insert) * sizeof(vmapentry));	/* !1.3! */
	at->key = key;
	at->value = 0;
	++vm->size;
	return insert;
}

size_t
vmap_insert(vmap * vm, const int key, void *data)
/* inserts an object into the vmap, identifies it with the 'key' which must be
 * unique, and returns the vmapentry it created. */
{
	size_t insert = vmap_lowerbound(vm, key);
	vmapentry *at;

	/* make sure it's a unique key: */
	assert(insert == vm->size || vm->data[insert].key != key);
	/* insert at this position: */
	if (vm->size == vm->maxsize) {
		/* need more space */
		vm->maxsize = vm->maxsize ? (vm->maxsize * 2) : 4;
		vm->data = realloc(vm->data, vm->maxsize * sizeof(vmapentry));
	}
	at = vm->data + insert;
	memmove(at + 1, at, (vm->size - insert) * sizeof(vmapentry));
	at->key = key;
	at->value = data;
	++vm->size;
	return insert;
}

size_t
vmap_find(const vmap * vm, const int key)
/* returns the index of the vmapentry that's identified by the key or size (a
 * past-the-end value) if it is not found. */
{
	vmapentry *first = vm->data, *middle;
	unsigned int half, len = vm->size;
	while (len > 0) {
		half = len / 2;
		middle = first;
		middle += half;
		if (middle->key < key) {
			first = middle + 1;
			len = len - half - 1;
		} else
			len = half;
	}
	if (first != vm->data + vm->size && first->key == key)
		return first - vm->data;
	else
		return vm->size;
}
