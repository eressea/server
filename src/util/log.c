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
#include "unicode.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* TODO: set from external function */
int log_flags = LOG_FLUSH | LOG_CPERROR | LOG_CPWARNING | LOG_CPDEBUG;
int log_stderr = LOG_FLUSH | LOG_CPERROR | LOG_CPWARNING;

#ifdef STDIO_CP
static int stdio_codepage = STDIO_CP;
#else
static int stdio_codepage = 0;
#endif
static FILE *logfile;

#define MAXLENGTH 4096          /* because I am lazy, CP437 output is limited to this many chars */
#define LOG_MAXBACKUPS 5
void log_flush(void)
{
    if (logfile) fflush(logfile);
}

void log_puts(const char *str)
{
    fflush(stdout);
    if (logfile) {
        fputs(str, logfile);
    }
}

static int
cp_convert(const char *format, char *buffer, size_t length, int codepage)
{
    /* when console output on MSDOS, convert to codepage */
    const char *input = format;
    char *pos = buffer;

    while (pos + 1 < buffer + length && *input) {
        size_t length = 0;
        int result = 0;
        if (codepage == 437) {
            result = unicode_utf8_to_cp437(pos, input, &length);
        }
        else if (codepage == 1252) {
            result = unicode_utf8_to_cp1252(pos, input, &length);
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
    int n;
    if (_access(filename, 4) == 0) {
        char buffer[2][MAX_PATH];
        int src = 1;
        assert(strlen(filename) < sizeof(buffer[0]) - 4);
        for (n = 0; n < maxindex; ++n) {
            sprintf(buffer[0], "%s.%d", filename, n);
            if (_access(filename, 0) != 0) {
                break;
            }
        }
        if (_access(buffer[0], 0) == 0) {
            unlink(buffer[0]);
        }
        while (n--) {
            int dst = 1 - src;
            sprintf(buffer[src], "%s.%d", filename, n);
            rename(buffer[src], buffer[dst]);
            src = dst;
        }
        rename(filename, buffer[1 - src]);
    }
}

void log_open(const char *filename)
{
    if (logfile) {
        log_close();
    }
    log_rotate(filename, LOG_MAXBACKUPS);
    logfile = fopen(filename, "a");
    if (logfile) {
        /* Get UNIX-style time and display as number and string. */
        time_t ltime;
        time(&ltime);
        fprintf(logfile, "===\n=== Logfile started at %s===\n", ctime(&ltime));
    }
}

void log_close(void)
{
    if (!logfile || logfile == stderr || logfile == stdout)
        return;
    if (logfile) {
        /* Get UNIX-style time and display as number and string. */
        time_t ltime;
        time(&ltime);
        fprintf(logfile, "===\n=== Logfile closed at %s===\n\n", ctime(&ltime));
        fclose(logfile);
    }
    logfile = 0;
}

static int check_dupe(const char *format, const char *type)
{
    static const char *last_type; /* STATIC_XCALL: used across calls */
    static char last_message[32]; /* STATIC_XCALL: used across calls */
    static int dupes = 0;         /* STATIC_XCALL: used across calls */
    if (strncmp(last_message, format, sizeof(last_message)) == 0) {
        ++dupes;
        return 1;
    }
    if (dupes) {
        if (log_flags & LOG_CPERROR) {
            fprintf(stderr, "%s: last message repeated %d times\n", last_type,
                dupes + 1);
        }
        dupes = 0;
    }
    strncpy(last_message, format, sizeof(last_message));
    last_type = type;
    return 0;
}

static void _log_write(FILE * stream, int codepage, const char * prefix, const char *format, va_list args)
{
    if (stream) {
        fprintf(stream, "%s: ", prefix);
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
}

static void _log_writeln(FILE * stream, int codepage, const char * prefix, const char *format, va_list args)
{
    size_t len = strlen(format);
    _log_write(stream, codepage, prefix, format, args);
    if (format[len - 1] != '\n') {
        fputc('\n', stream);
    }
}
void log_debug(const char *format, ...)
{
    const char * prefix = "DEBUG";
    const int mask = LOG_CPDEBUG;

    /* write to the logfile, always */
    if (logfile && (log_flags & mask)) {
        va_list args;
        va_start(args, format);
        _log_writeln(logfile, 0, prefix, format, args);
        va_end(args);
    }

    /* write to stderr, if that's not the logfile already */
    if (logfile != stderr && (log_stderr & mask)) {
        int dupe = check_dupe(format, prefix);
        if (!dupe) {
            va_list args;
            va_start(args, format);
            _log_writeln(stderr, stdio_codepage, prefix, format, args);
            va_end(args);
        }
    }
    if (log_flags & LOG_FLUSH) {
        log_flush();
    }
}

void log_warning(const char *format, ...)
{
    const char * prefix = "WARNING";
    const int mask = LOG_CPWARNING;

    /* write to the logfile, always */
    if (logfile && (log_flags & mask)) {
        va_list args;
        va_start(args, format);
        _log_writeln(logfile, 0, prefix, format, args);
        va_end(args);
    }

    /* write to stderr, if that's not the logfile already */
    if (logfile != stderr && (log_stderr & mask)) {
        int dupe = check_dupe(format, prefix);
        if (!dupe) {
            va_list args;
            va_start(args, format);
            _log_writeln(stderr, stdio_codepage, prefix, format, args);
            va_end(args);
        }
    }
    if (log_flags & LOG_FLUSH) {
        log_flush();
    }
}

void log_error(const char *format, ...)
{
    const char * prefix = "ERROR";
    const int mask = LOG_CPERROR;

    /* write to the logfile, always */
    if (logfile && (log_flags & mask)) {
        va_list args;
        va_start(args, format);
        _log_writeln(logfile, 0, prefix, format, args);
        va_end(args);
    }

    /* write to stderr, if that's not the logfile already */
    if (logfile != stderr && (log_stderr & mask)) {
        int dupe = check_dupe(format, prefix);
        if (!dupe) {
            va_list args;
            va_start(args, format);
            _log_writeln(stderr, stdio_codepage, prefix, format, args);
            va_end(args);
        }
    }
    if (log_flags & LOG_FLUSH) {
        log_flush();
    }
}

void log_info(const char *format, ...)
{
    const char * prefix = "INFO";
    const int mask = LOG_CPINFO;

    /* write to the logfile, always */
    if (logfile && (log_flags & mask)) {
        va_list args;
        va_start(args, format);
        _log_writeln(logfile, 0, prefix, format, args);
        va_end(args);
    }

    /* write to stderr, if that's not the logfile already */
    if (logfile != stderr && (log_stderr & mask)) {
        int dupe = check_dupe(format, prefix);
        if (!dupe) {
            va_list args;
            va_start(args, format);
            _log_writeln(stderr, stdio_codepage, prefix, format, args);
            va_end(args);
        }
    }
    if (log_flags & LOG_FLUSH) {
        log_flush();
    }
}

void log_printf(FILE * io, const char *format, ...)
{
    const char * prefix = "INFO";
    const int mask = LOG_CPINFO;

    /* write to the logfile, always */
    if (logfile && (log_flags & mask)) {
        int codepage = (logfile == stderr || logfile == stdout) ? stdio_codepage : 0;
        va_list args;
        va_start(args, format);
        _log_write(logfile, codepage, prefix, format, args);
        va_end(args);
    }

    /* write to io, if that's not the logfile already */
    if (logfile != io && (log_stderr & mask)) {
        int dupe = check_dupe(format, prefix);
        if (!dupe) {
            va_list args;
            va_start(args, format);
            _log_write(io, stdio_codepage, prefix, format, args);
            va_end(args);
        }
    }
    if (log_flags & LOG_FLUSH) {
        log_flush();
    }
}
