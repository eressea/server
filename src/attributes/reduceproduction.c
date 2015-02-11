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
#include "reduceproduction.h"
#include <kernel/save.h>
#include <util/attrib.h>

static int age_reduceproduction(attrib * a)
{
    int reduce = 100 - (5 * --a->data.sa[1]);
    if (reduce < 10)
        reduce = 10;
    a->data.sa[0] = (short)reduce;
    return (a->data.sa[1] > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

attrib_type at_reduceproduction = {
    "reduceproduction",
    NULL,
    NULL,
    age_reduceproduction,
    a_writeshorts,
    a_readshorts,
    ATF_UNIQUE
};

attrib *make_reduceproduction(int percent, int time)
{
    attrib *a = a_new(&at_reduceproduction);
    a->data.sa[0] = (short)percent;
    a->data.sa[1] = (short)time;
    return a;
}
