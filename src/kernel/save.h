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

#ifndef H_KRNL_SAVE
#define H_KRNL_SAVE

#include <stream.h>
#ifdef __cplusplus
extern "C" {
#endif

    struct attrib;
    struct item;
    struct storage;
    struct spell;
    struct spellbook;
    struct unit;
    struct building;
    struct ship;
    struct gamedata;

#define MAX_INPUT_SIZE	DISPLAYSIZE*2
    /* Nach MAX_INPUT_SIZE brechen wir das Einlesen der Zeile ab und nehmen an,
     * dass hier ein Fehler (fehlende ") vorliegt */

    extern int data_version;

    // TODO: is this *really* still in use?
    extern int enc_gamedata;

    int readorders(const char *filename);
    int creategame(void);
    int readgame(const char *filename, bool backup);
    int writegame(const char *filename);

    int current_turn(void);

    void read_items(struct storage *store, struct item **it);
    void write_items(struct storage *store, struct item *it);

    void read_spellbook(struct spellbook **bookp, struct gamedata *data, int(*get_level)(const struct spell * sp, void *), void * cbdata);
    void write_spellbook(const struct spellbook *book, struct storage *store);

    void write_attribs(struct storage *store, struct attrib *alist, const void *owner);
    int read_attribs(struct gamedata *store, struct attrib **alist, void *owner);

    void write_unit(struct gamedata *data, const struct unit *u);
    struct unit *read_unit(struct gamedata *data);
    
    void write_faction(struct gamedata *data, const struct faction *f);
    struct faction *read_faction(struct gamedata *data);

    void write_region(struct gamedata *data, const struct region *r);
    struct region *read_region(struct gamedata *data);

    void write_building(struct gamedata *data, const struct building *b);
    struct building *read_building(struct gamedata *data);

    void write_ship(struct gamedata *data, const struct ship *sh);
    struct ship *read_ship(struct gamedata *data);

    void create_backup(char *file);

    int write_game(struct gamedata *data);
    int read_game(struct gamedata *data);

    /* test-only functions that give access to internal implementation details (BAD) */
    void _test_write_password(struct gamedata *data, const struct faction *f);
    void _test_read_password(struct gamedata *data, struct faction *f);
#ifdef __cplusplus
}
#endif
#endif
