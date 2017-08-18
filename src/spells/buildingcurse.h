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

#ifndef _BCURSE_H
#define _BCURSE_H
#include <kernel/objtypes.h>
#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct curse;
    struct message;
    struct curse_type;

    extern const struct curse_type ct_magicwalls;

    extern void register_buildingcurse(void);
    struct message *cinfo_building(const void *obj, objtype_t typ, const struct curse * c, int self);

#ifdef __cplusplus
}
#endif
#endif                          /* _BCURSE_H */
