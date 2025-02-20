#pragma once

struct castorder;
struct unit;
enum direction_t;

int sp_firewall(struct castorder *co);
void create_firewall(struct unit *mage, struct region *r, enum direction_t d, double force, int duration);

extern const struct curse_type ct_firewall;
