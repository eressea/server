#ifndef H_KRNL_ITEM
#define H_KRNL_ITEM

#include <util/variant.h>
#include "skill.h"

#ifdef __cplusplus
extern "C" {
#endif

    struct unit;
    struct attrib;
    struct attrib_type;
    struct race;
    struct region;
    struct resource_type;
    struct locale;
    struct troop;
    struct item;
    struct order;
    struct storage;
    struct gamedata;
    struct rawmaterial_type;
    struct resource_mod;
    struct weapon_type;

    typedef struct item {
        struct item *next;
        const struct item_type *type;
        int number;
    } item;

    typedef struct resource {
        const struct resource_type *type;
        int number;
        struct resource *next;
    } resource;

    /* bitfield values for resource_type::flags */
#define RTF_NONE     0
#define RTF_ITEM     (1<<0)     /* this resource is an item */
#define RTF_LIMITED  (1<<1)     /* a resource that's freely available, but in
                                     * limited supply */
#define RTF_POOLED   (1<<2)     /* resource is available in pool */

    /* flags for resource_type::name() */
#define NMF_PLURAL     0x01
#define NMF_APPEARANCE 0x02

    void item_done(void);

    typedef bool(*wtype_attack)(const struct troop *,
        const struct weapon_type *, int *);
    typedef int(*rtype_uchange) (struct unit * user,
        const struct resource_type * rtype, int delta);
    typedef char *(*rtype_name) (const struct resource_type * rtype, int flags);

    typedef struct resource_type {
        /* --- constants --- */
        char *_name;             /* wie es heisst */
        unsigned int flags;
        /* --- functions --- */
        rtype_uchange uchange;
        rtype_name name;
        struct rawmaterial_type *raw;
        struct resource_mod *modifiers;
        /* --- pointers --- */
        struct item_type *itype;
        struct luxury_type *ltype;
        struct weapon_type *wtype;
        struct armor_type *atype;
    } resource_type;

    const char *resourcename(const resource_type * rtype, int flags);
    const resource_type *findresourcetype(const char *name,
        const struct locale *lang);

    /* bitfield values for item_type::flags */
#define ITF_NONE             0x0000
#define ITF_HERB             0x0001     /* this item is a herb */
#define ITF_CURSED           0x0002     /* cursed object, cannot be given away */
#define ITF_NOTLOST          0x0004     /* special object (quests), cannot be lost through death etc. */
#define ITF_BIG              0x0008     /* big item, e.g. does not fit in a bag of holding */
#define ITF_ANIMAL           0x0010     /* an animal */
#define ITF_VEHICLE          0x0020     /* a vehicle, drawn by two animals */
#define ITF_CANUSE           0x0040     /* can be used with use_item_fun callout */
#define ITF_POTION           0x0080     /* is a potion (for use_potion) */

    /* error codes for item_type::use */
#define ECUSTOM   -1
#define ENOITEM   -2
#define ENOSKILL  -3
#define EUNUSABLE -4

    typedef struct item_type {
        resource_type *rtype;
        /* --- constants --- */
        unsigned int flags;
        int weight;
        int capacity;
        int mask_allow;
        int mask_deny;
        struct construction *construction;
        char *_appearance[2];       /* wie es fuer andere aussieht */
        int score;
    } item_type;

    const item_type *finditemtype(const char *name, const struct locale *lang);

    typedef struct luxury_type {
        struct luxury_type *next;
        const item_type *itype;
        int price;
    } luxury_type;
    extern luxury_type *luxurytypes;

#define WMF_WALKING         0x0001
#define WMF_RIDING          0x0002
#define WMF_ANYONE          0x000F      /* convenience */

#define WMF_AGAINST_RIDING  0x0010
#define WMF_AGAINST_WALKING 0x0020
#define WMF_AGAINST_ANYONE  0x00F0      /* convenience */

#define WMF_OFFENSIVE       0x0100
#define WMF_DEFENSIVE       0x0200
#define WMF_ANYTIME         0x0F00      /* convenience */

#define WMF_DAMAGE          0x1000
#define WMF_SKILL           0x2000
#define WMF_MISSILE_TARGET  0x4000

    struct race_list;
    typedef struct weapon_mod {
        int value;
        int flags;
        int race_mask;
    } weapon_mod;

#define ATF_NONE   0x00
#define ATF_SHIELD 0x01
#define ATF_LAEN   0x02

    typedef struct armor_type {
        const item_type *itype;
        unsigned int flags;
        double penalty;
        double projectile; /* chance, dass ein projektil abprallt */
        variant magres;
        int prot;
    } armor_type;

#define WTF_NONE          0x00
#define WTF_MISSILE       0x01
#define WTF_MAGICAL       0x02
#define WTF_PIERCE        0x04
#define WTF_CUT           0x08
#define WTF_BLUNT         0x10
#define WTF_SIEGE         0x20
#define WTF_ARMORPIERCING 0x40  /* armor has only half value */
#define WTF_HORSEBONUS    0x80
#define WTF_USESHIELD     0x100

    typedef struct weapon_type {
        const item_type *itype;
        char *damage[2];
        unsigned int flags;
        skill_t skill;
        int offmod;
        int defmod;
        variant magres;
        unsigned char reload;                 /* time to reload this weapon */
        weapon_mod *modifiers;
        /* --- functions --- */
        bool(*attack) (const struct troop *, const struct weapon_type *,
            int *deaths);
    } weapon_type;

    resource_type *rt_find(const char *name);
    item_type *it_find(const char *name);

    void it_set_appearance(item_type *itype, const char *appearance);

    const item_type *resource2item(const resource_type * rtype);
    const resource_type *item2resource(const item_type * i);

    const weapon_type *resource2weapon(const resource_type * i);
    const luxury_type *resource2luxury(const resource_type * i);

    item **i_find(item ** pi, const item_type * it);
    item *const *i_findc(item * const *pi, const item_type * it);
    item *i_add(item ** pi, item * it);
    void i_merge(item ** pi, item ** si);
    item *i_remove(item ** pi, item * it);
    void i_free(item * i);
    void i_freeall(item ** i);
    item *i_new(const item_type * it, int number);

    void read_items(struct storage *store, struct item **it);
    void write_items(struct storage *store, struct item *it);

    /* convenience: */
    item *i_change(item ** items, const item_type * it, int delta);
    int i_get(const item * items, const item_type * it);

    /* creation */
    resource_type *rt_get_or_create(const char *name);
    item_type *it_get_or_create(resource_type *rtype);
    luxury_type *new_luxurytype(item_type * itype, int price);
    weapon_type *new_weapontype(item_type * itype, int wflags,
        variant magres, const char *damage[], int offmod, int defmod, unsigned char reload,
        skill_t sk);
    void free_wtype(struct weapon_type *wtype);
    armor_type *new_armortype(item_type * itype, double penalty,
        variant magres, int prot, unsigned int flags);
    void free_atype(struct armor_type *wtype);
    /* these constants are used with get_resourcetype.
     * The order of the enum is not important for stored data.
     * The resourcenames array must be updated to match.
     */

    typedef enum {
        /* SPECIAL */
        R_SILVER,
        R_AURA,                     /* Aura */
        R_PERMAURA,                 /* Permanente Aura */
        R_LIFE,
        R_PEASANT,
        R_SAPLING,
        R_MALLORN_SAPLING,
        R_TREE,
        R_MALLORN_TREE,
        /* ITEMS: */
        R_SEED,
        R_MALLORN_SEED,
        R_IRON,
        R_STONE,
        R_HORSE,
        R_AMULET_OF_HEALING,
        R_AMULET_OF_TRUE_SEEING,
        R_RING_OF_INVISIBILITY,
        R_RING_OF_POWER,
        R_CHASTITY_BELT,
        R_EOG,
        R_FEENSTIEFEL,
        R_BIRTHDAYAMULET,
        R_PEGASUS,
        R_UNICORN,
        R_CHARGER,
        R_DOLPHIN,
        R_RING_OF_NIMBLEFINGER,
        R_TROLLBELT,
        R_AURAKULUM,
        R_SPHERE_OF_INVISIBILITY,
        R_BAG_OF_HOLDING,
        R_SACK_OF_CONSERVATION,
        R_TACTICCRYSTAL,
        R_WATER_OF_LIFE,
        /* SONSTIGE */

        MAX_RESOURCES,              /* do not use outside item.c ! */
        NORESOURCE = -1
    } resource_t;

    extern const struct item_type *oldpotiontype[];
    extern struct attrib_type at_showitem;        /* show this potion's description */

    void show_item(struct unit * u, const struct item_type * itype);

    const struct resource_type *get_resourcetype(resource_t rt);
    struct item *item_spoil(const struct race *rc, int size);

    int get_item(const struct unit * u, const struct item_type *itype);
    int get_money(const struct unit *);
    int set_money(struct unit *, int);
    int change_money(struct unit *, int);

    void register_resources(void);
    void init_resources(void);

    void register_item_give(int(*foo) (struct unit *, struct unit *,
        const struct item_type *, int, struct order *), const char *name);
    void register_item_use(int(*foo) (struct unit *,
        const struct item_type *, int, struct order *), const char *name);

    void free_resources(void);

#ifdef __cplusplus
}
#endif
#endif                          /* _ITEM_H */
