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

#ifdef __cplusplus
extern "C" {
#endif

    struct gamedata;
    
    /* maintenance::flags */
#define MTF_NONE     0x00
#define MTF_VARIABLE 0x01       /* resource usage scales with size */
#define MTF_VITAL    0x02       /* if resource missing, building may crash */

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
#define BTF_FORTIFICATION 0x200 /* safe from monsters */

    typedef enum {
        DEFENSE_BONUS,
        CLOSE_COMBAT_ATTACK_BONUS,
        RANGED_ATTACK_BONUS,
    } building_bonus;

    typedef struct building_type {
        char *_name;

        int flags;                  /* flags */
        int capacity;               /* Kapazität pro Größenpunkt */
        int maxcapacity;            /* Max. Kapazität */
        int maxsize;                /* how big can it get, with all the extensions? */
        int magres;                 /* how well it resists against spells */
        int magresbonus;            /* bonus it gives the target against spells */
        int fumblebonus;            /* bonus that reduces fumbling */
        double auraregen;           /* modifier for aura regeneration inside building */
        struct maintenance *maintenance;    /* array of requirements */
        struct construction *construction;  /* construction of 1 building-level */

        const char *(*name) (const struct building_type *,
            const struct building * b, int size);
        void(*init) (struct building_type *);
        void(*age) (struct building *);
        int(*protection) (struct building *, struct unit *, building_bonus);
        double(*taxes) (const struct building *, int size);
        struct attrib *attribs;
    } building_type;

    extern struct quicklist *buildingtypes;
    extern struct attrib_type at_building_action;

    building_type *bt_get_or_create(const char *name);
    const building_type *bt_find(const char *name);
    void free_buildingtypes(void);
    void register_buildings(void);
    void bt_register(struct building_type *type);
    int bt_effsize(const struct building_type *btype,
        const struct building *b, int bsize);

    bool in_safe_building(struct unit *u1, struct unit *u2);
    /* buildingt => building_type
     * Name => locale_string(name)
     * MaxGroesse => levels
     * MinBauTalent => construction->minskill
     * Kapazitaet => capacity, maxcapacity
     * Materialien => construction->materials
     * UnterSilber, UnterSpezialTyp, UnterSpezial => maintenance
     * per_size => !maintenance->fixed
     */
#define BFL_NONE           0x00
#define BLD_MAINTAINED     0x01 /* vital maintenance paid for */
#define BLD_WORKING        0x02 /* full maintenance paid, it works */
#define BLD_UNGUARDED      0x04 /* you can enter this building anytime */
#define BLD_EXPANDED       0x08 /* has been expanded this turn */
#define BLD_SELECT         0x10 /* formerly FL_DH */
#define BLD_DONTPAY        0x20 /* PAY NOT */

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

    /* Alte Gebäudetypen: */

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
    struct building *active_building(const struct unit *u, const struct building_type *btype);

#ifdef WDW_PYRAMID
    extern int wdw_pyramid_level(const struct building *b);
#endif

    extern const char *buildingname(const struct building *b);

    extern const char *building_getname(const struct building *b);
    extern void building_setname(struct building *self, const char *name);

    struct region *building_getregion(const struct building *b);
    void building_setregion(struct building *bld, struct region *r);

#ifdef __cplusplus
}
#endif
#endif
