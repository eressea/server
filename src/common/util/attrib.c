/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "attrib.h"

#include "log.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MAXATHASH 61
attrib_type * at_hash[MAXATHASH];

static unsigned int
__at_hashkey(const char* s)
{
	int key = 0;
	size_t i = strlen(s);

	while (i>0) {
    key = (s[--i] + key*37);
	}
	return key & 0x7fffffff;
}

void
at_register(attrib_type * at)
{
  attrib_type * find;

  if (at->read==NULL) {
    log_warning(("registering non-persistent attribute %s.\n", at->name));
  }
  at->hashkey = __at_hashkey(at->name);
  find = at_hash[at->hashkey % MAXATHASH];
  while (find && at->hashkey!=find->hashkey) find = find->nexthash;
  if (find && find==at) {
    log_warning(("attribute '%s' was registered more than once\n", at->name));
    return;
  } else {
    assert(!find || !"hashkey is already in use");
  }
  at->nexthash = at_hash[at->hashkey % MAXATHASH];
  at_hash[at->hashkey % MAXATHASH] = at;
}

static attrib_type *
at_find(unsigned int hk)
{
	const char* translate[3][2] = {
		{ "zielregion", "targetregion" }, /* remapping: früher zielregion, heute targetregion */
		{ "verzaubert", "curse" }, /* remapping: früher verzaubert, jetzt curse */
		{ NULL, NULL }
	};
	attrib_type * find = at_hash[hk % MAXATHASH];
	while (find && hk!=find->hashkey) find = find->nexthash;
	if (!find) {
		int i = 0;
		while (translate[i][0]) {
			if (__at_hashkey(translate[i][0])==hk)
				return at_find(__at_hashkey(translate[i][1]));
			++i;
		}
	}
	return find;
}

attrib *
a_select(attrib * a, const void * data, boolean(*compare)(const attrib *, const void *))
{
	while (a && !compare(a, data)) a = a->next;
	return a;
}

attrib *
a_find(attrib * a, const attrib_type * at)
{
	while (a && a->type!=at) a = a->next;
	return a;
}

const attrib *
a_findc(const attrib * a, const attrib_type * at)
{
	while (a && a->type!=at) a = a->next;
	return a;
}

attrib *
a_add(attrib ** pa, attrib * a) 
{
	attrib ** find = pa;
	assert(a->next==NULL && a->nexttype==NULL);
	while (*find && (*find)->type!=a->type) find = &(*find)->next;
	if (a->type->flags & ATF_PRESERVE) {
		while (*find) find = &(*find)->nexttype;
	}
	if (a->type->flags & ATF_UNIQUE && *find) {
		if ((*find)->type == a->type) {
			log_error(("duplicate attribute: %s\n", a->type->name));
			return a;
		}
	}
	if (*find) {
		attrib ** last = find;
		while (*last) last = &(*last)->nexttype;
		*last = a;
		while (*find && (*find)->type==a->type) find = &(*find)->next;
	}
	a->next = *find;
	*find = a;
	return a;
}

void
a_free(attrib * a) {
	const attrib_type * at = a->type;
	if (at->finalize) at->finalize(a);
	free(a);
}

static int
a_unlink(attrib ** p, attrib * a) {
	attrib ** pa = p;
	while (*pa && *pa!=a) pa = &(*pa)->next;
	if (*pa) {
		attrib ** pnt;
		for (pnt=p;*pnt;pnt=&(*pnt)->next)
			if ((*pnt)->nexttype == a) {
				(*pnt)->nexttype = a->nexttype;
				break;
			}
		*pa = (*pa)->next;
		return 1;
	}
	return 0;
}

int
a_remove(attrib ** p, attrib * a) {
	int ok;
	ok = a_unlink(p, a);
	if( ok ) a_free(a);
	return ok;
}

void
a_removeall(attrib **p, const attrib_type * at)
{
	attrib *find = *p, *findnext;
	if (find && find->type != at){
		find = a_find(find, at);
	}
	while(find && find->type == at) {
		findnext = find->next;
		a_remove(p, find);
		find = findnext;
	}
}

attrib *
a_new(const attrib_type * at) {
	attrib * a = (attrib*)calloc(1, sizeof(attrib));
  assert(at!=NULL);
	a->type = at;
	if (at->initialize) at->initialize(a);
	return a;
}

int
a_age(attrib ** p)
{
	attrib ** ap = p;
	/* Attribute altern, und die Entfernung (age()==0) eines Attributs
	 * hat Einfluß auf den Besitzer */
	while(*ap) {
		attrib * a = *ap;
		if (a->type->age && a->type->age(a)==0) a_remove(p, a);
		else ap=&a->next;
	}
	return (*p!=NULL);
}

int
a_read(FILE * f, attrib ** attribs)
{
	int key, retval = AT_READ_OK;
	char zText[128];
	strcpy(zText, "unknown");

	key = -1;
	fscanf(f, "%s", zText);
	if (strcmp(zText, "end")==0) return retval;
	else key = __at_hashkey(zText);

	while(key!=-1) {
		attrib_type * at = at_find(key);
		if (!at) {
			fprintf(stderr, "attribute hash: %d (%s)\n", key, zText);
			assert(at || !"attribute not registered");
		}
		if (at->read) {
			attrib * na = a_new(at);
			int i = at->read(na, f);
			switch (i) {
			case AT_READ_OK:
				a_add(attribs, na);
				break;
			case AT_READ_FAIL:
        retval = AT_READ_FAIL;
				a_free(na);
				break;
			default:
				assert(!"invalid return value");
				break;
			}
		} else {
			assert(!"fehler: keine laderoutine für attribut");
		}

		fscanf(f, " %s", zText);
		if (!strcmp(zText, "end")) break;
		key = __at_hashkey(zText);
	}
  return retval;
}

void
a_write(FILE * f, const attrib * attribs)
{
	const attrib * na = attribs;

	while(na) {
		if (na->type->write) {
			assert(na->type->hashkey || !"attribute not registered");
			fprintf(f, "%s ", na->type->name);
			na->type->write(na, f);
		}
		na = na->next;
	}
	fprintf(f, "end ");
}
