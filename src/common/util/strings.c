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

/* libc includes */
#include <string.h>

INLINE_FUNCTION unsigned int
hashstring(const char* s)
{
  unsigned int key = 0;
  while (*s) {
    key = key*37 + *s++;
  }
  return key & 0x7FFFFFFF;
}

INLINE_FUNCTION const char *
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

