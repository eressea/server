/* vi: set ts=2:
 *
 *	$Id: objtypes.c,v 1.2 2001/01/26 16:19:40 enno Exp $
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
#include "objtypes.h"

/* kernel includes */
#include "building.h"
#include "faction.h"
#include "region.h"
#include "ship.h"
#include "unit.h"

/* libc includes */
#include <assert.h>
#include <stdlib.h>

obj_ID
get_ID(void *obj, typ_t typ)
{
	ID_fun fun = typdata[typ].getID;
	return fun(obj);
}

void
write_ID(FILE *f, obj_ID id)
{
	fprintf(f, "%d:%d ", id.a, id.b);
}

obj_ID
read_ID(FILE *f)
{
	obj_ID id;

	fscanf(f, "%d:%d", &id.a, &id.b);
	return id;
}

/****** Not implemented ******/
obj_ID default_ID;
/* die müssen schon ein value zurückliefern... */
#ifdef OLD_TRIGGER
static obj_ID notimplemented_ID(void *p) { unused(p); assert(0); return default_ID; }
static void * notimplemented_find(obj_ID id) { unused(id); assert(0); return 0; }
static void notimplemented_destroy(void *p) { unused(p); assert(0); }
#endif
static char * notimplemented_desc(void *p) { unused(p); assert(0); return 0; }

static void cannot_destroy(void *p) {
	fprintf(stderr, "** destroy-method unzerstörbares Objekt <%p> aufgerufen! **\n", p);
}

/****** Unit ******/
static obj_ID unit_ID(void *p) {
	obj_ID id; id.a = p ? ((unit *)p)->no : 0; id.b = 0; return id;
}
static void * unit_find(obj_ID id) {
	return (void *)ufindhash(id.a);
}
static attrib ** unit_attribs( void *p ) {
	return &((unit *)p)->attribs;
}
static void * unit_deref( void *pp ) {
	return (void *) (*((unit **)pp));
}
static void unit_set( void *pp, void *p ) {
	*(unit **)pp = (unit *)p;
}

/****** Region ******/
static obj_ID region_ID(void *p) {
	/* Bei Regionen gibts keine NULL-Pointer */
	obj_ID id; id.a = ((region *)p)->x; id.b = ((region *)p)->y; return id;
}
static void * region_find(obj_ID id) {
	return (void *)findregion(id.a, id.b);
}
static attrib ** region_attribs( void *p ) {
	return &((region *)p)->attribs;
}
static void * region_deref( void *pp ) {
	return (void *) (*((region **)pp));
}
static void region_set( void *pp, void *p ) {
	*(region **)pp = (region *)p;
}

/****** Building ******/
static obj_ID building_ID(void *p) {
	obj_ID id; id.a = p ? ((building *)p)->no : 0; id.b = 0; return id;
}
static void * building_find(obj_ID id) {
	return (void *)findbuilding(id.a);
}
static attrib ** building_attribs( void *p ) {
	return &((building *)p)->attribs;
}
static void * building_deref( void *pp ) {
	return (void *) (*((building **)pp));
}
static void building_set( void *pp, void *p ) {
	*(building **)pp = (building *)p;
}

/****** Ship ******/
static void * ship_find(obj_ID id) {
	return (void *)findship(id.a);
}
static obj_ID ship_ID(void *p) {
	obj_ID id; id.a = p ? ((ship *)p)->no : 0; id.b = 0; return id;
}
static attrib ** ship_attribs( void *p ) {
	return &((ship *)p)->attribs;
}
static void * ship_deref( void *pp ) {
	return (void *) (*((ship **)pp));
}
static void ship_set( void *pp, void *p ) {
	*(ship **)pp = (ship *)p;
}

/****** Faction ******/
static void * faction_find(obj_ID id) {
	return (void *)findfaction(id.a);
}
static obj_ID faction_ID(void *p) {
	obj_ID id; id.a = p ? ((faction *)p)->no : 0; id.b = 0; return id;
}
static attrib ** faction_attribs( void *p ) {
	return &((faction *)p)->attribs;
}
static void * faction_deref( void *pp ) {
	return (void *) (*((faction **)pp));
}
static void faction_set( void *pp, void *p ) {
	*(faction **)pp = (faction *)p;
}

#ifdef OLD_TRIGGER
/****** Action ******/
static attrib ** action_attribs( void *p ) {
	return &((action *)p)->attribs;
}
static void * action_deref( void *pp ) {
	return (void *) (*((action **)pp));
}
static void action_set( void *pp, void *p ) {
	if( (*(action **)pp)->magic != ACTION_MAGIC ) {
			fprintf(stderr, "Error: action_set(pp=%p, p=%p): (*pp)->magic ungueltig!\n", pp, p);
			return;
	}
	*(action **)pp = (action *)p;
}

