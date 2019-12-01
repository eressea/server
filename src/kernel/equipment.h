#ifndef H_KRNL_EQUIPMENT_H
#define H_KRNL_EQUIPMENT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
    struct unit;

#define EQUIP_SKILLS  (1<<1)
#define EQUIP_SPELLS  (1<<2)
#define EQUIP_ITEMS   (1<<3)
#define EQUIP_SPECIAL (1<<4)
#define EQUIP_ALL     (0xFF)
    bool equip_unit_mask(struct unit *u, const char *eqname, int mask);
    bool equip_unit(struct unit *u, const char *eqname);

#ifdef __cplusplus
}
#endif
#endif
