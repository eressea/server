/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "xmas2002.h"

/* kernel includes */
#include <item.h>
#include <unit.h>
#include <region.h>

/* gamecode includes */
#include "xmas.h"

/* util includes */
#include <goodies.h>
#include <resolve.h>
#include <base36.h>

#include <stdlib.h>

static void
presents(unit * senior)
{
	item_type * itype;
	/* Geschenke für alle */
	/* itype = olditemtype[(rand() % 4) + I_KEKS]; */
	if (rand () % 4){
		itype = it_find("snowball");
	} else {
		itype = it_find("snowman");
	}
	assert(itype!=NULL);
	i_change(&senior->items, itype, 1);
}

int
xmas2002(void)
{
	region * r = findregion(0, 0);
	unit * santa = make_santa(r);

	santa_comes_to_town(r, santa, presents);
	return 0;
}
