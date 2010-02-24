/* vi: set ts=2:
*
*
* Eressea PB(E)M host Copyright (C) 1998-2003
*      Christian Schlittchen (corwin@amber.kn-bremen.de)
*      Katja Zedel (katze@felidae.kn-bremen.de)
*      Henning Peters (faroul@beyond.kn-bremen.de)
*      Enno Rehling (enno@eressea.de)
*      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
*
* This program may not be used, modified or distributed without
* prior permission by the authors of Eressea.
*/

#include <platform.h>
#include "follow.h"

#include <kernel/config.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/storage.h>
#include <util/variant.h>

static int
read_follow(attrib * a, void * owner, struct storage * store)
{
  read_unit_reference(store); /* skip it */
  return AT_READ_FAIL;
}

attrib_type at_follow = {
  "follow", NULL, NULL, NULL, NULL, read_follow
};

attrib *
make_follow(struct unit * u)
{
  attrib * a = a_new(&at_follow);
  a->data.v = u;
  return a;
}
