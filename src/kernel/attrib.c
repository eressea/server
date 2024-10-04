#include "attrib.h"

#include <util/log.h>
#include <util/variant.h>
#include <kernel/gamedata.h>

#include <critbit.h>
#include <storage.h>
#include <strings.h>

#include <stb_ds.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

int read_attribs(gamedata *data, attrib **alist, void *owner) {
    int result;
    if (data->version < ATHASH_VERSION) {
        result = a_read_orig(data, alist, owner);
    }
    else {
        result = a_read(data, alist, owner);
    }
    if (result == AT_READ_DEPR) {
        /* handle deprecated attributes */
        attrib **ap = alist;
        while (*ap) {
            attrib *a = *ap;
            if (a->type->upgrade) {
                a->type->upgrade(alist, a);
            }
            ap = &a->nexttype;
        }
    }
    return result;
}

void write_attribs(storage *store, attrib *alist, const void *owner)
{
    a_write(store, alist, owner);
}

int a_readint(variant * var, void *owner, struct gamedata *data)
{
    int n;
    READ_INT(data->store, &n);
    var->i = n;
    return AT_READ_OK;
}

void a_writeint(const variant * var, const void *owner, struct storage *store)
{
    WRITE_INT(store, var->i);
}

int a_readshorts(variant * var, void *owner, struct gamedata *data)
{
    int n;
    READ_INT(data->store, &n);
    var->sa[0] = (short)n;
    READ_INT(data->store, &n);
    var->sa[1] = (short)n;
    return AT_READ_OK;
}

void a_writeshorts(const variant * var, const void *owner, struct storage *store)
{
    WRITE_INT(store, var->sa[0]);
    WRITE_INT(store, var->sa[1]);
}

int a_readchars(variant * var, void *owner, struct gamedata *data)
{
    int i;
    for (i = 0; i != 4; ++i) {
        int n;
        READ_INT(data->store, &n);
        var->ca[i] = (char)n;
    }
    return AT_READ_OK;
}

void a_writechars(const variant * var, const void *owner, struct storage *store)
{
    int i;

    for (i = 0; i != 4; ++i) {
        WRITE_INT(store, var->ca[i]);
    }
}

#define DISPLAYSIZE 8192
int a_readstring(variant * var, void *owner, struct gamedata *data)
{
    char buf[DISPLAYSIZE];

    READ_STR(data->store, buf, sizeof(buf));
    var->v = str_strdup(buf);
    return AT_READ_OK;
}

void a_writestring(const variant * var, const void *owner, struct storage *store)
{
    const char* str = (const char*)var->v;
    assert(var && var->v);
    assert(strlen(str) < DISPLAYSIZE);
    WRITE_STR(store, str);
}

void a_finalizestring(variant * var)
{
    free(var->v);
}

static struct at_hash_t {
    unsigned int key;
    attrib_type *value;
} *at_hash;

static unsigned int __at_hashkey(const char *s)
{
    int key = 0;
    size_t i = strlen(s);

    while (i > 0) {
        key = (s[--i] + key * 37);
    }
    return key & 0x7fffffff;
}

void at_register(attrib_type * at)
{
    unsigned int hash = __at_hashkey(at->name);
#ifndef NDEBUG
    ptrdiff_t i = hmgeti(at_hash, hash);
    if (i >= 0) {
        if (at == at_hash[i].value) {
            log_error("attribute %s is already registered", at->name);
            return;
        }
        else {
            log_fatal("attributes %s and %s hash to the same key", at->name, at_hash[i].value->name);
        }
    }
#endif
    if (at->read == NULL) {
        log_error("registering non-persistent attribute %s.\n", at->name);
    }
    hmput(at_hash, hash, at);
}

static attrib_type *at_find_key(unsigned int hk)
{
    ptrdiff_t i = hmgeti(at_hash, hk);
    if (i < 0) {
        int k;
        const char *aliases[3][2] = {
            { "zielregion", "targetregion" }, /* remapping: from 'zielregion, heute targetregion */
            { "verzaubert", "curse" },        /* remapping: frueher verzaubert, jetzt curse */
            { NULL, NULL }
        };
        for (k = 0; aliases[k][0]; ++k) {
            if (__at_hashkey(aliases[k][0]) == hk)
                return at_find_key(__at_hashkey(aliases[k][1]));
        }
        return NULL;
    }
    return at_hash[i].value;
}

