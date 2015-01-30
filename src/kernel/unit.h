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

#ifndef H_KRNL_UNIT_H
#define H_KRNL_UNIT_H

#include <util/variant.h>
#include "types.h"
#include "skills.h"
#ifdef __cplusplus
extern "C" {
#endif

    struct skill;
    struct item;
    struct sc_mage;

#define UFL_DEAD          (1<<0)
#define UFL_ISNEW         (1<<1)        /* 2 */
#define UFL_LONGACTION    (1<<2)        /* 4 */
#define UFL_OWNER         (1<<3)        /* 8 */
#define UFL_ANON_FACTION  (1<<4)        /* 16 */
#define UFL_DISBELIEVES   (1<<5)        /* 32 */
#define UFL_WARMTH        (1<<6)        /* 64 */
#define UFL_HERO          (1<<7)
#define UFL_MOVED         (1<<8)
#define UFL_NOTMOVING     (1<<9)        /* Die Einheit kann sich wg. langen Kampfes nicht bewegen */
#define UFL_DEFENDER      (1<<10)
#define UFL_HUNGER        (1<<11)       /* kann im Folgemonat keinen langen Befehl außer ARBEITE ausführen */
#define UFL_SIEGE         (1<<12)       /* speedup: belagert eine burg, siehe attribut */
#define UFL_TARGET        (1<<13)       /* speedup: hat ein target, siehe attribut */
#define UFL_WERE          (1<<14)
#define UFL_ENTER         (1<<15)       /* unit has entered a ship/building and will not leave it */

    /* warning: von 512/1024 gewechslet, wegen konflikt mit NEW_FOLLOW */
#define UFL_LOCKED        (1<<16)       /* Einheit kann keine Personen aufnehmen oder weggeben, nicht rekrutieren. */
#define UFL_FLEEING       (1<<17)       /* unit was in a battle, fleeing. */
#define UFL_STORM         (1<<19)       /* Kapitän war in einem Sturm */
#define UFL_FOLLOWING     (1<<20)
#define UFL_FOLLOWED      (1<<21)

#define UFL_NOAID         (1<<22)       /* Einheit hat Noaid-Status */
#define UFL_MARK          (1<<23)       /* same as FL_MARK */
#define UFL_ORDERS        (1<<24)       /* Einheit hat Befehle erhalten */
#define UFL_TAKEALL       (1<<25)       /* Einheit nimmt alle Gegenstände an */

    /* flags that speed up attribute access: */
#define UFL_STEALTH       (1<<26)
#define UFL_GUARD         (1<<27)
#define UFL_GROUP         (1<<28)

    /* Flags, die gespeichert werden sollen: */
#define UFL_SAVEMASK (UFL_DEFENDER|UFL_MOVED|UFL_NOAID|UFL_ANON_FACTION|UFL_LOCKED|UFL_HUNGER|UFL_TAKEALL|UFL_GUARD|UFL_STEALTH|UFL_GROUP|UFL_HERO)

#define UNIT_MAXSIZE 50000
    extern int maxheroes(const struct faction *f);
    extern int countheroes(const struct faction *f);

    typedef struct reservation {
        struct reservation *next;
        const struct item_type *type;
        int value;
    } reservation;

    typedef struct unit {
        struct unit *next;          /* needs to be first entry, for region's unitlist */
        struct unit *nextF;         /* nächste Einheit der Partei */
        struct unit *prevF;         /* vorherige Einheit der Partei */
        struct region *region;
        int no;
        int hp;
        char *name;
        char *display;
        struct faction *faction;
        struct building *building;
        struct ship *ship;
        unsigned short number;
        short age;

        /* skill data */
        short skill_size;
        struct skill *skills;
        struct item *items;
        reservation *reservations;

        /* orders */
        struct order *orders;
        struct order *thisorder;
        struct order *old_orders;

        /* race and illusionary race */
        const struct race *_race;
        const struct race *irace;

        int flags;
        struct attrib *attribs;
        status_t status;
        int n;                      /* enno: attribut? */
        int wants;                  /* enno: attribut? */
    } unit;

    extern struct attrib_type at_creator;
    extern struct attrib_type at_alias;
    extern struct attrib_type at_siege;
    extern struct attrib_type at_target;
    extern struct attrib_type at_potionuser;
    extern struct attrib_type at_contact;
    extern struct attrib_type at_effect;
    extern struct attrib_type at_private;
    extern struct attrib_type at_showskchange;

    int ualias(const struct unit *u);
    int weight(const struct unit *u);

    const struct race *u_irace(const struct unit *u);
    const struct race *u_race(const struct unit *u);
    void u_setrace(struct unit *u, const struct race *);
    struct building *usiege(const struct unit *u);
    void usetsiege(struct unit *u, const struct building *b);

    struct unit *utarget(const struct unit *u);
    void usettarget(struct unit *u, const struct unit *b);

    struct unit *utarget(const struct unit *u);
    void usettarget(struct unit *u, const struct unit *b);

    extern const struct race *urace(const struct unit *u);

    const char *uprivate(const struct unit *u);
    void usetprivate(struct unit *u, const char *c);

    const struct potion_type *ugetpotionuse(const struct unit *u);        /* benutzt u einein trank? */
    void usetpotionuse(struct unit *u, const struct potion_type *p);      /* u benutzt trank p (es darf halt nur einer pro runde) */

    bool ucontact(const struct unit *u, const struct unit *u2);
    void usetcontact(struct unit *u, const struct unit *c);

    struct unit *findnewunit(const struct region *r, const struct faction *f,
        int alias);

    const char *u_description(const unit * u, const struct locale *lang);
    struct skill *add_skill(struct unit *u, skill_t id);
    void remove_skill(struct unit *u, skill_t sk);
    struct skill *unit_skill(const struct unit *u, skill_t id);
    bool has_skill(const unit * u, skill_t sk);
    int effskill(const struct unit *u, skill_t sk);
    int produceexp(struct unit *u, skill_t sk, int n);
    int SkillCap(skill_t sk);

    extern void set_level(struct unit *u, skill_t id, int level);
    extern int get_level(const struct unit *u, skill_t id);
    extern void transfermen(struct unit *u, struct unit *u2, int n);

    extern int eff_skill(const struct unit *u, skill_t sk,
        const struct region *r);
    extern int eff_skill_study(const struct unit *u, skill_t sk,
        const struct region *r);

    extern int get_modifier(const struct unit *u, skill_t sk, int lvl,
        const struct region *r, bool noitem);
    extern int remove_unit(struct unit **ulist, struct unit *u);

#define GIFT_SELF     1<<0
#define GIFT_FRIENDS  1<<1
#define GIFT_PEASANTS 1<<2
    int gift_items(struct unit *u, int flags);
    void make_zombie(unit * u);

    /* see resolve.h */
    extern int resolve_unit(variant data, void *address);
    extern void write_unit_reference(const struct unit *u, struct storage *store);
    extern variant read_unit_reference(struct storage *store);

    extern bool leave(struct unit *u, bool force);
    extern bool can_leave(struct unit *u);

    extern void u_set_building(struct unit * u, struct building * b);
    extern void u_set_ship(struct unit * u, struct ship * sh);
    extern void leave_ship(struct unit * u);
    extern void leave_building(struct unit * u);

    extern void set_leftship(struct unit *u, struct ship *sh);
    extern struct ship *leftship(const struct unit *);
    extern bool can_survive(const struct unit *u, const struct region *r);
    extern void move_unit(struct unit *u, struct region *target,
    struct unit **ulist);

    extern struct building *inside_building(const struct unit *u);

    /* cleanup code for this module */
    extern void free_units(void);
    extern struct faction *dfindhash(int no);
    extern void u_setfaction(struct unit *u, struct faction *f);
    extern void set_number(struct unit *u, int count);

    extern bool learn_skill(struct unit *u, skill_t sk, double chance);

    extern int invisible(const struct unit *target, const struct unit *viewer);
    extern void free_unit(struct unit *u);

    extern void name_unit(struct unit *u);
    extern struct unit *create_unit(struct region *r1, struct faction *f,
        int number, const struct race *rc, int id, const char *dname,
    struct unit *creator);

    void uhash(struct unit *u);
    void uunhash(struct unit *u);
    struct unit *ufindhash(int i);

    const char *unit_getname(const struct unit *u);
    void unit_setname(struct unit *u, const char *name);
    const char *unit_getinfo(const struct unit *u);
    void unit_setinfo(struct unit *u, const char *name);
    int unit_getid(const unit * u);
    void unit_setid(unit * u, int id);
    int unit_gethp(const unit * u);
    void unit_sethp(unit * u, int id);
    status_t unit_getstatus(const unit * u);
    void unit_setstatus(unit * u, status_t status);
    int unit_getweight(const unit * u);
    int unit_getcapacity(const unit * u);
    void unit_addorder(unit * u, struct order *ord);
    int unit_max_hp(const struct unit *u);
    void scale_number(struct unit *u, int n);

    struct spellbook * unit_get_spellbook(const struct unit * u);
    void unit_add_spell(struct unit * u, struct sc_mage * m, struct spell * sp, int level);
    void remove_empty_units_in_region(struct region * r);
    void remove_empty_units(void);

    struct unit *findunitg(int n, const struct region *hint);
    struct unit *findunit(int n);

    struct unit *findunitr(const struct region *r, int n);

    const char *unitname(const struct unit *u);
    char *write_unitname(const struct unit *u, char *buffer, size_t size);
    bool unit_name_equals_race(const struct unit *u);
    bool unit_can_study(const struct unit *u);

#ifdef __cplusplus
}
#endif
#endif
