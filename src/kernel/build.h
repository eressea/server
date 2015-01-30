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

#ifndef H_KRNL_BUILD
#define H_KRNL_BUILD

#include "types.h"
#include "direction.h"
#include "skill.h"

#ifdef __cplusplus
extern "C" {
#endif

    /* Die enums fuer Gebauede werden nie gebraucht, nur bei der Bestimmung
     * des Schutzes durch eine Burg wird die Reihenfolge und MAXBUILDINGS
     * wichtig
     */

    struct xml_tag;

    typedef struct requirement {
        const struct resource_type *rtype;
        int number;
    } requirement;

    typedef struct construction {
        skill_t skill;              /* skill req'd per point of size */
        int minskill;               /* skill req'd per point of size */

        int maxsize;                /* maximum size of this type */
        int reqsize;                /* size of object using up 1 set of requirement. */
        int defense_bonus;          /* protection bonus (defense) during combat */
        int close_combat_bonus;     /* close combat attack bonus*/
        int ranged_bonus;           /* ranged attack bonus */
        requirement *materials;     /* material req'd to build one object */
        const struct building_type *btype;
        /* building type required to make this thing */

        struct construction *improvement;
        /* next level, if upgradable. if more than one of these items
         * can be built (weapons, armour) per turn, must not be NULL,
         * but point to the same type again:
         *   const_sword.improvement = &const_sword
         * last level of a building points to NULL, as do objects of
         * an unlimited size.
         */
        struct attrib *attribs;
        /* stores skill modifiers and other attributes */

    } construction;

    extern int destroy_cmd(struct unit *u, struct order *ord);
    extern int leave_cmd(struct unit *u, struct order *ord);

    void build_road(struct region *r, struct unit *u, int size, direction_t d);
    void create_ship(struct region *r, struct unit *u,
        const struct ship_type *newtype, int size, struct order *ord);
    void continue_ship(struct region *r, struct unit *u, int size);

    struct building *getbuilding(const struct region *r);
    struct ship *getship(const struct region *r);

    void reportevent(struct region *r, char *s);

    void shash(struct ship *sh);
    void sunhash(struct ship *sh);
    extern int roqf_factor(void);

    int build(struct unit *u, const construction * ctype, int completed, int want);
    extern int maxbuild(const struct unit *u, const construction * cons);
    extern struct message *msg_materials_required(struct unit *u,
    struct order *ord, const struct construction *ctype, int multi);
    /** error messages that build may return: */
#define ELOWSKILL -1
#define ENEEDSKILL -2
#define ECOMPLETE -3
#define ENOMATERIALS -4
#define EBUILDINGREQ -5

#ifdef __cplusplus
}
#endif
#endif
