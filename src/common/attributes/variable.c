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

#include <platform.h>
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

static int
read_variable(struct attrib *a, storage * store)
{
  char * key = store->r_tok(store);
  char * value = store->r_str(store);
  free(key);
  free(value);

  return AT_READ_FAIL;
}

attrib_type at_variable = {
	"variable", NULL, NULL, NULL,
	NULL, read_variable
};
void
init_variable(void)
{
  at_register(&at_variable);
}
