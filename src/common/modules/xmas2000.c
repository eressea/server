/* vi: set ts=2:
 *
 *	$Id: xmas2000.c,v 1.2 2001/01/28 08:01:52 enno Exp $
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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
#include "xmas2000.h"

/* gamecode includes */
#include <creation.h>

/* kernel includes */
#include <plane.h>
#include <item.h>
#include <unit.h>
#include <region.h>
#include <building.h>
#include <movement.h>
#include <event.h>
#include <faction.h>
#include <race.h>

/* util includes */
#include <goodies.h>
#include <resolve.h>
#include <base36.h>

#include <stdlib.h>

static int
xmasgate_handle(trigger * t, void * data)
{
	/* call an event handler on xmasgate.
	 * data.v -> ( variant event, int timer )
	 */
	unit * santa = ufindhash(atoi36("xmas"));
	building *b = (building *)t->data.v;
	if (b!=NULL) {
		unit ** up = &b->region->units;
		if (santa->region!=b->region) santa = NULL;
		while (*up) {
			unit * u = *up;
			if (u->building==b) {
				region * r = u->region;
				faction * f = u->faction;
				unit * home = f->units;
				unit * u2 = r->units;
				while (u2) {
					if (u2->faction==f && u2!=u && u2->number) break;
					u2 = u2->next;
				}
				while (home && (home->region==b->region || home->region->land==NULL)) home = home->nextF;
				if (home==NULL) continue;
				if (santa!=NULL && u2==NULL) {
					char zText[256];
					item_type * itype = olditemtype[(rand() % 4) + I_KEKS];
					i_change(&u->items, itype, 1);
					sprintf(zText, "%s gibt %d %s an %s.", unitname(santa), 1, locale_string(f->locale, resourcename(itype->rtype, GR_PLURAL)), unitname(u));
					i_change(&u->items, itype, 1);
					addmessage(home->region, u->faction, zText, MSG_COMMERCE, ML_INFO);
				}
				move_unit(u, home->region, NULL);
			}
			if (*up==u) up = &u->next;
		}
	} else
		fprintf(stderr, "\aERROR: could not perform xmasgate::handle()\n");
	unused(data);
	return 0;
}

static void
xmasgate_write(const trigger * t, FILE * F)
{
	building *b = (building *)t->data.v;
	fprintf(F, "%s ", itoa36(b->no));
}

static int
xmasgate_read(trigger * t, FILE * F)
{
	char zText[128];
	int i;

	fscanf(F, "%s", zText);
	i = atoi36(zText);
	t->data.v = findbuilding(i);
	if (t->data.v==NULL) ur_add((void*)i, &t->data.v, resolve_building);

	return 1;
}

struct trigger_type tt_xmasgate = {
	"xmasgate",
	NULL,
	NULL,
	xmasgate_handle,
	xmasgate_write,
	xmasgate_read
};

static trigger *
trigger_xmasgate(building * b)
{
	trigger * t = t_new(&tt_xmasgate);
	t->data.v = b;
	return t;
}

static void
make_gates(region * r)
{
	const building_type * btype = bt_find("xmas_exit");
	building * b;
	if (btype==NULL)
		btype = bt_make("xmas_exit", BTF_NOBUILD | BTF_INDESTRUCTIBLE, -1, 2, 10);
	else {
		b = r->buildings;
		while (b!=NULL && b->type!=btype) b = b->next;
		if (b!=NULL) return; /* gibt schon einen */
	}

	b = new_building(btype, r, NULL);
	b->size = btype->maxsize;
	b->name = strdup("Der Weg nach Hause");
	b->display = strdup("Achtung, hier gibt es die Geschenke!");
	add_trigger(&b->attribs, "timer", trigger_xmasgate(b));
}

static unit *
make_santa(region * r)
{
	unit * santa = ufindhash(atoi36("xmas"));

	while (santa && santa->race!=RC_ILLUSION) {
		uunhash(santa);
		santa->no = newunitid();
		uhash(santa);
		santa = ufindhash(atoi36("xmas"));
	}
	if (!santa) {
		faction * f = findfaction(atoi36("rr"));
		if (f==NULL) return NULL;
		f->alive = true;
		santa = createunit(r, f, 1, RC_ILLUSION);
		uunhash(santa);
		santa->no = atoi36("xmas");
		uhash(santa);
		fset(santa, FL_PARTEITARNUNG);
		santa->irace = RC_GNOME;
		set_string(&santa->name, "Ein dicker Gnom mit einem Rentierschlitten");
		set_string(&santa->display, "hat: 12 Rentiere, Schlitten, Sack mit Geschenken, Kekse für Khorne");
	}
	return santa;
}

static void
santa_comes_to_town(region * r)
{
	unit * santa = make_santa(r);
	faction * f;

	fset(santa, FL_TRAVELTHRU);
	for (f = factions;f;f=f->next) {
		unit * u;
		unit * senior = f->units;
		if (nonplayer_race(f->race)) continue;
		for (u = f->units; u; u=u->nextF) {
			if (senior->age < u->age) senior = u;
		}
		if (!senior) continue;

		sprintf(buf, "von %s: 'Ho ho ho. Frohe Weihnachten, und alles Gute für dein Volk, %s.'", unitname(santa), unitname(senior));
		addmessage(senior->region, 0, buf, MSG_MESSAGE, ML_IMPORTANT);

		travelthru(santa, senior->region);
	}
}

void
init_xmas2000(void)
{
	tt_register(&tt_xmasgate);
}

void
create_xmas2000(int x, int y)
{
	const int prefixes = 18;
	const char *prefix[] = {
		"Artige", "Brave", "Liebe", "Nette", "Anständige",
	    "Rechtschaffene", "Tugendhafte", "Sittsame", "Ehrbare", "Keusche",
		"Tugendsame", "Züchtige", "Fromme", "Musterhafte", "Manierliche",
		"Tadellose", "Unverdorbene", "Höfliche"
	};
	plane * xmas = getplanebyname("Nordpol");
	faction * f;
	region * r = findregion(x, y);
	direction_t dir;

	if (r!=NULL) {
		make_santa(r);
		return;
	}
	if (xmas==NULL) xmas = create_new_plane(2000, "Nordpol", x-1, x+1, y-1, y+1, PFL_NORECRUITS|PFL_NOALLIANCES|PFL_LOWSTEALING|PFL_NOGIVE|PFL_NOATTACK|PFL_NOMAGIC|PFL_NOSTEALTH|PFL_NOTEACH|PFL_NOBUILD|PFL_NOFEED|PFL_FRIENDLY);
	r = new_region(x, y);
	terraform(r, T_PLAIN);
	set_string(&r->land->name, "Weihnachtsinsel");
	rsettrees(r, 1000);
	rsetpeasants(r, 0);
	rsetmoney(r, 0);
	for (dir=0;dir!=MAXDIRECTIONS;++dir) {
		region * n = findregion(x + delta_x[dir], y + delta_y[dir]);
		if (n==NULL) n = new_region(x + delta_x[dir], y + delta_y[dir]);
		terraform(n, T_OCEAN);
	}
	santa_comes_to_town(r);
	for (f = factions; f != NULL; f = f->next) if (f->alive && f->units) {
		char zText[128];
		unit * u;

		u = createunit(r, f, 2, f->race);
		if (f->race==RC_DAEMON) u->irace = RC_HUMAN;
		sprintf(zText, "%s %s", prefix[rand()%prefixes], race[u->irace].name[1]);
		fset(u, FL_PARTEITARNUNG);
		set_string(&u->name, zText);
	}
	make_gates(r);
}
