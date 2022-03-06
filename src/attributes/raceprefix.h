#pragma once
#ifndef H_ATTRIBUTE_RACEPREFIX
#define H_ATTRIBUTE_RACEPREFIX

struct attrib;

extern struct attrib_type at_raceprefix;

void set_prefix(struct attrib **ap, const char *str);
const char *get_prefix(struct attrib *a);

#endif
