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
#include "eressea.h"
#include "faction.h"
#include "unit.h"
#include "race.h"
#include "region.h"
#include "plane.h"

/* util includes */
#include <base36.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>

const char *
factionname(const faction * f)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;

	char *buf = idbuf[(++nextbuf) % 8];

	if (f && f->name) {
		sprintf(buf, "%s (%s)", strcheck(f->name, NAMESIZE), itoa36(f->no));
	} else {
		sprintf(buf, "Unbekannte Partei (?)");
	}
	return buf;
}

void *
resolve_faction(void * id) {
   return findfaction((int)id);
}

#define MAX_FACTION_ID (36*36*36*36)

static int
unused_faction_id(void)
{
	int id = rand()%MAX_FACTION_ID;

	while(!faction_id_is_unused(id)) {
		id++; if(id == MAX_FACTION_ID) id = 0;
	}

	return id;
}

unit *
addplayer(region *r, char *email, race_t frace)
{
	int i;
	unit *u;

	faction *f = (faction *) calloc(1, sizeof(faction));

	set_string(&f->email, email);

	for (i = 0; i < 6; i++) buf[i] = (char) (97 + rand() % 26); buf[i] = 0;
	set_string(&f->passw, buf);

	f->lastorders = turn;
	f->alive = 1;
	f->age = 0;
	f->race = frace;
	f->magiegebiet = 0;
	set_ursprung(f, 0, r->x, r->y);

	f->options = Pow(O_REPORT) | Pow(O_ZUGVORLAGE) | Pow(O_SILBERPOOL);

	f->no = unused_faction_id();
	register_faction_id(f->no);

	f->unique_id = max_unique_id + 1;
	max_unique_id++;

	sprintf(buf, "Partei %s", factionid(f));
	set_string(&f->name, buf);
	fset(f, FL_UNNAMED);

	addlist(&factions, f);

	u = createunit(r, f, 1, f->race);
	give_starting_equipment(r, u);
	fset(u, FL_ISNEW);
	if (f->race == RC_DAEMON) {
		do
			u->irace = (char) (rand() % MAXRACES);
		while (u->irace == RC_DAEMON || race[u->irace].nonplayer);
	}
	fset(u, FL_PARTEITARNUNG);

	return u;
}

