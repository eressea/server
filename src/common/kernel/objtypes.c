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
static char * notimplemented_desc(void *p) { unused(p); assert(0); return 0; }

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
static void faction_set( void *pp, void *p ) {
	*(faction **)pp = (faction *)p;
}

/******* Typ-Funktionstabelle ********/

typdata_t typdata[] = {
	/* TYP_UNIT */ {
		(ID_fun)unit_ID,
		(find_fun)unit_find,
		(attrib_fun)unit_attribs,
		(set_fun)unit_set,
	},
	/* TYP_REGION */ {
		(ID_fun)region_ID,
		(find_fun)region_find,
		(attrib_fun)region_attribs,
		(set_fun)region_set,
	},
	/* TYP_BUILDING */ {
		(ID_fun)building_ID,
		(find_fun)building_find,
		(attrib_fun)building_attribs,
		(set_fun)building_set,
	},
	/* TYP_SHIP */ {
		(ID_fun)ship_ID,
		(find_fun)ship_find,
		(attrib_fun)ship_attribs,
		(set_fun)ship_set,
	},
	/* TYP_FACTION */ {
		(ID_fun)faction_ID,
		(find_fun)faction_find,
		(attrib_fun)faction_attribs,
		(set_fun)faction_set,
	},
};

/******** Resolver-Funktionen für obj_ID ********/

#define tag_t int
#define TAG_NOTAG (-1)

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
		free(ur);
	}
}

