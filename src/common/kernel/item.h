/* vi: set ts=2:
 *
 *	$Id: item.h,v 1.5 2001/02/14 07:44:57 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef _ITEM_H
#define _ITEM_H

struct unit;
struct attrib;
struct attrib_type;
struct region;
struct resource_type;
struct locale;
struct troop;

typedef struct item {
	const struct item_type * type;
	int number;
	struct item * next;
} item;

/* bitfield values for resource_type::flags */
#define RTF_NONE     0
#define RTF_ITEM     (1<<0) /* this resource is an item */
#define RTF_SNEAK    (1<<1) /* can be sneaked to another struct unit, e.g. P_FOOL */
#define RTF_LIMITED  (1<<2) /* a resource that's freely available, but in
                             * limited supply */
#define RTF_DYNAMIC  (1<<3) /* dynamic type, must be saved */
#define RTF_POOLED   (1<<4) /* resource is available in pool */

#undef AT_ITYPE /* should resource_type use an attribute for items ? */
#undef AT_HTYPE /* should resource_type use an attribute for herbs ? */
#undef AT_PTYPE /* should resource_type use an attribute for potions ? */
#undef AT_LTYPE /* should resource_type use an attribute for luxuries ? */
#undef AT_WTYPE /* should resource_type use an attribute for weapons ? */

/* flags for resource_type::name() */
#define NMF_PLURAL     0x01
#define NMF_APPEARANCE 0x02

typedef struct resource_type {
	/* --- constants --- */
	const char * _name[2]; /* wie es heißt */
	const char * _appearance[2]; /* wie es für andere aussieht */
	unsigned int flags;
	/* --- functions --- */
	int    (*uchange)(struct unit * user, const struct resource_type * rtype, int delta);
	int    (*uget)(const struct unit * user, const struct resource_type * rtype);
	char * (*name)(const struct resource_type * rtype, int flags);
	/* --- pointers --- */
	struct attrib * attribs;
	struct resource_type * next;
	unsigned int hashkey;
#ifndef AT_ITYPE
	struct item_type * itype;
#endif
#ifndef AT_HTYPE
	struct herb_type * htype;
#endif
#ifndef AT_PTYPE
	struct potion_type * ptype;
#endif
#ifndef AT_LTYPE
	struct luxury_type * ltype;
#endif
#ifndef AT_WTYPE
	struct weapon_type * wtype;
#endif
} resource_type;
extern resource_type * resourcetypes;
extern const char* resourcename(const resource_type * rtype, int flags);
extern const resource_type * findresourcetype(const char * name, const struct locale * lang);

/* resource-limits for regions */
extern struct attrib_type at_resourcelimit;
typedef struct resource_limit {
	int (*limit)(const struct region * r, const struct resource_type * rtype);
	void (*use)(struct region * r, const struct resource_type * rtype, int n);
	int value;
} resource_limit;


/* bitfield values for item_type::flags */
#define ITF_NONE             0x0000
#define ITF_HERB             0x0001 /* this item is a herb */
#define ITF_WEAPON           0x0002 /* this item is a weapon */
#define ITF_LUXURY           0x0004 /* this item is a luxury item */
#define ITF_POTION           0x0008 /* this item is a potion */
#define ITF_CURSED           0x0010 /* cursed object, cannot be given away */
#define ITF_NOTLOST          0x0020 /* special object (quests), cannot be lost through death etc. */
#define ITF_BIG              0x0040 /* big item, e.g. does not fit in a bag of holding */
#define ITF_ANIMAL           0x0080 /* an animal */
#define ITF_NOBUILDBESIEGED  0x0100 /* cannot be built under siege */
#define ITF_DYNAMIC          0x0200 /* dynamic type, must be saved */

/* error codes for item_type::use */
#define ECUSTOM   -1;
#define ENOITEM   -2;
#define ENOSKILL  -3;
#define EUNUSABLE -4;

typedef struct itemtype_list {
	struct itemtype_list * next;
	const struct item_type * type;
} itemtype_list;

typedef struct item_type {
	resource_type * rtype;
	/* --- constants --- */
	unsigned int flags;
	int weight;
	int capacity;
	int minskill;
	skill_t skill;
	const struct construction * construction;
	/* --- functions --- */
	int (*use)(struct unit * user, const struct item_type * itype, const char * cmd);
	int (*give)(const struct unit * src, const struct unit * dest, const struct item_type * itm, int number, const char * cmd);
	/* --- pointers --- */
	struct item_type * next;
} item_type;

extern item_type * itemtypes;
extern const item_type * finditemtype(const char * name, const struct locale * lang);

typedef struct luxury_type {
	const item_type * itype;
	int price;
	/* --- pointers --- */
	struct luxury_type * next;
} luxury_type;
extern luxury_type * luxurytypes;

