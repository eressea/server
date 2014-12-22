/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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
    struct region;
    struct resource_type;
    struct locale;
    struct troop;
    struct item;
    struct order;

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

  typedef int (*rtype_uchange) (struct unit * user,
    const struct resource_type * rtype, int delta);
  typedef int (*rtype_uget) (const struct unit * user,
    const struct resource_type * rtype);
  typedef char *(*rtype_name) (const struct resource_type * rtype, int flags);
  typedef struct resource_type {
    /* --- constants --- */
    char *_name;             /* wie es hei�t */
    unsigned int flags;
    /* --- functions --- */
    rtype_uchange uchange;
    rtype_uget uget;
    rtype_name name;
    /* --- pointers --- */
    struct attrib *attribs;
    struct item_type *itype;
    struct potion_type *ptype;
    struct luxury_type *ltype;
    struct weapon_type *wtype;
    struct armor_type *atype;
  } resource_type;

  extern const char *resourcename(const resource_type * rtype, int flags);
  extern const resource_type *findresourcetype(const char *name,
    const struct locale *lang);

/* resource-limits for regions */
#define RMF_SKILL         0x01  /* int, bonus on resource production skill */
#define RMF_SAVEMATERIAL  0x02  /* float, multiplier on resource usage */
#define RMF_SAVERESOURCE  0x03  /* int, bonus on resource production skill */
#define RMF_REQUIREDBUILDING 0x04       /* building, required to build */

  typedef struct resource_mod {
    variant value;
    const struct building_type *btype;
    const struct race *race;
    unsigned int flags;
  } resource_mod;

  extern struct attrib_type at_resourcelimit;
  typedef int (*rlimit_limit) (const struct region * r,
    const struct resource_type * rtype);
  typedef void (*rlimit_produce) (struct region * r,
    const struct resource_type * rtype, int n);
  typedef struct resource_limit {
    rlimit_limit limit;
    rlimit_produce produce;
    unsigned int guard;         /* how to guard against theft */
    int value;
    resource_mod *modifiers;
  } resource_limit;

/* bitfield values for item_type::flags */
#define ITF_NONE             0x0000
#define ITF_HERB             0x0001     /* this item is a herb */
#define ITF_CURSED           0x0002     /* cursed object, cannot be given away */
#define ITF_NOTLOST          0x0004     /* special object (quests), cannot be lost through death etc. */
#define ITF_BIG              0x0008     /* big item, e.g. does not fit in a bag of holding */
#define ITF_ANIMAL           0x0010     /* an animal */
#define ITF_VEHICLE          0x0020     /* a vehicle, drawn by two animals */

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
    struct construction *construction;
    char *_appearance[2];       /* wie es f�r andere aussieht */
    /* --- functions --- */
     bool(*canuse) (const struct unit * user,
      const struct item_type * itype);
    int (*use) (struct unit * user, const struct item_type * itype, int amount,
      struct order * ord);
    int (*useonother) (struct unit * user, int targetno,
      const struct item_type * itype, int amount, struct order * ord);
    int (*give) (struct unit * src, struct unit * dest,
      const struct item_type * itm, int number, struct order * ord);
#if SCORE_MODULE
    int score;
#endif
  } item_type;

  extern const item_type *finditemtype(const char *name, const struct locale *lang);

  typedef struct luxury_type {
    struct luxury_type *next;
    const item_type *itype;
    int price;
  } luxury_type;
  extern luxury_type *luxurytypes;

  typedef struct potion_type {
    struct potion_type *next;
    const item_type *itype;
    int level;
  } potion_type;
  extern potion_type *potiontypes;

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
    unsigned int flags;
    struct race_list *races;
  } weapon_mod;

#define ATF_NONE   0x00
#define ATF_SHIELD 0x01
#define ATF_LAEN   0x02

  typedef struct armor_type {
    const item_type *itype;
    double penalty;
    double magres;
    int prot;
    float projectile;           /* chance, dass ein projektil abprallt */
    unsigned int flags;
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
    const char *damage[2];
    unsigned int flags;
    skill_t skill;
    int minskill;
    int offmod;
    int defmod;
    double magres;
    int reload;                 /* time to reload this weapon */
    weapon_mod *modifiers;
    /* --- functions --- */
     bool(*attack) (const struct troop *, const struct weapon_type *,
      int *deaths);
  } weapon_type;

  resource_type *rt_find(const char *name);
  item_type *it_find(const char *name);

  void it_set_appearance(item_type *itype, const char *appearance);

  extern const item_type *resource2item(const resource_type * rtype);
  extern const resource_type *item2resource(const item_type * i);

  extern const weapon_type *resource2weapon(const resource_type * i);
  extern const potion_type *resource2potion(const resource_type * i);
  extern const luxury_type *resource2luxury(const resource_type * i);

  extern item **i_find(item ** pi, const item_type * it);
  extern item *const *i_findc(item * const *pi, const item_type * it);
  extern item *i_add(item ** pi, item * it);
  extern void i_merge(item ** pi, item ** si);
  extern item *i_remove(item ** pi, item * it);
  extern void i_free(item * i);
  extern void i_freeall(item ** i);
  extern item *i_new(const item_type * it, int number);

/* convenience: */
  extern item *i_change(item ** items, const item_type * it, int delta);
  extern int i_get(const item * i, const item_type * it);

/* creation */
    resource_type *rt_get_or_create(const char *name);
    item_type *it_get_or_create(resource_type *rtype);
  extern item_type *new_itemtype(resource_type * rtype, int iflags, int weight,
    int capacity);
  extern luxury_type *new_luxurytype(item_type * itype, int price);
  extern weapon_type *new_weapontype(item_type * itype, int wflags,
    double magres, const char *damage[], int offmod, int defmod, int reload,
    skill_t sk, int minskill);
  extern armor_type *new_armortype(item_type * itype, double penalty,
    double magres, int prot, unsigned int flags);
  extern potion_type *new_potiontype(item_type * itype, int level);

  typedef enum {
    /* ITEMS: */
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
    R_SEED,
    R_MALLORNSEED,
    /* SONSTIGE */
    R_SILVER,
    R_AURA,                     /* Aura */
    R_PERMAURA,                 /* Permanente Aura */
    R_LIFE,
    R_PEASANT,
    R_PERSON,

    MAX_RESOURCES,              /* do not use outside item.c ! */
    NORESOURCE = -1
  } resource_t;

  extern const struct potion_type *oldpotiontype[];
  const struct resource_type *get_resourcetype(resource_t rt);

  int get_money(const struct unit *);
  int set_money(struct unit *, int);
  int change_money(struct unit *, int);

  extern struct attrib_type at_showitem;        /* show this potion's description */

  extern void register_resources(void);
  extern void init_resources(void);
  extern void init_itemtypes(void);

  extern void register_item_give(int (*foo) (struct unit *, struct unit *,
      const struct item_type *, int, struct order *), const char *name);
  extern void register_item_use(int (*foo) (struct unit *,
      const struct item_type *, int, struct order *), const char *name);
  extern void register_item_useonother(int (*foo) (struct unit *, int,
      const struct item_type *, int, struct order *), const char *name);

#ifndef DISABLE_TESTS
  void test_clear_resources(void);
#endif

#ifdef __cplusplus
}
#endif
#endif                          /* _ITEM_H */
