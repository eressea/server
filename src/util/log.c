#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "log.h"

#include "path.h"
#include "strings.h"
#include "unicode.h"

#include "stb_sprintf.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef WIN32
#include <io.h>
#define FISATTY(F) (_isatty(_fileno(F)))
#else
#include <unistd.h>
#define FISATTY(F) (isatty(fileno(F)))
#endif

void errno_check(const char * file, int line) {
    if (errno) {
        log_info("errno is %d (%s) at %s:%d", 
                 errno, strerror(errno), file, line);
        errno = 0;
    }
}

#if WIN32
static int stdio_codepage = 1252;
#else
static int stdio_codepage = 0;
#endif

typedef struct log_t {
    void(*log)(void *data, int level, const char *module, const char *format, va_list args);
    void *data;
    int flags;
    struct log_t *next;
} log_t;

static log_t *loggers;

log_t *log_create(int flags, void *data, log_fun call) {
    log_t *lgr = malloc(sizeof(log_t));
    if (!lgr) abort();
    lgr->log = call;
    lgr->flags = flags;
    lgr->data = data;
    lgr->next = loggers;
    loggers = lgr;
    return lgr;
}

void log_destroy(log_t *handle) {
    log_t ** lp = &loggers;
    while (*lp) {
        log_t *lg = *lp;
        if (lg==handle) {
            *lp = lg->next;
            free(lg);
            break;
        }
        lp = &lg->next;
    }
}

#define MAXLENGTH 4096          /* because I am lazy, tty output is limited to this many chars */
#define LOG_MAXBACKUPS 5

static int
cp_convert(const char *format, unsigned char *buffer, size_t length, int codepage)
{
    /* when console output on MSDOS, convert to codepage */
    const char *input = format;
    unsigned char *pos = buffer;

    while (pos + 1 < buffer + length && *input) {
        size_t size = 0;
        int result = 0;
        if (codepage == 437) {
            result = unicode_utf8_to_cp437(pos, input, &size);
        }
        else if (codepage == 1252) {
            result = unicode_utf8_to_cp1252(pos, input, &size);
        }
        if (result != 0) {
            *pos = 0;                 /* just in case caller ignores our return value */
            return result;
        }
        ++pos;
        input += size;
    }
    *pos = 0;
    return 0;
}

void log_rotate(const char *filename, int maxindex)
{
    char buffer[2][PATH_MAX];
    int dst = 1;
    assert(strlen(filename) < sizeof(buffer[0]) - 4);

    stbsp_sprintf(buffer[dst], "%s.%d", filename, maxindex);
    if (remove(buffer[dst]) != 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "log rotate %s: %d %s", buffer[dst], errno, strerror(errno));
        }
        errno = 0;
    }

    while (maxindex > 0) {
        int src = 1 - dst;
        stbsp_sprintf(buffer[src], "%s.%d", filename, --maxindex);
        if (rename(buffer[src], buffer[dst]) != 0) {
            if (errno != ENOENT) {
                fprintf(stderr, "log rotate %s: %d %s", buffer[dst], errno, strerror(errno));
            }
            errno = 0;
        }
        dst = src;
    }
    if (rename(filename, buffer[dst]) != 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "log rotate %s: %d %s", buffer[dst], errno, strerror(errno));
        }
        errno = 0;
    }
}

static const char *log_prefix(int level) {
    const char * prefix = "ERROR";
    if (level == LOG_CPWARNING) prefix = "WARNING";
    else if (level == LOG_CPDEBUG) prefix = "DEBUG";
    else if (level == LOG_CPINFO) prefix = "INFO";
    return prefix;
}

static int check_dupe(const char *format, int level)
{
    static int last_type; /* STATIC_XCALL: used across calls */
    static char last_message[32] = { 0 }; /* STATIC_XCALL: used across calls */
    static int dupes = 0;         /* STATIC_XCALL: used across calls */
    if (strncmp(last_message, format, sizeof(last_message)) == 0) {
        /* TODO: C6054: String 'last_message' might not be zero - terminated. */
        ++dupes;
        return 1;
    }
    if (dupes) {
        if (level & LOG_CPERROR) {
            fprintf(stderr, "%s: last message repeated %d times\n", log_prefix(last_type),
                dupes + 1);
        }
        dupes = 0;
    }
    str_strlcpy(last_message, format, sizeof(last_message));
    last_type = level;
    return 0;
}

