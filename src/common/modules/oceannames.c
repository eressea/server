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
#include "oceannames.h"

/* kernel includes */
#include <region.h>
#include <faction.h>

/* util includes */
#include <attrib.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

typedef struct namehash {
	struct namehash * next;
	const char * name;
} namehash;

#define NMAXHASH 1023
namehash names[NMAXHASH];

void nhash(const char * name);

typedef struct name {
	struct name * next;
	struct faction_list * factions;
	const char * name;
} name;

static void 
free_names(attrib * a)
{
	name * data = (name*)a->data.v;
	while (a->data.v) {
		a->data.v = data->next;
		free(data);
	}
}

struct attrib_type at_oceanname = { "names", NULL, free_names, NULL/*, write_names, read_names, ATF_UNIQUE */};

const char * 
oceanname(const struct region * r, const struct faction * f)
{
	attrib * a = a_find(r->attribs, &at_oceanname);
	if (a) {
		name * names = (name*)a->data.v;
		while (names) {
			faction_list * fl = names->factions;
			while (fl) {
				if (fl->data==f) return names->name;
				fl=fl->next;
			}
			names = names->next;
		}
	}
	return NULL;
}

void
nameocean(struct region *r, struct faction * f, const char * newname)
{
	attrib * a = a_find(r->attribs, &at_oceanname);
	if (!a && newname) a = a_add(&r->attribs, a_new(&at_oceanname));
	if (a) {
		faction_list **oldf = NULL, **newf = NULL;
		faction_list * fl = NULL;
		name * names = (name*)a->data.v;
		while ((names && (!newf && newname)) || !oldf) {
			faction_list ** fli = &names->factions;
			if (oldf==NULL) while (*fli) {
				if ((*fli)->data==f) {
					oldf = fli;
					break;
				}
				fli=&(*fli)->next;
			}
			if (newname && !newf && !strcmp(names->name, newname)) {
				newf = fli;
			}
			names = names->next;
		}

		if (oldf) {
			fl = *oldf;
			*oldf = fl->next;
		} else if (newname) {
			fl = calloc(1, sizeof(faction_list));
		}

		if (newf) {
			fl->data = f;
			fl->next = *newf;
			*newf = fl;
		} else if (newname) {
			name * nm = calloc(1, sizeof(name));
			nm->factions = fl;
			fl->data = f;
			fl->next = NULL;
			nm->next = (name*)a->data.v;
			a->data.v = nm;
		} else if (fl) {
			free(fl);
		}
	}
}
