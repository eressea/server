/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "sql.h"

#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static FILE * sqlstream = NULL;
static char * sqlfilename = NULL;

void
sql_init(const char * filename)
{
  if (sqlfilename!=NULL) free(sqlfilename);
  sqlfilename = strdup(filename);
}

void 
_sql_print(const char * format, ...)
{
  if (!sqlstream && sqlfilename) {
    sqlstream=fopen(sqlfilename, "wt+");
    free(sqlfilename);
    sqlfilename=NULL;
  }
  if (sqlstream!=NULL) {
    va_list marker;
    va_start(marker, format);
    vfprintf(sqlstream, format, marker);
    va_end(marker);
  }
}

void
sql_done(void)
{
	if (sqlstream) fclose(sqlstream);
	if (sqlfilename) free(sqlfilename);
  sqlstream=NULL;
  sqlfilename=NULL;
}

const char * 
sqlquote(const char * str)
{
#define BUFFERS 4
#define BUFSIZE 1024
	static char sqlstring[BUFSIZE*BUFFERS];
	static int index = 0;
	char * start = sqlstring+index*BUFSIZE;
	char * o = start;
	const char * i = str;
	while (*i && o-start < BUFSIZE-1) {
		if (*i!='\'' && *i!='\"') {
			*o++ = *i++;
		} else ++i;
	}
	*o = '\0';
	index = (index+1) % BUFFERS;
	return start;
}

