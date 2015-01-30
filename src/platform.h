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

#endif /* _MSC_VER_ */

#if defined __GNUC__
# undef _BSD_SOURCE
# define _BSD_SOURCE
# undef __USE_BSD
# define __USE_BSD
#endif

#ifdef _BSD_SOURCE
# undef __EXTENSIONS__
# define __EXTENSIONS__
#endif

#ifdef SOLARIS
# define _SYS_PROCSET_H
#undef _XOPEN_SOURCE
# define _XOPEN_SOURCE
#endif

#define unused_arg (void)

#ifndef INLINE_FUNCTION
# define INLINE_FUNCTION
#endif

#define iswxspace(c) (c==160 || iswspace(c))
#define isxspace(c) (c==160 || isspace(c))

#define TOLUA_CAST (char*)

#ifdef USE_AUTOCONF
# include <autoconf.h>
#endif

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

#endif