/****** old_trigger ******/
static attrib ** trigger_attribs( void *p ) {
	return &((old_trigger *)p)->attribs;
}
static void * trigger_deref( void *pp ) {
	return (void *) (*((old_trigger **)pp));
}
static void trigger_set( void *pp, void *p ) {
	*(old_trigger **)pp = (old_trigger *)p;
}

/****** timeout ******/
static attrib ** timeout_attribs( void *p ) {
	return &((timeout *)p)->attribs;
}
static void * timeout_deref( void *pp ) {
	return (void *) (*((timeout **)pp));
}
static void timeout_set( void *pp, void *p ) {
	*(timeout **)pp = (timeout *)p;
}
#endif

/******* Typ-Funktionstabelle ********/

typdata_t typdata[] = {
	/* TYP_UNIT */ {
		(ID_fun)unit_ID,
		(find_fun)unit_find,
		(desc_fun)unitname,
		(attrib_fun)unit_attribs,
		(destroy_fun)destroy_unit,
		(deref_fun)unit_deref,
		(set_fun)unit_set,
	},
	/* TYP_REGION */ {
		(ID_fun)region_ID,
		(find_fun)region_find,
		(desc_fun)regionid,
		(attrib_fun)region_attribs,
		(destroy_fun)cannot_destroy,
		(deref_fun)region_deref,
		(set_fun)region_set,
	},
	/* TYP_BUILDING */ {
		(ID_fun)building_ID,
		(find_fun)building_find,
		(desc_fun)buildingname,
		(attrib_fun)building_attribs,
		(destroy_fun)destroy_building,
		(deref_fun)building_deref,
		(set_fun)building_set,
	},
	/* TYP_SHIP */ {
		(ID_fun)ship_ID,
		(find_fun)ship_find,
		(desc_fun)shipname,
		(attrib_fun)ship_attribs,
		(destroy_fun)destroy_ship,
		(deref_fun)ship_deref,
		(set_fun)ship_set,
	},
	/* TYP_FACTION */ {
		(ID_fun)faction_ID,
		(find_fun)faction_find,
		(desc_fun)notimplemented_desc,
		(attrib_fun)faction_attribs,
		(destroy_fun)cannot_destroy,
		(deref_fun)faction_deref,
		(set_fun)faction_set,
	},
#ifdef OLD_TRIGGER
	/* TYP_ACTION */ {
		(ID_fun)notimplemented_ID,
		(find_fun)notimplemented_find,
		(desc_fun)notimplemented_desc,
		(attrib_fun)action_attribs,
		(destroy_fun)notimplemented_destroy,
		(deref_fun)action_deref,
		(set_fun)action_set,
	},
	/* TYP_TRIGGER */ {
		(ID_fun)notimplemented_ID,
		(find_fun)notimplemented_find,
		(desc_fun)notimplemented_desc,
		(attrib_fun)trigger_attribs,
		(destroy_fun)notimplemented_destroy,
		(deref_fun)trigger_deref,
		(set_fun)trigger_set,
	},
	/* TYP_TIMEOUT */ {
		(ID_fun)notimplemented_ID,
		(find_fun)notimplemented_find,
		(desc_fun)notimplemented_desc,
		(attrib_fun)timeout_attribs,
		(destroy_fun)notimplemented_destroy,
		(deref_fun)timeout_deref,
		(set_fun)timeout_set,
	},
#endif
};

/******** Resolver-Funktionen für obj_ID ********/

#ifdef OLD_TRIGGER
#include "old/pointertags.h"
#include "old/trigger.h"
#else
#define tag_t int
#define TAG_NOTAG (-1)
#endif

typedef struct unresolved2 {
	struct unresolved2 *next;

	obj_ID	id;
	void *	objPP;
	typ_t	typ;
	tag_t	tag; /* completely useless. eliminate */
} unresolved2;

static unresolved2 *ur2_list;


void
add_ID_resolve2(obj_ID id, void *objPP, typ_t typ, tag_t tag)
{
	unresolved2 *ur = calloc(1, sizeof(unresolved2));

	ur->id = id;
	ur->objPP = objPP;
	ur->typ = typ;
	ur->tag = tag;
	ur->next = ur2_list;
	ur2_list = ur;
}

void
add_ID_resolve(obj_ID id, void *objPP, typ_t typ)
{
	add_ID_resolve2(id, objPP, typ, TAG_NOTAG);
}

void
resolve_IDs(void)
{
	unresolved2 *ur;
	void *robj;

	while( (ur = ur2_list) != NULL ) {
		ur2_list = ur->next;

		robj = typdata[ur->typ].find(ur->id);
		typdata[ur->typ].ppset(ur->objPP, robj);
#ifdef OLD_TRIGGER
		if( ur->tag != TAG_NOTAG )
			tag_pointer(ur->objPP, ur->typ, ur->tag);
#endif
		free(ur);
	}
}

