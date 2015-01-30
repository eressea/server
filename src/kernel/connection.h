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

#ifndef H_KRNL_BORDER
#define H_KRNL_BORDER

#include <util/variant.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct attrib;
    struct attrib_type;
    struct faction;
    struct region;
    struct storage;
    struct unit;

    extern int nextborder;

    typedef struct connection {
        struct border_type *type;   /* the type of this connection */
        struct connection *next;    /* next connection between these regions */
        struct connection *nexthash;        /* next connection between these regions */
        struct region *from, *to;   /* borders can be directed edges */
        variant data;
        int id;            /* unique id */
    } connection;

    typedef struct border_type {
        const char *__name;         /* internal use only */
        variant_type datatype;
        bool(*transparent) (const connection *, const struct faction *);
        /* is it possible to see through this? */
        void(*init) (connection *);
        /* constructor: initialize the connection. allocate extra memory if needed */
        void(*destroy) (connection *);
        /* destructor: remove all extra memory for destruction */
        void(*read) (connection *, struct storage *);
        void(*write) (const connection *, struct storage *);
        bool(*block) (const connection *, const struct unit *,
            const struct region * r);
        /* return true if it blocks movement of u from
         * r to the opposite struct region.
         * warning: struct unit may be NULL.
         */
        const char *(*name) (const connection * b, const struct region * r,
            const struct faction * f, int gflags);
        /* for example "a wall of fog" or "a doorway from r1 to r2"
         * may depend on the struct faction, for example "a wall" may
         * turn out to be "an illusionary wall"
         */
        bool(*rvisible) (const connection *, const struct region *);
        /* is it visible to everyone in r ?
         * if not, it may still be fvisible() for some f.
         */
        bool(*fvisible) (const connection *, const struct faction *,
            const struct region *);
        /* is it visible to units of f in r?
         * the function shall not check for
         * existence of a struct unit in r. Example: a spell
         * may be visible only to the struct faction that created it and thus
         * appear in it's report (if there is a struct unit of f in r, which
         * the reporting function will have to assure).
         * if not true, it may still be uvisible() for some u.
         */
        bool(*uvisible) (const connection *, const struct unit *);
        /* is it visible to u ?
         * a doorway may only be visible to a struct unit with perception > 5
         */
        bool(*valid) (const connection *);
        /* is the connection in a valid state,
         * or should it be erased at the end of this turn to save space?
         */
        struct region *(*move) (const connection *, struct unit * u,
        struct region * from, struct region * to, bool routing);
        /* executed when the units traverses this connection */
        int(*age) (struct connection *);
        /* return 0 if connection needs to be removed. >0 if still aging, <0 if not aging */
        struct border_type *next;   /* for internal use only */
    } border_type;

    connection *find_border(int id);
    int resolve_borderid(variant data, void *addr);
    void free_borders(void);

    void walk_connections(struct region *r, void(*cb)(struct connection *, void *), void *data);
    connection *get_borders(const struct region *r1,
        const struct region *r2);
    /* returns the list of borders between r1 and r2 or r2 and r1 */
    connection *new_border(border_type * type, struct region *from,
    struct region *to);
    /* creates a connection of the specified type */
    void erase_border(connection * b);
    /* remove the connection from memory */
    border_type *find_bordertype(const char *name);
    void register_bordertype(border_type * type);
    /* register a new bordertype */

    int read_borders(struct storage *store);
    void write_borders(struct storage *store);
    void age_borders(void);

    /* provide default implementations for some member functions: */
    void b_read(connection * b, struct storage *store);
    void b_write(const connection * b, struct storage *store);
    bool b_blockall(const connection *, const struct unit *,
        const struct region *);
    bool b_blocknone(const connection *, const struct unit *,
        const struct region *);
    bool b_rvisible(const connection *, const struct region *r);
    bool b_fvisible(const connection *, const struct faction *f,
        const struct region *);
    bool b_uvisible(const connection *, const struct unit *u);
    bool b_rinvisible(const connection *, const struct region *r);
    bool b_finvisible(const connection *, const struct faction *f,
        const struct region *);
    bool b_uinvisible(const connection *, const struct unit *u);
    bool b_transparent(const connection *, const struct faction *);
    bool b_opaque(const connection *, const struct faction *);  /* !transparent */

    extern border_type bt_fogwall;
    extern border_type bt_noway;
    extern border_type bt_wall;
    extern border_type bt_illusionwall;
    extern border_type bt_road;
    extern border_type bt_questportal;

    extern struct attrib_type at_countdown;

    /* game-specific callbacks */
    void(*border_convert_cb) (struct connection * con, struct attrib * attr);

#ifdef __cplusplus
}
#endif
#endif
