#pragma once

#ifndef UNILIB_H
#define UNILIB_H

#ifdef _MSC_VER
#ifndef __STDC__
#define __STDC__ 1 // equivalent to /Za
#endif
#define NO_STRDUP
#define NO_MKDIR
#define mkdir(d, a) _mkdir(d)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable: 4710 4820)
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4456) // declaration hides previous
#pragma warning(disable: 4457) // declaration hides function parameter
#pragma warning(disable: 4459) // declaration hides global
#endif

#ifndef MAX_PATH
# define MAX_PATH 4096
#endif

#define UNUSED_ARG(a) (void)(a)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#ifdef NO_STRDUP
char * strdup(const char *s);
#endif

#ifdef NO_MKDIR
int mkdir(const char *pathname, int mode);
#endif

#endif