struct attrib_type *at_find(const char *name) {
    unsigned int hash = __at_hashkey(name);
    return at_find_key(hash);
}

attrib *a_select(attrib * a, const void *data,
    bool(*compare) (const attrib *, const void *))
{
    while (a && !compare(a, data))
        a = a->next;
    return a;
}

attrib *a_find(attrib * a, const attrib_type * at)
{
    assert(at);
    while (a && a->type != at)
        a = a->nexttype;
    return a;
}

static attrib *a_insert(attrib * head, attrib * a)
{
    attrib **pa;
    assert(!(a->type->flags & ATF_UNIQUE));
    assert(head && head->type == a->type);

    pa = &head->next;
    while (*pa && (*pa)->type == a->type) {
        pa = &(*pa)->next;
    }
    a->next = *pa;
    return *pa = a;
}

attrib *a_add(attrib ** pa, attrib * a)
{
    attrib *first = *pa;
    assert(a->next == NULL && a->nexttype == NULL);

    if (first == NULL)
        return *pa = a;
    if (first->type == a->type) {
        return a_insert(first, a);
    }
    for (;;) {
        attrib *next = first->nexttype;
        if (next == NULL) {
            /* the type is not in the list, append it behind the last type */
            attrib **insert = &first->next;
            first->nexttype = a;
            while (*insert)
                insert = &(*insert)->next;
            *insert = a;
            break;
        }
        if (next->type == a->type) {
            return a_insert(next, a);
        }
        first = next;
    }
    return a;
}

static int a_unlink(attrib ** pa, attrib * a)
{
    attrib **pnexttype = pa;
    attrib **pnext = NULL;

    assert(a != NULL);
    while (*pnexttype) {
        attrib *next = *pnexttype;
        if (next->type == a->type)
            break;
        pnexttype = &next->nexttype;
        pnext = &next->next;
    }
    if (*pnexttype && (*pnexttype)->type == a->type) {
        if (*pnexttype == a) {
            *pnexttype = a->next;
            if (a->next != a->nexttype) {
                a->next->nexttype = a->nexttype;
            }
            if (pnext == NULL)
                return 1;
            while (*pnext && (*pnext)->type != a->type)
                pnext = &(*pnext)->next;
        }
        else {
            pnext = &(*pnexttype)->next;
        }
        while (*pnext && (*pnext)->type == a->type) {
            if (*pnext == a) {
                *pnext = a->next;
                return 1;
            }
            pnext = &(*pnext)->next;
        }
    }
    return 0;
}

void a_free_voidptr(union variant *var)
{
    free(var->v);
}

static void a_free(attrib * a)
{
    const attrib_type *at = a->type;
    if (at->finalize)
        at->finalize(&a->data);
    free(a);
}

int a_remove(attrib ** pa, attrib * a)
{
    attrib *head = *pa;
    int ok;

    assert(a != NULL);
    ok = a_unlink(pa, a);
    if (ok) {
        if (head == a) {
            *pa = a->next;
        }
        a_free(a);
    }
    return ok;
}

void a_removeall(attrib ** pa, const attrib_type * at)
{
    attrib **pnexttype = pa;

    if (!at) {
        while (*pnexttype) {
            attrib *a = *pnexttype;
            *pnexttype = a->next;
            a_free(a);
        }
    }
    else {
        attrib **pnext = NULL;
        while (*pnexttype) {
            attrib *a = *pnexttype;
            if (a->type == at)
                break;
            pnexttype = &a->nexttype;
            pnext = &a->next;
        }
        if (*pnexttype && (*pnexttype)->type == at) {
            attrib *a = *pnexttype;

            *pnexttype = a->nexttype;
            if (pnext) {
                while (*pnext && (*pnext)->type != at)
                    pnext = &(*pnext)->next;
                *pnext = a->nexttype;
            }
            while (a && a->type == at) {
                attrib *ra = a;
                a = a->next;
                a_free(ra);
            }
        }
    }
}

