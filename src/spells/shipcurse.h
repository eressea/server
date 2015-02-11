/*
 * Eressea PB(E)M host Copyright (C) 1998-2015
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef _SCURSE_H
#define _SCURSE_H

#include <kernel/objtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct locale;
    struct message;
    struct ship;
    struct unit;
    struct curse;

    struct message *cinfo_ship(const void *obj, objtype_t typ,
        const struct curse *c, int self);
    void register_shipcurse(void);
    struct curse *shipcurse_flyingship(struct ship *sh, struct unit *mage,
        float power, int duration);
    int levitate_ship(struct ship *sh, struct unit *mage, float power,
        int duration);

#ifdef __cplusplus
}
#endif
#endif                          /* _SCURSE_H */