typedef struct herb_type {
	const item_type * itype;
	terrain_t terrain;
	/* --- pointers --- */
	struct herb_type * next;
} herb_type;
extern herb_type * herbtypes;

typedef struct potion_type {
	const item_type * itype;
	int level;
	const char * text;
	int (*use)(struct unit *, const struct potion_type *, const char *);
	/* --- pointers --- */
	struct potion_type * next;
} potion_type;
extern potion_type * potiontypes;

#define WMF_WALKING         0x0001
#define WMF_RIDING          0x0002
#define WMF_ANYONE          0x000F /* convenience */

#define WMF_AGAINST_RIDING  0x0010
#define WMF_AGAINST_WALKING 0x0020
#define WMF_AGAINST_ANYONE  0x00F0 /* convenience */

#define WMF_OFFENSIVE       0x0100
#define WMF_DEFENSIVE       0x0200
#define WMF_ANYTIME         0x0F00 /* convenience */

#define WMF_DAMAGE          0x1000
#define WMF_SKILL           0x2000
#define WMF_MISSILE_TARGET  0x4000

typedef struct weapon_mod {
	int value;
	unsigned int flags;
} weapon_mod;

#define WTF_NONE         0x00
#define WTF_MISSILE      0x01
#define WTF_MAGICAL      0x02
#define WTF_PIERCE       0x04
#define WTF_CUT          0x08
#define WTF_BLUNT        0x10
#define WTF_BOW          0x20 /* elves like 'em */

typedef struct weapon_type {
	const item_type * itype;
	const char * damage[2];
	unsigned int flags;
	skill_t skill;
	int minskill;
	int offmod;
	int defmod;
	double magres;
	int reload; /* time to reload this weapon */
	weapon_mod * modifiers;
	/* --- functions --- */
	boolean (*attack)(const struct troop *, int *deaths);
	/* --- pointers --- */
	struct weapon_type * next;
} weapon_type;
extern weapon_type * weapontypes;

extern void rt_register(resource_type * it);
extern resource_type * rt_find(const char * name);
extern item_type * it_find(const char * name);
extern herb_type * ht_find(const char * name);
extern luxury_type * lt_find(const char * name);
extern potion_type * pt_find(const char * name);
extern weapon_type * wt_find(const char * name);

extern void it_register(item_type * it);
extern void wt_register(weapon_type * wt);

extern struct attrib_type at_weapon;
extern struct attrib_type at_item;
extern struct attrib_type at_herb;
extern struct attrib_type at_luxury;
extern struct attrib_type at_potion;

extern const item_type * resource2item(const resource_type * rtype);
extern const resource_type * item2resource(const item_type * i);

extern const weapon_type * resource2weapon(const resource_type * i);
extern const potion_type * resource2potion(const resource_type * i);
extern const luxury_type * resource2luxury(const resource_type * i);
extern const herb_type * resource2herb(const resource_type * i);

#define weapon2resource(x) ((x)->itype->rtype)
#define luxury2resource(x) ((x)->itype->rtype)
#define herb2resource(x) ((x)->itype->rtype)
#define potion2resource(x) ((x)->itype->rtype)

extern item ** i_find(item ** pi, const item_type * it);
extern item * i_add(item ** pi, item * it);
extern void i_merge(item ** pi, item ** si);
extern item * i_remove(item ** pi, item * it);
extern void i_free(item * i);
extern void i_freeall(item * i);
extern item * i_new(const item_type * it);

/* convenience: */
extern item * i_change(item ** items, const item_type * it, int delta);
extern int i_get(const item * i, const item_type * it);

/* creation */
extern resource_type * new_resourcetype(const char ** names, const char ** appearances, int flags);
extern item_type * new_itemtype(resource_type * rtype, int iflags, int weight, int capacity, int minskill, skill_t skill);
extern luxury_type * new_luxurytype(item_type * itype, int price);
extern weapon_type * new_weapontype(item_type * itype, int wflags, double magres, const char* damage[], int offmod, int defmod, int reload, skill_t skill, int minskill);
extern potion_type * new_potiontype(item_type * itype, int level);
extern herb_type * new_herbtype(item_type * itype, terrain_t terrain);

/* for lack of another file: */

/* sonstige resourcen */
extern resource_type * r_silver;
extern resource_type * r_aura;
extern resource_type * r_permaura;
extern resource_type * r_peasants;
extern resource_type * r_unit;
extern resource_type * r_hp;
extern resource_type * r_person;

typedef struct t_item {
	const char *name[4];
	/* [0]: Einzahl für eigene; [1]: Mehrzahl für eigene;
	 * [2]: Einzahl für Fremde; [3]: Mehrzahl für Fremde */
	int gr_geschlecht;
	item_t typ;
	skill_t skill;
	int minskill;
	int material[6];
	int gewicht;
	int preis;
	unsigned int flags;
	void (*benutze_funktion) (struct region *, struct unit *, strlist *);
} t_item;

