#pragma once
#ifndef H_ATTRIBUTE_OTHERFACTION
#define H_ATTRIBUTE_OTHERFACTION

struct unit;
struct faction;

extern struct attrib_type at_otherfaction;

struct faction *get_otherfaction(const struct unit *u);
void set_otherfaction(struct unit* u, struct faction* f);
struct faction *visible_faction(const struct faction *f,
    const struct unit *u, struct faction* other_faction);

#endif
