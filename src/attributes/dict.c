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
#include <util/attrib.h>
#include <util/base36.h>
#include <util/log.h>
#include <util/gamedata.h>
#include <util/resolve.h>

#include <storage.h>

/* stdc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct dict_data {
    dict_type type;
    char *name;
    union {
        int i;
        char *str;
        double real;
        struct unit *u;
        struct region *r;
        struct building *b;
        struct ship *sh;
        struct faction *f;
    } data;
} dict_data;

static int dict_read(attrib * a, void *owner, gamedata *data)
{
    storage *store = data->store;
    char name[NAMESIZE];
    dict_data *dd = (dict_data *)a->data.v;
    int result, n;
    float flt;

    READ_STR(store, name, sizeof(name));
    dd->name = strdup(name);
    READ_INT(store, &n);
    dd->type = (dict_type)n;
    switch (dd->type) {
    case TINTEGER:
        READ_INT(store, &dd->data.i);
        break;
    case TREAL:
        READ_FLT(store, &flt);
        if ((int)flt == flt) {
            dd->type = TINTEGER;
            dd->data.i = (int)flt;
        }
        else {
            dd->data.real = flt;
        }
        break;
    case TSTRING:
        READ_STR(store, name, sizeof(name));
        dd->data.str = strdup(name);
        break;
    case TBUILDING:
        result =
            read_reference(&dd->data.b, data, read_building_reference, 
                resolve_building);
        if (result == 0 && !dd->data.b) {
            return AT_READ_FAIL;
        }
        break;
    case TUNIT:
        result =
            read_reference(&dd->data.u, data, read_unit_reference, resolve_unit);
        if (result == 0 && !dd->data.u) {
            return AT_READ_FAIL;
        }
        break;
    case TFACTION:
        result =
            read_reference(&dd->data.f, data, read_faction_reference,
            resolve_faction);
        if (result == 0 && !dd->data.f) {
            return AT_READ_FAIL;
        }
        break;
    case TREGION:
        result =
            read_reference(&dd->data.r, data, read_region_reference,
            RESOLVE_REGION(data->version));
        if (result == 0 && !dd->data.r) {
            return AT_READ_FAIL;
        }
        break;
    case TSHIP:
        /* return read_ship_reference(&data->data.sh, store); */
        assert(!"not implemented");
        break;
    case TNONE:
        break;
    default:
        return AT_READ_FAIL;
    }
    return AT_READ_DEPR;
}

static void dict_init(attrib * a)
{
    dict_data *data;
    a->data.v = malloc(sizeof(dict_data));
    data = (dict_data *)a->data.v;
    data->type = TNONE;
}

static void dict_done(attrib * a)
{
    dict_data *data = (dict_data *)a->data.v;
    if (data->type == TSTRING)
        free(data->data.str);
    free(data->name);
    free(a->data.v);
}

static void dict_upgrade(attrib **alist, attrib *abegin) {
    int n = 0, *keys = 0;
    int i = 0, val[4];
    attrib *a, *ak = a_find(*alist, &at_keys);
    if (ak) {
        keys = (int *)ak->data.v;
        if (keys) n = keys[0];
    }
    for (a = abegin; a && a->type == abegin->type; a = a->next) {
        dict_data *dd = (dict_data *)a->data.v;
        if (dd->type != TINTEGER) {
            log_error("dict conversion, bad type %d for %s", dd->type, dd->name);
        }
        else {
            if (strcmp(dd->name, "embassy_muschel")==0) {
                val[i++] = atoi36("mupL");
            }
            else {
                log_error("dict conversion, bad entry %s", dd->name);
            }
        }
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

attrib_type at_dict = {
    "object", dict_init, dict_done, NULL,
    NULL, dict_read, dict_upgrade
};
