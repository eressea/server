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

#include <platform.h>

/* libc includes */
#include <string.h>
#include <assert.h>

unsigned int hashstring(const char *s)
{
    unsigned int key = 0;
    while (*s) {
        key = key * 37 + *s++;
    }
    return key & 0x7FFFFFFF;
}

const char *escape_string(const char *str, char *buffer,
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
