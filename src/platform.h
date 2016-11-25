/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
                         Katja Zedel <katze@felidae.kn-bremen.de
                         Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef CONFIG_H
#define CONFIG_H

#ifdef NDEBUG
#define LOMEM
#endif

#ifdef _MSC_VER
# define VC_EXTRALEAN
# define WIN32_LEAN_AND_MEAN
#pragma warning(push)
#pragma warning(disable:4820 4255 4668)
# include <windows.h>
# include <io.h>
#pragma warning(pop)
# undef MOUSE_MOVED
# define STDIO_CP 1252          /* log.c, convert to console character set */
# pragma warning (disable: 4201 4214 4514 4115 4711)
#if _MSC_VER >= 1900
# pragma warning(disable: 4710)
/* warning C4710: function not inlined */
# pragma warning(disable: 4456)
/* warning C4456 : declaration of <name> hides previous local declaration */
# pragma warning(disable: 4457)
/* warning C4457: declaration of <name> hides function parameter */
# pragma warning(disable: 4459)
/* warning C4459: declaration of <name> hides global declaration */
#endif
# pragma warning(disable: 4056)
/* warning C4056: overflow in floating point constant arithmetic */
# pragma warning(disable: 4201)
/* warning C4201: nonstandard extension used : nameless struct/union */
# pragma warning(disable: 4214)
/* warning C4214: nonstandard extension used : bit field types other than int */
# pragma warning(disable: 4100)
/* warning C4100: <name> : unreferenced formal parameter */
# pragma warning(disable: 4996)
/* <name> is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' */
# pragma warning(disable: 4668)
/* <type>: <num> bytes padding after data member <member> */
# pragma warning(disable: 4820)

/* warning C4100: <name> was declared deprecated */
#ifndef _CRT_SECURE_NO_DEPRECATE
# define _CRT_SECURE_NO_DEPRECATE
#endif

/*
 * http://msdn2.microsoft.com/en-us/library/ms235505(VS.80).aspx
 * Defining _CRT_DISABLE_PERFCRIT_LOCKS forces all I/O operations to assume a 
 * single-threaded I/O model and use the _nolock forms of the functions. 
 */
#ifndef _CRT_DISABLE_PERFCRIT_LOCKS
# define _CRT_DISABLE_PERFCRIT_LOCKS
#endif

/* define CRTDBG to enable MSVC CRT Debug library functions */
#if defined(_DEBUG) && defined(CRTDBG)
# include <crtdbg.h>
# define _CRTDBG_MAP_ALLOC
#endif

#elif defined __GLIBC__
# define _POSIX_C_SOURCE 200809L
# undef _DEFAULT_SOURCE
# define _DEFAULT_SOURCE
# undef __USE_BSD
# define __USE_BSD
# define HAVE_SNPRINTF
# define HAVE_SYS_STAT_MKDIR
# define HAVE_STRDUP
# define HAVE_UNISTD_H
#elif defined SOLARIS
# define _SYS_PROCSET_H
#undef _XOPEN_SOURCE
# define _XOPEN_SOURCE
#else
#include <autoconf.h>
#endif

#define unused_arg (void)

#ifndef INLINE_FUNCTION
# define INLINE_FUNCTION
#endif

#define iswxspace(c) (c==160 || iswspace(c))
#define isxspace(c) (c==160 || isspace(c))

#define TOLUA_CAST (char*)

#if !defined(MAX_PATH)
#if defined(PATH_MAX)
# define MAX_PATH PATH_MAX
#else
# define MAX_PATH 256
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#if defined(HAVE_STDBOOL_H)
# include <stdbool.h>
#else
# if ! HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
typedef unsigned char _Bool;
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif

#ifndef HAVE__ACCESS
#ifdef HAVE_ACCESS
#define _access(path, mode) access(path, mode)
#endif
#endif

#if defined(HAVE_DIRECT__MKDIR)
#include <direct.h>
#elif defined(HAVE_DIRECT_MKDIR)
#include <direct.h>
#define _mkdir(a) mkdir(a)
#elif defined(HAVE_SYS_STAT_MKDIR)
#include <sys/stat.h>
#define _mkdir(a) mkdir(a, 0777)
#endif

#ifndef _min
#define _min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef _max
#define _max(a,b) ((a) > (b) ? (a) : (b))
#endif

#if !defined(HAVE__STRDUP)
#if defined(HAVE_STRDUP)
#define _strdup(a) strdup(a)
#else
#define _strdup(a) lcp_strdup(a)
#endif
#endif

#if !defined(HAVE__SNPRINTF)
#if defined(HAVE_SNPRINTF)
#define _snprintf snprintf
#endif
#endif

#if !defined(HAVE__STRCMPL)
#if defined(HAVE_STRCMPL)
#define _strcmpl(a, b) strcmpl(a, b)
#elif defined(HAVE__STRICMP)
#define _strcmpl(a, b) _stricmp(a, b)
#elif defined(HAVE_STRICMP)
#define _strcmpl(a, b) stricmp(a, b)
#elif defined(HAVE_STRCASECMP)
#define _strcmpl(a, b) strcasecmp(a, b)
#else
#define _strcmpl(a, b) lcp_strcmpl(a, b)
#endif
#endif

#endif

