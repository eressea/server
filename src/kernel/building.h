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

#ifndef H_KRNL_BUILDING
#define H_KRNL_BUILDING

#include <kernel/types.h>
#include <util/variant.h>

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct gamedata;
    
    /* maintenance::flags */
#define MTF_NONE     0x00
#define MTF_VARIABLE 0x01       /* resource usage scales with size */

    typedef struct maintenance {
        const struct resource_type *rtype;  /* type of resource required */
        int number;                 /* amount of resources */
        int flags;         /* misc. flags */
    } maintenance;

    /* building_type::flags */
#define BTF_NONE           0x00
#define BTF_INDESTRUCTIBLE 0x01 /* cannot be torm down */
#define BTF_NOBUILD        0x02 /* special, can't be built */
#define BTF_UNIQUE         0x04 /* only one per struct region (harbour) */
#define BTF_DECAY          0x08 /* decays when not occupied */
#define BTF_DYNAMIC        0x10 /* dynamic type, needs bt_write */
#define BTF_MAGIC          0x40 /* magical effect */
#define BTF_ONEPERTURN     0x80 /* one one sizepoint can be added per turn */
#define BTF_NAMECHANGE    0x100 /* name and description can be changed more than once */
#define BTF_FORTIFICATION 0x200 /* building_protection, safe from monsters */

    typedef struct building_type {
        char *_name;

        int flags;                  /* flags */
        int capacity;               /* Kapazit�t pro Gr��enpunkt */
        int maxcapacity;            /* Max. Kapazit�t */
        int maxsize;                /* how big can it get, with all the extensions? */
        variant magres;                 /* how well it resists against spells */
        int magresbonus;            /* bonus it gives the target against spells */
        int fumblebonus;            /* bonus that reduces fumbling */
        double auraregen;           /* modifier for aura regeneration inside building */
        struct maintenance *maintenance;    /* array of requirements */
        struct construction *construction;  /* construction of 1 building-level */
        struct resource_mod *modifiers; /* modify production skills */

        const char *(*name) (const struct building_type *,
            const struct building * b, int size);
        double(*taxes) (const struct building *, int level);
        struct attrib *attribs;
    } building_type;

    extern struct selist *buildingtypes;
    extern struct attrib_type at_building_action;

    int cmp_castle_size(const struct building *b, const struct building *a);
    int building_protection(const struct building_type *btype, int stage);
    building_type *bt_get_or_create(const char *name);
    bool bt_changed(int *cache);
    const building_type *bt_find(const char *name);
    void free_buildingtypes(void);
    void bt_register(struct building_type *type);
    int bt_effsize(const struct building_type *btype,
        const struct building *b, int bsize);

    bool in_safe_building(struct unit *u1, struct unit *u2);

#define BFL_NONE           0x00
#define BLD_MAINTAINED     0x01 /* vital maintenance paid for */
#define BLD_DONTPAY        0x02 /* PAY NOT */
#define BLD_UNGUARDED      0x04 /* you can enter this building anytime */
#define BLD_EXPANDED       0x08 /* has been expanded this turn */
#define BLD_SELECT         0x10 /* formerly FL_DH */

#define BLD_SAVEMASK       0x00 /* mask for persistent flags */

    typedef struct building {
        struct building *next;
        struct building *nexthash;

        const struct building_type *type;
        struct region *region;
        struct unit *_owner; /* you should always use building_owner(), never this naked pointer */
        char *name;
        char *display;
        struct attrib *attribs;
        int no;
        int size;
        int sizeleft;               /* is only used during battle. should be a temporary attribute */
        int besieged;               /* should be an attribute */
        int flags;
    } building;

    extern struct attrib_type at_building_generic_type;
    extern const char *buildingtype(const building_type * btype,
        const struct building *b, int bsize);
    extern const char *write_buildingname(const building * b, char *ibuf,
        size_t size);
    extern int buildingcapacity(const struct building *b);
    extern struct building *new_building(const struct building_type *typ,
    struct region *r, const struct locale *lang);
    int build_building(struct unit *u, const struct building_type *typ,
        int id, int size, struct order *ord);
    bool building_finished(const struct building *b);

    int wage(const struct region *r, const struct faction *f,
        const struct race *rc, int in_turn);

    typedef int(*cmp_building_cb) (const struct building * b,
        const struct building * a);
    struct building *largestbuilding(const struct region *r, cmp_building_cb,
        bool imaginary);
    int cmp_wage(const struct building *b, const struct building *bother);
    int cmp_taxes(const struct building *b, const struct building *bother);
    int cmp_current_owner(const struct building *b,
        const struct building *bother);
    int building_taxes(const building *b, int bsize);

    /* old functions, still in build.c: */
    int buildingeffsize(const building * b, int imaginary);
    void bhash(struct building *b);
    void bunhash(struct building *b);
    int buildingcapacity(const struct building *b);

    extern void remove_building(struct building **blist, struct building *b);
    extern void free_building(struct building *b);
    extern void free_buildings(void);

    const struct building_type *findbuildingtype(const char *name,
        const struct locale *lang);

#include "build.h"
#define NOBUILDING NULL

    extern int resolve_building(variant data, void *address);
    extern void write_building_reference(const struct building *b,
    struct storage *store);
    extern variant read_building_reference(struct gamedata *data);

    extern struct building *findbuilding(int n);

    extern struct unit *building_owner(const struct building *b);
    extern void building_set_owner(struct unit * u);
    extern void building_update_owner(struct building * bld);

    bool buildingtype_exists(const struct region *r,
        const struct building_type *bt, bool working);
    bool building_is_active(const struct building *b);
    bool is_building_type(const struct building_type *btype, const char *name);
    struct building *active_building(const struct unit *u, const struct building_type *btype);

    extern const char *buildingname(const struct building *b);

    extern const char *building_getname(const struct building *b);
    extern void building_setname(struct building *self, const char *name);

    struct region *building_getregion(const struct building *b);
    void building_setregion(struct building *bld, struct region *r);

#ifdef __cplusplus
}
#endif
#endif
