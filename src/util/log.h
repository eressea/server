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

    void log_fatal(const char *format, ...);
    void log_error(const char *format, ...);
    void log_warning(const char *format, ...);
    void log_debug(const char *format, ...);
    void log_info(const char *format, ...);
    void log_printf(FILE * ios, const char *format, ...);

    void errno_check(const char *file, int line);

    int stats_count(const char *stat, int delta);
    void stats_write(FILE *F, const char *prefix);
    int stats_walk(const char *prefix, int (*callback)(const char *key, int val, void * udata), void *udata);
    void stats_close(void);

#define ERRNO_CHECK() errno_check(__FILE__, __LINE__)

    
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
