/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "xmas.h"

/* kernel includes */
#include <unit.h>
#include <building.h>
#include <region.h>
#include <event.h>
#include <movement.h>
#include <faction.h>
#include <item.h>
#include <race.h>

/* util includes */
#include <base36.h>
#include <goodies.h>

/* libc includes */
#include <stdlib.h>

static int
xmasgate_handle(trigger * t, void * data)
{
	return -1;
}

static void
xmasgate_write(const trigger * t, FILE * F)
{
	building *b = (building *)t->data.v;
	fprintf(F, "%s ", itoa36(b->no));
}

static int
xmasgate_read(trigger * t, FILE * F)
{
	return read_building_reference((building**)&t->data.v, F);
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

