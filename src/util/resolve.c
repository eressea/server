/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
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

#include <platform.h>
#include <assert.h>
#include <stdlib.h>
#include "resolve.h"
#include "storage.h"
#include "variant.h"

typedef struct unresolved {
    void *ptrptr;
    /* address to pass to the resolve-function */
    variant data;
    /* information on how to resolve the missing object */
    resolve_fun resolve;
    /* function to resolve the unknown object */
} unresolved;

#define BLOCKSIZE 1024
static unresolved *ur_list;
static unresolved *ur_begin;
static unresolved *ur_current;

variant read_int(struct storage *store)
{
    variant var;
    READ_INT(store, &var.i);
    return var;
}

int
read_reference(void *address, struct gamedata * data, read_fun reader,
    resolve_fun resolver)
{
    variant var = reader(data);
    int result = resolver(var, address);
    if (result != 0) {
        ur_add(var, address, resolver);
    }
    return result;
}

void ur_add(variant data, void *ptrptr, resolve_fun fun)
{
    assert(ptrptr);
    if (ur_list == NULL) {
        ur_list = malloc(BLOCKSIZE * sizeof(unresolved));
        ur_begin = ur_current = ur_list;
    }
    else if (ur_current - ur_begin == BLOCKSIZE - 1) {
        ur_begin = malloc(BLOCKSIZE * sizeof(unresolved));
        ur_current->data.v = ur_begin;
        ur_current = ur_begin;
    }
    ur_current->data = data;
    ur_current->resolve = fun;
    ur_current->ptrptr = ptrptr;

    ++ur_current;
    ur_current->resolve = NULL;
    ur_current->data.v = NULL;
}

void resolve(void)
{
    unresolved *ur = ur_list;
    while (ur) {
        if (ur->resolve == NULL) {
            ur = ur->data.v;
            free(ur_list);
            ur_list = ur;
            continue;
        }
        assert(ur->ptrptr);
        ur->resolve(ur->data, ur->ptrptr);
        ++ur;
    }
    free(ur_list);
    ur_list = NULL;
}
