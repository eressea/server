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

    struct message;
    struct curse;

    extern const struct curse_type ct_shipspeedup;

    struct message *cinfo_ship(const void *obj, objtype_t typ,
        const struct curse *c, int self);
    void register_shipcurse(void);

#ifdef __cplusplus
}
#endif
#endif                          /* _SCURSE_H */
