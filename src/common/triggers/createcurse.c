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
#include <kernel/config.h>
#include "createcurse.h"

/* kernel includes */
#include <kernel/version.h>
#include <kernel/curse.h>
#include <kernel/unit.h>

/* util includes */
#include <util/attrib.h>
#include <util/event.h>
#include <util/log.h>
#include <util/resolve.h>
#include <util/storage.h>
#include <util/base36.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#include <stdio.h>
/***
 ** restore a mage that was turned into a toad
 **/

typedef struct createcurse_data {
	struct unit * mage;
	struct unit * target;
	const curse_type * type;
	double vigour;
	int duration;
	double effect;
	int men;
} createcurse_data;

static void
createcurse_init(trigger * t)
{
	t->data.v = calloc(sizeof(createcurse_data), 1);
}

static void
createcurse_free(trigger * t)
{
	free(t->data.v);
}

static int
createcurse_handle(trigger * t, void * data)
{
  /* call an event handler on createcurse.
  * data.v -> ( variant event, int timer )
  */
  createcurse_data * td = (createcurse_data*)t->data.v;
  if (td->mage && td->target && td->mage->number && td->target->number) {
    create_curse(td->mage, &td->target->attribs,
      td->type, td->vigour, td->duration, td->effect, td->men);
  } else {
    log_error(("could not perform createcurse::handle()\n"));
  }
  unused(data);
  return 0;
}

static void
createcurse_write(const trigger * t, struct storage * store)
{
  createcurse_data * td = (createcurse_data*)t->data.v;
  write_unit_reference(td->mage, store);
  write_unit_reference(td->target, store);
  store->w_tok(store, td->type->cname);
  store->w_flt(store, (float)td->vigour);
  store->w_int(store, td->duration);
  store->w_flt(store, (float)td->effect);
  store->w_int(store, td->men);
}

static int
createcurse_read(trigger * t, struct storage * store)
{
  createcurse_data * td = (createcurse_data*)t->data.v;
  char zText[128];

  read_reference(&td->mage, store, read_unit_reference, resolve_unit);
  read_reference(&td->target, store, read_unit_reference, resolve_unit);

  if (store->version<CURSETYPE_VERSION) {
    int id1, id2;
    id1 = store->r_int(store);
    id2 = store->r_int(store);
    assert(id2==0);
    td->vigour = store->r_flt(store);
    td->duration = store->r_int(store);
    td->effect = store->r_int(store);
    td->men = store->r_int(store);
    td->type = ct_find(oldcursename(id1));
  } else {
    store->r_tok_buf(store, zText, sizeof(zText));
    td->type = ct_find(zText);
    td->vigour = store->r_flt(store);
    td->duration = store->r_int(store);
    if (store->version<CURSEFLOAT_VERSION) {
      td->effect = (double)store->r_int(store);
    } else {
      td->effect = store->r_flt(store);
    }
    td->men = store->r_int(store);
  }
  return AT_READ_OK;
}

trigger_type tt_createcurse = {
	"createcurse",
	createcurse_init,
	createcurse_free,
	createcurse_handle,
	createcurse_write,
	createcurse_read
};

trigger *
trigger_createcurse(struct unit * mage, struct unit * target,
					const curse_type * ct, double vigour, int duration,
					double effect, int men)
{
	trigger * t = t_new(&tt_createcurse);
	createcurse_data * td = (createcurse_data*)t->data.v;
	td->mage = mage;
	td->target = target;
	td->type = ct;
	td->vigour = vigour;
	td->duration = duration;
	td->effect = effect;
	td->men = men;
	return t;
}
