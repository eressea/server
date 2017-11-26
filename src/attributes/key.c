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
#include <util/log.h>
#include <storage.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void a_writekeys(const attrib *a, const void *o, storage *store) {
    int i, *keys = (int *)a->data.v;
    int n = 0;
    if (keys) {
        assert(keys[0] < 4096 && keys[0]>0);
        n = keys[0];
    }
    WRITE_INT(store, n);
    for (i = 0; i < n; ++i) {
        WRITE_INT(store, keys[i * 2 + 1]);
        WRITE_INT(store, keys[i * 2 + 2]);
    }
}

static int keys_lower_bound(int *base, int k, int l, int r) {
    int km = k;
    int *p = base + 1;

    while (l != r) {
        int m = (l + r) / 2;
        km = p[m * 2];
        if (km < k) {
            if (l == m) l = r;
            else l = m;
        }
        else {
            if (r == m) r = l;
            else r = m;
        }
    }
    return l;
}

static int keys_size(int n) {
    /* TODO maybe use log2 from https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog */
    assert(n > 0 && n <= 4096);
    if (n <= 1) return 1;
    if (n <= 4) return 4;
    if (n <= 8) return 8;
    if (n <= 16) return 16;
    if (n <= 256) return 256;
    if (n <= 512) return 512;
    if (n <= 1024) return 1024;
    if (n <= 2048) return 2048;
    return 4096;
}

static int a_readkeys(attrib * a, void *owner, gamedata *data) {
    int i, n, *keys;

    READ_INT(data->store, &n);
    assert(n < 4096 && n >= 0);
    if (n == 0) {
        return AT_READ_FAIL;
    }
    keys = malloc(sizeof(int)*(keys_size(n) * 2 + 1));
    *keys = n;
    for (i = 0; i != n; ++i) {
        READ_INT(data->store, keys + i * 2 + 1);
        if (data->version >= KEYVAL_VERSION) {
            READ_INT(data->store, keys + i * 2 + 2);
        }
        else {
            keys[i * 2 + 2] = 1;
        }
    }
    if (data->version < SORTKEYS_VERSION) {
        int e = 1;
        for (i = 1; i != n; ++i) {
            int k = keys[i * 2 + 1];
            int v = keys[i * 2 + 2];
            int l = keys_lower_bound(keys, k, 0, e);
            if (l != e) {
                int km = keys[l * 2 + 1];
                if (km == k) {
                    int vm = keys[l * 2 + 2];
                    if (v != vm) {
                        log_error("key %d has values %d and %d", k, v, vm);
                    }
                    --e;
                }
                else {
                    if (e > l) {
                        memmove(keys + 2 * l + 3, keys + 2 * l + 1, (e - l) * 2 * sizeof(int));
                    }
                    keys[2 * l + 1] = k;
                    keys[2 * l + 2] = v;
                }
            }
            ++e;
        }
        if (e != n) {
            int sz = keys_size(n);
            if (e > sz) {
                int *k;
                sz = keys_size(e);
                k = realloc(keys, sizeof(int)*(2 * sz + 1));
                if (!k) {
                    free(keys);
                    abort();
                }
                keys = k;
                keys[0] = e;
            }
        }
    }
    a->data.v = keys;
    return AT_READ_OK;
}

static int a_readkey(attrib *a, void *owner, struct gamedata *data) {
    int res = a_readint(a, owner, data);
    if (data->version >= KEYVAL_VERSION) {
        return AT_READ_FAIL;
    }
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
    attrib *a, *ak;

    ak = a_find(*alist, &at_keys);
    if (ak) alist = &ak;
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        key_set(alist, a->data.i, 1);
    }
}

attrib_type at_key = {
    "key",
    NULL,
    NULL,
    NULL,
    NULL,
    a_readkey,
    a_upgradekeys
};

static int* keys_get(int *base, int i)
{
    int n = base[0];
    assert(i >= 0 && i < n);
    return base + 1 + i * 2;
}

static int *keys_update(int *base, int key, int val)
{
    int *kv;
    int n = base[0];
    int l = keys_lower_bound(base, key, 0, n);
    if (l < n) {
        kv = keys_get(base, l);
        if (kv[0] == key) {
            kv[1] = val;
        }
        else {
            int sz = keys_size(n);
            assert(kv[0] > key);
            if (n + 1 > sz) {
                ptrdiff_t diff = kv - base;
                sz = keys_size(n + 1);
                base = realloc(base, (sz * 2 + 1) * sizeof(int));
                kv = base + diff;
            }
            base[0] = n + 1;
            memmove(kv + 2, kv, 2 * sizeof(int) * (n - l));
            kv[0] = key;
            kv[1] = val;
        }
    }
    else {
        int sz = keys_size(n);
        if (n + 1 > sz) {
            sz = keys_size(n + 1);
            base = realloc(base, (sz * 2 + 1) * sizeof(int));
        }
        base[0] = n + 1;
        kv = keys_get(base, l);
        kv[0] = key;
        kv[1] = val;
    }
    return base;
}

void key_set(attrib ** alist, int key, int val)
{
    int *keys;
    attrib *a;
    assert(key != 0);
    a = a_find(*alist, &at_keys);
    if (!a) {
        a = a_add(alist, a_new(&at_keys));
    }
    keys = (int *)a->data.v;
    if (!keys) {
        int sz = keys_size(1);
        a->data.v = keys = malloc((2 * sz + 1) * sizeof(int));
        keys[0] = 1;
        keys[1] = key;
        keys[2] = val;
    }
    else {
        a->data.v = keys = keys_update(keys, key, val);
        assert(keys[0] < 4096 && keys[0] >= 0);
    }
}

void key_unset(attrib ** alist, int key)
{
    attrib *a;
    assert(key != 0);
    a = a_find(*alist, &at_keys);
    if (a) {
        int *keys = (int *)a->data.v;
        if (keys) {
            int n = keys[0];
            int l = keys_lower_bound(keys, key, 0, n);
            if (l < n) {
                int *kv = keys_get(keys, l);
                if (kv[0] == key) {
                    kv[1] = 0; /* do not delete, just set to 0 */
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
        int *keys = (int *)a->data.v;
        if (keys) {
            int n = keys[0];
            int l = keys_lower_bound(keys, key, 0, n);
            if (l < n) {
                int * kv = keys_get(keys, l);
                if (kv[0] == key) {
                    return kv[1];
                }
            }
        }
    }
    return 0;
}
