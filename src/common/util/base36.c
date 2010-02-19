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
#include "base36.h"

#include <stdlib.h>
#include <assert.h>

#include <ctype.h>
int
atoi36(const char * str)
{
  /* cannot use strtol, because invalid strings will cause crash */
  const unsigned char * s = (const unsigned char *)str;
  int i = 0, sign = 1;
  assert(s);
  if(!(*s)) return 0;

  while(isxspace(*(unsigned char*)s)) ++s;
  if (*s == '-') {
    sign = -1;
    ++s;
  }
  while(isalnum(*(unsigned char*)s)) {
    if (isupper(*(unsigned char*)s)) i = i*36 + (*s)-'A' + 10;
    else if (islower(*(unsigned char*)s)) i=i*36 + (*s)-'a' + 10;
    else if (isdigit(*(unsigned char*)s)) i=i*36 + (*s)-'0';
    else
      break;
    ++s;
  }
  if (i<0) return 0;
  return i * sign;
}

const char*
itoab(int i, int base)
{
  static char **as = NULL; /* STATIC_RETURN: used for return, not across calls */
  char * s, * dst;
  static int index = 0; /* STATIC_XCALL: used across calls */
  int neg = 0;

  if (!as) {
    int j;
    char * x = (char*)calloc(sizeof(char), 8*4); /* STATIC_LEAK: malloc in static variable */
    as = (char **)calloc(sizeof(char*), 4);
    for (j=0;j!=4;++j) as[j] = x+j*8;
  }
  s = as[index];
  index = (index+1) & 3; /* quick for % 4 */
  dst = s+7;
  (*dst--)=0;
  if (i!=0) {
    if (i<0) {
      i=-i;
      neg = 1;
    }
    while (i) {
      int x = i % base;
      i = i / base;
      if (x<10) *(dst--) = (char)('0' + x);
      else if ('a' + x - 10 == 'l') *(dst--) = 'L';
      else *(dst--) = (char)('a' + (x-10));
    }
    if (neg) *(dst) = '-';
    else ++dst;
  }
  else *dst = '0';

  return dst;
}

const char*
itoa36(int i)
{
  return itoab(i, 36);
}

const char*
itoa10(int i)
{
  return itoab(i, 10);
}

int
i10toi36(int i)
{
  int r = 0;
  while(i) {
    r = r*36 + i % 10;
    i = i / 10;
  }
  return r;
}
