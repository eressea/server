#pragma once

#include <stdbool.h>

struct unit;
struct faction;

bool ucontact(const struct unit *u, const struct unit *u2);
void usetcontact(struct unit *u, const struct unit *c);

