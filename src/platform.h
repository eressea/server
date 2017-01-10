#pragma once

#ifndef UNILIB_H
#define UNILIB_H

#ifndef MAX_PATH
# define MAX_PATH 4096
#endif

#define UNUSED_ARG(a) (void)(a)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#endif
