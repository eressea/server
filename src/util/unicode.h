/*
Copyright (c) 1998-2015, Enno Rehling <enno@eressea.de>
Katja Zedel <katze@felidae.kn-bremen.de
Christian Schlittchen <corwin@amber.kn-bremen.de>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
**/

#ifndef _UNICODE_H
#define _UNICODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <wchar.h>
#define USE_UNICODE
    typedef unsigned long ucs4_t;
    typedef char utf8_t;

    int unicode_utf8_to_cp437(char *result, const utf8_t * utf8_string,
        size_t * length);
    int unicode_utf8_to_cp1252(char *result, const utf8_t * utf8_string,
        size_t * length);
    int unicode_utf8_to_ucs4(ucs4_t * result, const utf8_t * utf8_string,
        size_t * length);
    int unicode_ucs4_to_utf8(utf8_t * result, size_t * size,
        ucs4_t ucs4_character);
    int unicode_utf8_to_ascii(char *cp_character, const utf8_t * utf8_string,
        size_t *length);
    int unicode_utf8_strcasecmp(const utf8_t * a, const utf8_t * b);
    int unicode_latin1_to_utf8(utf8_t * out, size_t * outlen,
        const char *in, size_t * inlen);
    int unicode_utf8_tolower(utf8_t * out, size_t outlen,
        const utf8_t * in);

#ifdef __cplusplus
}
#endif
#endif
