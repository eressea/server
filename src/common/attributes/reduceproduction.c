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
#include <eressea.h>
#include "reduceproduction.h"
#include <kernel/save.h>
#include <util/attrib.h>

static int
age_reduceproduction(attrib *a)
{
	int reduce = 100 - (5 * --a->data.sa[1]);
	if (reduce < 10) reduce = 10;
	a->data.sa[0] = (short)reduce;
	return a->data.sa[1];
}

attrib_type at_reduceproduction = {
	"reduceproduction",
	NULL,
	NULL,
	age_reduceproduction,
	a_writeshorts,
	a_readshorts,
	ATF_UNIQUE
};

attrib *
make_reduceproduction(int percent, int time)
{
	attrib * a = a_new(&at_reduceproduction);
	int reduce = 100 - (5 * time);
	if (reduce < 10) reduce = 10;
	a->data.sa[0] = (short)reduce;
	a->data.sa[1] = (short)time;
	return a;
}

void
init_reduceproduction(void)
{
	at_register(&at_reduceproduction);
}
