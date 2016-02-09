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
#include <kernel/config.h>
#include "key.h"

#include <kernel/save.h>
#include <util/attrib.h>
#include <storage.h>

#include <stdlib.h>
#include <assert.h>

static void a_writekeys(const attrib *a, const void *o, storage *store) {
    int i, *keys = (int *)a->data.v;
    for (i = 0; i != keys[0]; ++i) {
        WRITE_INT(store, keys[i]);
    }
}

static int a_readkeys(attrib * a, void *owner, struct storage *store) {
    int i, *p = 0;
    READ_INT(store, &i);
    assert(i < 4096 && i>0);
    a->data.v = p = malloc(sizeof(int)*(i + 1));
    *p++ = i;
    while (--i) {
        READ_INT(store, p++);
    }
    return AT_READ_OK;
}

static int a_readkey(attrib *a, void *owner, struct storage *store) {
    int res = a_readint(a, owner, store);
    return (res != AT_READ_FAIL) ? AT_READ_DEPR : res;
}

attrib_type at_keys = {
    "keys",
    NULL,
    NULL,
    NULL,
    a_writekeys,
    a_readkeys,
    NULL
};

void a_upgradekeys(attrib **alist, attrib *abegin) {
    int n = 0, *keys = 0;
    int i = 0, val[4];
    attrib *a, *ak = a_find(*alist, &at_keys);
    if (!ak) {
        ak = a_add(alist, a_new(&at_keys));
        keys = (int *)ak->data.v;
        n = keys ? keys[0] : 0;
    }
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        val[i++] = a->data.i;
        if (i == 4) {
            keys = realloc(keys, sizeof(int) * (n + i + 1));
            memcpy(keys + n + 1, val, sizeof(int)*i);
            n += i;
        }
    }
    if (i > 0) {
        keys = realloc(keys, sizeof(int) * (n + i + 1));
        memcpy(keys + n + 1, val, sizeof(int)*i);
    }
    keys[0] = n + i;
}

attrib_type at_key = {
    "key",
    NULL,
    NULL,
    NULL,
    a_writeint,
    a_readkey,
    a_upgradekeys
};

static attrib *make_key(int key)
{
    attrib *a = a_new(&at_key);
    assert(key != 0);
    a->data.i = key;
    return a;
}

static attrib *find_key(attrib * alist, int key)
{
    attrib *a = a_find(alist, &at_key);
    assert(key != 0);
    while (a && a->type == &at_key && a->data.i != key) {
        a = a->next;
    }
    return (a && a->type == &at_key) ? a : NULL;
}

void key_set(attrib ** alist, int key)
{
    attrib *a;
    assert(key != 0);
    a = find_key(*alist, key);
    if (!a) {
        a = a_add(alist, make_key(key));
    }
}

void key_unset(attrib ** alist, int key)
{
    attrib *a;
    assert(key != 0);
    a = find_key(*alist, key);
    if (a) {
        a_remove(alist, a);
    }
}

bool key_get(attrib *alist, int key) {
    attrib *a;
    assert(key != 0);
    a = find_key(alist, key);
    return a != NULL;
}
