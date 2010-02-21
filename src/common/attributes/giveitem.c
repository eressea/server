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
#include "giveitem.h"
#include <kernel/config.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/item.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/resolve.h>
#include <util/storage.h>
#include <util/goodies.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

typedef struct give_data {
	struct building * building;
	struct item * items;
} give_data;

static void
a_writegive(const attrib * a, struct storage * store)
{
	give_data * gdata = (give_data*)a->data.v;
	item * itm;
    write_building_reference(gdata->building, store);
	for (itm=gdata->items;itm;itm=itm->next) {
      store->w_tok(store, resourcename(itm->type->rtype, 0));
      store->w_int(store, itm->number);
	}
	store->w_tok(store, "end");
}

static int
a_readgive(attrib * a, struct storage * store)
{
  give_data * gdata = (give_data*)a->data.v;
  variant var;
  char zText[32];

  var.i = store->r_id(store);
  if (var.i>0) {
    gdata->building = findbuilding(var.i);
    if (gdata->building==NULL) {
      ur_add(var, &gdata->building, resolve_building);
    }
  } else {
    gdata->building=NULL;
  }
  for (;;) {
    int i;
    store->r_tok_buf(store, zText, sizeof(zText));
    if (!strcmp("end", zText)) break;
    i = store->r_int(store);
    if (i==0) i_add(&gdata->items, i_new(it_find(zText), i));
  }
  return AT_READ_OK;
}

static void
a_initgive(struct attrib * a)
{
	a->data.v = calloc(sizeof(give_data), 1);
}

static void
a_finalizegive(struct attrib * a)
{
	free(a->data.v);
}

static int
a_giveitem(attrib * a)
{
	give_data * gdata = (give_data*)a->data.v;
	region * r;
	unit * u;
	if (gdata->building==NULL || gdata->items==NULL) return 0;
	r = gdata->building->region;
	u = building_owner(gdata->building);
	if (u==NULL) return 1;
	while (gdata->items) {
		item * itm = gdata->items;
		i_change(&u->items, itm->type, itm->number);
		i_free(i_remove(&gdata->items, itm));
	}
	return 0;
}

attrib_type at_giveitem = {
	"giveitem",
	a_initgive, a_finalizegive,
	a_giveitem,
	a_writegive, a_readgive
};

attrib *
make_giveitem(struct building * b, struct item * ip)
{
	attrib * a = a_new(&at_giveitem);
	give_data * gd = (give_data*)a->data.v;
	gd->building = b;
	gd->items = ip;
	return a;
}

void
init_giveitem(void)
{
	at_register(&at_giveitem);
}
