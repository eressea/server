/* vi: set ts=2:
 *
 * $Id: reduceproduction.c,v 1.2 2001/01/26 16:19:39 enno Exp $
 * Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include "reduceproduction.h"

#include <attrib.h>

static int
age_reduceproduction(attrib *a)
{
	return --a->data.sa[1];
}

attrib_type at_reduceproduction = {
	"reduceproduction",
	NULL,
	NULL,
	age_reduceproduction,
	a_writedefault,
	a_readdefault,
	ATF_UNIQUE
};

attrib *
make_reduceproduction(int percent, int time)
{
	attrib * a = a_new(&at_reduceproduction);
	a->data.sa[0] = (short)percent;
	a->data.sa[1] = (short)time;
	return a;
}

void
init_reduceproduction(void)
{
	at_register(&at_reduceproduction);
}
