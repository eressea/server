#pragma once

struct castorder;
struct unit;
enum direction_t;

void create_firewall(struct unit *mage, struct region *r, enum direction_t d, double force, int duration);
int sp_firewall(struct castorder *co);

extern const struct curse_type ct_firewall;
