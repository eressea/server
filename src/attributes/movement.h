#pragma once
#ifndef H_ATTRIBUTE_MOVEMENT
#define H_ATTRIBUTE_MOVEMENT

extern struct attrib_type at_movement;
extern struct attrib_type at_speedup;

struct attrib;

bool get_movement(struct attrib *const *alist, int type);
void set_movement(struct attrib **alist, int type);

#endif
