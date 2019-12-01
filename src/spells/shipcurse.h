#ifndef _SCURSE_H
#define _SCURSE_H

#include <kernel/objtypes.h>

#ifdef __cplusplus
extern "C" {
#endif

    struct message;
    struct curse;
    struct curse_type;

    extern const struct curse_type ct_shipspeedup;
    extern const struct curse_type ct_stormwind;
    extern const struct curse_type ct_nodrift;

    struct message *cinfo_ship(const void *obj, objtype_t typ,
        const struct curse *c, int self);
    void register_shipcurse(void);

#ifdef __cplusplus
}
#endif
#endif                          /* _SCURSE_H */
