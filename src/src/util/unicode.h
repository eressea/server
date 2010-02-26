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
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef _UNICODE_H
#define _UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>
#define USE_UNICODE
  typedef unsigned long ucs4_t;
  typedef char utf8_t;

  extern int unicode_utf8_to_cp437(char *result, const utf8_t *utf8_string, size_t *length);
  extern int unicode_utf8_to_cp1252(char *result, const utf8_t *utf8_string, size_t *length);
  extern int unicode_utf8_to_ucs4(ucs4_t *result, const utf8_t *utf8_string, size_t *length);
  extern int unicode_ucs4_to_utf8 (utf8_t *result, size_t *size, ucs4_t ucs4_character);
  extern int unicode_utf8_strcasecmp(const utf8_t * a, const utf8_t * b);
  extern int unicode_latin1_to_utf8(utf8_t *out, size_t *outlen, const char *in, size_t *inlen);
  extern int unicode_utf8_tolower(utf8_t *out, size_t outlen, const utf8_t *in);

#ifdef __cplusplus
}
#endif
#endif
