#pragma once
#ifndef H_ATTRIBUTE_REDUCEPRODUCTION
#define H_ATTRIBUTE_REDUCEPRODUCTION

extern struct attrib_type at_reduceproduction;

struct attrib;

struct attrib *make_reduceproduction(int percent, int time);

#endif
