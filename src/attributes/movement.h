#pragma once

#include <stdbool.h>

struct attrib;

extern struct attrib_type at_speedup;

int get_speedup(struct attrib* attribs);
bool set_speedup(struct attrib** attribs, int speed, int duration);
