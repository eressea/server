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
#include <eressea.h>
#include "trigger.h"

#include <item.h>
#include <faction.h>
#include <unit.h>
#include <region.h>
#include <save.h>
#include <magic.h>
#include <spell.h>

#include <resolve.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#if defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER)

#define NULLSTRING	"<iuw_null>"

static int action_resid;
static int timeout_resid;
static int trigger_resid;

static int
make_id(int *itemidP, int *globalidP)
{
	if( *(itemidP) <= 0 ) {
		(*globalidP)++;
		*itemidP = *globalidP;
	}
	return *itemidP;
}

#define action_id(a)	make_id(&(a)->resid, &action_resid)
#define trigger_id(t)	make_id(&(t)->resid, &trigger_resid)
#define timeout_id(t)	make_id(&(t)->resid, &timeout_resid)

/********** action attribute **********/

static action *all_actions;

static void
action_init(attrib *a)
{
	action *act;

	act = calloc(1, sizeof(action));
	act->next = all_actions;
	all_actions = act;
	act->magic = ACTION_MAGIC;
	a->data.v = act;
}

static void
action_done(attrib *a)
{
	action *act = (action *)a->data.v;

	if( act->string )
		free(act->string);
	free(act);
}
#ifdef OLD_TRIGGER
static void
action_save(const attrib *a, FILE *f)
{
	action *act = (action *)a->data.v;
	int nints, j;

	fprintf(f, "%d ", action_id(act));
	fprintf(f, "%d ", (int)act->atype);
	write_ID(f, get_ID(act->obj, act->typ));
	fprintf(f, "%d ", (int)act->typ);
#if RELEASE_VERSION < ACTIONFIX1_VERSION
	fprintf(f, "%d %d ", act->i[0], act->i[1]);
#else
	for( nints = 0, j = 0; j < ACTION_INTS; j++ ) {
			if( act->i[j] != 0 )
					nints = j+1;
	}
	fprintf(f, "%d ", nints);
	for( j = 0; j < nints; j++ )
			fprintf(f, "%d ", act->i[j]);
#endif
	fprintf(f, "%s\n", act->string ? estring(act->string) : NULLSTRING);
}
#endif
static int
action_load(attrib *a, FILE *f)
{
	action *act = (action *)a->data.v;
	int i, j, nints;
	obj_ID id;

	fscanf(f, "%d", &i); act->resid = -(i);
	fscanf(f, "%d", &i); act->atype = i;
	id = read_ID(f);
	fscanf(f, "%d", &i); act->typ = i;
	add_ID_resolve(id, &act->obj, act->typ);

	if( global.data_version < ACTIONFIX1_VERSION )
	  fscanf(f, "%d %d", &act->i[0], &act->i[1]);
	else {
			fscanf(f, "%d", &nints);
			assert(nints <= ACTION_INTS);
			for( j = 0; j < nints ; j++ )
				fscanf(f, "%d", &act->i[j]);
	}
	fscanf(f, "%s", buf);
	if( strcmp(buf, NULLSTRING) != 0 )
		act->string = strdup(cstring(buf));
	else
		act->string = NULL;

#if 0
	/* speziell für Runde 199->200 */
	iuw_fix_action(act);
#endif

	/* irace fix für AC_CHANGERACE */
	if (act->atype == AC_CHANGERACE){
		if (!act->i[1]){
			act->i[1] = act->i[0];
			while (act->i[1] == RC_DAEMON){
				act->i[1] = rand()%11;
			}
		}
	}

	return 1;
}

/* garbage collection */
#ifdef OLD_TRIGGER
static int
action_age(attrib *a)
{
	action *act = (action *)a->data.v;

	if( act->atype == AC_NONE ) {
		change_all_pointers(act, TYP_ACTION, NULL);	/* zur Sicherheit */
		return 0;	/* dieses Attrib kann gelöscht werden */
	}

	/* wenn keine Auslöser (old_trigger/Timeouts) mehr auf uns zeigen, dann
	 * kann sie auch gelöscht werden.
	 */
	return (count_all_pointers(act, TYP_ACTION) != 0);
}
#endif
attrib_type at_action = {
	"event_action",
	action_init,
	action_done,
#ifdef CONVERT_TRIGGER
	NULL, NULL,
#else
	action_age,
	action_save,
#endif
	action_load,
};

void *
action_resolve(void *pp)
{
	int id = (int)pp;
	action *a;

	for( a = all_actions; a != NULL; a = a->next ) {
		if( a->resid == -(id) ) {
			return a;
		}
	}
#ifdef DIE
	assert(0);
#else
	fprintf(stderr, "WARNING: could not resolve action %d\n", id);
#endif
	return NULL;
}

