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

#ifndef H_KRNL_ALLIANCE
#define H_KRNL_ALLIANCE

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct plane;
    struct attrib;
    struct unit;
    struct faction;
    struct region;

    enum {
        ALLIANCE_KICK,
        ALLIANCE_LEAVE,
        ALLIANCE_TRANSFER,
        ALLIANCE_NEW,
        ALLIANCE_INVITE,
        ALLIANCE_JOIN,
        ALLIANCE_MAX
    };

    extern const char* alliance_kwd[ALLIANCE_MAX];
#define ALF_NON_ALLIED (1<<0)   /* this alliance is just a default for a non-allied faction */

#define ALLY_ENEMY (1<<0)

    typedef struct alliance {
        struct alliance *next;
        struct faction *_leader;
        struct quicklist *members;
        int flags;
        int id;
        char *name;
        struct ally *allies;
    } alliance;

    extern alliance *alliances;
    alliance *findalliance(int id);
    alliance *new_alliance(int id, const char *name);
    alliance *makealliance(int id, const char *name);
    const char *alliancename(const struct alliance *al);
    void setalliance(struct faction *f, alliance * al);
    void free_alliances(void);
    struct faction *alliance_get_leader(struct alliance *al);
    void alliance_cmd(void);
    bool is_allied(const struct faction *f1, const struct faction *f2);

    void alliance_setname(alliance * self, const char *name);
    /* execute commands */

#ifdef __cplusplus
}
#endif
#endif
