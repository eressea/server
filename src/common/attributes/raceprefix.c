/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "raceprefix.h"

#include <util/attrib.h>

#include <assert.h>
#include <string.h>

attrib_type at_raceprefix = { 
  "raceprefix", NULL, a_finalizestring, NULL, a_writestring, a_readstring, ATF_UNIQUE
};

void
set_prefix(attrib ** ap, const char * str)
{
  attrib * a = a_find(*ap, &at_raceprefix);
  if (a==NULL) {
    a = a_add(ap, a_new(&at_raceprefix));
  } else {
    free(a->data.v);
  }
  assert(a->type==&at_raceprefix);
  a->data.v = strdup(str);
}

const char * 
get_prefix(const attrib * a)
{
  char * str;
  a = a_findc(a, &at_raceprefix);
  if (a==NULL) return NULL;
  str = (char *)a->data.v;
  /* conversion of old prefixes */
  if (strncmp(str, "prefix_", 7)==0) {
    ((attrib*)a)->data.v = strdup(str+7);
    free(str);
    str = (char *)a->data.v;
  }
  return str;
}
