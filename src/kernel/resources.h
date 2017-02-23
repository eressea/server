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

    struct building_type;
    struct race;

    enum {
        RM_USED = 1 << 0,           /* resource has been used */
        RM_MALLORN = 1 << 1         /* this is not wood. it's mallorn */
    };

    typedef struct rawmaterial {
        const struct rawmaterial_type *type;
#ifdef LOMEM
        int amount:16;
        int level:8;
        int flags:8;
        int base:8;
        int divisor:8;
        int startlevel:8;
#else
        int amount;
        int level;
        int flags;
        int base;
        int divisor;
        int startlevel;
#endif
        struct rawmaterial *next;
    } rawmaterial;

    typedef int(*rlimit_limit) (const struct region * r,
        const struct resource_type * rtype);
    typedef void(*rlimit_produce) (struct region * r,
        const struct resource_type * rtype, int n);

    typedef struct resource_mod {
        variant value;
        const struct building_type *btype;
        const struct race *race;
        unsigned int flags;
    } resource_mod;

    typedef struct resource_limit {
        rlimit_limit limit;
        rlimit_produce produce;
        int value;
        resource_mod *modifiers;
    } resource_limit;

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
    struct rawmaterial_type *rmt_find(const char *str);
    struct rawmaterial_type *rmt_get(const struct resource_type *);

    void add_resource(struct region *r, int level, int base, int divisor,
        const struct resource_type *rtype);
    struct rawmaterial_type *rmt_create(struct resource_type *rtype,
        const char *name);

#ifdef __cplusplus
}
#endif
#endif
