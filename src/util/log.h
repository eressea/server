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
#include <stdarg.h>

    struct log_t;

    typedef void(*log_fun)(void *data, int level, const char *module, const char *format, va_list args);

    struct log_t * log_open(const char *filename, int flags);
    struct log_t * log_create(int flags, void *data, log_fun call);
    void log_destroy(struct log_t *handle);
    struct log_t * log_to_file(int flags, FILE *out);
    int log_level(struct log_t *log, int flags);
    void log_close(void);

    extern void log_fatal(const char *format, ...);
    extern void log_error(const char *format, ...);
    extern void log_warning(const char *format, ...);
    extern void log_debug(const char *format, ...);
    extern void log_info(const char *format, ...);
    extern void log_printf(FILE * ios, const char *format, ...);

#define LOG_CPERROR    0x01
#define LOG_CPWARNING  0x02
#define LOG_CPINFO     0x04
#define LOG_CPDEBUG    0x08
#define LOG_LEVELS     0x0F
#define LOG_FLUSH      0x10
#define LOG_BRIEF      0x20


    extern int log_stderr;
#ifdef __cplusplus
}
#endif
#endif
