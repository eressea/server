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

#ifndef LISTS_H
#define LISTS_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

    typedef struct strlist {
        struct strlist *next;
        char *s;
    } strlist;

    void addstrlist(strlist ** SP, const char *s);
    void freestrlist(strlist * s);
    void addlist(void *l1, void *p1);
    void translist(void *l1, void *l2, void *p);
    void freelist(void *p1);
    void removelist(void *l, void *p);
    unsigned int listlen(void *l);

#ifdef __cplusplus
}
#endif
#endif
