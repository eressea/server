#pragma once

#ifndef UNILIB_H
#define UNILIB_H

#define _POSIX_C_SOURCE 200809L
#ifdef _MSC_VER
#ifndef __STDC__
#define __STDC__ 1 // equivalent to /Za
#endif
#define NO_STRDUP
#define NO_MKDIR
#define NO_UNLINK
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

#define TOLUA_CAST (char*)

#ifdef NO_STRDUP
char * strdup(const char *s);
#endif

#ifdef NO_MKDIR
int mkdir(const char *pathname, int mode);
#endif

#ifdef NO_UNLINK
int unlink(const char *pathname);
#endif
#endif
