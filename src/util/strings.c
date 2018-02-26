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

#ifdef _MSC_VER
#include <platform.h>
#endif
#include "strings.h"
#include "assert.h"

/* libc includes */
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#ifdef HAVE_LIBBSD
#include <bsd/string.h>
#else
#include <string.h>
#endif

size_t str_strlcpy(char *dst, const char *src, size_t len)
{
#ifdef HAVE_BSDSTRING
    return strlcpy(dst, src, len);
#else
    register char *d = dst;
    register const char *s = src;
    register size_t n = len;

    assert(src);
    assert(dst);
    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (len != 0)
            *d = '\0';                /* NUL-terminate dst */
        while (*s++);
    }

    return (s - src - 1);         /* count does not include NUL */
#endif
}

size_t str_strlcat(char *dst, const char *src, size_t len)
{
#ifdef HAVE_BSDSTRING
    return strlcat(dst, src, len);
#else
    register char *d = dst;
    register const char *s = src;
    register size_t n = len;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while (*d != '\0' && n-- != 0)
        d++;
    dlen = d - dst;
    n = len - dlen;

    if (n == 0)
        return (dlen + strlen(s));
    while (*s != '\0') {
        if (n != 1) {
            *d++ = *s;
            n--;
        }
        s++;
    }
    *d = '\0';

    return (dlen + (s - src));    /* count does not include NUL */
#endif
}

size_t str_slprintf(char * dst, size_t size, const char * format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    result = vsnprintf(dst, size, format, args);
    va_end(args);
    if (result < 0 || result >= (int)size) {
        dst[size - 1] = '\0';
        return size;
    }

    return (size_t)result;
}

void str_replace(char *buffer, size_t size, const char *tmpl, const char *var, const char *value)
{
    size_t val_len = strlen(value);
    size_t var_len = strlen(var);
    char *s = buffer;
    while (buffer + size > s) {
        char *p = strstr(tmpl, var);
        size_t len;
        if (p) {
            len = p - tmpl;
        }
        else {
            len = strlen(tmpl);
        }
        if (len < size) {
            memmove(s, tmpl, len);
            tmpl += len;
            s += len;
            size -= len;
            if (p && val_len < size) {
                tmpl += var_len;
                memmove(s, value, val_len);
                s += val_len;
                size -= val_len;
            }
        }
        if (!p) {
            break;
        }
    }
    *s = 0;
}

unsigned int str_hash(const char *s)
{
    unsigned int key = 0;
    assert(s);
    while (*s) {
        key = key * 37 + *s++;
    }
    return key & 0x7FFFFFFF;
}

const char *str_escape(const char *str, char *buffer,
    size_t len)
{
    const char *start = strchr(str, '\"');
    if (!start) start = strchr(str, '\\');
    assert(buffer);
    if (start) {
        const char *p;
        char *o;
        size_t skip = start - str;

        if (skip > len) {
            skip = len;
        }
        memcpy(buffer, str, skip);
        o = buffer + skip;
        p = str + skip;
        len -= skip;
        do {
            if (*p == '\"' || *p == '\\') {
                if (len < 2) {
                    *o = '\0';
                    break;
                }
                (*o++) = '\\';
                len -= 2;
            }
            else {
                if (len < 1) {
                    *o = '\0';
                    break;
                }
                --len;
            }
            (*o++) = (*p);
        } while (*p++);
        return buffer;
    }
    return str;
}

unsigned int jenkins_hash(unsigned int a)
{
    a = (a + 0x7ed55d16) + (a << 12);
    a = (a ^ 0xc761c23c) ^ (a >> 19);
    a = (a + 0x165667b1) + (a << 5);
    a = (a + 0xd3a2646c) ^ (a << 9);
    a = (a + 0xfd7046c5) + (a << 3);
    a = (a ^ 0xb55a4f09) ^ (a >> 16);
    return a;
}

unsigned int wang_hash(unsigned int a)
{
    a = ~a + (a << 15);           /*  a = (a << 15) - a - 1; */
    a = a ^ (a >> 12);
    a = a + (a << 2);
    a = a ^ (a >> 4);
    a = a * 2057;                 /*  a = (a + (a << 3)) + (a << 11); */
    a = a ^ (a >> 16);
    return a;
}

char *str_strdup(const char *s) {
#ifdef HAVE_STRDUP
    return strdup(s);
#elif defined(_MSC_VER)
    return _strdup(s);
#else
    size_t len = strlen(s);
    char *dup = malloc(len+1);
    memcpy(dup, s, len+1);
    return dup;
#endif
}

void sbs_init(struct sbstring *sbs, char *buffer, size_t size)
{
    assert(sbs);
    assert(size>0);
    sbs->begin = buffer;
    sbs->size = size;
    sbs->end = buffer;
    buffer[0] = '\0';
}

void sbs_strncat(struct sbstring *sbs, const char *str, size_t size)
{
    size_t len;
    assert(sbs);
    len = sbs->size - (sbs->end - sbs->begin) - 1;
    if (len < size) {
        size = len;
    }
    memcpy(sbs->end, str, size);
    sbs->end[size] = '\0';
    sbs->end += size;
}

void sbs_strcat(struct sbstring *sbs, const char *str)
{
    size_t len;
    assert(sbs);
    len = sbs->size - (sbs->end - sbs->begin);
    len = str_strlcpy(sbs->end, str, len);
    sbs->end += len;
}

void sbs_strcpy(struct sbstring *sbs, const char *str)
{
    size_t len = str_strlcpy(sbs->begin, str, sbs->size);
    if (len >= sbs->size) {
        len = sbs->size - 1;
    }
    sbs->end = sbs->begin + len;
}
