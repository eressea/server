/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
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
#include "goodies.h"

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>


/* Simple Integer-Liste */

char *
fstrncat(char * buffer, const char * str, unsigned int size)
{
	static char * b = NULL;
	static char * end = NULL;
	int n = 0;
	if (b==buffer) {
		end += strlen(end);
		size -= (end-b);
	} else {
		end = b = buffer;
	}
	while (size-- > 0 && (*end++=*str++)!=0) ++n;
	*end='\0';
	return b;
}
	  
int *
intlist_init(void)
{
	return (calloc(1, sizeof(int)));
}

int *
intlist_add(int *i_p, int i)
{
	i_p[0]++;
	i_p = realloc(i_p, (i_p[0]+1) * sizeof(int));

	i_p[i_p[0]] = i;
	return (i_p);
}

int *
intlist_find(int *i_p, int fi)
{
	int i;

	for(i=1; i <= i_p[0]; i++) {
		if(i_p[i] == fi) return (&i_p[i]);
	}
	return NULL;
}

unsigned int
hashstring(const char* s)
{
	unsigned int key = 0;
	int i = strlen(s);

	while (i>0) {
		key = (s[--i] + key*37);
	}
	return key % 0x7FFFFFFF;
}

const char *
escape_string(const char * str, char * buffer, unsigned int len)
{
	static char s_buffer[4096];
	const char * p = str;
	char * o;
	if (buffer==NULL) {
		buffer = s_buffer;
		len = sizeof(s_buffer);
	}
	o = buffer;
	do {
		switch (*p) {
		case '\"':
		case '\\':
			(*o++) = '\\';
		}
		(*o++) = (*p);
	} while (*p++);
	return buffer;
}

char *
set_string (char **s, const char *neu)
{
	*s = realloc(*s, strlen(neu) + 1);
	return strcpy(*s, neu);
}

boolean
locale_check(void) 
{
	int i, errorlevel = 0;
	unsigned char * umlaute = (unsigned char*)"äöüÄÖÜß";
	/* E: das prüft, ob umlaute funktionieren. Wenn äöü nicht mit isalpha() true sind, kriegen wir ärger. */
	for (i=0;i!=3;++i) {
		if (toupper(umlaute[i])!=(int)umlaute[i+3]) {
			++errorlevel;
		}
	}
	for (i=0;umlaute[i]!=0;++i) {
		if (!isalpha(umlaute[i]) || isspace(umlaute[i]) || iscntrl(umlaute[i])) {
			++errorlevel;
		}
	}
	if (errorlevel) return false;
	return true;
}
