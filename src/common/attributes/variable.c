/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2004
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include "variable.h"

#include <kernel/save.h>

#include <util/attrib.h>
#include <util/storage.h>

/* libc includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct {
	char *key;
	char *value;
} variable;

static void
initialize_variable(struct attrib * a)
{
	a->data.v = malloc(sizeof(variable));
}

static void
finalize_variable(struct attrib * a)
{
	free(a->data.v);
}

static void
write_variable(const struct attrib * a, storage * store)
{
  variable * var = (variable *)a->data.v;
  store->w_tok(store, var->key);
  store->w_str(store, var->value);
}

static int
read_variable(struct attrib *a, storage * store)
{
  variable * var = (variable *)a->data.v;

  var->key = store->r_tok(store);
  var->value = store->r_str(store);

  return AT_READ_OK;
}

attrib_type at_variable = {
	"variable", initialize_variable, finalize_variable, NULL,
		write_variable, read_variable
};

const char *
get_variable(attrib *a, const char *key)
{
  attrib *ap = a_find(a, &at_variable);

  while (ap && ap->type==&at_variable) {
    variable * var = (variable *)ap->data.v;
    if (strcmp(key, var->key) == 0) {
      return var->value;
    }
    ap = ap->next;
  }

  return NULL;
}

void
set_variable(attrib **app, const char *key, const char *value)
{
  attrib *ap = a_find(*app, &at_variable);

  assert(value);

  while (ap && ap->type==&at_variable) {
    variable * var = (variable *)ap->data.v;
    if (strcmp(key, var->key) == 0) {
      free(var->value);
      var->value = strdup(value);
      return;
    }
    ap = ap->next;
  }

  ap = a_add(app, a_new(&at_variable));
  ((variable *)ap->data.v)->key = strdup(key);
  ((variable *)ap->data.v)->value = strdup(value);
}

void
delete_variable(attrib **app, const char *key)
{
  attrib *ap = a_find(*app, &at_variable);

  while (ap && ap->type==&at_variable) {
    variable * var = (variable *)ap->data.v;
    if (strcmp(key, var->key) == 0) {
      free(var->key);
      free(var->value);
      a_remove(app, ap);
      break;
    }
    ap = ap->next;
  }

}

void
init_variable(void)
{
  at_register(&at_variable);
}
