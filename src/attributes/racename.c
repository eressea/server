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
#include "racename.h"

#include <kernel/save.h>
#include <util/attrib.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

attrib_type at_racename = {
    "racename", NULL, a_finalizestring, NULL, a_writestring, a_readstring
};

const char *get_racename(attrib * alist)
{
    attrib *a = a_find(alist, &at_racename);
    if (a)
        return (const char *)a->data.v;
    return NULL;
}

void set_racename(attrib ** palist, const char *name)
{
    attrib *a = a_find(*palist, &at_racename);
    if (!a && name) {
        a = a_add(palist, a_new(&at_racename));
        a->data.v = _strdup(name);
    }
    else if (a && !name) {
        a_remove(palist, a);
    }
    else if (a) {
        if (strcmp(a->data.v, name) != 0) {
            free(a->data.v);
            a->data.v = _strdup(name);
        }
    }
}
