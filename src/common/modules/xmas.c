/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "xmas.h"

/* kernel includes */
#include <unit.h>
#include <region.h>
#include <movement.h>
#include <faction.h>
#include <race.h>

/* util includes */
#include <base36.h>

void
santa_comes_to_town(region * r, unit * santa, void (*action)(unit*))
{
	faction * f;

	fset(santa, FL_TRAVELTHRU);
	for (f = factions;f;f=f->next) {
		unit * u;
		unit * senior = f->units;
		if (!playerrace(f->race)) continue;
		for (u = f->units; u; u=u->nextF) {
			if (senior->age < u->age || effstealth(senior) > effstealth(u)) senior = u;
		}
		if (!senior) continue;

		sprintf(buf, "von %s: 'Ho ho ho. Frohe Weihnachten, und alles Gute für dein Volk, %s.'", unitname(santa), unitname(senior));
		addmessage(senior->region, 0, buf, MSG_MESSAGE, ML_IMPORTANT);

		travelthru(santa, senior->region);
		if (action) action(senior);
	}
}

unit *
make_santa(region * r)
{
	unit * santa = ufindhash(atoi36("xmas"));

	while (santa && santa->race!=new_race[RC_ILLUSION]) {
		uunhash(santa);
		santa->no = newunitid();
		uhash(santa);
		santa = ufindhash(atoi36("xmas"));
	}
	if (!santa) {
		faction * f = findfaction(atoi36("rr"));
		if (f==NULL) f = findfaction(MONSTER_FACTION);
		if (f==NULL) return NULL;
		f->alive = true;
		santa = createunit(r, f, 1, new_race[RC_ILLUSION]);
		uunhash(santa);
		santa->no = atoi36("xmas");
		uhash(santa);
		fset(santa, FL_PARTEITARNUNG);
		santa->irace = new_race[RC_GNOME];
		set_string(&santa->name, "Ein dicker Gnom mit einem Rentierschlitten");
		set_string(&santa->display, "hat: 12 Rentiere, Schlitten, Sack mit Geschenken, Kekse für Khorne");
	}
	return santa;
}
