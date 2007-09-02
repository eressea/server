/* vi: set ts=2:
+-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
|                   |  Enno Rehling <enno@eressea.de>
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/* TODO: set from external function */
int log_flags = LOG_FLUSH|LOG_CPERROR|LOG_CPWARNING;
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
  if (log_flags & LOG_FLUSH) {
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

static int
check_dupe(const char * format, const char * type) 
{
  static const char * last_type;
  static char last_message[32];
  static int dupes = 0;
  if (strncmp(last_message, format, sizeof(last_message))==0) {
    ++dupes;
    return 1;
  }
  if (dupes) {
    if (log_flags & LOG_CPERROR) {
      fprintf(stderr, "%s: last message repeated %d times\n", last_type, dupes+1);
    }
    dupes = 0;
  }
  strncpy(last_message, format, sizeof(last_message));
  last_type = type;
  return 0;
}

void 
_log_warn(const char * format, ...)
{
  int dupe = check_dupe(format, "WARNING");

  fflush(stdout);
  if (!logfile) logfile = stderr;
  if (logfile!=stderr) {
    va_list marker;
    fputs("WARNING: ", logfile);
    va_start(marker, format);
    vfprintf(logfile, format, marker);
    va_end(marker);
  }
  if (!dupe) {
    if (log_flags & LOG_CPWARNING) {
      va_list marker;
      fputs("WARNING: ", stderr);
      va_start(marker, format);
      vfprintf(stderr, format, marker);
      va_end(marker);
    }
    if (log_flags & LOG_FLUSH) {
      log_flush();
    }
  }
}

void 
_log_error(const char * format, ...)
{
  int dupe = check_dupe(format, "ERROR");
  fflush(stdout);
  if (!logfile) logfile = stderr;

  if (logfile!=stderr) {
    va_list marker;
    fputs("ERROR: ", logfile);
    va_start(marker, format);
    vfprintf(logfile, format, marker);
    va_end(marker);
  }
  if (!dupe) {
    if (logfile!=stderr) {
      if (log_flags & LOG_CPERROR) {
        va_list marker;
        fputs("ERROR: ", stderr);
        va_start(marker, format);
        vfprintf(stderr, format, marker);
        va_end(marker);
      }
      log_flush();
    }
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
    if (log_flags & flag) {
      fprintf(stderr, "INFO[%u]: ", flag);
      va_start(marker, format);
      vfprintf(stderr, format, marker);
      va_end(marker);
    }
    if (log_flags & LOG_FLUSH) {
      log_flush();
    }
  }
}
