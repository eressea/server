#pragma once
#ifndef H_ATTRIBUTE_KEY
#define H_ATTRIBUTE_KEY

extern struct attrib_type at_key; /* DEPRECATED: at_key has been replaced with at_keys */
extern struct attrib_type at_keys;

struct attrib;

void key_set(struct attrib **alist, unsigned int key, int value);
void key_unset(struct attrib **alist, unsigned int key);
int key_get(struct attrib *alist, unsigned int key);

#endif
