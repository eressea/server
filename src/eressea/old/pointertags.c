/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2000
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "pointertags.h"

#include <stdlib.h>

typedef struct ptrlist {
	struct ptrlist *next;

	void *objPP;
	typ_t typ;
} ptrlist;

typedef struct {
	tag_t tag;

	ptrlist *ptrs;
} ptrref;

static ptrlist *freelist;

/* at_pointer_tag */

static void
tag_init(attrib *a)
{
	a->data.v = calloc(1, sizeof(ptrref));
}

static void
tag_done(attrib *a)
{
	ptrref *ref;
	ptrlist *p;

	ref = (ptrref *)a->data.v;
	while( (p = ref->ptrs) != NULL ) {
		ref->ptrs = p->next;
		p->next = freelist;
		freelist = p;
	}
	free(ref);
}

attrib_type at_pointer_tag = {
	"pointer tags",
	tag_init,
	tag_done,
	NULL,		/* age */
	NO_WRITE,		/* write */
	NO_READ, 		/* read */
};

static ptrref *
find_ref(attrib **ap, tag_t tag)
{
	attrib *a;
	ptrref *ref;

	a = a_find(*ap, &at_pointer_tag);
	while( a ) {
		ref = (ptrref *)a->data.v;

		if( ref->tag == tag )
			return  ref;
		a = a->nexttype;
	}
	return NULL;
}

static ptrref *
make_ref(attrib **ap, tag_t tag)
{
	attrib *a;
	ptrref *ref;

	ref = find_ref(ap, tag);
	if( !ref ) {
		a = a_new(&at_pointer_tag);
		a_add(ap, a);
		ref = (ptrref *)a->data.v;
		ref->ptrs = NULL;
		ref->tag = tag;
	}
	return ref;
}


void
tag_pointer(void *objPP, typ_t typ, tag_t tag)
{
	void *obj;
	attrib **ap;
	ptrref *ref;
	ptrlist *p;

	obj = typdata[typ].ppget(objPP);
	if( !obj )
		return;
	ap = typdata[typ].getattribs(obj);
	ref = make_ref(ap, tag);

	if( (p = freelist) != NULL )
		freelist = p->next;
	else
		p = calloc(1, sizeof(ptrlist));
	p->objPP = objPP;
	p->typ   = typ;

	p->next = ref->ptrs;
	ref->ptrs = p;
}

void
untag_pointer(void *objPP, typ_t typ, tag_t tag)
{
	void *obj;
	attrib **ap;
	ptrref *ref;
	ptrlist *p, **prevP;

	obj = typdata[typ].ppget(objPP);
	if( !obj )
		return;
	ap = typdata[typ].getattribs(obj);

	ref = find_ref(ap, tag);
	if( !ref )
		return;

	prevP = &ref->ptrs;
	for( p = ref->ptrs; p; p = p->next ) {
		if( p->objPP == objPP ) {
			*prevP = p->next;		/* unlink */

			p->next = freelist;		/* p freigeben */
			freelist = p;
			return;
		}
		prevP = &p->next;
	}
}

int
count_tagged_pointers(void *obj, typ_t typ, tag_t tag)
{
	attrib **ap;
	ptrref *ref;
	ptrlist *p;
	int count;

	if( !obj )
		return 0;
	ap = typdata[typ].getattribs(obj);
	ref = find_ref(ap, tag);
	if( !ref )
		return 0;

	count = 0;
	for( p = ref->ptrs; p; p = p->next )
		++count;

	return count;
}

int
count_all_pointers(void *obj, typ_t typ)
{
	tag_t tag;
	int count;

	count = 0;
	for( tag = 0; tag < MAXTAGS; tag++ )
		count += count_tagged_pointers(obj, typ, tag);

	return count;
}


static void
change_tagged_pointers(void *obj1, typ_t typ, tag_t tag, void *obj2)
{
	attrib **ap;
	ptrref *ref1, *ref2 = NULL;
	ptrlist *p;

	if( !obj1 )
		return;

	ap = typdata[typ].getattribs(obj1);
	ref1 = find_ref(ap, tag);
	if( !ref1 )
		return;
	if( obj2 ) {
		ap = typdata[typ].getattribs(obj2);
		ref2 = make_ref(ap, tag);
	}

	while( (p = ref1->ptrs) != NULL ) {
		ref1->ptrs = p->next;

		typdata[typ].ppset(p->objPP, obj2);
		if( obj2 ) {
			p->next = ref2->ptrs;	/* Referenz jetzt bei obj2 */
			ref2->ptrs = p;
		} else {
			p->next = freelist;		/* p freigeben */
			freelist = p;
		}
	}
	/* Wir lassen das Attrib mit der leeren Pointer-Liste beim
	 * Objekt bestehen, das erspart eine De- und Neu-Allokation,
	 * wenn nochmal Pointer auf dieses Objekt so ge'tag't werden.
	 */
}


void
change_all_pointers(void *obj1, typ_t typ, void *obj2)
{
	tag_t tag;

	for( tag = 0; tag < MAXTAGS; tag++ )
		change_tagged_pointers(obj1, typ, tag, obj2);
}
