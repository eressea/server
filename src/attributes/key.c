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
#include <util/gamedata.h>
#include <storage.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void a_writekeys(const attrib *a, const void *o, storage *store) {
    int i, *keys = (int *)a->data.v;
    for (i = 0; i <= keys[0]; ++i) {
        WRITE_INT(store, keys[i]);
    }
}

static int a_readkeys(attrib * a, void *owner, gamedata *data) {
    int i, *p = 0;
    READ_INT(data->store, &i);
    assert(i < 4096 && i>0);
    a->data.v = p = malloc(sizeof(int)*(i + 1));
    *p++ = i;
    while (i--) {
        READ_INT(data->store, p++);
    }
    return AT_READ_OK;
}

static int a_readkey(attrib *a, void *owner, struct gamedata *data) {
    int res = a_readint(a, owner, data);
    return (res != AT_READ_FAIL) ? AT_READ_DEPR : res;
}

static void a_freekeys(attrib *a) {
    free(a->data.v);
}

attrib_type at_keys = {
    "keys",
    NULL,
    a_freekeys,
    NULL,
    a_writekeys,
    a_readkeys,
    NULL
};

void a_upgradekeys(attrib **alist, attrib *abegin) {
    int n = 0, *keys = 0;
    int i = 0, val[4];
    attrib *a, *ak = a_find(*alist, &at_keys);
    if (ak) {
        keys = (int *)ak->data.v;
        if (keys) n = keys[0];
    }
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        val[i++] = a->data.i;
        if (i == 4) {
            keys = realloc(keys, sizeof(int) * (n + i + 1));
            memcpy(keys + n + 1, val, sizeof(int)*i);
            n += i;
            i = 0;
        }
    }
    if (i > 0) {
        keys = realloc(keys, sizeof(int) * (n + i + 1));
        memcpy(keys + n + 1, val, sizeof(int)*i);
        if (!ak) {
            ak = a_add(alist, a_new(&at_keys));
        }
    }
    ak->data.v = keys;
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

void key_set(attrib ** alist, int key)
{
    int *keys, n = 1;
    attrib *a;
    assert(key != 0);
    a = a_find(*alist, &at_keys);
    if (!a) {
        a = a_add(alist, a_new(&at_keys));
    }
    keys = (int *)a->data.v;
    if (keys) {
        n = keys[0] + 1;
    }
    keys = realloc(keys, sizeof(int) *(n + 1));
    // TODO: does insertion sort pay off here?
    keys[0] = n;
    keys[n] = key;
    a->data.v = keys;
}

void key_unset(attrib ** alist, int key)
{
    attrib *a;
    assert(key != 0);
    a = a_find(*alist, &at_keys);
    if (a) {
        int i, *keys = (int *)a->data.v;
        if (keys) {
            for (i = 1; i <= keys[0]; ++i) {
                if (keys[i] == key) {
                    keys[i] = keys[keys[0]];
                    keys[0]--;
                }
            }
        }
    }
}

bool key_get(attrib *alist, int key) {
    attrib *a;
    assert(key != 0);
    a = a_find(alist, &at_keys);
    if (a) {
        int i, *keys = (int *)a->data.v;
        if (keys) {
            for (i = 1; i <= keys[0]; ++i) {
                if (keys[i] == key) {
                    return true;
                }
            }
        }
    }
    return false;
}
