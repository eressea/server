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
#include <selist.h>
#include "pathfinder.h"

#include "region.h"
#include "terrain.h"

#include <limits.h>
#include <stdlib.h>
#include <assert.h>

#define MAXDEPTH 1024

bool allowed_walk(const region * src, const region * r)
{
    if (fval(r->terrain, WALK_INTO))
        return true;
    return false;
}

bool allowed_fly(const region * src, const region * r)
{
    if (fval(r->terrain, FLY_INTO))
        return true;
    return false;
}

typedef struct node {
    struct node *next;
    region *r;
    struct node *prev;
    int distance;
} node;

static node *node_garbage;

void pathfinder_cleanup(void)
{
    while (node_garbage) {
        node *n = node_garbage;
        node_garbage = n->next;
        free(n);
    }
}

static node *new_node(region * r, int distance, node * prev)
{
    node *n;
    if (node_garbage != NULL) {
        n = node_garbage;
        node_garbage = n->next;
    }
    else
        n = malloc(sizeof(node));
    n->next = NULL;
    n->prev = prev;
    n->r = r;
    n->distance = distance;
    return n;
}

static node *free_node(node * n)
{
    node *s = n->next;
    n->next = node_garbage;
    node_garbage = n;
    return s;
}

static void free_nodes(node * root)
{
    while (root != NULL) {
        region *r = root->r;
        freset(r, RF_MARK);
        root = free_node(root);
    }
}

struct selist *regions_in_range(struct region *start, int maxdist,
    bool(*allowed) (const struct region *, const struct region *))
{
    selist * rlist = NULL;
    node *root = new_node(start, 0, NULL);
    node **end = &root->next;
    node *n = root;

    while (n != NULL) {
        region *r = n->r;
        int depth = n->distance + 1;
        int d;

        if (n->distance >= maxdist)
            break;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = rconnect(r, d);
            if (rn == NULL)
                continue;
            if (fval(rn, RF_MARK))
                continue;               /* already been there */
            if (allowed && !allowed(r, rn))
                continue;               /* can't go there */

            /* add the region to the list of available ones. */
            selist_push(&rlist, rn);

            /* make sure we don't go here again, and put the region into the set for
               further BFS'ing */
            fset(rn, RF_MARK);
            *end = new_node(rn, depth, n);
            end = &(*end)->next;
        }
        n = n->next;
    }
    free_nodes(root);

    return rlist;
}

static region **internal_path_find(region * start, const region * target,
    int maxlen, bool(*allowed) (const region *, const region *))
{
    static region *path[MAXDEPTH + 2];    /* STATIC_RETURN: used for return, not across calls */
    direction_t d;
    node *root = new_node(start, 0, NULL);
    node **end = &root->next;
    node *n = root;
    bool found = false;
    assert(maxlen <= MAXDEPTH);
    fset(start, RF_MARK);

    while (n != NULL) {
        region *r = n->r;
        int depth = n->distance + 1;
        if (n->distance >= maxlen)
            break;
        for (d = 0; d != MAXDIRECTIONS; ++d) {
            region *rn = rconnect(r, d);
            if (rn == NULL)
                continue;
            if (fval(rn, RF_MARK))
                continue;               /* already been there */
            if (!allowed(r, rn))
                continue;               /* can't go there */
            if (rn == target) {
                int i = depth;
                path[i + 1] = NULL;
                path[i] = rn;
                while (n) {
                    path[--i] = n->r;
                    n = n->prev;
                }
                found = true;
                break;
            }
            else {
                fset(rn, RF_MARK);
                *end = new_node(rn, depth, n);
                end = &(*end)->next;
            }
        }
        if (found)
            break;
        n = n->next;
    }
    free_nodes(root);
    if (found)
        return path;
    return NULL;
}

bool
path_exists(region * start, const region * target, int maxlen,
bool(*allowed) (const region *, const region *))
{
    assert((!fval(start, RF_MARK) && !fval(target, RF_MARK))
        || !"Some Algorithm did not clear its RF_MARKs!");
    if (start == target)
        return true;
    if (internal_path_find(start, target, maxlen, allowed) != NULL)
        return true;
    return false;
}

region **path_find(region * start, const region * target, int maxlen,
    bool(*allowed) (const region *, const region *))
{
    assert((!fval(start, RF_MARK) && !fval(target, RF_MARK))
        || !"Did you call path_init()?");
    return internal_path_find(start, target, maxlen, allowed);
}
