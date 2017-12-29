#pragma once

#ifndef _LP64
#define _LP64 0 /* fix a warning in pdcurses 3.4 */
#endif

#ifdef _MSC_VER
#if _MSC_VER >= 1900
#pragma warning(disable: 4710 4820 4001)
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4456) // declaration hides previous
#pragma warning(disable: 4457) // declaration hides function parameter
#pragma warning(disable: 4459) // declaration hides global
#pragma warning(disable: 4224) // formal parameter was previously defined as a type
#endif
#else /* assume gcc */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
# define va_copy(a,b) __va_copy(a,b)
#endif

#endif

/* #define _POSIX_C_SOURCE 200809L
*/
#ifndef MAX_PATH
# define MAX_PATH 4096
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define TOLUA_CAST (char*)
