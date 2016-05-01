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
#include <platform.h>
#include "log.h"
#include "bsdstring.h"
#include "unicode.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* TODO: set from external function */
int log_flags = LOG_FLUSH | LOG_CPERROR | LOG_CPWARNING | LOG_CPDEBUG;

#ifdef STDIO_CP
static int stdio_codepage = STDIO_CP;
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

#define MAXLENGTH 4096          /* because I am lazy, CP437 output is limited to this many chars */
#define LOG_MAXBACKUPS 5

static int
cp_convert(const char *format, char *buffer, size_t length, int codepage)
{
    /* when console output on MSDOS, convert to codepage */
    const char *input = format;
    char *pos = buffer;

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
        input += length;
    }
    *pos = 0;
    return 0;
}

void log_rotate(const char *filename, int maxindex)
{
    char buffer[2][MAX_PATH];
    int dst = 1;
    assert(strlen(filename) < sizeof(buffer[0]) - 4);

    sprintf(buffer[dst], "%s.%d", filename, maxindex);
#ifdef HAVE_UNISTD_H
    /* make sure we don't overwrite an existing file (hard links) */
    unlink(buffer[dst]);
#endif
    while (maxindex > 0) {
        int err, src = 1 - dst;
        sprintf(buffer[src], "%s.%d", filename, --maxindex);
        err = rename(buffer[src], buffer[dst]);
        if (err != 0) {
            log_debug("log rotate %s: %s", buffer[dst], strerror(errno));
        }
        dst = src;
    }
    if (rename(filename, buffer[dst]) != 0) {
        log_debug("log rotate %s: %s", buffer[dst], strerror(errno));
    }
}

static const char *log_prefix(int level) {
    const char * prefix = "ERROR";
    if (level == LOG_CPWARNING) prefix = "WARNING";
    else if (level == LOG_CPDEBUG) prefix = "DEBUG";
    else if (level == LOG_CPINFO) prefix = "INFO";
    return prefix;
}

static int check_dupe(const char *format, int type)
{
    static int last_type; /* STATIC_XCALL: used across calls */
    static char last_message[32]; /* STATIC_XCALL: used across calls */
    static int dupes = 0;         /* STATIC_XCALL: used across calls */
    if (strncmp(last_message, format, sizeof(last_message)) == 0) {
        ++dupes;
        return 1;
    }
    if (dupes) {
        fprintf(stderr, "%s: last message repeated %d times\n", log_prefix(last_type),
            dupes + 1);
        dupes = 0;
    }
    strlcpy(last_message, format, sizeof(last_message));
    last_type = type;
    return 0;
}

static void _log_write(FILE * stream, int codepage, const char *format, va_list args)
{
    if (codepage) {
        char buffer[MAXLENGTH];
        char converted[MAXLENGTH];
        vsnprintf(buffer, sizeof(buffer), format, args);
        if (cp_convert(buffer, converted, MAXLENGTH, codepage) == 0) {
            fputs(converted, stream);
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

static void log_stdio(void *data, int level, const char *module, const char *format, va_list args) {
    FILE *out = (FILE *)data;
    int codepage = (out == stderr || out == stdout) ? stdio_codepage : 0;
    const char *prefix = log_prefix(level);
    size_t len = strlen(format);

    fprintf(out, "%s: ", prefix);

    _log_write(out, codepage, format, args);
    if (format[len - 1] != '\n') {
        fputc('\n', out);
    }
}

log_t *log_to_file(int flags, FILE *out) {
    return log_create(flags, out, log_stdio);
}

static void log_write(int flags, const char *module, const char *format, va_list args) {
    log_t *lg;
    for (lg = loggers; lg; lg = lg->next) {
        int level = flags & LOG_LEVELS;
        if (lg->flags & level) {
            int dupe = 0;
            va_list copy;

            va_copy(copy, args);
            if (lg->flags & LOG_BRIEF) {
                dupe = check_dupe(format, level);
            }
            if (dupe == 0) {
                lg->log(lg->data, level, NULL, format, copy);
            }
            va_end(copy);
        }
    }
}


void log_fatal(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_write(LOG_CPERROR, NULL, format, args);
    va_end(args);
}

void log_error(const char *format, ...)
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

void log_printf(FILE * io, const char *format, ...)
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
    return 0;
}

int log_level(log_t * log, int flags)
{
    int old = log->flags;
    log->flags = flags;
    return old;
}
