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
#include "movement.h"

#include <kernel/save.h>
#include <util/attrib.h>

#include <storage.h>

static void
write_movement(const attrib * a, const void *owner, struct storage *store)
{
  WRITE_INT(store, a->data.i);
}

static int read_movement(attrib * a, void *owner, struct storage *store)
{
  READ_INT(store, &a->data.i);
  if (a->data.i != 0)
    return AT_READ_OK;
  else
    return AT_READ_FAIL;
}

attrib_type at_movement = {
  "movement", NULL, NULL, NULL, write_movement, read_movement
};

bool get_movement(attrib * const *alist, int type)
{
  const attrib *a = a_findc(*alist, &at_movement);
  if (a == NULL)
    return false;
  if (a->data.i & type)
    return true;
  return false;
}

void set_movement(attrib ** alist, int type)
{
  attrib *a = a_find(*alist, &at_movement);
  if (a == NULL)
    a = a_add(alist, a_new(&at_movement));
  a->data.i |= type;
}

static int age_speedup(attrib * a)
{
    if (a->data.sa[0] > 0) {
        a->data.sa[0] = a->data.sa[0] - a->data.sa[1];
    }
    return (a->data.sa[0] > 0) ? AT_AGE_KEEP : AT_AGE_REMOVE;
}

attrib_type at_speedup = {
    "speedup",
    NULL, NULL,
    age_speedup,
    a_writeint,
    a_readint
};

