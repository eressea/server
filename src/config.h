/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#ifndef CONFIG_H
#define CONFIG_H

#ifdef _MSC_VER
# define STDIO_CP 1252 /* log.c, convert to console character set */
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
# include <strings.h>	/* strncasecmp-Prototyp */
#endif

#ifdef _BSD_SOURCE
# define __EXTENSIONS__
#endif

#ifdef WIN32
# include <common/util/windir.h>
# define HAVE_READDIR
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
# include <dirent.h>
# define HAVE_READDIR
# define HAVE_OPENDIR
# include <string.h>
# define HAVE_STRDUP
# define HAVE_SNPRINTF
#ifdef _POSIX_SOURCE /* MINGW doesn't seem to have these */
# define HAVE_EXECINFO
# define HAVE_MKDIR_WITH_PERMISSION
# define HAVE_SIGACTION
# define HAVE_LINK
# define HAVE_SLEEP
#endif
#endif

/* egcpp 4 dos */
#ifdef MSDOS
# include <dir.h>
# define HAVE_MKDIR_WITH_PERMISSION
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
# include <string.h> /* must be included here so strdup is not redefined */
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

#ifdef HAVE_MKDIR_WITH_PERMISSION
# define makedir(d, p) mkdir(d, p)
#elif defined(HAVE_MKDIR_WITHOUT_PERMISSION)
# define makedir(d, p) mkdir(d)
#elif defined(HAVE__MKDIR_WITHOUT_PERMISSION)
_CRTIMP int __cdecl _mkdir(const char *);
# define makedir(d, p) _mkdir(d)
#endif

#ifndef HAVE_STRDUP
extern char * strdup(const char *s);
#endif

#ifndef HAVE_SLEEP
#ifdef HAVE__SLEEP_MSEC
# define sleep(sec) _sleep(1000*sec)
#elif defined(HAVE__SLEEP)
# define sleep(sec) _sleep(sec)
#endif
#endif

#if !defined(MAX_PATH)
# ifdef WIN32
#  define MAX_PATH _MAX_PATH
# elif defined(PATH_MAX)
#  define MAX_PATH PATH_MAX
# else
#  define MAX_PATH 1024
# endif
#endif

/****            ****
 ** min/max macros **
 ****            ****/

#ifndef NOMINMAX
#ifndef min
# define min(a,b)            ((a) < (b) ? (a) : (b))
#endif
#ifndef max
# define max(a,b)            ((a) > (b) ? (a) : (b))
#endif
#endif

#if defined (ghs) || defined (__GNUC__) || defined (__hpux) || defined (__sgi) || defined (__DECCXX) || defined (__KCC) || defined (__rational__) || defined (__USLC__) || defined (ACE_RM544)
# define unused(a) do {/* null */} while (&a == 0)
#else /* ghs || __GNUC__ || ..... */
# define unused(a) (a)
#endif /* ghs || __GNUC__ || ..... */

/****                      ****
 ** The Eressea boolean type **
 ****                      ****/
#if defined(WIN32) && defined(USE_MYSQL)
  typedef unsigned char boolean;
#else
  typedef int boolean; /* not bool! wrong size. */
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

#endif

