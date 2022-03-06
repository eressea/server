#pragma once
#ifndef STEALTH_H
#define STEALTH_H

extern struct attrib_type at_stealth;

struct unit;
struct region;

int eff_stealth(const struct unit *u, const struct region *r);
void u_seteffstealth(struct unit *u, int value);
int u_geteffstealth(const struct unit *u);

#endif
