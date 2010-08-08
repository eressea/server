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
