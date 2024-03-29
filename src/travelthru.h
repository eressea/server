#pragma once

#include <stdbool.h>

struct attrib;
struct stream;
struct region;
struct faction;
struct unit;

void travelthru_map(struct region * r, void(*cb)(struct region *r, const struct unit *, void *), void *cbdata);
bool travelthru_cansee(const struct region *r, const struct faction *f, const struct unit *u);
void travelthru_add(struct region * r, struct unit * u);