static action *
alloc_action(action_t atype, void *obj2, typ_t typ2, spread_t spr2)
{
	attrib *a;
	attrib **ap;
	action *act;

	ap = typdata[typ2].getattribs(obj2);
	a = a_new(&at_action);
	a_add(ap, a);

	act = (action *)a->data.v;
	act->atype = atype;
	act->obj = obj2;
	act->typ = typ2;
	act->spread = spr2;

	return act;
}

/********** actionlist **********/

static actionlist *
new_actionlist_entry(actionlist **al_startP)
{
	actionlist *al;
	al = calloc(1, sizeof(actionlist));

	al->next = (*al_startP);
	(*al_startP) = al;
	return al;
}

static void
add_actionlist_entry(actionlist **al_startP, action *a)
{
	actionlist *al;
	al = new_actionlist_entry(al_startP);
	al->act = a;
	tag_pointer(&al->act, TYP_ACTION, TAG_NORMAL);
}

static void
free_actionlist_entry(actionlist *al)
{
#if 0
	if( al->act )
		untag_pointer(&al->act, TYP_ACTION, TAG_NORMAL);
	free(al);
#endif
}

static void
free_actionlist(actionlist *al_start)
{
	actionlist *al;

	while( (al = al_start)!=NULL ) {
		al_start = al->next;
		free_actionlist_entry(al);
	}
}

static void
save_actionlist(actionlist *al_start, FILE *f)
{
	actionlist *al;
	int count;

	count = 0;
	for( al = al_start; al != NULL; al = al->next ) {
		if( al->act && al->act->atype != AC_NONE )
			++count;
	}

	fprintf(f, " %d\n", count);
	for( al = al_start; al != NULL; al = al->next ) {
		if( al->act && al->act->atype != AC_NONE )
			save_action_pointer(f, al->act, TAG_NORMAL);
	}
}

static actionlist *
load_actionlist(FILE *f)
{
	actionlist *al, *al_start = NULL;
	int count;

	fscanf(f, "%d", &count);
	while( count-- ) {
		al = new_actionlist_entry(&al_start);
		load_action_pointer(f, &al->act);
	}
	return al_start;
}

/********** old_trigger attribute **********/

static old_trigger *all_triggers;

static void
trigger_init(attrib *a)
{
	old_trigger *t;

	t = calloc(1, sizeof(old_trigger));
	a->data.v = t;
}

static void
trigger_done(attrib *a)
{
	old_trigger *t = (old_trigger *)a->data.v;
	free_actionlist(t->acts);
	while (t->attribs) {
		a_remove(&t->attribs, t->attribs);
	}
	free(t);
}
#ifdef OLD_TRIGGER
static void
trigger_save(const attrib *a, FILE *f)
{
	old_trigger *t = (old_trigger *)a->data.v;

	fprintf(f, "%d ", trigger_id(t));
	write_ID(f, get_ID(t->obj, t->typ));
	fprintf(f, "%d ", (int)t->typ);
	fprintf(f, "%d ", (int)t->condition);
	save_actionlist(t->acts, f);
}
#endif
static int
trigger_load(attrib *a, FILE *f)
{
	old_trigger *t = (old_trigger *)a->data.v;
	int i;
	obj_ID id;

	fscanf(f, "%d", &i); t->resid = -(i);
	id = read_ID(f);
	fscanf(f, "%d", &i); t->typ = i;
	add_ID_resolve(id, &t->obj, t->typ);
	fscanf(f, "%d", &i); t->condition = i;
	t->acts = load_actionlist(f);
	return 1;
}
#ifdef OLD_TRIGGER
/* garbage collection */
static int
trigger_age(attrib *a)
{
	old_trigger *t = (old_trigger *)a->data.v;
	actionlist *al;
	int count;

	if( t->condition == TR_NONE ) {
		change_all_pointers(t, TYP_TRIGGER, NULL);	/* zur Sicherheit */
		return 0;
	}

	count = 0;
	for( al = t->acts; al != NULL; al = al->next ) {
		if( al->act != NULL )
			++count;
	}
	/* wenn keine Aktionen mehr von diesem old_trigger abhängen, dann
	 * kann dieses Attrib gelöscht werden.
	 */
	return (count != 0);
}
#endif
attrib_type at_trigger = {
	"event_trigger",
	trigger_init,
	trigger_done,
#ifdef CONVERT_TRIGGER
	NULL, NULL,
#else
	trigger_age,
	trigger_save,
#endif
	trigger_load,
};


