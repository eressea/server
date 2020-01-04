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
    extern const struct curse_type ct_strongwall;
    extern const struct curse_type ct_magicrunes;
    extern const struct curse_type ct_nocostbuilding;

    extern void register_buildingcurse(void);
    struct message *cinfo_building(const void *obj, objtype_t typ, const struct curse * c, int self);

#ifdef __cplusplus
}
#endif
#endif                          /* _BCURSE_H */
