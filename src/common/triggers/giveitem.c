/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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
#include <kernel/eressea.h>
#include "giveitem.h"

/* kernel includes */
#include <kernel/item.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/storage.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/***
 ** give an item to someone
 **/

typedef struct giveitem_data {
	struct unit * u;
	const struct item_type * itype;
	int number;
} giveitem_data;

static void
giveitem_init(trigger * t)
{
	t->data.v = calloc(sizeof(giveitem_data), 1);
}

static void
giveitem_free(trigger * t)
{
	free(t->data.v);
}

static int
giveitem_handle(trigger * t, void * data)
{
  /* call an event handler on giveitem.
  * data.v -> ( variant event, int timer )
  */
  giveitem_data * td = (giveitem_data*)t->data.v;
  if (td->u && td->u->number) {
    i_change(&td->u->items, td->itype, td->number);
  } else {
    log_error(("could not perform giveitem::handle()\n"));
  }
  unused(data);
  return 0;
}

static void
giveitem_write(const trigger * t, struct storage * store)
{
	giveitem_data * td = (giveitem_data*)t->data.v;
    write_unit_reference(td->u, store);
    store->w_int(store, td->number);
    store->w_tok(store, td->itype->rtype->_name[0]);
}

static int
giveitem_read(trigger * t, struct storage * store)
{
  giveitem_data * td = (giveitem_data*)t->data.v;
  char zText[128];

  int result = read_reference(&td->u, store, read_unit_reference, resolve_unit);

  td->number = store->r_int(store);
  store->r_tok_buf(store, zText, sizeof(zText));
  td->itype = it_find(zText);
  assert(td->itype);

  if (result==0 && td->u==NULL) {
    return AT_READ_FAIL;
  }
  return AT_READ_OK;
}

trigger_type tt_giveitem = {
	"giveitem",
	giveitem_init,
	giveitem_free,
	giveitem_handle,
	giveitem_write,
	giveitem_read
};

trigger *
trigger_giveitem(unit * u, const item_type * itype, int number)
{
	trigger * t = t_new(&tt_giveitem);
	giveitem_data * td = (giveitem_data*)t->data.v;
	td->number = number;
	td->u = u;
	td->itype = itype;
	return t;
}
