/*
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea.de>
| Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
| (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
|                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
+-------------------+  Stefan Reich <reich@halbling.de>

This program may not be used, modified or distributed
without prior permission by the authors of Eressea.
*/
#ifndef H_UTIL_LOG
#define H_UTIL_LOG
#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
    extern void log_open(const char *filename);
    extern void log_close(void);
    extern void log_flush(void);

    /* use macros above instead of these: */
    extern void log_warning(const char *format, ...);
    extern void log_error(const char *format, ...);
    extern void log_debug(const char *format, ...);
    extern void log_info(const char *format, ...);
    extern void log_printf(FILE * ios, const char *format, ...);

#define LOG_FLUSH      0x01
#define LOG_CPWARNING  0x02
#define LOG_CPERROR    0x04
#define LOG_CPDEBUG    0x08
#define LOG_CPINFO     0x10

    extern int log_flags;
    extern int log_stderr;
#ifdef __cplusplus
}
#endif
#endif
