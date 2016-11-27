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

#ifndef H_KRNL_SPELLBOOK_H
#define H_KRNL_SPELLBOOK_H

#ifdef __cplusplus
extern "C" {
#endif

    struct spell;
    struct storage;
    struct gamedata;
    struct quicklist;

    typedef struct spellbook_entry {
        struct spell * sp;
        int level;
    } spellbook_entry;

    typedef struct spellbook
    {
        char * name;
        struct quicklist * spells;
    } spellbook;

    spellbook * create_spellbook(const char * name);

    void read_spellbook(struct spellbook **bookp, struct gamedata *data, int(*get_level)(const struct spell * sp, void *), void * cbdata);
    void write_spellbook(const struct spellbook *book, struct storage *store);

    void spellbook_add(spellbook *sbp, struct spell * sp, int level);
    int spellbook_foreach(spellbook *sb, int(*callback)(spellbook_entry *, void *), void * data);
    void spellbook_clear(spellbook *sb);
    spellbook_entry * spellbook_get(spellbook *sb, const struct spell * sp);

#ifdef __cplusplus
}
#endif
#endif
