#ifndef UTIL_ASSERT_H
#define UTIL_ASSERT_H

#include <assert.h>

#define assert_alloc(expr) assert((expr) || !"out of memory")
#endif