attrib *a_new(const attrib_type * at)
{
    attrib *a = (attrib *)calloc(1, sizeof(attrib));
    assert(at != NULL);
    if (!a) abort();
    a->type = at;
    if (at->initialize)
        at->initialize(&a->data);
    return a;
}

int a_age(attrib ** p, void *owner)
{
    attrib **ap = p;
    /* Attribute altern, und die Entfernung (age()==0) eines Attributs
     * hat Einfluss auf den Besitzer */
    while (*ap) {
        attrib *a = *ap;
        if (a->type->age) {
            int result = a->type->age(a, owner);
            assert(result >= 0 || !"age() returned a negative value");
            if (result == AT_AGE_REMOVE) {
                a_remove(p, a);
                continue;
            }
        }
        ap = &a->next;
    }
    return (*p != NULL);
}

static critbit_tree cb_deprecated = { 0 };

typedef struct deprecated_s {
    unsigned int hash;
    int(*reader)(variant *, void *, struct gamedata *);
} deprecated_t;

void at_deprecate(const char * name, int(*reader)(variant *, void *, struct gamedata *))
{
    deprecated_t value;
    
    value.hash = __at_hashkey(name);
    value.reader = reader;
    cb_insert(&cb_deprecated, &value, sizeof(value));
}

static int a_read_i(gamedata *data, attrib ** attribs, void *owner, unsigned int key) {
    int retval = AT_READ_OK;
    int(*reader)(variant *, void *, struct gamedata *) = 0;
    attrib_type *at = at_find_key(key);
    attrib * na = NULL;
    variant var, *vp = &var;

    if (at) {
        reader = at->read;
        na = a_new(at);
        vp = &na->data;
    }
    else {
        void *match;
        if (cb_find_prefix(&cb_deprecated, &key, sizeof(key), &match, 1, 0)>0) {
            deprecated_t *value = (deprecated_t *)match;
            reader = value->reader;
        }
        else {
            log_error("unknown attribute hash: %u\n", key);
            assert(at || !"attribute not registered");
        }
    }
    if (reader) {
        int ret = reader(vp, owner, data);
        if (na) {
            switch (ret) {
            case AT_READ_DEPR:
            case AT_READ_OK:
                if (na) {
                    a_add(attribs, na);
                }
                retval = ret;
                break;
            case AT_READ_FAIL:
                if (na) {
                    a_free(na);
                }
                break;
            default:
                assert(!"invalid return value");
                break;
            }
        }
    }
    else {
        assert(!"error: no registered callback can read attribute");
    }
    return retval;
}

int a_read(gamedata *data, attrib ** attribs, void *owner) {
    struct storage *store = data->store;
    int key, retval = AT_READ_OK;

    key = -1;
    READ_INT(store, &key);
    while (key > 0) {
        int ret = a_read_i(data, attribs, owner, key);
        if (ret == AT_READ_DEPR) {
            retval = AT_READ_DEPR;
        }
        READ_INT(store, &key);
    }
    return retval;
}

int a_read_orig(gamedata *data, attrib ** attribs, void *owner)
{
    int key, retval = AT_READ_OK;
    char zText[128];

    zText[0] = 0;
    key = -1;
    READ_TOK(data->store, zText, sizeof(zText));
    if (strcmp(zText, "end") == 0) {
        return retval;
    }
    else {
        key = __at_hashkey(zText);
    }
    while (key > 0) {
        retval = a_read_i(data, attribs, owner, key);
        READ_TOK(data->store, zText, sizeof(zText));
        if (!strcmp(zText, "end"))
            break;
        key = __at_hashkey(zText);
    }
    return retval;
}

void a_write(struct storage *store, const attrib * attribs, const void *owner) {
    const attrib *na = attribs;

    while (na) {
        if (na->type->write) {
            unsigned int hashkey = __at_hashkey(na->type->name);
            WRITE_INT(store, hashkey);
            na->type->write(&na->data, owner, store);
            na = na->next;
        }
        else {
            na = na->nexttype;
        }
    }
    WRITE_INT(store, 0);
}

void attrib_done(void) {
    cb_clear(&cb_deprecated);
    hmfree(at_hash);
}
