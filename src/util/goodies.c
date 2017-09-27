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
#include "goodies.h"

/* libc includes */
#include <stdlib.h>
#include <string.h>

/* Simple Integer-Liste */

int *intlist_init(void)
{
    return (calloc(1, sizeof(int)));
}

int *intlist_add(int *i_p, int i)
{
    i_p[0]++;
    i_p = realloc(i_p, (i_p[0] + 1) * sizeof(int));

    i_p[i_p[0]] = i;
    return (i_p);
}

int *intlist_find(int *i_p, int fi)
{
    int i;

    for (i = 1; i <= i_p[0]; i++) {
        if (i_p[i] == fi)
            return (&i_p[i]);
    }
    return NULL;
}

static int spc_email_isvalid(const char *address)
{
    int count = 0;
    const char *c, *domain;
    static const char *rfc822_specials = "()<>@,;:\\\"[]";        /* STATIC_CONST: contains a constant value */

    /* first we validate the name portion (name@domain) */
    for (c = address; *c; c++) {
        if (*c == '\"' && (c == address || *(c - 1) == '.' || *(c - 1) == '\"')) {
            while (*++c) {
                if (*c == '\"')
                    break;
                if (*c == '\\' && (*++c == ' '))
                    continue;
                if (*c <= ' ' || *c >= 127)
                    return 0;
            }
            if (!*c++)
                return 0;
            if (*c == '@')
                break;
            if (*c != '.')
                return 0;
            continue;
        }
        if (*c == '@')
            break;
        if (*c <= ' ' || *c >= 127)
            return 0;
        if (strchr(rfc822_specials, *c))
            return 0;
    }
    if (*c!='@') {
        /* no @ symbol */
        return -1;
    }
    domain = ++c;
    if (!*c) {
        return -1;
    }
    if (c == address || *(c - 1) == '.')
        return 0;

    /* next we validate the domain portion (name@domain) */
    do {
        if (*c == '.') {
            if (c == domain || *(c - 1) == '.')
                return 0;
            count++;
        }
        if (*c <= ' ' || *c >= 127)
            return 0;
        if (strchr(rfc822_specials, *c))
            return 0;
    } while (*++c);

    return (count >= 1);
}

int set_email(char **pemail, const char *newmail)
{
    if (newmail && *newmail) {
        if (spc_email_isvalid(newmail) <= 0)
            return -1;
    } else {
      return -1;
    }
    if (*pemail)
        free(*pemail);
    *pemail = 0;
    if (newmail) {
        *pemail = strdup(newmail);
    }
    return 0;
}
