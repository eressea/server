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
#include "viewrange.h"

/* util includes */
#include <util/attrib.h>
#include <util/functions.h>
#include <util/storage.h>

/* libc includes */
#include <assert.h>

static void
a_writefunction(const struct attrib *a, const void *owner,
  struct storage *store)
{
  const char *str = get_functionname((pf_generic) a->data.f);
  store->w_tok(store, str);
}

static int a_readfunction(struct attrib *a, void *owner, struct storage *store)
/* return 1 on success, 0 if attrib needs removal */
{
  char buf[64];
  store->r_tok_buf(store, buf, sizeof(buf));
  a->data.f = get_function(buf);
  return AT_READ_OK;
}

attrib_type at_viewrange = {
  "viewrange",
  NULL,
  NULL,
  NULL,
  a_writefunction,
  a_readfunction,
};

attrib *add_viewrange(attrib ** alist, const char *function)
{
  attrib *a = a_find(*alist, &at_viewrange);
  if (a == NULL)
    a = a_add(alist, make_viewrange(function));
  return a;
}

attrib *make_viewrange(const char *function)
{
  attrib *a = a_new(&at_viewrange);
  a->data.f = get_function(function);
  assert(a->data.f);
  return a;
}

void init_viewrange(void)
{
  at_register(&at_viewrange);
}
