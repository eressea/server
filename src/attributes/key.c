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

#include <util/attrib.h>
#include <util/gamedata.h>
#include <storage.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void a_writekeys(const attrib *a, const void *o, storage *store) {
    int i, *keys = (int *)a->data.v;
    WRITE_INT(store, keys[0]);
    for (i = 0; i < keys[0]; ++i) {
        WRITE_INT(store, keys[i * 2 + 1]);
        WRITE_INT(store, keys[i * 2 + 2]);
    }
}

static int a_readkeys(attrib * a, void *owner, gamedata *data) {
    int i, *p = 0;
    READ_INT(data->store, &i);
    assert(i < 4096 && i>=0);
    if (i == 0) {
        return AT_READ_FAIL;
    }
    a->data.v = p = malloc(sizeof(int)*(i*2 + 1));
    *p++ = i;
    while (i--) {
        READ_INT(data->store, p++);
        if (data->version >= KEYVAL_VERSION) {
            READ_INT(data->store, p++);
        }
        else {
            *p++ = 1;
        }
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

static void a_upgradekeys(attrib **alist, attrib *abegin) {
    int n = 0, *keys = 0;
    int i = 0, val[8];
    attrib *a, *ak = a_find(*alist, &at_keys);
    if (ak) {
        keys = (int *)ak->data.v;
        if (keys) n = keys[0];
    }
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        val[i * 2] = a->data.i;
        val[i * 2 + 1] = 1;
        if (++i == 4) {
            keys = realloc(keys, sizeof(int) * (2 * (n + i) + 1));
            memcpy(keys + 2 * n + 1, val, sizeof(val));
            n += i;
            i = 0;
        }
    }
    if (i > 0) {
        keys = realloc(keys, sizeof(int) * (2 * (n + i) + 1));
        memcpy(keys + 2 * n + 1, val, sizeof(int)*i*2);
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

void key_set(attrib ** alist, int key, int val)
{
    int *keys, n = 0;
    attrib *a;
    assert(key != 0);
    a = a_find(*alist, &at_keys);
    if (!a) {
        a = a_add(alist, a_new(&at_keys));
    }
    keys = (int *)a->data.v;
    if (keys) {
        n = keys[0];
    }
    keys = realloc(keys, sizeof(int) *(2 * n + 3));
    // TODO: does insertion sort pay off here? prob. not.
    keys[0] = n + 1;
    keys[2 * n + 1] = key;
    keys[2 * n + 2] = val;
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
            int n = keys[0];
            for (i = 0; i != n; ++i) {
                if (keys[2 * i + 1] == key) {
                    memmove(keys + 2 * i + 1, keys + 2 * n - 1, 2 * sizeof(int));
                    keys[0]--;
                    break;
                }
            }
        }
    }
}

int key_get(attrib *alist, int key) {
    attrib *a;
    assert(key != 0);
    a = a_find(alist, &at_keys);
    if (a) {
        int i, *keys = (int *)a->data.v;
        if (keys) {
            for (i = 0; i != keys[0]; ++i) {
                if (keys[i*2+1] == key) {
                    return keys[i * 2 + 2];
                }
            }
        }
    }
    return 0;
}
