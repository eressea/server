/* vi: set ts=2:
 *
 * 
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
#include "otherfaction.h"

#include <eressea.h>
#include <faction.h>
#include <attrib.h>

/*
 * simple attributes that do not yet have their own file 
 */

attrib_type at_otherfaction = {
	"otherfaction", NULL, NULL, NULL, a_writedefault, a_readdefault, ATF_UNIQUE
};

struct faction *
get_otherfaction(const struct attrib * a)
{
	return findfaction(a->data.i);
}

struct attrib * 
make_otherfaction(const struct faction * f)
{
	attrib * a = a_new(&at_otherfaction);
	a->data.i = f->no;
	return a;
}

void
init_otherfaction(void)
{
	at_register(&at_otherfaction);
}
