#include <config.h>
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define LOG_FLUSH      (1<<0)
#define LOG_CPERROR    (1<<1)
#define LOG_CPWARNING  (1<<1)

/* TODO: set from external function */
static int flags = LOG_FLUSH|LOG_CPERROR;
static FILE * logfile;

void
log_puts(const char * str)
{
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
}

void 
_log_warn(const char * format, ...)
{
	va_list marker;
	if (!logfile) logfile = stderr;
	fputs("WARNING: ", logfile);
	va_start(marker, format);
	vfprintf(logfile, format, marker);
	va_end(marker);
	if (logfile!=stderr) {
		if (flags & LOG_CPWARNING) {
			fputs("\bWARNING: ", stderr);
			va_start(marker, format);
			vfprintf(stderr, format, marker);
			va_end(marker);
		}
		if (flags & LOG_FLUSH) {
			fflush(logfile);
		}
	}
}

void 
_log_error(const char * format, ...)
{
	va_list marker;
	if (!logfile) logfile = stderr;

	fputs("ERROR: ", logfile);
	va_start(marker, format);
	vfprintf(logfile, format, marker);
	va_end(marker);
	if (logfile!=stderr) {
		if (flags & LOG_CPERROR) {
			fputs("\bERROR: ", stderr);
			va_start(marker, format);
			vfprintf(stderr, format, marker);
			va_end(marker);
		}
		if (flags & LOG_FLUSH) {
			fflush(logfile);
		}
	}
}
