#pragma once

#ifndef _LP64
#define _LP64 0 /* fix a warning in pdcurses 3.4 */
#endif

/* @see https://developercommunity.visualstudio.com/content/problem/69874/warning-c4001-in-standard-library-stringh-header.html */
#if _MSC_VER >= 1900
#pragma warning(disable: 4710 4820 4001)
#pragma warning(disable: 4100) // unreferenced formal parameter
#pragma warning(disable: 4456) // declaration hides previous
#pragma warning(disable: 4457) // declaration hides function parameter
#pragma warning(disable: 4459) // declaration hides global
#pragma warning(disable: 4224) // formal parameter was previously defined as a type
#pragma warning(disable: 4214) // bit field types other than int
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
