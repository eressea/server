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
#include "dict.h"
#include "key.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

/* util includes */
#include <kernel/attrib.h>
#include <util/base36.h>
#include <util/log.h>
#include <kernel/gamedata.h>
#include <util/resolve.h>
#include <util/strings.h>

#include <storage.h>

/* stdc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef enum dict_type {
    TNONE = 0, TINTEGER = 1, TREAL = 2
} dict_type;

typedef struct dict_data {
    dict_type type;
    char *name;
    union {
        int i;
        double real;
    } data;
} dict_data;

static int dict_read(variant * var, void *owner, gamedata *data)
{
    storage *store = data->store;
    char name[NAMESIZE];
    dict_data *dd = (dict_data *)var->v;
    int n;

    READ_STR(store, name, sizeof(name));
    dd->name = str_strdup(name);
    READ_INT(store, &n);
    dd->type = (dict_type)n;
    if (dd->type == TINTEGER) {
        READ_INT(store, &dd->data.i);
    }
    else if (dd->type == TREAL) {
        float flt;
        READ_FLT(store, &flt);
        dd->data.real = flt;
    }
    else {
        log_error("read dict, invalid type %d", n);
        return AT_READ_FAIL;
    }
    return AT_READ_DEPR;
}

static void dict_init(variant *var)
{
    dict_data *dd;
    var->v = malloc(sizeof(dict_data));
    dd = (dict_data *)var->v;
    dd->type = TNONE;
}

static void dict_done(variant *var)
{
    dict_data *dd = (dict_data *)var->v;
    free(dd->name);
    free(var->v);
}

static void upgrade_keyval(const dict_data *dd, int keyval[], int v) {
    if (strcmp(dd->name, "embassy_muschel") == 0) {
        keyval[0] = atoi36("mupL");
        keyval[1] = v;
    }
    else {
        log_error("dict conversion, bad entry %s", dd->name);
    }
}

static void dict_upgrade(attrib **alist, attrib *abegin) {
    int n = 0, *keys = 0;
    int i = 0, val[8];
    attrib *a, *ak = a_find(*alist, &at_keys);
    if (ak) {
        keys = (int *)ak->data.v;
        if (keys) n = keys[0];
    }
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        dict_data *dd = (dict_data *)a->data.v;
        if (dd->type == TINTEGER) {
            upgrade_keyval(dd, val + i * 2, dd->data.i);
            ++i;
        }
        else if (dd->type == TREAL) {
            upgrade_keyval(dd, val + i * 2, (int)dd->data.real);
            ++i;
        }
        else {
            log_error("dict conversion, bad type %d for %s", dd->type, dd->name);
            assert(!"invalid input");
        }
        if (i == 4) {
            int *k;
            k = realloc(keys, sizeof(int) * (n + i + 1));
            if (!k) {
                free(keys);
                abort();
            }
            keys = k;
            memcpy(keys + n + 1, val, sizeof(val));
            n += i;
            i = 0;
        }
    }
    if (i > 0) {
        keys = realloc(keys, sizeof(int) * (2 * (n + i) + 1));
        memcpy(keys + n*2 + 1, val, sizeof(int)*i*2);
        if (!ak) {
            ak = a_add(alist, a_new(&at_keys));
        }
    }
    if (ak) {
        ak->data.v = keys;
        if (keys) {
            keys[0] = n + i;
        }
    }
}

attrib_type at_dict = {
    "object", dict_init, dict_done, NULL,
    NULL, dict_read, dict_upgrade
};

void dict_set(attrib * a, const char * name, int value)
{
    dict_data *dd = (dict_data *)a->data.v;
    dd->name = str_strdup(name);
    dd->type = TINTEGER;
    dd->data.i = value;
}
