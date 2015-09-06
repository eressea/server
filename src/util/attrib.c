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
#include "attrib.h"

#include "log.h"
#include "storage.h"

#include <critbit.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MAXATHASH 61
static attrib_type *at_hash[MAXATHASH];

static unsigned int __at_hashkey(const char *s)
{
    int key = 0;
    size_t i = strlen(s);

    while (i > 0) {
        key = (s[--i] + key * 37);
    }
    return key & 0x7fffffff; //TODO: V112 http://www.viva64.com/en/V112 Dangerous magic number 0x7fffffff used: return key & 0x7fffffff;.
}

void at_register(attrib_type * at)
{
    attrib_type *find;

    if (at->read == NULL) {
        log_warning("registering non-persistent attribute %s.\n", at->name); //TODO: V111 http://www.viva64.com/en/V111 Call of function 'log_warning' with variable number of arguments. Second argument has memsize type.
    }
    at->hashkey = __at_hashkey(at->name);
    find = at_hash[at->hashkey % MAXATHASH];
    while (find && at->hashkey != find->hashkey) {
        find = find->nexthash;
    }
    if (find && find == at) {
        log_warning("attribute '%s' was registered more than once\n", at->name); //TODO: V111 http://www.viva64.com/en/V111 Call of function 'log_warning' with variable number of arguments. Second argument has memsize type.
        return;
    }
    else {
        assert(!find || !"hashkey is already in use");
    }
    at->nexthash = at_hash[at->hashkey % MAXATHASH];
    at_hash[at->hashkey % MAXATHASH] = at;
}

static attrib_type *at_find(unsigned int hk)
{
    const char *translate[3][2] = {
        { "zielregion", "targetregion" },     /* remapping: from 'zielregion, heute targetregion */
        { "verzaubert", "curse" },    /* remapping: früher verzaubert, jetzt curse */
        { NULL, NULL }
    };
    attrib_type *find = at_hash[hk % MAXATHASH];
    while (find && hk != find->hashkey)
        find = find->nexthash;
    if (!find) {
        int i = 0;
        while (translate[i][0]) {
            if (__at_hashkey(translate[i][0]) == hk)
                return at_find(__at_hashkey(translate[i][1]));
            ++i;
        }
    }
    return find;
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
    while (a && a->type != at)
        a = a->nexttype;
    return a;
}

const attrib *a_findc(const attrib * a, const attrib_type * at)
{
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

static void a_free(attrib * a)
{
    const attrib_type *at = a->type;
    if (at->finalize)
        at->finalize(a);
    free(a);
}

int a_remove(attrib ** pa, attrib * a)
{
    int ok;
    assert(a != NULL);
    ok = a_unlink(pa, a);
    if (ok)
        a_free(a);
    return ok;
}

void a_removeall(attrib ** pa, const attrib_type * at)
{
    attrib **pnexttype = pa;
    attrib **pnext = NULL;

    while (*pnexttype) {
        attrib *next = *pnexttype;
        if (next->type == at)
            break;
        pnexttype = &next->nexttype;
        pnext = &next->next;
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

attrib *a_new(const attrib_type * at)
{
    attrib *a = (attrib *)calloc(1, sizeof(attrib));
    assert(at != NULL);
    a->type = at;
    if (at->initialize)
        at->initialize(a);
    return a;
}

int a_age(attrib ** p)
{
    attrib **ap = p;
    /* Attribute altern, und die Entfernung (age()==0) eines Attributs
     * hat Einfluß auf den Besitzer */
    while (*ap) {
        attrib *a = *ap;
        if (a->type->age) {
            int result = a->type->age(a);
            assert(result >= 0 || !"age() returned a negative value");
            if (result == 0) {
                a_remove(p, a);
                continue;
            }
        }
        ap = &a->next;
    }
    return (*p != NULL);
}

static critbit_tree cb_deprecated = { 0 };

void at_deprecate(const char * name, int(*reader)(attrib *, void *, struct storage *))
{
    char buffer[64];
    size_t len = strlen(name);
    len = cb_new_kv(name, len, &reader, sizeof(reader), buffer);
    cb_insert(&cb_deprecated, buffer, len);
}

int a_read(struct storage *store, attrib ** attribs, void *owner)
{
    int key, retval = AT_READ_OK;
    char zText[128];

    zText[0] = 0;
    key = -1;
    READ_TOK(store, zText, sizeof(zText));
    if (strcmp(zText, "end") == 0)
        return retval;
    else
        key = __at_hashkey(zText);

    while (key != -1) {
        int(*reader)(attrib *, void *, struct storage *) = 0;
        attrib_type *at = at_find(key);
        attrib * na = 0;

        if (at) {
            reader = at->read;
            na = a_new(at);
        }
        else {
            void * kv = 0;
            cb_find_prefix(&cb_deprecated, zText, strlen(zText) + 1, &kv, 1, 0);
            if (kv) {
                cb_get_kv(kv, &reader, sizeof(reader));
            }
            else {
                fprintf(stderr, "attribute hash: %d (%s)\n", key, zText);
                assert(at || !"attribute not registered");
            }
        }
        if (reader) {
            int i = reader(na, owner, store);
            if (na) {
                switch (i) {
                case AT_READ_OK:
                    a_add(attribs, na);
                    break;
                case AT_READ_FAIL:
                    retval = AT_READ_FAIL;
                    a_free(na);
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

        READ_TOK(store, zText, sizeof(zText));
        if (!strcmp(zText, "end"))
            break;
        key = __at_hashkey(zText);
    }
    return retval;
}

void a_write(struct storage *store, const attrib * attribs, const void *owner)
{
    const attrib *na = attribs;

    while (na) {
        if (na->type->write) {
            assert(na->type->hashkey || !"attribute not registered");
            WRITE_TOK(store, na->type->name);
            na->type->write(na, owner, store);
            na = na->next;
        }
        else {
            na = na->nexttype;
        }
    }
    WRITE_TOK(store, "end");
}

void free_attribs(void) {
    cb_clear(&cb_deprecated);
}
