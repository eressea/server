#pragma once
#ifndef H_ATTRIBUTE_ATTRIBUTES
#define H_ATTRIBUTE_ATTRIBUTES

struct attrib_type;
struct region;
struct faction;

void register_attributes(void);

void set_observer(struct region *r, struct faction *f, int perception, int turns);
int get_observer(const struct region *r, const struct faction *f);

extern struct attrib_type at_unitdissolve;

#endif
