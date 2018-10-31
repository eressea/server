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

#ifndef ALLY_H
#define ALLY_H

#ifdef __cplusplus
extern "C" {
#endif

    struct attrib_type;
    struct faction;
    struct group;
    struct gamedata;
    struct unit;
    struct ally;
    struct allies;

    extern struct attrib_type at_npcfaction;

    int allies_get(struct allies *al, const struct faction *f);
    void allies_set(struct allies **p_al, const struct faction *f, int status);
    void allies_write(struct gamedata * data, const struct allies *alist);
    void allies_read(struct gamedata * data, struct allies **sfp);

    void read_allies(struct gamedata * data, struct ally **alist);
    void write_allies(struct gamedata * data, const struct ally *alist);
    typedef int (*cb_ally_walk)(struct ally *, struct faction *, int, void *);
    int ally_walk(struct ally *allies, cb_ally_walk callback, void *udata);
    struct ally *ally_clone(const struct ally *al);
    void allies_free(struct ally *al);

    struct ally* ally_find(struct ally*al, const struct faction *f);
    void ally_set(struct ally**al_p, struct faction *f, int status);
    int ally_get(struct ally *al, const struct faction *f);

    int AllianceAuto(void);        /* flags that allied factions get automatically */
    int HelpMask(void);    /* flags restricted to allied factions */
    int alliedunit(const struct unit *u, const struct faction *f2,
        int mask);

    int alliedfaction(const struct faction *f, const struct faction *f2,
        int mask);
    int alliedgroup(const struct faction *f, const struct faction *f2,
        const struct group *g, int mask);
    int alliance_status(const struct faction *f, const struct faction *f2, int status);

#ifdef __cplusplus
}
#endif

#endif
