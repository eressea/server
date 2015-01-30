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

#ifndef H_MOD_GMCMD
#define H_MOD_GMCMD
#ifdef __cplusplus
extern "C" {
#endif

    struct plane;
    struct attrib;
    struct unit;
    struct faction;
    struct region;

    extern void register_gmcmd(void);
    /* initialize this module */

    extern void gmcommands(void);
    /* execute commands */

    extern struct faction *gm_addfaction(const char *email, struct plane *p,
    struct region *r);
    extern struct plane *gm_addplane(int radius, unsigned int flags,
        const char *name);

    /*
     * doesn't belong in here:
     */
    struct attrib *find_key(struct attrib *attribs, int key);

#ifdef __cplusplus
}
#endif
#endif
