/* 
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef _UCURSE_H
#define _UCURSE_H

#include <kernel/objtypes.h>
#ifdef __cplusplus
extern "C" {
#endif

    struct curse;
    struct curse_type;
    struct message;

    extern const struct curse_type ct_slavery;
    extern const struct curse_type ct_calmmonster;
    extern const struct curse_type ct_speed;
    extern const struct curse_type ct_worse;
    extern const struct curse_type ct_skillmod;
    extern const struct curse_type ct_oldrace;
    extern const struct curse_type ct_fumble;
    extern const struct curse_type ct_orcish;
    extern const struct curse_type ct_itemcloak;
    extern const struct curse_type ct_insectfur;
    extern const struct curse_type ct_sparkle;
    extern const struct curse_type ct_magicboost;
    extern const struct curse_type ct_auraboost;

    struct message *cinfo_unit(const void *obj, objtype_t typ,
        const struct curse *c, int self);

    extern void register_unitcurse(void);

#ifdef __cplusplus
}
#endif
#endif                          /* _UCURSE_H */
