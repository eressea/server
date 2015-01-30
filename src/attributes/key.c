/*
Copyright (c) 1998-2015, Enno Rehling Rehling <enno@eressea.de>
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
#include "key.h"

#include <kernel/save.h>
#include <util/attrib.h>

attrib_type at_key = {
    "key",
    NULL,
    NULL,
    NULL,
    a_writeint,
    a_readint,
};

attrib *add_key(attrib ** alist, int key)
{
    attrib *a = find_key(*alist, key);
    if (a == NULL)
        a = a_add(alist, make_key(key));
    return a;
}

attrib *make_key(int key)
{
    attrib *a = a_new(&at_key);
    a->data.i = key;
    return a;
}

attrib *find_key(attrib * alist, int key)
{
    attrib *a = a_find(alist, &at_key);
    while (a && a->type == &at_key && a->data.i != key) {
        a = a->next;
    }
    return (a && a->type == &at_key) ? a : NULL;
}
