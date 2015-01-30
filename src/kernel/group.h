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

#ifndef H_KERNEL_GROUP
#define H_KERNEL_GROUP
#ifdef __cplusplus
extern "C" {
#endif

    struct gamedata;

    typedef struct group {
        struct group *next;
        struct group *nexthash;
        struct faction *f;
        struct attrib *attribs;
        char *name;
        struct ally *allies;
        int gid;
        int members;
    } group;

    extern struct attrib_type at_group;   /* attribute for units assigned to a group */
    extern bool join_group(struct unit *u, const char *name);
    extern void set_group(struct unit *u, struct group *g);
    extern struct group * get_group(const struct unit *u);
    extern void free_group(struct group *g);
    struct group *new_group(struct faction * f, const char *name, int gid);

    extern void write_groups(struct storage *data, const struct faction *f);
    extern void read_groups(struct storage *data, struct faction *f);

#ifdef __cplusplus
}
#endif
#endif