enum {
	G_M,
	G_F,
	G_N
};

enum {
	I_IRON,						/* 0 */
	I_WOOD,
	I_STONE,
	I_HORSE,
	I_WAGON,
	I_CATAPULT,
	I_SWORD,
	I_SPEAR,
	I_CROSSBOW,
	I_LONGBOW,
	I_CHAIN_MAIL,			/* 10 */
	I_PLATE_ARMOR,
	I_BALM,
	I_SPICES,
	I_JEWELERY,
	I_MYRRH,
	I_OIL,
	I_SILK,
	I_INCENSE,
	/* alte Artefakte */
#ifdef COMPATIBILITY
	I_AMULET_OF_DARKNESS, /* weg */
	I_AMULET_OF_DEATH,  /* weg */
#endif
	I_AMULET_OF_HEALING,
	I_AMULET_OF_TRUE_SEEING,
#ifdef COMPATIBILITY
	I_CLOAK_OF_INVULNERABILITY, /* weg */
#endif
	I_RING_OF_INVISIBILITY,
	I_RING_OF_POWER,
	I_FIRESWORD, /* Runenschwert ersetzen durch Flammenschwert */
#ifdef COMPATIBILITY
	I_SHIELDSTONE, /* weg */
	I_STAFF_OF_FIRE, /* weg */
	I_STAFF_OF_LIGHTNING, /* weg */
	I_WAND_OF_TELEPORTATION, /* weg */
	/* Ende alte Artefakte */
	I_EYE_OF_HORAX,
	I_TELEPORTCRYSTAL,
#endif
	I_DRAGONHEAD,
	I_CHASTITY_BELT, /* bleibt */
	I_GREATSWORD,
	I_AXE,
	I_GREATBOW,
	I_EOGSWORD,
	I_EOGSHIELD,
	I_EOGCHAIN,
	I_EOG,
	I_SHIELD,
	I_HALBERD,
	I_LANCE,
	I_MALLORN,
	I_KEKS,
	I_APFEL,
	I_NUSS,
	I_MANDELKERN,
#ifdef COMPATIBILITY
	I_STAB_DES_SCHICKSALS,
	I_STAB_DER_VERDAMMNIS,
	I_STAB_DES_TODES,
	I_STAB_DES_CHAOS,
	I_STECKEN_DER_MACHT,
#endif
	I_AMULETT_DES_TREFFENS,
	I_DRACHENBLUT,
	I_FEENSTIEFEL,
	I_BIRTHDAYAMULET,
	I_PEGASUS,
	I_UNICORN,
	I_DOLPHIN,
	I_ANTIMAGICCRYSTAL,
	I_RING_OF_NIMBLEFINGER,
	I_TROLLBELT,
	I_PRESSCARD,
	I_RUNESWORD,
	I_AURAKULUM,
	I_SEASERPENTHEAD,
	I_TACTICCRYSTAL,
	I_RING_OF_REGENERATION,
	I_TOADSLIME,
	I_BAG_OF_HOLDING,
	I_RUSTY_SWORD,
	I_RUSTY_SHIELD,
	I_RUSTY_CHAIN_MAIL,
	I_SACK_OF_CONSERVATION,
	MAX_ITEMS /* do not use outside item.c ! */
};

