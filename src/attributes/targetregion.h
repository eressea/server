#pragma once
#ifndef H_ATTRIBUTE_TARGETREGION
#define H_ATTRIBUTE_TARGETREGION

extern struct attrib_type at_targetregion;

struct region;
struct attrib;

struct attrib *make_targetregion(struct region *);

#endif
