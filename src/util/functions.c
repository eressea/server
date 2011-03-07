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
#include "functions.h"

/* libc includes */
#include <stdlib.h>
#include <string.h>



typedef struct function_list {
  struct function_list *next;
  pf_generic fun;
  const char *name;
} function_list;

static function_list *functionlist;

pf_generic get_function(const char *name)
{
  function_list *fl = functionlist;

  if (name == NULL)
    return NULL;
  while (fl && strcmp(fl->name, name) != 0)
    fl = fl->next;
  if (fl)
    return fl->fun;
  return NULL;
}

const char *get_functionname(pf_generic fun)
{
  function_list *fl = functionlist;

  while (fl && fl->fun != fun)
    fl = fl->next;
  if (fl)
    return fl->name;
  return NULL;
}

void register_function(pf_generic fun, const char *name)
{
  function_list *fl = (function_list *) malloc(sizeof(function_list));

  fl->next = functionlist;
  fl->fun = fun;
  fl->name = strdup(name);
  functionlist = fl;
}

void list_registered_functions(void)
{
  function_list *fl = functionlist;

  while (fl) {
    printf("%s\n", fl->name);
    fl = fl->next;
  }
}