void *
trigger_resolve(void * data)
{
	int id = (int)data;
	old_trigger *a;

	for( a = all_triggers; a != NULL; a = a->next ) {
		if( a->resid == -(id) ) {
			return a;
		}
	}
#ifdef DIE
	assert(0);
#else
	fprintf(stderr, "WARNING: could not resolve old_trigger %d\n", id);
#endif
	return NULL;
}

/********** timeout **********/

timeout *all_timeouts;

static timeout *
alloc_timeout(int ticks)
{
	timeout *t;

	t = calloc(1, sizeof(timeout));
	t->next = all_timeouts;
	all_timeouts = t;
	t->ticks = ticks;
	return t;
}

/******************************************************/

static actionlist *datalist, *deathlist;

static void
prepare_actions(actionlist **al_startP)
{
	actionlist *al;

	while( (al = *al_startP)!=NULL ) {
		*al_startP = al->next;	/* unlink */
		/* TODO */
		if( !al->act ) continue;
		if( al->act->atype == AC_DESTROY ) {
			al->next = deathlist;
			deathlist = al;
		} else {
			al->next = datalist;
			datalist = al;
		}
	}
}

static void
do_actions(void)
{
	actionlist *al;
	action *act;
	static int in_progress;

	if( in_progress )
			return;
	in_progress = 1;

	for(;;) {
		if( datalist ) {
			al = datalist;
			datalist = al->next;
		} else if( deathlist ) {
			al = deathlist;
			deathlist = al->next;
		} else
			break;	/* keine Einträge mehr in den Listen */

		act = al->act;
		free_actionlist_entry(al);
		if( !act )
			continue;

		switch( act->atype ) {
			case AC_NONE:
				break;
			case AC_DESTROY:
				if( typdata[act->typ].destroy )
					typdata[act->typ].destroy(act->obj);
				break;
			case AC_REMOVECURSE: {
				attrib **ap;
				ap = typdata[act->typ].getattribs(act->obj);
				remove_curse(ap, (curse_t)act->i[0], act->i[1]);
				break;
			}
			case AC_REMOVERELATION:
				remove_relation(act->obj, act->typ, act->i[0]);
				break;
			case AC_SENDMESSAGE: {
				unit *u;
				assert(act->typ == TYP_UNIT);
				u = (unit*)act->obj;
				addmessage(u->region, u->faction, act->string, act->i[0], act->i[1]);
				break;
			}
			case AC_CHANGERACE: {
				unit *u;
				assert(act->typ == TYP_UNIT);
				u = (unit*)act->obj;
				if(u->race == RC_TOAD && rand()%100 > 20){
					change_item(u, I_TOADSLIME, 1);
				}
				u->race = (race_t)act->i[0];
				u->irace = (race_t)act->i[1];
				break;
			}
			case AC_CHANGEIRACE: {
				unit *u;
				assert(act->typ == TYP_UNIT);
				u = (unit*)act->obj;
				u->irace = (race_t)act->i[0];
				break;
			}
			case AC_SHOCK:
				assert(act->typ == TYP_UNIT);
				do_shock((unit*)act->obj, "");
				break;
			case AC_CHANGEFACTION: {
				faction *f;
				assert(act->typ == TYP_UNIT);
				f = findfaction_unique_id(act->i[0]);
				u_setfaction((unit*)act->obj, f);
				break;
			}
			case AC_CREATEUNIT:{
				faction *f;
				f = findfaction_unique_id(act->i[0]);
				createunit((region *)act->obj, f, act->i[1], (race_t)act->i[2]);
				break;
			}
			case AC_CREATEMAGICBOOSTCURSE:{
				unit *mage = (unit*)act->obj;
				create_curse(mage, &mage->attribs, C_AURA, 0, act->i[0], 6, 50, 1);
				break;
			}
			default:
				assert(0);
		}

		remove_action(act);
	}

	in_progress = 0;
}

/***************************************************************
 PUBLIC FUNCTIONS
 ***************************************************************/

struct old_trigger *
create_trigger(void *obj1, typ_t typ1, spread_t spread1, trigger_t condition)
{
	attrib *a;
	attrib **ap;
	old_trigger *t;

	ap = typdata[typ1].getattribs(obj1);
	a = a_new(&at_trigger);
	a_add(ap, a);

	t = (old_trigger *)a->data.v;
	t->obj = obj1;
	t->typ = typ1;
	t->condition = condition;
	t->spread = spread1;

	return t;
}

