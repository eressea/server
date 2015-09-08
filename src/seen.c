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
#include "seen.h"

#include <kernel/region.h>

#include <assert.h>
#include <stdlib.h>

#define MAXSEEHASH 0x1000
seen_region *reuse;

seen_region **seen_init(void)
{
    return (seen_region **)calloc(MAXSEEHASH, sizeof(seen_region *));
}

void seen_done(seen_region * seehash[])
{
    int i;
    for (i = 0; i != MAXSEEHASH; ++i) {
        seen_region *sd = seehash[i];
        if (sd == NULL)
            continue;
        while (sd->nextHash != NULL)
            sd = sd->nextHash;
        sd->nextHash = reuse;
        reuse = seehash[i];
        seehash[i] = NULL;
    }
    /*  free(seehash); */
}

void free_seen(void)
{
    while (reuse) {
        seen_region *r = reuse;
        reuse = reuse->nextHash;
        free(r);
    }
}

void
link_seen(seen_region * seehash[], const region * first, const region * last)
{
    const region *r = first;
    seen_region *sr = NULL;

    if (first == last)
        return;

    do {
        sr = find_seen(seehash, r);
        r = r->next;
    } while (sr == NULL && r != last);

    while (r != last) {
        seen_region *sn = find_seen(seehash, r);
        if (sn != NULL) {
            sr->next = sn;
            sr = sn;
        }
        r = r->next;
    }
    if (sr) sr->next = 0;
}

seen_region *find_seen(struct seen_region *seehash[], const region * r)
{
    unsigned int index = reg_hashkey(r) & (MAXSEEHASH - 1);
    seen_region *find = seehash[index];
    while (find) {
        if (find->r == r) {
            return find;
        }
        find = find->nextHash;
    }
    return NULL;
}

void get_seen_interval(struct seen_region *seen[], struct region **firstp, struct region **lastp)
{
    /* this is required to find the neighbour regions of the ones we are in,
     * which may well be outside of [firstregion, lastregion) */
    int i;
    region *first, *last;

    assert(seen && firstp && lastp);
    first = *firstp;
    last = *lastp;
    for (i = 0; i != MAXSEEHASH; ++i) {
        seen_region *sr = seen[i];
        while (sr != NULL) {
            if (first == NULL || sr->r->index < first->index) {
                first = sr->r;
            }
            if (last != NULL && sr->r->index >= last->index) {
                last = sr->r->next;
            }
            sr = sr->nextHash;
        }
    }
}

bool add_seen(struct seen_region *seehash[], struct region *r, int mode, bool dis)
{
    seen_region *find = find_seen(seehash, r);
    if (find == NULL) {
        unsigned int index = reg_hashkey(r) & (MAXSEEHASH - 1);
        if (!reuse)
            reuse = (seen_region *)calloc(1, sizeof(struct seen_region));
        find = reuse;
        reuse = reuse->nextHash;
        find->nextHash = seehash[index];
        seehash[index] = find;
        find->r = r;
    }
    else if (find->mode >= mode) {
        return false;
    }
    find->mode = mode;
    find->disbelieves |= dis;
    return true;
}
