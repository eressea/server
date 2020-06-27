#include <platform.h>
#include <kernel/config.h>
#include "key.h"

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <storage.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void a_writekeys(const variant *var, const void *o, storage *store) {
    int i, *keys = (int *)var->v;
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

static int read_flags(gamedata *data, int *keys, int n) {
    int i;
    for (i = 0; i != n; ++i) {
        int key;
        READ_INT(data->store, &key);
        keys[i * 2] = key;
        keys[i * 2 + 1] = 1;
    }
    return n;
}

#ifdef KEYVAL_VERSION
static int read_keyval(gamedata *data, int *keys, int n) {
    int i;
    for (i = 0; i != n; ++i) {
        int key, val;
        READ_INT(data->store, &key);
        READ_INT(data->store, &val);
        keys[i * 2] = key;
        keys[i * 2 + 1] = val;
    }
    return n;
}
#endif

#ifdef FIXATKEYS_VERSION
static int read_keyval_orig(gamedata *data, int *keys, int n) {
    int i, j = 0, dk = -1;
    for (i = 0; i != n; ++i) {
        int key, val;
        READ_INT(data->store, &key);
        READ_INT(data->store, &val);
        if (key > dk) {
            keys[j * 2] = key;
            keys[j * 2 + 1] = val;
            dk = key;
            ++j;
        }
    }
    return j;
}
#endif

static int a_readkeys(variant *var, void *owner, gamedata *data) {
    int n, ksn, *keys;

    READ_INT(data->store, &n);
    assert(n < 4096 && n >= 0);
    if (n == 0) {
        return AT_READ_FAIL;
    }
    ksn = keys_size(n);
    keys = malloc((ksn * 2 + 1) * sizeof(int));
    if (data->version >= FIXATKEYS_VERSION) {
        n = read_keyval(data, keys + 1, n);
    }
    else if (data->version >= KEYVAL_VERSION) {
        int m = read_keyval_orig(data, keys + 1, n);
        if (n != m) {
            int ksm = keys_size(m);
            if (ksm != ksn) {
                int *nkeys = (int *)realloc(keys, (ksm * 2 + 1) * sizeof(int));
                if (nkeys != NULL) {
                    keys = nkeys;
                }
                else {
                    log_error("a_readkeys allocation failed: %s", strerror(errno));
                    return AT_READ_FAIL;
                }
            }
            n = m;
        }
    }
    else {
        n = read_flags(data, keys + 1, n);
    }
    keys[0] = n;
    if (data->version < SORTKEYS_VERSION) {
        int i, e = 1;
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
    var->v = keys;
    return AT_READ_OK;
}

static int a_readkey(variant *var, void *owner, struct gamedata *data) {
    int res = a_readint(var, owner, data);
    if (data->version >= KEYVAL_VERSION) {
        return AT_READ_FAIL;
    }
    return (res != AT_READ_FAIL) ? AT_READ_DEPR : res;
}

attrib_type at_keys = {
    "keys",
    NULL,
    a_free_voidptr,
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
                int *tmp;
                ptrdiff_t diff = kv - base;
                sz = keys_size(n + 1);
                tmp = realloc(base, (sz * 2 + 1) * sizeof(int));
                if (!tmp) abort();
                base = tmp;
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
            void * tmp;
            sz = keys_size(n + 1);
            tmp = realloc(base, (sz * 2 + 1) * sizeof(int));
            if (!tmp) abort();
            base = (int *)tmp;
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
        if (!keys) abort();
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
