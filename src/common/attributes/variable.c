/* vi: set ts=2:
 *
 * 
 * Eressea PB(E)M host Copyright (C) 1998-2004
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <kernel/eressea.h>
#include <attrib.h>
#include "variable.h"

/* libc includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct {
	char *key;
	char *value;
} variable;

static void
initialize_variable(struct attrib * a)
{
	a->data.v = malloc(sizeof(variable));
}

static void
finalize_variable(struct attrib * a)
{
	free(a->data.v);
}

static void
write_variable(const struct attrib * a, FILE *F)
{
	fwritestr(F, ((variable *)(a->data.v))->key);
	fputc(' ', F);
	fwritestr(F, ((variable *)(a->data.v))->value);
	fputc(' ', F);
}

static int
read_variable(struct attrib *a, FILE *F)
{
	char localBuffer[BUFSIZE];

	freadstr(F, localBuffer, sizeof(localBuffer));
	((variable *)(a->data.v))->key = strdup(localBuffer);

	freadstr(F, localBuffer, sizeof(localBuffer));
	((variable *)(a->data.v))->value = strdup(localBuffer);

	return AT_READ_OK;
}

attrib_type at_variable = {
	"variable", initialize_variable, finalize_variable, NULL,
		write_variable, read_variable
};

const char *
get_variable(attrib *a, const char *key)
{
	attrib *ap;

	for(ap = a_find(a, &at_variable); ap; ap=ap->nexttype) {
		if(strcmp(key, ((variable *)ap->data.v)->key) == 0) {
			break;
		}
	}

	if(ap) {
		return ((variable *)ap->data.v)->value;
	}

	return NULL;
}

void
set_variable(attrib **app, const char *key, const char *value)
{
	attrib *ap;

	assert(value);

	for(ap = a_find(*app, &at_variable); ap; ap=ap->nexttype) {
		if(strcmp(key, ((variable *)ap->data.v)->key) == 0) {
			break;
		}
	}

	if(ap) {
		free(((variable *)ap->data.v)->value);
		((variable *)ap->data.v)->value = strdup(value);
	} else {
		ap = a_add(app, a_new(&at_variable));
		((variable *)ap->data.v)->key = strdup(key);
		((variable *)ap->data.v)->value = strdup(value);
	}
}

void
delete_variable(attrib **app, const char *key)
{
	attrib *ap;

	for(ap = a_find(*app, &at_variable); ap; ap=ap->nexttype) {
		if(strcmp(key, ((variable *)ap->data.v)->key) == 0) {
			break;
		}
	}

	if(ap) {
		free(((variable *)ap->data.v)->key);
		free(((variable *)ap->data.v)->value);
		a_remove(app, ap);
	}
}

void
init_variable(void)
{
	at_register(&at_variable);
}