static void _log_write(FILE * stream, int codepage, const char *format, va_list args)
{
    if (codepage) {
        char buffer[MAXLENGTH];
        unsigned char converted[MAXLENGTH];
        stbsp_vsnprintf(buffer, sizeof(buffer), format, args);
        if (cp_convert(buffer, converted, MAXLENGTH, codepage) == 0) {
            fputs((char *)converted, stream);
        }
        else {
            /* fall back to non-converted output */
            vfprintf(stream, format, args);
        }
    }
    else {
        vfprintf(stream, format, args);
    }
}

static void log_stdio(void* data, int level, const char* module, const char* format, va_list args) {
    FILE* out = (FILE*)data;
    int codepage = 0;
    const char* prefix = log_prefix(level);
    size_t len = strlen(format);

    (void)module;
    if (stdio_codepage && (out == stderr || out == stdout)) {
        codepage = FISATTY(out) ? stdio_codepage : 0;
    }
    fprintf(out, "%s: ", prefix);

    _log_write(out, codepage, format, args);
    if (format[len - 1] != '\n') {
        fputc('\n', out);
    }
    fflush(out);
}

log_t *log_to_file(int flags, FILE *out) {
    return log_create(flags, out, log_stdio);
}

/*
 * Notes for va_copy compatibility:
 * MSVC: https://social.msdn.microsoft.com/Forums/vstudio/en-US/53a4fd75-9f97-48b2-aa63-2e2e5a15efa3/stdcversion-problem?forum=vclanguage
 * GNU: https://www.gnu.org/software/libc/manual/html_node/Argument-Macros.html
 */
static void vlog(log_t *lg, int level, const char *format, va_list args) {
    va_list copy;
    va_copy(copy, args);
    lg->log(lg->data, level, NULL, format, copy);
    va_end(copy);
}

static void log_write(int flags, const char *module, const char *format, va_list args) {
    log_t *lg;

    (void)module;

    for (lg = loggers; lg; lg = lg->next) {
        int level = flags & LOG_LEVELS;
        if (lg->flags & level) {
            int dupe = 0;
            if (lg->flags & LOG_BRIEF) {
                dupe = check_dupe(format, level);
            }
            if (dupe == 0) {
                vlog(lg, level, format, args);
            }
        }
    }
}

void log_fatal(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_CPERROR, NULL, format, args);
    va_end(args);
    abort();
}

void log_error(const char *format, ...) /*-V524 */
{
    va_list args;
    va_start(args, format);
    log_write(LOG_CPERROR, NULL, format, args);
    va_end(args);
}

void log_warning(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_CPWARNING, NULL, format, args);
    va_end(args);
}

void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_CPDEBUG, NULL, format, args);
    va_end(args);
}

void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_CPINFO, NULL, format, args);
    va_end(args);
}

static FILE *logfile;

void log_close(void)
{
    while (loggers) {
        log_t *lgr = loggers;
        loggers = lgr->next;
        free(lgr);
    }
    if (logfile) {
        time_t ltime;
        time(&ltime);
        fprintf(logfile, "===\n=== Logfile closed at %s===\n\n", ctime(&ltime));
        fclose(logfile);
    }
    logfile = 0;
}

log_t *log_open(const char *filename, int log_flags)
{
    log_rotate(filename, LOG_MAXBACKUPS);
    logfile = fopen(filename, "a");
    if (logfile) {
        /* Get UNIX-style time and display as number and string. */
        time_t ltime;
        time(&ltime);
        fprintf(logfile, "===\n=== Logfile started at %s===\n", ctime(&ltime));
        return log_create(log_flags, logfile, log_stdio);
    }
    return NULL;
}

int log_level(log_t * log, int flags)
{
    int old = log->flags;
    log->flags = flags;
    return old;
}
