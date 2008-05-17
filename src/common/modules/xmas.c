/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <kernel/eressea.h>
#include "xmas.h"

/* kernel includes */
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/move.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/unit.h>

/* util includes */
#include <util/base36.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/storage.h>

/* libc includes */
#include <stdlib.h>

static int
xmasgate_handle(trigger * t, void * data)
{
	return -1;
}

static void
xmasgate_write(const trigger * t, struct storage * store)
{
  building *b = (building *)t->data.v;
  store->w_tok(store, itoa36(b->no));
}

static int
xmasgate_read(trigger * t, struct storage * store)
{
  building * b;
  int result =read_building_reference(&b, store);
  t->data.v = b;
  return result;
}

struct trigger_type tt_xmasgate = {
	"xmasgate",
	NULL,
	NULL,
	xmasgate_handle,
	xmasgate_write,
	xmasgate_read
};

trigger *
trigger_xmasgate(building * b)
{
	trigger * t = t_new(&tt_xmasgate);
	t->data.v = b;
	return t;
}

void
init_xmas(void)
{
	tt_register(&tt_xmasgate);
}

