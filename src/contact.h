#pragma once

#include <stdbool.h>

struct faction;
struct order;
struct unit;

bool ucontact(const struct unit *u, const struct unit *u2);
void contact_unit(struct unit *u, const struct unit *c);
void contact_faction(struct unit * u, const struct faction * f);
int contact_cmd(struct unit * u, struct order * ord);
