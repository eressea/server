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
#include <kernel/config.h>
#include "moved.h"

#include <util/attrib.h>

#include <storage.h>

static int age_moved(attrib * a, void *owner)
{
    unused_arg(owner);
    --a->data.i;
    return a->data.i > 0;
}

static void
write_moved(const attrib * a, const void *owner, struct storage *store)
{
    WRITE_INT(store, a->data.i);
}

static int read_moved(attrib * a, void *owner, struct storage *store)
{
    READ_INT(store, &a->data.i);
    if (a->data.i != 0)
        return AT_READ_OK;
    else
        return AT_READ_FAIL;
}

attrib_type at_moved = {
    "moved", NULL, NULL, age_moved, write_moved, read_moved
};

bool get_moved(attrib ** alist)
{
    return a_find(*alist, &at_moved) ? true : false;
}

void set_moved(attrib ** alist)
{
    attrib *a = a_find(*alist, &at_moved);
    if (a == NULL)
        a = a_add(alist, a_new(&at_moved));
    a->data.i = 2;
}
