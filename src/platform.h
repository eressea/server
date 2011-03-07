/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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

#ifdef _MSC_VER
# define VC_EXTRALEAN
# define WIN32_LEAN_AND_MEAN
# include <Windows.h>
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

/* warning C4100: <name> was declared deprecated */
#ifndef _CRT_SECURE_NO_DEPRECATE
# define _CRT_SECURE_NO_DEPRECATE
#endif

/* http://msdn2.microsoft.com/en-us/library/ms235505(VS.80).aspx */
#ifndef _CRT_DISABLE_PERFCRIT_LOCKS
# define _CRT_DISABLE_PERFCRIT_LOCKS
#endif

#endif /* _MSC_VER_ */


#ifdef __cplusplus
# include <cstdio>
# include <cstdlib>
extern "C" {
#else
# include <stdio.h>
# include <stdlib.h>
#endif

/****                 ****
 ** Debugging Libraries **
 ****                 ****/
#if defined __GNUC__
# define HAVE_INLINE
# define INLINE_FUNCTION static __inline
#endif

/* define USE_DMALLOC to enable use of the dmalloc library */
#ifdef USE_DMALLOC
# include <stdlib.h>
# include <string.h>
# include <dmalloc.h>
#endif

/* define CRTDBG to enable MSVC CRT Debug library functions */
#if defined(_DEBUG) && defined(_MSC_VER) && defined(CRTDBG)
# include <crtdbg.h>
# define _CRTDBG_MAP_ALLOC
#endif

/****                    ****
 ** Architecture Dependent **
 ****                    ****/

/* für solaris: */
#ifdef SOLARIS
# define _SYS_PROCSET_H
# define _XOPEN_SOURCE
#endif

#ifdef __GNUC__
# ifndef _BSD_SOURCE
#  define _BSD_SOURCE
#  define __USE_BSD
# endif
/* # include <features.h> */
# include <strings.h>           /* strncasecmp-Prototyp */
#endif

#ifdef _BSD_SOURCE
# define __EXTENSIONS__
#endif

#ifdef WIN32
# define HAVE__MKDIR_WITHOUT_PERMISSION
# define HAVE__SLEEP_MSEC
#endif

#if defined(__USE_SVID) || defined(_BSD_SOURCE) || defined(__USE_XOPEN_EXTENDED) || defined(_BE_SETUP_H) || defined(CYGWIN)
# include <unistd.h>
# define HAVE_UNISTD_H
# define HAVE_STRCASECMP
# define HAVE_STRNCASECMP
# define HAVE_ACCESS
# define HAVE_STAT
typedef struct stat stat_type;

# include <string.h>
# define HAVE_STRDUP
# define HAVE_SNPRINTF
#ifdef _POSIX_SOURCE            /* MINGW doesn't seem to have these */
# define HAVE_EXECINFO
# define HAVE_SIGACTION
# define HAVE_LINK
# define HAVE_SLEEP
#endif
#endif

/* TinyCC */
#ifdef TINYCC
# undef HAVE_INLINE
# define INLINE_FUNCTION
#endif

/* lcc-win32 */
#ifdef __LCC__
# include <string.h>
# include <direct.h>
# include <io.h>
# define HAVE_ACCESS
# define HAVE_STAT
typedef struct stat stat_type;

# define HAVE_STRICMP
# define HAVE_STRNICMP
# define HAVE_STRDUP
# define HAVE_SLEEP
# define snprintf _snprintf
# define HAVE_SNPRINTF
# undef HAVE_STRCASECMP
# undef HAVE_STRNCASECMP
# define R_OK 4
#endif

/* Microsoft Visual C */
#ifdef _MSC_VER
# include <string.h>            /* must be included here so strdup is not redefined */
# define R_OK 4
# define HAVE_INLINE
# define INLINE_FUNCTION __inline

# define snprintf _snprintf
# define HAVE_SNPRINTF

/* MSVC has _access, not access */
#ifndef access
# define access(f, m) _access(f, m)
#endif
#define HAVE_ACCESS

/* MSVC has _stat, not stat */
# define HAVE_STAT
#include <sys/stat.h>
# define stat(a, b) _stat(a, b)
typedef struct _stat stat_type;

/* MSVC has _strdup */
# define strdup _strdup
# define HAVE_STRDUP

# define stricmp(a, b) _stricmp(a, b)
# define HAVE_STRICMP

# define strnicmp(a, b, c) _strnicmp(a, b, c)
# define HAVE_STRNICMP
# undef HAVE_STRCASECMP
# undef HAVE_STRNCASECMP
#endif

/* replacements for missing functions: */

#ifndef HAVE_STRCASECMP
# if defined(HAVE_STRICMP)
#  define strcasecmp stricmp
# elif defined(HAVE__STRICMP)
#  define strcasecmp _stricmp
# endif
#endif

#ifndef HAVE_STRNCASECMP
# if defined(HAVE_STRNICMP)
#  define strncasecmp strnicmp
# elif defined(HAVE__STRNICMP)
#  define strncasecmp _strnicmp
# endif
#endif

#ifndef HAVE_STRDUP
extern char *strdup(const char *s);
#endif

#ifndef HAVE_SLEEP
#ifdef HAVE__SLEEP_MSEC
# define sleep(sec) _sleep(1000*sec)
#elif defined(HAVE__SLEEP)
# define sleep(sec) _sleep(sec)
#endif
#endif

#if !defined(MAX_PATH)
# if defined(PATH_MAX)
#  define MAX_PATH PATH_MAX
# else
#  define MAX_PATH 1024
# endif
#endif

/****            ****
 ** min/max macros **
 ****            ****/
#ifndef NOMINMAX
#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#endif

#if defined (__GNUC__)
# define unused(a)              /* unused: a */
#elif defined (ghs) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__KCC) || defined (__rational__) || defined (__USLC__) || defined (ACE_RM544)
# define unused(a) do {/* null */} while (&a == 0)
#else /* ghs || __GNUC__ || ..... */
# define unused(a) (a)
#endif /* ghs || __GNUC__ || ..... */

/****                      ****
 ** The Eressea boolean type **
 ****                      ****/
#if defined(BOOLEAN)
# define boolean BOOLEAN
#else
typedef int boolean;            /* not bool! wrong size. */
#endif
#ifndef __cplusplus
# define false ((boolean)0)
# define true ((boolean)!false)
#endif
#ifdef __cplusplus
}
#endif

#ifndef INLINE_FUNCTION
# define INLINE_FUNCTION
#endif

#define iswxspace(c) (c==160 || iswspace(c))
#define isxspace(c) (c==160 || isspace(c))

#define TOLUA_CAST (char*)
#endif