enum {
	/* ITEMS: */
	R_IRON,
	R_WOOD,
	R_STONE,
	R_HORSE,
	R_WAGON,
	R_CATAPULT,
	R_SWORD,
	R_SPEAR,
	R_CROSSBOW,
	R_LONGBOW,
	R_CHAIN_MAIL,
	R_PLATE_ARMOR,
	R_BALM,
	R_SPICES,
	R_JEWELERY,
	R_MYRRH,
	R_OIL,
	R_SILK,
	R_INCENSE,
#ifdef COMPATIBILITY
	R_AMULET_OF_DARKNESS,
	R_AMULET_OF_DEATH,
#endif
	R_AMULET_OF_HEALING,
	R_AMULET_OF_TRUE_SEEING,
#ifdef COMPATIBILITY
	R_CLOAK_OF_INVULNERABILITY,
#endif
	R_RING_OF_INVISIBILITY,
	R_RING_OF_POWER,
	R_FIRESWORD,
#ifdef COMPATIBILITY
	R_SHIELDSTONE,
	R_STAFF_OF_FIRE,
	R_STAFF_OF_LIGHTNING,
	R_WAND_OF_TELEPORTATION,
	R_EYE_OF_HORAX,
	R_TELEPORTCRYSTAL,
#endif
	R_DRAGONHEAD,
	R_CHASTITY_BELT,
	R_GREATSWORD,
	R_AXE,
	R_GREATBOW,
	R_EOGSWORD,
	R_EOGSHIELD,
	R_EOGCHAIN,
	R_EOG,
	R_SHIELD,
	R_HALBEARD,
	R_LANCE,
	R_MALLORN,
	R_KEKS,
	R_APFEL,
	R_NUSS,
	R_MANDELKERN,
#ifdef COMPATIBILITY
	R_STAB_DES_SCHICKSALS,
	R_STAB_DER_VERDAMMNIS,
	R_STAB_DES_TODES,
	R_STAB_DES_CHAOS,
	R_STECKEN_DER_MACHT,
#endif
	R_AMULETT_DES_TREFFENS,
	R_DRACHENBLUT,
	R_FEENSTIEFEL,
	R_BIRTHDAYAMULET,
	R_PEGASUS,
	R_UNICORN,
	R_DOLPHIN,
	R_ANTIMAGICCRYSTAL,
	R_RING_OF_NIMBLEFINGER,
	R_TROLLBELT,
	R_PRESSCARD,
	R_RUNESWORD,
	R_AURAKULUM,
	R_SEASERPENTHEAD,
	R_TACTICCRYSTAL,
	R_RING_OF_REGENERATION,
	R_TOADSLIME,
	R_BAG_OF_HOLDING,
	R_RUSTY_SWORD,
	R_RUSTY_SHIELD,
	R_RUSTY_CHAIN_MAIL,
	R_SACK_OF_CONSERVATION,

	/* POTIONS: */
	R_FAST,
	R_STRONG,
	R_TREES,
	R_DOMORE,
	R_UGLY,
#ifdef INSECT_POTION
	R_WARMTH,
#else
	R_STEEL,
#endif
	R_WISE,
	R_FOOL,
	R_ROSE,
	R_HORSEPOTION,
	R_BERSERK,
	R_PEOPLE,
	R_GRASS,
	R_ILL,
	R_HEAL,

	/* HERBS */
	R_PLAIN_1, /* Flachwurz */
	R_PLAIN_2, /* Würziger Wagemut */
	R_PLAIN_3, /* Eulenauge */
	R_FOREST_1, /* Grüner Spinnerich */
	R_FOREST_2, /* Blauer Baumringel */
	R_FOREST_3, /* Elfenlieb */
	R_SWAMP_1,  /* Gurgelkraut */
	R_SWAMP_2,  /* Knotiger Saugwurz */
	R_SWAMP_3,  /* Blasenmorchel */
	R_DESERT_1, /* Wasserfinder */
	R_DESERT_2, /* Kakteenschwitz */
	R_DESERT_3, /* Sandfäule */
	R_HIGHLAND_1, /* Windbeutel */
	R_HIGHLAND_2, /* Fjordwuchs */
	R_HIGHLAND_3, /* Alraune */
	R_MOUNTAIN_1, /* Steinbeißer */
	R_MOUNTAIN_2, /* Spaltwachs */
	R_MOUNTAIN_3, /* Höhlenglimm */
	R_GLACIER_1,  /* Eisblume */
	R_GLACIER_2,  /* Weißer Wüterich */
	R_GLACIER_3,  /* Schneekristall */

	/* SONSTIGE */
	R_SILVER,
	R_AURA,      /* Aura */
	R_PERMAURA,  /* Permanente Aura */
	R_HITPOINTS, /* Trefferpunkte */
	R_PEASANTS,  /* Regionsbauern */


	R_UNIT, /* Einheit (gib x einheit) */
	R_PERSON,

	MAX_RESOURCES, /* do not use outside item.c ! */
	NORESOURCE = (resource_t) - 1
};

extern luxury_type * oldluxurytype[];
extern potion_type * oldpotiontype[];
extern item_type * olditemtype[];
extern resource_type * oldresourcetype[];
extern herb_type * oldherbtype[];

int get_item(const struct unit *, item_t);
int set_item(struct unit *, item_t, int);
int change_item(struct unit *, item_t, int);

int get_herb(const struct unit *, herb_t);
int set_herb(struct unit *, herb_t, int);
int change_herb(struct unit *, herb_t, int);

int get_potion(const struct unit *, potion_t);

int get_money(const struct unit *);
int set_money(struct unit *, int);
int change_money(struct unit *, int);
int res_changeitem(struct unit * u, const resource_type * rtype, int delta);

extern struct attrib_type at_showitem; /* show this potion's description */
extern struct attrib_type at_seenitem; /* knows this potion's description, no need to reshow */

extern item_type * it_read(FILE * F);
extern resource_type * rt_read(FILE * F);
extern void it_write(FILE * F, const item_type * it);
extern void rt_write(FILE * F, const resource_type * rt);
extern void init_resources(void);

extern const char* resname(resource_t res, int index);
#endif /* _ITEM_H */
