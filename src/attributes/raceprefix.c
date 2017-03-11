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
#include "raceprefix.h"

#include <util/attrib.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

attrib_type at_raceprefix = {
    "raceprefix", NULL, a_finalizestring, NULL, a_writestring, a_readstring, NULL,
    ATF_UNIQUE
};

void set_prefix(attrib ** ap, const char *str)
{
    attrib *a = a_find(*ap, &at_raceprefix);
    if (a == NULL) {
        a = a_add(ap, a_new(&at_raceprefix));
    }
    else {
        free(a->data.v);
    }
    assert(a->type == &at_raceprefix);
    a->data.v = strdup(str);
}

const char *get_prefix(attrib * a)
{
    char *str;
    a = a_find(a, &at_raceprefix);
    if (a == NULL)
        return NULL;
    str = (char *)a->data.v;
    /* conversion of old prefixes */
    if (strncmp(str, "prefix_", 7) == 0) {
        ((attrib *)a)->data.v = strdup(str + 7);
        free(str);
        str = (char *)a->data.v;
    }
    return str;
}
