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
#include "functions.h"

#include <critbit.h>

/* libc includes */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static critbit_tree cb_functions;

pf_generic get_function(const char *name)
{
    void * matches;
    pf_generic result;
    if (cb_find_prefix(&cb_functions, name, strlen(name) + 1, &matches, 1, 0)) {
        cb_get_kv(matches, &result, sizeof(result));
        return result;
    }
    return NULL;
}

void register_function(pf_generic fun, const char *name)
{
    char buffer[64];
    size_t len = strlen(name);

    assert(len < sizeof(buffer) - sizeof(fun));
    len = cb_new_kv(name, len, &fun, sizeof(fun), buffer);
    cb_insert(&cb_functions, buffer, len);
}
