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

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
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

static void
dict_write(const attrib * a, const void *owner, struct storage *store)
{
    const dict_data *data = (dict_data *)a->data.v;
    int type = (int)data->type;
    WRITE_TOK(store, data->name);
    WRITE_INT(store, type);
    switch (data->type) {
    case TINTEGER:
        WRITE_INT(store, data->data.i);
        break;
    case TREAL:
        WRITE_FLT(store, (float)data->data.real);
        break;
    case TSTRING:
        WRITE_STR(store, data->data.str);
        break;
    case TUNIT:
        write_unit_reference(data->data.u, store);
        break;
    case TFACTION:
        write_faction_reference(data->data.f, store);
        break;
    case TBUILDING:
        write_building_reference(data->data.b, store);
        break;
    case TSHIP:
        /* write_ship_reference(data->data.sh, store); */
        assert(!"not implemented");
        break;
    case TREGION:
        write_region_reference(data->data.r, store);
        break;
    case TNONE:
        break;
    default:
        assert(!"illegal type in object-attribute");
    }
}

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
    return AT_READ_OK;
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

attrib_type at_dict = {
    "object", dict_init, dict_done, NULL,
    dict_write, dict_read
};

const char *dict_name(const attrib * a)
{
    dict_data *data = (dict_data *)a->data.v;
    return data->name;
}

struct attrib *dict_create(const char *name, dict_type type, variant value)
{
    attrib *a = a_new(&at_dict);
    dict_data *data = (dict_data *)a->data.v;
    data->name = strdup(name);

    dict_set(a, type, value);
    return a;
}

void dict_set(attrib * a, dict_type type, variant value)
{
    dict_data *data = (dict_data *)a->data.v;

    if (data->type == TSTRING)
        free(data->data.str);
    data->type = type;
    switch (type) {
    case TSTRING:
        data->data.str = value.v ? strdup(value.v) : NULL;
        break;
    case TINTEGER:
        data->data.i = value.i;
        break;
    case TREAL:
        data->data.real = value.f;
        break;
    case TREGION:
        data->data.r = (region *)value.v;
        break;
    case TBUILDING:
        data->data.b = (building *)value.v;
        break;
    case TFACTION:
        data->data.f = (faction *)value.v;
        break;
    case TUNIT:
        data->data.u = (unit *)value.v;
        break;
    case TSHIP:
        data->data.sh = (ship *)value.v;
        break;
    case TNONE:
        break;
    default:
        assert(!"invalid object-type");
        break;
    }
}

void dict_get(const struct attrib *a, dict_type * type, variant * value)
{
    dict_data *data = (dict_data *)a->data.v;
    *type = data->type;
    switch (data->type) {
    case TSTRING:
        value->v = data->data.str;
        break;
    case TINTEGER:
        value->i = data->data.i;
        break;
    case TREAL:
        value->f = (float)data->data.real;
        break;
    case TREGION:
        value->v = data->data.r;
        break;
    case TBUILDING:
        value->v = data->data.b;
        break;
    case TFACTION:
        value->v = data->data.f;
        break;
    case TUNIT:
        value->v = data->data.u;
        break;
    case TSHIP:
        value->v = data->data.sh;
        break;
    case TNONE:
        break;
    default:
        assert(!"invalid object-type");
        break;
    }
}
