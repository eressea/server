#pragma once
#ifndef H_ATTRIBUTE_FOLLOW
#define H_ATTRIBUTE_FOLLOW

extern struct attrib_type at_follow;

struct unit;
struct attrib;

struct attrib *make_follow(struct unit *u);

#endif
