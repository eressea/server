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
#include <kernel/eressea.h>
#include "orcification.h"

#include <kernel/save.h>
#include <util/attrib.h>

/*
 * simple attributes that do not yet have their own file 
 */

attrib_type at_orcification = {
	"orcification", NULL, NULL, NULL, a_writeint, a_readint, ATF_UNIQUE
};

attrib *
make_orcification(int orcification)
{
	attrib * a = a_new(&at_orcification);
	a->data.i = orcification;
	return a;
}

void
init_orcification(void)
{
	at_register(&at_orcification);
}
