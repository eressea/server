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
  while (*s) {
    key = key*37 + *s++;
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
  if (neu==NULL) {
    free(*s);
    *s = NULL;
  } else if (*s == NULL) {
    *s = malloc(strlen(neu)+1);
    strcpy(*s, neu);
  } else {
    *s = realloc(*s, strlen(neu) + 1);
    strcpy(*s, neu);
  }
	return *s;
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

static int 
spc_email_isvalid(const char *address) 
{
  int        count = 0;
  const char *c, *domain;
  static const char *rfc822_specials = "()<>@,;:\\\"[]";

  /* first we validate the name portion (name@domain) */
  for (c = address;  *c;  c++) {
    if (*c == '\"' && (c == address || *(c - 1) == '.' || *(c - 1) == 
      '\"')) {
        while (*++c) {
          if (*c == '\"') break;
          if (*c == '\\' && (*++c == ' ')) continue;
          if (*c <= ' ' || *c >= 127) return 0;
        }
        if (!*c++) return 0;
        if (*c == '@') break;
        if (*c != '.') return 0;
        continue;
      }
      if (*c == '@') break;
      if (*c <= ' ' || *c >= 127) return 0;
      if (strchr(rfc822_specials, *c)) return 0;
  }
  if (c == address || *(c - 1) == '.') return 0;

  /* next we validate the domain portion (name@domain) */
  if (!*(domain = ++c)) return 0;
  do {
    if (*c == '.') {
      if (c == domain || *(c - 1) == '.') return 0;
      count++;
    }
    if (*c <= ' ' || *c >= 127) return 0;
    if (strchr(rfc822_specials, *c)) return 0;
  } while (*++c);

  return (count >= 1);
}

int 
set_email(char** pemail, const char *newmail)
{
  if (newmail && *newmail) {
    if (spc_email_isvalid(newmail)<=0) return -1;
  }
  if (*pemail) free(*pemail);
  *pemail = 0;
  if (newmail) {
    *pemail = strdup(newmail);
  }
  return 0;
}
