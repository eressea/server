/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
 */

#include <config.h>

#include "eressea.h"

/* kernel includes */
#include "unit.h"
#include "region.h"
#include "faction.h"
#include "ugroup.h"

/* attributes includes */
#include <attributes/ugroup.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>

/* TODO:
 * - Anzeige (CR)
 * - Speicherung
 * - Evt. NACH (Betrete/Verlasse?) des Leaders vererben auf Members.
 * - Routine zur automatischen Aufsplittung.
 */

/* Nur die erste Einheit in der Liste 
	(ugroup->unit_array[0] == u) kann NACH ausführen. */

#ifdef USE_UGROUPS
boolean
is_ugroupleader(const unit *u, const ugroup *ug)
{
	if(ug->unit_array[0] == u) return true;
	return false;
}

ugroup *
findugroupid(const faction *f, int id)
{
	ugroup *ug;

	for(ug=f->ugroups;ug; ug=ug->next) {
		if(ug->id == id) return ug;
	}
	return NULL;
}

ugroup *
findugroup(const unit *u)
{
	attrib *a = a_find(u->attribs, &at_ugroup);
	if(!a) return NULL;
	return findugroupid(u->faction, a->data.i);
}

static int
ugroupfreeid(const ugroup *ug)
{
	const ugroup *ug2 = ug;
	int id = 0;
	
	while(ug2) {
		if(ug2->id == id) {
			id++;
			ug2 = ug;
		} else {
			ug2 = ug2->next;
		}
	}
	return id;
}

ugroup *
createugroup(unit *uleader, unit *umember)
{
	ugroup *ug  = calloc(1,sizeof(ugroup));

	ug->id   = ugroupfreeid(uleader->faction->ugroups);
	ug->members = 2;
	ug->unit_array = malloc(2 * sizeof(unit *));
	ug->unit_array[0] = uleader;
	a_add(&uleader->attribs, a_new(&at_ugroup))->data.i = ug->id;
	ug->unit_array[1] = umember;
	a_add(&umember->attribs, a_new(&at_ugroup))->data.i = ug->id;
	ug->next = uleader->faction->ugroups;
	uleader->faction->ugroups = ug;

	return ug;
}

void
join_ugroup(unit *u, strlist *order)
{
	unit *u2;
	ugroup *ug;

	u2 = getunit(u->region, u->faction);
	if(!u2 || cansee(u->faction, u->region, u2, 0) == false) {
		cmistake(u, order->s, 63, MSG_EVENT);
		return;
	}
	if(u2 == u) {
		cmistake(u, order->s, 292, MSG_EVENT);
		return;
	}
	if(u2->faction != u->faction) {
		cmistake(u, order->s, 293, MSG_EVENT);
		return;
	}
	if(u2->building != u->building || u2->ship != u->ship) {
		cmistake(u, order->s, 294, MSG_EVENT);
		return;
	}
	if(a_find(u->attribs, &at_ugroup)) {
		cmistake(u, order->s, 290, MSG_EVENT);
		return;
	}

	ug = findugroup(u2);

	if(ug) {
		ug->members++;
		ug->unit_array = realloc(ug->unit_array,
			ug->members * sizeof(unit *));
		ug->unit_array[ug->members-1] = u;
		a_add(&u->attribs, a_new(&at_ugroup))->data.i = ug->id;
	} else {
		createugroup(u2, u);
	}
}

void
leave_ugroup(unit *u, strlist *order)
{
	attrib *a  = a_find(u->attribs, &at_ugroup);
	ugroup *ug;
	boolean found = false;
	int i;

	if(!a) {
		cmistake(u, order->s, 286, MSG_EVENT);
		return;
	}

	ug = findugroupid(u->faction, a->data.i);
	for(i=0; i<ug->members; i++) {
		if(found == true) {
			ug->unit_array[i-1] = ug->unit_array[i];
		} else if(ug->unit_array[i] == u) {
			found = true;
		}
	}
	ug->members--;
	a_remove(&u->attribs, a);

	if(ug->members == 1) {
		a_remove(&ug->unit_array[0]->attribs, a);
		free(ug->unit_array);
		removelist(&u->faction->ugroups, ug);
	} else {
		ug->unit_array = realloc(ug->unit_array,
			ug->members * sizeof(unit *));
	}
}

void
ugroups(void)
{
	region *r;
	unit *u;
	strlist *o;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			for(o = u->orders; o; o=o->next) {
				if(igetkeyword(o->s, u->faction->locale) == K_LEAVEUGROUP) {
					leave_ugroup(u, o);
				}
			}
		}
		for(u=r->units; u; u=u->next) {
			for(o = u->orders; o; o=o->next) {
				if(igetkeyword(o->s, u->faction->locale) == K_JOINUGROUP) {
					join_ugroup(u, o);
				}
			}
		}
	}
}

#endif /* USE_UGROUPS */
