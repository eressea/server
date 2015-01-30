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

#ifndef TERRAIN_H
#define TERRAIN_H
#ifdef __cplusplus
extern "C" {
#endif

    /* diverse Flags */
    /* Strassen und Gebäude können gebaut werden, wenn max_road > 0 */
#define LAND_REGION      (1<<0) /* Standard-Land-struct region */
#define SEA_REGION       (1<<1) /* hier braucht man ein Boot */
#define FOREST_REGION    (1<<2) /* Elfen- und Kampfvorteil durch Bäume */
#define ARCTIC_REGION    (1<<3) /* Gletscher & co = Keine Insekten! */
#define CAVALRY_REGION   (1<<4) /* riding in combat is possible */
    /* Achtung: SEA_REGION ist nicht das Gegenteil von LAND_REGION. Die zwei schliessen sich nichtmal aus! */
#define FORBIDDEN_REGION (1<<5) /* unpassierbare Blockade-struct region */
#define SAIL_INTO		     (1<<6)     /* man darf hierhin segeln */
#define FLY_INTO		     (1<<7)     /* man darf hierhin fliegen */
#define SWIM_INTO		     (1<<8)     /* man darf hierhin schwimmen */
#define WALK_INTO		     (1<<9)     /* man darf hierhin laufen */

    typedef struct production_rule {
        char *name;
        const struct resource_type *rtype;

        void(*terraform) (struct production_rule *, const struct region *);
        void(*update) (struct production_rule *, const struct region *);
        void(*use) (struct production_rule *, const struct region *, int amount);
        int(*visible) (const struct production_rule *, int skilllevel);

        /* no initialization required */
        struct production_rule *next;
    } production_rule;

    typedef struct terrain_production {
        const struct resource_type *type;
        const char *startlevel;
        const char *base;
        const char *divisor;
        float chance;
    } terrain_production;

    typedef struct terrain_type {
        char *_name;
        int size;                   /* how many peasants can work? */
        int flags;
        short max_road;             /* this many stones make a full road */
        short distribution;         /* multiplier used for seeding */
        struct terrain_production *production;
        const struct item_type **herbs;     /* zero-terminated array of herbs */
        const char *(*name) (const struct region * r);
        struct terrain_type *next;
    } terrain_type;

    extern terrain_type *get_or_create_terrain(const char *name);
    extern const terrain_type *terrains(void);
    extern const terrain_type *get_terrain(const char *name);
    extern const char *terrain_name(const struct region *r);

    extern void init_terrains(void);
    void free_terrains(void);

#ifdef __cplusplus
}
#endif
#endif                          /* TERRAIN_H */
