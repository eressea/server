/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
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


void 
write_of(const struct attrib * a, FILE* F)
{
	const faction * f = (faction*)a->data.v;
	fprintf(F, "%d", f->no);
}

int
read_of(struct attrib * a, FILE* F) /* return 1 on success, 0 if attrib needs removal */
{
	int of;
	fscanf(F, "%d", &of);
	a->data.v = findfaction(of);
	if (a->data.v) return AT_READ_OK;
	return AT_READ_FAIL;
}

attrib_type at_otherfaction = {
	"otherfaction", NULL, NULL, NULL, write_of, read_of, ATF_UNIQUE
};

struct faction *
get_otherfaction(const struct attrib * a)
{
	return (faction*)(a->data.v);
}

struct attrib * 
make_otherfaction(struct faction * f)
{
	attrib * a = a_new(&at_otherfaction);
	a->data.v = (void*)f;
	return a;
}

void
init_otherfaction(void)
{
	at_register(&at_otherfaction);
}
