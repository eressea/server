/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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


/****                 ****
 ** Debugging Libraries **
 ****                 ****/
/* 
 * MALLOCDBG is an integer >= 0 that specifies the level of
 * debugging. 0 = no debugging, >= 1 increasing levels of
 * debugging strength.
 */
#ifdef MPATROL
# ifndef MALLOCDBG
#  define MALLOCDBG 1
# endif
# include <mpatrol.h>
#endif

#ifdef DMALLOC
# ifndef MALLOCDBG
#  define MALLOCDBG 1
# endif
# include <stdlib.h>
# include <dmalloc.h>
#endif

#if defined(__GCC__)
# include <stdbool.h>
# define HAS_BOOLEAN
# define boolean bool
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
# ifndef MALLOCDBG
#  define MALLOCDBG 1
# endif
# include <crtdbg.h>
# define _CRTDBG_MAP_ALLOC
#endif

#ifndef MALLOCDBG
# define MALLOCDBG 0
#endif

/****                    ****
 ** Architecture Dependent **
 ****                    ****/

#if _MSC_VER
# define STRNCPY_HAS_ZEROTERMINATION
#endif

/* für solaris: */
#ifdef SOLARIS
# define _SYS_PROCSET_H
# define _XOPEN_SOURCE
#endif

#ifdef _MSC_VER
# pragma warning (disable: 4201 4214 4514 4115 4711)
# pragma warning(disable: 4056)
  /* warning C4056: overflow in floating point constant arithmetic */
# pragma warning(disable: 4201)
  /* warning C4201: Nicht dem Standard entsprechende Erweiterung : Struktur/Union ohne Namen */
# pragma warning(disable: 4214)
  /* warning C4214: Nicht dem Standard entsprechende Erweiterung : Basistyp fuer Bitfeld ist nicht int */
# pragma warning(disable: 4100)
  /* warning C4100: <name> : unreferenced formal parameter */
#endif

#ifdef __GNUC__
# ifndef _BSD_SOURCE
#  define _BSD_SOURCE
#  define __USE_BSD
# endif
# include <features.h>
# include <strings.h>	/* strncasecmp-Prototyp */
#endif

#ifdef _BSD_SOURCE
# define __EXTENSIONS__
#endif

#ifdef WIN32
# include <common/util/windir.h>
# define HAVE_READDIR
#endif

#if defined(__USE_SVID) || defined(_BSD_SOURCE) || defined(__USE_XOPEN_EXTENDED) || defined(_BE_SETUP_H) || defined(CYGWIN)
# include <unistd.h>
# define HAVE_STRCASECMP
# define HAVE_STRNCASECMP
# define HAVE_ACCESS
# include <dirent.h>
# define HAVE_READDIR
# define HAVE_OPENDIR
# include <sys/stat.h>
# define HAVE_MKDIR_WITH_PERMISSION
# include <string.h>
# define HAVE_STRDUP
# define HAVE_SNPRINTF
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
# define HAVE_MKDIR_WITHOUT_PERMISSION
# define HAVE_ACCESS
# define HAVE_STRICMP
# define HAVE_STRNICMP
# define HAVE_STRDUP
# define snprintf _snprintf
# define HAVE_SNPRINTF
# undef HAVE_STRCASECMP
# undef HAVE_STRNCASECMP
# define R_OK 4
#endif

/* Microsoft Visual C */
#ifdef _MSC_VER
# define R_OK 4
# define HAVE__MKDIR_WITHOUT_PERMISSION

# define snprintf _snprintf
# define HAVE_SNPRINTF

/* MSVC has _access */
_CRTIMP int __cdecl _access(const char *, int);
# define access(f, m) _access(f, m)
# define HAVE_ACCESS

/* MSVC has _strdup */
_CRTIMP char *  __cdecl _strdup(const char *);
# define strdup(s) _strdup(s)
# define HAVE_STRDUP

_CRTIMP int     __cdecl _stricmp(const char *, const char *);
# define stricmp(a, b) _stricmp(a, b)
# define HAVE_STRICMP

_CRTIMP int     __cdecl _strnicmp(const char *, const char *, size_t);
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

#define unused(var) var = var


/****                      ****
 ** The Eressea boolean type **
 ****                      ****/
#ifndef HAS_BOOLEAN
  typedef int boolean;
# define false ((boolean)0)
# define true ((boolean)!false)
#endif

#ifdef STRNCPY_HAS_ZEROTERMINATION
# define strnzcpy(dst, src, len)  strncpy(dst, src, len) 
#else
# define strnzcpy(dst, src, len) (strncpy(dst, src, len), len?dst[len-1]=0:0, dst)
#endif
#endif