#if 0

attrib *
a_find_by_vdata(attrib *attrs, const attrib_type *atP, void *vdata)
{
		attrib *a;

		a = a_find(attrs, atP);
		while( a ) {
				if( a->data.v == vdata )
						return a;
				a = a->nexttype;
		}
		return NULL;
}
#endif

void
remove_trigger(old_trigger *t)
{
#if 0
	attrib *a;
	attrib **ap;
#endif

	if( t ) {
		change_all_pointers(t, TYP_TRIGGER, NULL);
		t->condition = TR_NONE;
		/* die Struktur selber bleibt bis zum Ende des Programms im
		 * Speicher, weil evtl noch Stackvariablen hierauf zeigen.
		 */
#if 0
		ap = typdata[t->typ].getattribs(t->obj);
		a = a_find_by_vdata(*ap, &at_trigger, (void *)t);
		assert(a != NULL);
		a_remove(ap, a);
#endif
	}
}

extern void ur_add2(int id, void ** ptrptr, typ_t typ, tag_t tag, resolve_fun fun);

void
do_trigger(void *obj1, typ_t typ1, trigger_t condition)
{
#if 0
	attrib *a, *next_a;
	attrib **ap;
	old_trigger *t;

	assert(condition != TR_NONE);

	ap = typdata[typ1].getattribs(obj1);
	a = a_find(*ap, &at_trigger);
	while( a ) {
		next_a = a->nexttype;
		t = (old_trigger *)a->data.v;

		if( t->condition == condition ) {
			prepare_actions(&t->acts);
			t->condition = TR_NONE;
#if 0
			a_remove(ap, a);
#endif
		}
		a = next_a;
	}
	do_actions();
#endif
}

/******************************************/

struct timeout *
create_timeout(int ticks)
{
	return alloc_timeout(ticks+1);
}

void
remove_timeout(timeout *t)
{
	if( t ) {
		change_all_pointers(t, TYP_TIMEOUT, NULL);
		/* die Timeout-Struktur selber bleibt noch bis zum Ende des
		 * Programms erhalten, weil evtl noch Stackvariablen hierauf
		 * zeigen.
		 */
		t->ticks = -1;
		free_actionlist(t->acts);
		t->acts = NULL;
	}
}

void
save_timeout_pointer(FILE *f, timeout *ptr, tag_t tag)
{
	if( ptr && ptr->ticks < 0 )
		ptr = NULL;
	fprintf(f, " %d %d ", ptr ? timeout_id(ptr) : 0, (int)tag);
}

void
save_timeouts(FILE *f)
{
	timeout *t;
	int count;

	count = 0;
	for( t = all_timeouts; t != NULL; t = t->next ) {
		if( t->ticks > 0 )
			++count;
	}

	fprintf(f, "\n%d\n", count);
	for( t = all_timeouts; t != NULL; t = t->next ) {
		if( t->ticks > 0 ) {
			fprintf(f, " %d", timeout_id(t));
			fprintf(f, " %d", t->ticks);
			save_actionlist(t->acts, f);
			fprintf(f, "\n");
		}
	}
}

void
load_timeouts(FILE *f)
{
	timeout *t;
	int count, ticks, id;

	fscanf(f, "%d", &count);
	while( count-- ) {
		fscanf(f, "%d", &id);
		fscanf(f, "%d", &ticks);

		t = alloc_timeout(ticks);
		t->resid = -(id);
		t->acts = load_actionlist(f);
	}
}

/******************************************/

void
link_action_trigger(struct action *a, struct old_trigger *t)
{
	add_actionlist_entry(&t->acts, a);
}

void
link_action_timeout(struct action *a, struct timeout *t)
{
	add_actionlist_entry(&t->acts, a);
}

/******************************************/

void
countdown_timeouts(void)
{
	timeout *t;

	for( t = all_timeouts; t != NULL; t = t->next ) {
		t->ticks--;
		if( t->ticks == 0 )
			prepare_actions(&t->acts);
	}
	do_actions();
}

/********************************************************/

void
save_action_pointer(FILE *f, action *ptr, tag_t tag)
{
	if( ptr && ptr->atype == AC_NONE )
		ptr = NULL;
	fprintf(f, " %d %d ", ptr ? action_id(ptr) : 0, (int)tag);
}

int
load_action_pointer(FILE *f, action **ptrP)
{
	int id, tag;

	fscanf(f, "%d %d", &id, &tag);
	if( id )
		/* TODO: cast void ** richtig? */
		ur_add2(id, (void **)ptrP, TYP_ACTION, (tag_t)tag, action_resolve);
	else
		*ptrP = NULL;
	return id;
}

