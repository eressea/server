#ifndef H_KRNL_BUILDING
#define H_KRNL_BUILDING

#include <kernel/types.h>
#include <util/resolve.h>
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
#define BTF_MAGIC          0x10 /* magical effect */
#define BTF_NAMECHANGE     0x20 /* name and description can be changed more than once */
#define BTF_FORTIFICATION  0x40 /* building_protection, safe from monsters */
#define BTF_ONEPERTURN     0x80 /* one one sizepoint can be added per turn */
#define BTF_DYNAMIC        0x100 /* dynamic type, needs bt_write */

#define BTF_DEFAULT (BTF_NAMECHANGE)

    typedef struct building_stage {
        /* construction of this building stage: */
        struct construction *construction;  
        /* building stage name: */
        char * name;
        /* next stage, if upgradable: */
        struct building_stage * next; 
    } building_stage;

    typedef struct building_type {
        char *_name;

        int flags;                  /* flags */
        int capacity;               /* Kapazitaet pro Groessenpunkt */
        int maxcapacity;            /* Max. Kapazitaet */
        int maxsize;                /* how big can it get, with all the extensions? */
        variant magres;                 /* how well it resists against spells */
        int magresbonus;            /* bonus it gives the target against spells */
        int fumblebonus;            /* bonus that reduces fumbling */
        int taxes;                  /* receive $1 tax per `taxes` in region */
        double auraregen;           /* modifier for aura regeneration inside building */
        struct maintenance *maintenance;    /* array of requirements */
        struct resource_mod *modifiers; /* modify production skills */
        struct building_stage *stages;
    } building_type;

    extern struct selist *buildingtypes;
    extern struct attrib_type at_building_generic_type;

    int cmp_castle_size(const struct building *b, const struct building *a);
    int building_protection(const struct building_type *btype, int stage);
    building_type *bt_get_or_create(const char *name);
    bool bt_changed(int *cache);
    const building_type *bt_find(const char *name);
    void free_buildingtypes(void);
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
        int flags;
    } building;


    const char *buildingtype(const building_type * btype,
        const struct building *b, int bsize);
    const char *write_buildingname(const building * b, char *ibuf,
        size_t size);
    int buildingcapacity(const struct building *b);
    struct building *building_create(int id);
    struct building *new_building(const struct building_type *typ,
        struct region *r, const struct locale *lang, int size);
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
    int building_taxes(const building *b);

    /* old functions, still in build.c: */
    int buildingeffsize(const building * b, int imaginary);
    void bhash(struct building *b);
    void bunhash(struct building *b);
    int buildingcapacity(const struct building *b);

    void remove_building(struct building **blist, struct building *b);
    void free_building(struct building *b);
    void free_buildings(void);

    const struct building_type *findbuildingtype(const char *name,
        const struct locale *lang);

#include "build.h"
#define NOBUILDING NULL

#define RESOLVE_BUILDING (TYP_BUILDING << 24)
    void resolve_building(building *b);
    void write_building_reference(const struct building *b,
    struct storage *store);
    int read_building_reference(struct gamedata * data, struct building **bp);

    struct building *findbuilding(int n);

    struct unit *building_owner(const struct building *b);
    void building_set_owner(struct unit * u);
    void building_update_owner(struct building * bld);

    bool buildingtype_exists(const struct region *r,
        const struct building_type *bt, bool working);
    bool building_is_active(const struct building *b);
    bool is_building_type(const struct building_type *btype, const char *name);
    struct building *active_building(const struct unit *u, const struct building_type *btype);

    const char *buildingname(const struct building *b);

    const char *building_getname(const struct building *b);
    void building_setname(struct building *self, const char *name);

    struct region *building_getregion(const struct building *b);
    void building_setregion(struct building *bld, struct region *r);

#ifdef __cplusplus
}
#endif
#endif
