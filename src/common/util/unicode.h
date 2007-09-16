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
  extern int unicode_utf8_to_ucs4(wint_t *ucs4_character, const char *utf8_string, size_t *length);
  extern int unicode_utf8_strcasecmp(const char * a, const char * b);
  extern int unicode_latin1_to_utf8(unsigned char *out, size_t *outlen, const unsigned char *in, size_t *inlen);

#ifdef __cplusplus
}
#endif
#endif
