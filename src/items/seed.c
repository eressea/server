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

#include "seed.h"

/* kernel includes */
#include <kernel/build.h>
#include <kernel/config.h>
#include <kernel/item.h>
#include <kernel/region.h>

/* util includes */
#include <util/attrib.h>
#include <util/functions.h>

/* libc includes */
#include <assert.h>

static void produce_seeds(region * r, const resource_type * rtype, int norders)
{
    assert(r->land && r->land->trees[0] >= norders);
    r->land->trees[0] -= norders;
}

static int limit_seeds(const region * r, const resource_type * rtype)
{
    if (fval(r, RF_MALLORN)) {
        return 0;
    }
    return r->land ? r->land->trees[0] : 0;
}

void init_seed(void)
{
    attrib *a;
    resource_limit *rdata;
    resource_type *rtype;

    rtype = rt_find("seed");
    if (rtype != NULL) {
        a = a_add(&rtype->attribs, a_new(&at_resourcelimit));
        rdata = (resource_limit *)a->data.v;
        rdata->limit = limit_seeds;
        rdata->produce = produce_seeds;
    }
}

/* mallorn */

static void
produce_mallornseeds(region * r, const resource_type * rtype, int norders)
{
    assert(fval(r, RF_MALLORN));
    r->land->trees[0] -= norders;
}

static int limit_mallornseeds(const region * r, const resource_type * rtype)
{
    if (!fval(r, RF_MALLORN)) {
        return 0;
    }
    return r->land ? r->land->trees[0] : 0;
}

void init_mallornseed(void)
{
    attrib *a;
    resource_limit *rdata;
    resource_type *rtype;

    rtype = rt_find("mallornseed");
    if (rtype != NULL) {
        rtype->flags |= RTF_LIMITED;
        rtype->flags |= RTF_POOLED;

        a = a_add(&rtype->attribs, a_new(&at_resourcelimit));
        rdata = (resource_limit *)a->data.v;
        rdata->limit = limit_mallornseeds;
        rdata->produce = produce_mallornseeds;
    }
}
