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
