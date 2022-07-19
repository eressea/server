#pragma once

struct faction;
struct unit;

void defaultorders(void);
void update_long_order(struct unit *u);
void update_defaults(struct faction* f);
