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
#include <eressea.h>
#include "synonym.h"

#include <attrib.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

static void
a_initsynonym(struct attrib * a)
{
	a->data.v = calloc(sizeof(frace_synonyms), 1);
}

static void
a_finalizesynonym(struct attrib * a)
{
	frace_synonyms *frs= (frace_synonyms *)a->data.v;

	free((char *)frs->synonyms[0]);
	free((char *)frs->synonyms[1]);
	free((char *)frs->synonyms[2]);
	free((char *)frs->synonyms[3]);

	free(a->data.v);
}

static void
a_writesynonym(const attrib *a, FILE *F)
{
	frace_synonyms *frs= (frace_synonyms *)a->data.v;

	fputs(frs->synonyms[0],F);
	fputc(' ', F);
	fputs(frs->synonyms[1],F);
	fputc(' ', F);
	fputs(frs->synonyms[2],F);
	fputc(' ', F);
	fputs(frs->synonyms[3],F);
	fputc(' ', F);
}

static int
a_readsynonym(attrib * a, FILE * F)
{
	char lbuf[32];
	frace_synonyms *frs= (frace_synonyms *)a->data.v;

	fscanf(F, "%s", lbuf);
	frs->synonyms[0] = strdup(lbuf);
	fscanf(F, "%s", lbuf);
	frs->synonyms[1] = strdup(lbuf);
	fscanf(F, "%s", lbuf);
	frs->synonyms[2] = strdup(lbuf);
	fscanf(F, "%s", lbuf);
	frs->synonyms[3] = strdup(lbuf);

	return AT_READ_OK;
}

attrib_type at_synonym = {
	"synonym",
	a_initsynonym,
	a_finalizesynonym,
	NULL,
	a_writesynonym,
	a_readsynonym
};

void
init_synonym(void)
{
	at_register(&at_synonym);
}
