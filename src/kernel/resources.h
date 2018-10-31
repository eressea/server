/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.
 */
#ifndef H_KRNL_RESOURCES
#define H_KRNL_RESOURCES
#ifdef __cplusplus
extern "C" {
#endif

#include <util/variant.h>

    struct building_type;
    struct race;
    struct region;

    enum {
        RM_USED = 1 << 0,           /* resource has been used */
        RM_MALLORN = 1 << 1         /* this is not wood. it's mallorn */
    };

    typedef struct rawmaterial {
        const struct resource_type *rtype;
        int amount;
        int level;
        int flags;
        int base;
        int divisor;
        int startlevel;
        struct rawmaterial *next;
    } rawmaterial;

    /* resource-limits for regions */
    typedef enum resource_modifier_type {
        RMT_END, /* array terminator */
        RMT_PROD_SKILL, /* bonus on resource production skill */
        RMT_PROD_SAVE,   /* fractional multiplier when produced */
        RMT_PROD_REQUIRE, /* building or race is required to produce this item */
        RMT_USE_SAVE,  /* fractional multiplier when used to manufacture something */
    } resource_modifier_type;

    typedef struct resource_mod {
        resource_modifier_type type;
        variant value;
        const struct building_type *btype;
        int race_mask;
    } resource_mod;

    typedef struct rawmaterial_type {
        const struct resource_type *rtype;

        void(*terraform) (struct rawmaterial *, const struct region *);
        void(*update) (struct rawmaterial *, const struct region *);
        void(*use) (struct rawmaterial *, const struct region *, int amount);
        int(*visible) (const struct rawmaterial *, int skilllevel);
    } rawmaterial_type;

    extern struct rawmaterial_type *rawmaterialtypes;

    void update_resources(struct region *r);
    void terraform_resources(struct region *r);
    struct rawmaterial *rm_get(struct region *,
        const struct resource_type *);
    struct rawmaterial_type *rmt_get(const struct resource_type *);

    void set_resource(struct rawmaterial *rm, int level, int base, int divisor);
    struct rawmaterial *add_resource(struct region *r, int level, 
        int base, int divisor, const struct resource_type *rtype);
    struct rawmaterial_type *rmt_create(struct resource_type *rtype);

    int limit_resource(const struct region *r, const struct resource_type *rtype);
    void produce_resource(struct region *r, const struct resource_type *rtype, int amount);

#ifdef __cplusplus
}
#endif
#endif
