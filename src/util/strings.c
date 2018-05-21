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

/* libc includes */
#include <ctype.h>
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

void str_replace(char *buffer, size_t size, const char *tmpl, const char *var,
    const char *value)
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

int str_hash(const char *s)
{
    int key = 0;
    assert(s);
    while (*s) {
        key = key * 37 + *s++;
    }
    return key & 0x7FFFFFFF;
}

const char *str_escape_wrong(const char *str, char *buffer, size_t len)
{
    const char *handle_start = strchr(str, '\"');
    if (!handle_start) handle_start = strchr(str, '\\');
    assert(buffer);
    if (handle_start) {
        const char *p = str;
        char *o = buffer;
        size_t skip = handle_start - str;

        if (skip > len) {
            skip = len;
        }
        if (skip > 0) {
            memcpy(buffer, str, skip);
            o += skip;
            p += skip;
            len -= skip;
        }
        do {
            if (*p == '\"' || *p == '\\') {
                if (len < 2) {
                    break;
                }
                (*o++) = '\\';
                len -= 2;
            }
            else {
                if (len < 1) {
                    break;
                }
                --len;
            }
            if (len > 0) {
                (*o++) = (*p);
            }
        } while (len > 0 && *p++);
        *o = '\0';
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
    if (s == NULL) return NULL;
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

char *str_unescape(char *str) {
    char *read = str, *write = str;
    while (*read) {
        char * pos = strchr(read, '\\');
        if (pos) {
            size_t len = pos - read;
            memmove(write, read, len);
            write += len;
            read += (len + 1);
            switch (read[0]) {
            case 'r':
                *write++ = '\r';
                break;
            case 'n':
                *write++ = '\n';
                break;
            case 't':
                *write++ = '\t';
                break;
            default: 
                *write++ = read[0];
            }
            *write = 0;
            ++read;
        }
        else {
            size_t len = strlen(read);
            memmove(write, read, len);
            write[len] = 0;
            break;
        }
    }
    return str;
}

const char *str_escape_ex(const char *str, char *buffer, size_t size, const char *chars)
{
    const char *read = str;
    char *write = buffer;
    if (size < 1) return NULL;
    while (size > 1 && *read) {
        const char *pos = strpbrk(read, chars);
        size_t len = size;
        if (pos) {
            len = pos - read;
        }
        if (len < size) {
            unsigned char ch = *(const unsigned char *)pos;
            if (len > 0) {
                memmove(write, read, len);
                write += len;
                read += len;
                size -= len;
            }
            switch (ch) {
            case '\t':
                if (size > 2) {
                    *write++ = '\\';
                    *write++ = 't';
                    size -= 2;
                }
                else size = 1;
                break;
            case '\n':
                if (size > 2) {
                    *write++ = '\\';
                    *write++ = 'n';
                    size -= 2;
                }
                else size = 1;
                break;
            case '\r':
                if (size > 2) {
                    *write++ = '\\';
                    *write++ = 'r';
                    size -= 2;
                }
                else size = 1;
                break;
            case '\"':
            case '\'':
            case '\\':
                if (size > 2) {
                    *write++ = '\\';
                    *write++ = ch;
                    size -= 2;
                }
                break;
            default:
                if (size > 5) {
                    int n = sprintf(write, "\\%03o", ch);
                    if (n > 0) {
                        assert(n == 5);
                        write += n;
                        size -= n;
                    }
                    else size = 1;
                }
                else size = 1;
            }
            ++read;
        } else {
            /* end of buffer space */
            len = size - 1;
            if (len > 0) {
                memmove(write, read, len);
                write += len;
                size -= len;
                break;
            }
        }
    }
    *write = '\0';
    return buffer;
}

const char *str_escape(const char *str, char *buffer, size_t size) {
    return str_escape_ex(str, buffer, size, "\n\t\r\'\"\\");
}

const char *str_escape_slow(const char *str, char *buffer, size_t size) {
    const char *read = str;
    char *write = buffer;
    if (size < 1) return NULL;
    while (size > 1 && *read) {
        size_t len;
        const char *pos = read;
        while (pos + 1 < read + size && *pos) {
            unsigned char ch = *(unsigned char *)pos;
            if (iscntrl(ch) || ch == '\"' || ch == '\\' || ch == '\'' || ch == '\n' || ch == '\r' || ch == '\t') {
                len = pos - read;
                memmove(write, read, len);
                write += len;
                size -= len;
                switch (ch) {
                case '\t':
                    if (size > 2) {
                        *write++ = '\\';
                        *write++ = 't';
                        size -= 2;
                    }
                    else size = 1;
                    break;
                case '\n':
                    if (size > 2) {
                        *write++ = '\\';
                        *write++ = 'n';
                        size -= 2;
                    }
                    else size = 1;
                    break;
                case '\r':
                    if (size > 2) {
                        *write++ = '\\';
                        *write++ = 'r';
                        size -= 2;
                    }
                    else size = 1;
                    break;
                case '\"':
                case '\'':
                case '\\':
                    if (size > 2) {
                        *write++ = '\\';
                        *write++ = ch;
                        size -= 2;
                    }
                    break;
                default:
                    if (size > 5) {
                        int n = sprintf(write, "\\%03o", ch);
                        if (n > 0) {
                            assert(n == 5);
                            write += n;
                            size -= n;
                        }
                        else size = 1;
                    }
                    else size = 1;
                }
                assert(size > 0);
                read = pos + 1;
                break;
            }
            ++pos;
        }
        if (read < pos) {
            len = pos - read;
            memmove(write, read, len);
            read = pos;
            write += len;
            size -= len;
        }
    }
    *write = '\0';
    return buffer;
}
