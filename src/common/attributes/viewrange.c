/* vi: set ts=2:
 *
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
#include "viewrange.h"

/* util includes */
#include <attrib.h>
#include <functions.h>

/* libc includes */
#include <assert.h>

static void 
a_writefunction(const struct attrib * a, FILE* F)
{
	fprintf(F, "%s ", get_functionname((pf_generic)a->data.f));
}

static int 
a_readfunction(struct attrib *a, FILE *F)
/* return 1 on success, 0 if attrib needs removal */
{
	char buf[64];
	fscanf(F, "%s ", buf);
	a->data.f = get_function(buf);
	return AT_READ_OK;
}

attrib_type at_viewrange = {
	"viewrange",
	NULL,
	NULL,
	NULL,
	a_writefunction,
	a_readfunction,
};

attrib * 
add_viewrange(attrib ** alist, const char *function)
{
	attrib * a = a_find(*alist, &at_viewrange);
	if (a==NULL) a = a_add(alist, make_viewrange(function));
	return a;
}

attrib *
make_viewrange(const char *function)
{
	attrib * a = a_new(&at_viewrange);
	a->data.f = get_function(function);
	assert(a->data.f);
	return a;
}

void
init_viewrange(void)
{
	at_register(&at_viewrange);
}
