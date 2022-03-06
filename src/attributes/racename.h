#pragma once
#ifndef H_ATTRIBUTE_RACENAME_H
#define H_ATTRIBUTE_RACENAME_H

struct attrib;

extern struct attrib_type at_racename;

void set_racename(struct attrib **palist, const char *name);
const char *get_racename(struct attrib *alist);

#endif
