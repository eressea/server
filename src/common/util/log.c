/* vi: set ts=2:
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea-pbem.de>
| Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
| (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
|                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
+-------------------+  Stefan Reich <reich@halbling.de>

This program may not be used, modified or distributed 
without prior permission by the authors of Eressea.
*/
#include <config.h>
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* TODO: set from external function */
static int flags = LOG_FLUSH|LOG_CPERROR|LOG_CPWARNING;
static FILE * logfile;

void 
log_flush(void)
{
  fflush(logfile);
}

void
log_puts(const char * str)
{
  fflush(stdout);
  if (!logfile) logfile = stderr;
  fputs(str, logfile);
}

void 
log_printf(const char * format, ...)
{
  va_list marker;
  if (!logfile) logfile = stderr;
  va_start(marker, format);
  vfprintf(logfile, format, marker);
  va_end(marker);
  if (flags & LOG_FLUSH) {
    log_flush();
  }
}

void 
log_open(const char * filename)
{
  if (logfile) log_close();
  logfile = fopen(filename, "a");
  if (logfile) {
    /* Get UNIX-style time and display as number and string. */
    time_t ltime;
    time( &ltime );
    log_printf( "===\n=== Logfile started at %s===\n", ctime( &ltime ) );
  }
}

void 
log_close(void)
{
  if (!logfile || logfile == stderr || logfile == stdout) return;
  if (logfile) {
    /* Get UNIX-style time and display as number and string. */
    time_t ltime;
    time( &ltime );
    log_printf("===\n=== Logfile closed at %s===\n\n", ctime( &ltime ) );
  }
  fclose(logfile);
  logfile = 0;
}

void 
_log_warn(const char * format, ...)
{
  va_list marker;
  fflush(stdout);
  if (!logfile) logfile = stderr;
  fputs("WARNING: ", logfile);
  va_start(marker, format);
  vfprintf(logfile, format, marker);
  va_end(marker);
  if (logfile!=stderr) {
    if (flags & LOG_CPWARNING) {
      fputs("WARNING: ", stderr);
      va_start(marker, format);
      vfprintf(stderr, format, marker);
      va_end(marker);
    }
    if (flags & LOG_FLUSH) {
      log_flush();
    }
  }
}

void 
_log_error(const char * format, ...)
{
  va_list marker;
  fflush(stdout);
  if (!logfile) logfile = stderr;

  fputs("ERROR: ", logfile);
  va_start(marker, format);
  vfprintf(logfile, format, marker);
  va_end(marker);
  if (logfile!=stderr) {
    if (flags & LOG_CPERROR) {
      fputs("ERROR: ", stderr);
      va_start(marker, format);
      vfprintf(stderr, format, marker);
      va_end(marker);
    }
    log_flush();
  }
}

void 
_log_info(unsigned int flag, const char * format, ...)
{
  va_list marker;
  fflush(stdout);
  if (!logfile) logfile = stderr;

  fprintf(logfile, "INFO[%u]: ", flag);
  va_start(marker, format);
  vfprintf(logfile, format, marker);
  va_end(marker);
  if (logfile!=stderr) {
    if (flags & flag) {
      fprintf(stderr, "INFO[%u]: ", flag);
      va_start(marker, format);
      vfprintf(stderr, format, marker);
      va_end(marker);
    }
    if (flags & LOG_FLUSH) {
      log_flush();
    }
  }
}
