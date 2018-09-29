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
#include "follow.h"

#include <kernel/config.h>
#include <kernel/unit.h>

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/variant.h>

#include <storage.h>

static int read_follow(variant * var, void *owner, gamedata *data)
{
    READ_INT(data->store, NULL);   /* skip it */
    return AT_READ_FAIL;
}

attrib_type at_follow = {
    "follow", NULL, NULL, NULL, NULL, read_follow
};

attrib *make_follow(struct unit * u)
{
    attrib *a = a_new(&at_follow);
    a->data.v = u;
    return a;
}
