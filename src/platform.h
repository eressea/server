#pragma once

#ifndef UNILIB_H
#define UNILIB_H

#define _POSIX_C_SOURCE 200809L

#ifndef MAX_PATH
# define MAX_PATH 4096
#endif

#define UNUSED_ARG(a) (void)(a)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define TOLUA_CAST (char*)
#endif
