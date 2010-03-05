/* vi: set ts=2:
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
#include <kernel/config.h>
#include "otherfaction.h"

#include <kernel/faction.h>
#include <kernel/unit.h>
#include <util/attrib.h>
#include <util/storage.h>
#include <assert.h>

/*
 * simple attributes that do not yet have their own file 
 */


void 
write_of(const struct attrib * a, const void * owner, struct storage * store)
{
  const faction * f = (faction*)a->data.v;
  store->w_int(store, f->no);
}

int
read_of(struct attrib * a, void * owner, struct storage * store) /* return 1 on success, 0 if attrib needs removal */
{
  int of = store->r_int(store);
  a->data.v = findfaction(of);
  if (a->data.v) return AT_READ_OK;
  return AT_READ_FAIL;
}

attrib_type at_otherfaction = {
  "otherfaction", NULL, NULL, NULL, write_of, read_of, ATF_UNIQUE
};

struct faction *
  get_otherfaction(const struct attrib * a)
{
  return (faction*)(a->data.v);
}

struct attrib * 
  make_otherfaction(struct faction * f)
{
  attrib * a = a_new(&at_otherfaction);
  a->data.v = (void*)f;
  return a;
}

faction *
visible_faction(const faction *f, const unit * u)
{
  if (f==NULL || !alliedunit(u, f, HELP_FSTEALTH)) {
    attrib *a = a_find(u->attribs, &at_otherfaction);
    if (a) {
      faction *fv = get_otherfaction(a);
      assert (fv != NULL);	/* fv should never be NULL! */
      return fv;
    }
  }
  return u->faction;
}
