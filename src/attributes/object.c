/*
Copyright (c) 1998-2010, Enno Rehling <enno@eressea.de>
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
#include "object.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/unit.h>
#include <kernel/version.h>

/* util includes */
#include <util/attrib.h>
#include <util/resolve.h>

#include <storage.h>

/* stdc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct object_data {
  object_type type;
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
} object_data;

static void
object_write(const attrib * a, const void *owner, struct storage *store)
{
  const object_data *data = (object_data *) a->data.v;
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

static int object_read(attrib * a, void *owner, struct storage *store)
{
  char name[NAMESIZE];
  object_data *data = (object_data *) a->data.v;
  int result, n;
  float flt;

  READ_STR(store, name, sizeof(name));
  data->name = _strdup(name);
  READ_INT(store, &n);
  data->type = (object_type)n;
  switch (data->type) {
    case TINTEGER:
      READ_INT(store, &data->data.i);
      break;
    case TREAL:
      READ_FLT(store, &flt);
      if ((int)flt == flt) {
          data->type = TINTEGER;
          data->data.i = (int)flt;
      }
      else {
          data->data.real = flt;
      }
      break;
    case TSTRING:
      READ_STR(store, name, sizeof(name));
      data->data.str = _strdup(name);
      break;
    case TBUILDING:
      result =
        read_reference(&data->data.b, store, read_building_reference,
        resolve_building);
      if (result == 0 && !data->data.b) {
        return AT_READ_FAIL;
      }
      break;
    case TUNIT:
      result =
        read_reference(&data->data.u, store, read_unit_reference, resolve_unit);
      if (result == 0 && !data->data.u) {
        return AT_READ_FAIL;
      }
      break;
    case TFACTION:
      result =
        read_reference(&data->data.f, store, read_faction_reference,
        resolve_faction);
      if (result == 0 && !data->data.f) {
        return AT_READ_FAIL;
      }
      break;
    case TREGION:
      result =
        read_reference(&data->data.r, store, read_region_reference,
        RESOLVE_REGION(global.data_version));
      if (result == 0 && !data->data.r) {
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

static void object_init(attrib * a)
{
  object_data *data;
  a->data.v = malloc(sizeof(object_data));
  data = (object_data *) a->data.v;
  data->type = TNONE;
}

static void object_done(attrib * a)
{
  object_data *data = (object_data *) a->data.v;
  if (data->type == TSTRING)
    free(data->data.str);
  free(data->name);
  free(a->data.v);
}

attrib_type at_object = {
  "object", object_init, object_done, NULL,
  object_write, object_read
};

const char *object_name(const attrib * a)
{
  object_data *data = (object_data *) a->data.v;
  return data->name;
}

struct attrib *object_create(const char *name, object_type type, variant value)
{
  attrib *a = a_new(&at_object);
  object_data *data = (object_data *) a->data.v;
  data->name = _strdup(name);

  object_set(a, type, value);
  return a;
}

void object_set(attrib * a, object_type type, variant value)
{
  object_data *data = (object_data *) a->data.v;

  if (data->type == TSTRING)
    free(data->data.str);
  data->type = type;
  switch (type) {
    case TSTRING:
      data->data.str = value.v ? _strdup(value.v) : NULL;
      break;
    case TINTEGER:
      data->data.i = value.i;
      break;
    case TREAL:
      data->data.real = value.f;
      break;
    case TREGION:
      data->data.r = (region *) value.v;
      break;
    case TBUILDING:
      data->data.b = (building *) value.v;
      break;
    case TFACTION:
      data->data.f = (faction *) value.v;
      break;
    case TUNIT:
      data->data.u = (unit *) value.v;
      break;
    case TSHIP:
      data->data.sh = (ship *) value.v;
      break;
    case TNONE:
      break;
    default:
      assert(!"invalid object-type");
      break;
  }
}

void object_get(const struct attrib *a, object_type * type, variant * value)
{
  object_data *data = (object_data *) a->data.v;
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
