#pragma once

struct castorder;
struct unit;

int sp_charmingsong(struct castorder *co);
void charm_unit(struct unit *target, struct unit *mage, double force, int duration);

extern const struct curse_type ct_slavery;

void fix_slaves(void);
