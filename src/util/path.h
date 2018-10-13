#pragma once

#include <stddef.h>

#ifdef _MSC_VER
/* @see https://insanecoding.blogspot.no/2007/11/pathmax-simply-isnt.html */
#define PATH_MAX 260
#endif

char * path_join(const char *p1, const char *p2, char *dst, size_t len);
