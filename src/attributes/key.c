#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "key.h"

#include <kernel/attrib.h>
#include <kernel/gamedata.h>
#include <util/log.h>
#include <storage.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct keys_data {
    unsigned int count;
    struct key_value {
        unsigned int key;
        int value;
    } data[];
} keys_data;

static void a_writekeys(const variant *var, const void *o, storage *store) {
    unsigned int i;
    keys_data * keys = (keys_data*)var->v;
    unsigned int n = 0;
    if (keys) {
        assert(keys->count <= 4096 && keys->count>0);
        n = (unsigned int)keys->count;
    }
    WRITE_INT(store, n);
    for (i = 0; i < n; ++i) {
        WRITE_UINT(store, keys->data[i].key);
        WRITE_INT(store, keys->data[i].value);
    }
}

static unsigned int keys_lower_bound(keys_data const *keys, unsigned int k, unsigned int l, unsigned int r) {
    unsigned int km = k;
    struct key_value const* p = keys->data;

    while (l != r) {
        unsigned int m = (l + r) / 2;
        km = p[m].key;
        if (km == k) {
            return m;
        }
        else if (k < km) {
            r = m;
        }
        else {
            l = m + 1;
        }
    }
    return l;
}

static unsigned int keys_size(unsigned int n) {
    /* TODO maybe use log2 from https://graphics.stanford.edu/~seander/bithacks.html#IntegerLog */
    assert(n <= 4096);
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

/**
* Early versions of the game did not have a key/value store, but just a list
* of keys that could be set (flags).
*/
static int read_flags(gamedata *data, struct key_value* kv, unsigned int n) {
    unsigned int i;
    for (i = 0; i != n; ++i) {
        unsigned int key;
        READ_UINT(data->store, &key);
        kv[i].key = key;
        kv[i].value = 1;
    }
    return n;
}

#ifdef KEYVAL_VERSION
static int read_keyval(gamedata *data, struct key_value *kv, unsigned int n) {
    unsigned int i;
    for (i = 0; i != n; ++i) {
        int key, val;
        READ_UINT(data->store, &key);
        READ_INT(data->store, &val);
        kv[i].key = key;
        kv[i].value = val;
    }
    return n;
}
#endif

#ifdef FIXATKEYS_VERSION
static unsigned int read_keyval_orig(gamedata *data, struct key_value *kv, unsigned int n) {
    unsigned int i, j = 0;
    int dk = -1;
    for (i = 0; i != n; ++i) {
        int key, val;
        READ_UINT(data->store, &key);
        READ_INT(data->store, &val);
        if (key > dk) {
            kv[j].key = key;
            kv[j].value = val;
            dk = key;
            ++j;
        }
    }
    return j;
}
#endif

static int key_value_compare(const void *a, const void *b)
{
    struct key_value const* lhs = (struct key_value const*)a;
    struct key_value const* rhs = (struct key_value const*)b;
    if (lhs->key == rhs->key) return 0;
    if (lhs->key > rhs->key) return 1;
    return -1;
}

static void sort_keys(keys_data *keys) {
    qsort(keys->data, keys->count, sizeof(struct key_value), key_value_compare);
}

static keys_data* keys_alloc(unsigned int count)
{
    unsigned int ksn = keys_size(count);
    keys_data* keys = malloc(sizeof(keys_data) + sizeof(struct key_value) * ksn);
    if (keys) {
        keys->count = count;
    }
    return keys;
}

static keys_data* keys_realloc(keys_data* keys, unsigned int count)
{
    unsigned int ksn = keys_size(count);
    keys_data* result = realloc(keys, sizeof(keys_data) + sizeof(struct key_value) * ksn);
    if (result) {
        result->count = count;
    }
    return result;
}

static int a_readkeys(variant *var, void *owner, gamedata *data) {
    unsigned int n;
    keys_data * keys;

    READ_UINT(data->store, &n);
    assert(n <= 4096);
    if (n == 0) {
        return AT_READ_FAIL;
    }
    keys = keys_alloc(n);
    var->v = keys;
    if (keys == NULL) {
        log_error("a_readkeys allocation failed: %s", strerror(errno));
        return AT_READ_FAIL;
    }
    if (data->version >= FIXATKEYS_VERSION) {
        n = read_keyval(data, keys->data, n);
    }
    else if (data->version < KEYVAL_VERSION) {
        n = read_flags(data, keys->data, n);
    }
    else {
        unsigned int m = read_keyval_orig(data, keys->data, n);
        if (n != m) {
            unsigned int ksm = keys_size(m);
            if (ksm < keys->count) {
                keys_data* nkeys = (keys_data *)realloc(keys, sizeof(keys_data) + sizeof(struct key_value) * ksm);
                if (nkeys != NULL) {
                    var->v = nkeys;
                }
                else {
                    log_error("a_readkeys allocation failed: %s", strerror(errno));
                    return AT_READ_FAIL;
                }
            }
            n = m;
        }
    }
    keys->count = n;

    if (keys->count > 1 && data->version < FIXATKEYS_VERSION) {
        sort_keys(keys);
    }
    return AT_READ_OK;
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

static struct key_value* keys_get(keys_data *keys, unsigned int i)
{
    assert(i < keys->count);
    return keys->data + i;
}

static keys_data* keys_insert(keys_data* keys, unsigned int index, unsigned int key, int val)
{
    unsigned int sz = keys_size(keys->count);
    assert(index <= keys->count);
    if (keys->count < sz) {
        ++keys->count;
    }
    else {
        /* a resize is required */
        keys_data* tmp = keys_realloc(keys, keys->count + 1);
        if (tmp == NULL) {
            /* out of memory, cannot insert */
            log_error("keys_insert allocation failed: %s", strerror(errno));
            return NULL;
        }
        keys = tmp;
    }
    /* move the tail and overwrite at l */
    if (index < keys->count) {
        memmove(keys->data + index + 1, keys->data + index, (keys->count - index - 1) * sizeof(struct key_value));
    }
    keys->data[index].key = key;
    keys->data[index].value = val;
    return keys;
}

static keys_data* keys_update(keys_data* keys, unsigned int key, int val)
{
    unsigned int l = keys_lower_bound(keys, key, 0, keys->count);
    if (l < keys->count) {
        struct key_value* kv = keys_get(keys, l);
        if (kv->key == key) {
            kv->value = val;
            return keys;
        }
    }
    /* insert entry at index l */
    return keys_insert(keys, l, key, val);
}

void key_set(attrib ** alist, unsigned int key, int val)
{
    keys_data* keys;
    attrib* a;
    a = a_find(*alist, &at_keys);
    if (!a) {
        a = a_add(alist, a_new(&at_keys));
    }
    keys = (keys_data*)a->data.v;
    if (!keys) {
        a->data.v = keys = keys_alloc(1);
        if (!keys) abort();
        keys->data[0].key = key;
        keys->data[0].value = val;
    }
    else {
        a->data.v = keys = keys_update(keys, key, val);
        assert(keys->count <= 4096);
    }
}

void key_unset(attrib ** alist, unsigned int key)
{
    attrib *a;
    a = a_find(*alist, &at_keys);
    if (a) {
        keys_data *keys = (keys_data*)a->data.v;
        if (keys) {
            unsigned int l = keys_lower_bound(keys, key, 0, keys->count);
            if (l < keys->count) {
                struct key_value *kv = keys_get(keys, l);
                if (kv->key == key) {
                    kv->value = 0; /* do not delete, just set to 0 */
                }
            }
        }
    }
}

int key_get(attrib *alist, unsigned int key) {
    attrib *a;
    a = a_find(alist, &at_keys);
    if (a) {
        keys_data* keys = (keys_data*)a->data.v;
        if (keys) {
            unsigned int l = keys_lower_bound(keys, key, 0, keys->count);
            if (l < keys->count) {
                struct key_value* kv = keys_get(keys, l);
                if (kv->key == key) {
                    return kv->value;
                }
            }
        }
    }
    return 0;
}

static int a_readkey(variant* var, void* owner, struct gamedata* data) {
    int res = a_readint(var, owner, data);
    if (data->version >= KEYVAL_VERSION) {
        return AT_READ_FAIL;
    }
    return (res != AT_READ_FAIL) ? AT_READ_DEPR : res;
}

static void a_upgradekeys(attrib** alist, attrib* abegin) {
    attrib* a, * ak;

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

