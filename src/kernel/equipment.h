#pragma once

#include <stdbool.h>

struct unit;

#define EQUIP_SKILLS  (1<<1)
#define EQUIP_SPELLS  (1<<2)
#define EQUIP_ITEMS   (1<<3)
#define EQUIP_SPECIAL (1<<4)
#define EQUIP_ALL     (0xFF)
bool equip_unit_mask(struct unit *u, const char *eqname, int mask);
bool equip_unit(struct unit *u, const char *eqname);