void
remove_all_actions(void *obj, typ_t typ)
{
		attrib *a;
		attrib **ap;
		action *act;

		ap = typdata[typ].getattribs(obj);
		a = a_find(*ap, &at_action);
		while( a ) {
			act = (action *)a->data.v;
			change_all_pointers(act, TYP_ACTION, NULL);
			act->atype = AC_NONE;
			a = a->nexttype;
		}
}

void
remove_action(action *act)
{
#if 0
	attrib **ap;
	attrib *a;
#endif

	if( act ) {
		change_all_pointers(act, TYP_ACTION, NULL);
		act->atype = AC_NONE;
		/* die Aktionsstruktur selber bleibt noch erhalten und wird erst
		 * beim Garbage Collect vorm Speichern gelöscht, weil evtl noch
		 * lokale Stackvariablen auf diese Struktur zeigen.
		 */
#if 0
		ap = typdata[act->typ].getattribs(act->obj);
		a = a_find_by_vdata(*ap, &at_action, (void *)act);
		assert(a != NULL);
		a_remove(ap, a);
#endif
	}
}

/************ Frontends für die einzelnen Aktionen *************/

struct action *
action_destroy(void *obj2, typ_t typ2, spread_t spr2)
{
	return alloc_action(AC_DESTROY, obj2, typ2, spr2);
}

struct action *
action_removecurse(void *obj2, typ_t typ2, spread_t spr2, curse_t id, int id2)
{
	action *ac;
	ac = alloc_action(AC_REMOVECURSE, obj2, typ2, spr2);
	ac->i[0] = (int)id;
	ac->i[1] = id2;
	return ac;
}

struct action *
action_removerelation(void *obj2, typ_t typ2, spread_t spr2, relation_t id)
{
	action *ac;
	ac = alloc_action(AC_REMOVERELATION, obj2, typ2, spr2);
	ac->i[0] = (int)id;
	return ac;
}

struct action *
action_sendmessage(void *obj2, typ_t typ2, spread_t spr2,
	char *m, msg_t mtype, int mlevel)
{
	action *ac;
	assert(typ2 == TYP_UNIT);
	ac = alloc_action(AC_SENDMESSAGE, obj2, typ2, spr2);
	ac->string = strdup(m);
	ac->i[0] = (int)mtype;
	ac->i[1] = mlevel;
	return ac;
}

struct action *
action_changeirace(void *obj2, typ_t typ2, spread_t spr2,
		    race_t race)
{
	action *ac;
	assert(typ2 == TYP_UNIT);
	ac = alloc_action(AC_CHANGEIRACE, obj2, typ2, spr2);
	ac->i[0] = (int)race;
	return ac;
}

struct action *
action_changerace(void *obj2, typ_t typ2, spread_t spr2,
		    race_t race, race_t irace)
{
	action *ac;
	assert(typ2 == TYP_UNIT);
	ac = alloc_action(AC_CHANGERACE, obj2, typ2, spr2);
	ac->i[0] = (int)race;
	ac->i[1] = (int)irace;
	return ac;
}

struct action *
action_shock(void *obj2, typ_t typ2, spread_t spr2)
{
	action *ac;
	assert(typ2 == TYP_UNIT);
	ac = alloc_action(AC_SHOCK, obj2, typ2, spr2);
	return ac;
}

struct action *
action_changefaction(void *obj2, typ_t typ2, spread_t spr2,
		    int unique_id)
{
	action *ac;
	assert(typ2 == TYP_UNIT);
	ac = alloc_action(AC_CHANGEFACTION, obj2, typ2, spr2);
	ac->i[0] = unique_id;
	return ac;
}

struct action *
action_createunit(void *obj2, typ_t typ2, spread_t spr2,
		int fno, int number, race_t race)
{
	action *ac;
	assert(typ2 == TYP_REGION);
	ac = alloc_action(AC_CREATEUNIT, obj2, typ2, spr2);
	ac->i[0] = fno;
	ac->i[1] = number;
	ac->i[2] = race;
	return ac;
}

struct action *
action_createmagicboostcurse(void *obj2, typ_t typ2, spread_t spr2, int power)
{
	action *ac;
	assert(typ2 == TYP_UNIT);
	ac = alloc_action(AC_CREATEMAGICBOOSTCURSE, obj2, typ2, spr2);
	ac->i[0] = power;
	return ac;
}

#endif
