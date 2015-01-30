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

#ifndef H_KRNL_PATHFINDER
#define H_KRNL_PATHFINDER
#ifdef __cplusplus
extern "C" {
#endif

#define MAXDEPTH 1024

    extern int search[MAXDEPTH][2];
    extern int search_len;

    extern struct region **path_find(struct region *start,
        const struct region *target, int maxlen,
        bool(*allowed) (const struct region *, const struct region *));
    extern bool path_exists(struct region *start, const struct region *target,
        int maxlen, bool(*allowed) (const struct region *,
        const struct region *));
    extern bool allowed_swim(const struct region *src,
        const struct region *target);
    extern bool allowed_fly(const struct region *src,
        const struct region *target);
    extern bool allowed_walk(const struct region *src,
        const struct region *target);
    extern struct quicklist *regions_in_range(struct region *src, int maxdist,
        bool(*allowed) (const struct region *, const struct region *));

    extern void pathfinder_cleanup(void);

#ifdef __cplusplus
}
#endif
#endif
