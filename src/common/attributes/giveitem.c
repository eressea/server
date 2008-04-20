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

#include <config.h>
#include "giveitem.h"
#include <kernel/eressea.h>

/* kernel includes */
#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/item.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/resolve.h>
#include <util/goodies.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

typedef struct give_data {
	struct building * building;
	struct item * items;
} give_data;

static void
a_writegive(const attrib * a, FILE * F)
{
	give_data * gdata = (give_data*)a->data.v;
	item * itm;
	fprintf(F, "%s ", buildingid(gdata->building));
	for (itm=gdata->items;itm;itm=itm->next) {
		fprintf(F, "%s %d ", resourcename(itm->type->rtype, 0), itm->number);
	}
	fputs("end ", F);
}

static int
a_readgive(attrib * a, FILE * F)
{
  give_data * gdata = (give_data*)a->data.v;
  variant var;
  int result;
  char zText[32];

  result = fscanf(F, "%s ", zText);
  if (result<0) return result;
  var.i = atoi36(zText);
  gdata->building = findbuilding(var.i);
  if (gdata->building==NULL) ur_add(var, (void**)&gdata->building, resolve_building);
  for (;;) {
    int i;
    result = fscanf(F, "%s", zText);
    if (result==EOF) return EOF;
    if (!strcmp("end", zText)) break;
    result = fscanf(F, "%d", &i);
    if (result==EOF) return EOF;
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
give_item(attrib * a)
{
	give_data * gdata = (give_data*)a->data.v;
	region * r;
	unit * u;
	if (gdata->building==NULL || gdata->items==NULL) return 0;
	r = gdata->building->region;
	u = buildingowner(r, gdata->building);
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
	give_item,
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
