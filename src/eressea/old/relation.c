/* vi: set ts=2:
 *
 *	$Id: relation.c,v 1.2 2001/04/01 06:58:44 enno Exp $
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
#include "eressea.h"
#include "relation.h"
#if defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER)
/* util includes */
#include <attrib.h>

#include "pointertags.h"
#include <objtypes.h>

#include <stdlib.h>

typedef struct {
	void *obj2;
	typ_t typ2;
	relation_t id;
	spread_t spread;
} reldata;

static void
rel_init(attrib *a)
{
	reldata *rel;

	rel = calloc(1, sizeof(reldata));
	rel->obj2 = NULL;
	a->data.v = (void *)rel;
}

static void
rel_done(attrib *a)
{
#ifdef OLD_TRIGGER
	reldata *rel = (reldata *)a->data.v;
	if( rel->obj2 )
		untag_pointer(&rel->obj2, rel->typ2, TAG_RELATION);
	free(rel);
#else
	unused(a);
#endif
}

#ifdef OLD_TRIGGER
static void
rel_save(const attrib *a, FILE *f)
{
	reldata *rel = (reldata *)a->data.v;
	obj_ID id;
	ID_fun fun = typdata[rel->typ2].getID;

	id = fun(rel->obj2);
	write_ID(f, id);
	fprintf(f, "%d %d %d ", rel->typ2, rel->id, rel->spread);
}
#endif
static int
rel_load(attrib *a, FILE *f)
{
	reldata *rel = (reldata *)a->data.v;
	obj_ID id;

	id = read_ID(f);
	fscanf(f, "%d %d %d ", (int *)&rel->typ2, (int *)&rel->id, (int *)&rel->spread);
	add_ID_resolve2(id, &rel->obj2, rel->typ2, TAG_RELATION);
	return 1;
}

#ifdef OLD_TRIGGER
/* garbage collection */
static int
rel_age(attrib *a)
{
	reldata *rel = (reldata *)a->data.v;
	return (rel->obj2 != NULL);
}
#endif

attrib_type at_relation = {
	"unit_relations",
	rel_init,
	rel_done,
#ifdef CONVERT_TRIGGER
	NULL, NULL,
#else
	rel_age,
	rel_save,
#endif
	rel_load,
};

attrib_type at_relbackref = {
	"unit_relations_back_reference",
	rel_init,
	rel_done,
#ifdef CONVERT_TRIGGER
	NULL, NULL,
#else
	rel_age,
	rel_save,
#endif
	rel_load,
};

static attrib *
find_rel(attrib **ap, relation_t id, attrib_type *atype)
{
	attrib *a;
	reldata *rel;

	a = a_find(*ap, atype);
	while( a ) {
		rel = (reldata *)a->data.v;
		if( rel->id == id )
			return a;
		a = a->nexttype;
	}
	return NULL;
}

static void
rel_create(void *obj1, typ_t typ1,
		relation_t id,
		void *obj2, typ_t typ2,
		spread_t spread,
		attrib_type *atype)
{
	attrib *a;
	attrib **ap;
	reldata *rel;

	ap = typdata[typ1].getattribs(obj1);
	a = find_rel(ap, id, atype);
	if( !a ) {
		a = a_new(atype);
		a_add(ap, a);
		rel = (reldata *)a->data.v;
		rel->id = id;
	} else {
		rel = (reldata *)a->data.v;
		if( rel->obj2 )
			untag_pointer(&rel->obj2, rel->typ2, TAG_RELATION);
	}
	rel->obj2 = obj2;
	tag_pointer(&rel->obj2, typ2, TAG_RELATION);
	rel->typ2 = typ2;
	rel->spread = spread;
}

static reldata *
rel_get(const void *obj, typ_t typ,
		relation_t id,
		attrib_type *atype)
{
	attrib *a;
	attrib **ap;

	ap = typdata[typ].getattribs((void *)obj);
	a = find_rel(ap, id, atype);
	if( a )
		return (reldata *)(a->data.v);
	return NULL;
}

/*********************************************************
      PUBLIC FUNCTIONS
 *********************************************************/

void *
get_relation2(const void *obj, typ_t typ, relation_t id, typ_t *typ2P)
{
	reldata *rel;

	rel = rel_get(obj, typ, id, &at_relation);
	if( rel ) {
		if( typ2P )
				*typ2P = rel->typ2;
		return rel->obj2;
	}
	return NULL;
}

void *
get_relation(const void *obj, typ_t typ, relation_t id)
{
	typ_t dummy;
	return get_relation2(obj, typ, id, &dummy);
}

void *
get_rev_relation2(void *obj, typ_t typ, relation_t id, typ_t *typ2P)
{
	reldata *rel;

	rel = rel_get(obj, typ, id, &at_relbackref);
	if( rel ) {
		if( typ2P )
				*typ2P = rel->typ2;
		return rel->obj2;
	}
	return NULL;
}

void *
get_rev_relation(void *obj, typ_t typ, relation_t id)
{
	typ_t dummy;
	return get_rev_relation2(obj, typ, id, &dummy);
}

void
create_relation(void *obj1, typ_t typ1,
		relation_t id,
		void *obj2, typ_t typ2,
		spread_t spread)
{
	rel_create(obj1, typ1, id, obj2, typ2, spread, &at_relation);
	rel_create(obj2, typ2, id, obj1, typ1, SPREAD_TRANSFER, &at_relbackref);
}

void
remove_relation(void *obj, typ_t typ, relation_t id)
{
	attrib **ap;
	attrib *a;

	ap = typdata[typ].getattribs(obj);
	a = find_rel(ap, id, &at_relation);
	if( a ) {
		reldata *rel = (reldata *)a->data.v;
		obj = rel->obj2;	/* Objekt mit Backref-Attrib */
		a_remove(ap, a);	/* Relation entfernen */
		if( obj ) {
			ap = typdata[typ].getattribs(obj);
			a = find_rel(ap, id, &at_relbackref);
			if( a )
				a_remove(ap, a);	/* Backref entfernen */
		}
	}
}
#endif
