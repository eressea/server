/* vi: set ts=2:
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
#include "buildingcurse.h"

/* kernel includes */
#include <message.h>
#include <nrmessage.h>
#include <objtypes.h>
#include <building.h>
#include <ship.h>
#include <curse.h>

/* util includes */
#include <base36.h>
#include <functions.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>


int
cinfo_building(const locale * lang, void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(typ);
	assert(typ == TYP_BUILDING);

	if (self != 0){ /* owner or inside */
		msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
	} else { /* outside */
		msg = msg_message(mkname("curseinfo", "buildingunknown"), "id", c->no);
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);
	return 1;
}

/* CurseInfo mit Spezialabfragen */

/* C_MAGICSTONE*/
static int
cinfo_magicrunes(void * obj, typ_t typ, curse *c, int self)
{

	if (typ == TYP_BUILDING){
		building * b;
		b = (building*)obj;
		if (self != 0){
			sprintf(buf, "Auf den Mauern von %s erkennt man seltsame Runen. (%s)",
					b->name, curseid(c));
			return 1;
		}
	} else if (typ == TYP_SHIP) {
		ship *sh;
		sh = (ship*)obj;
		if (self != 0){
			sprintf(buf, "Auf den Planken von %s erkennt man seltsame Runen. (%s)",
					sh->name, curseid(c));
			return 1;
		}
	}

	return 0;
}
static struct curse_type ct_magicrunes = { "magicrunes",
	CURSETYP_NORM, 0, M_SUMEFFECT,
	"Dieses Zauber verstärkt die natürliche Widerstandskraft gegen eine "
	"Verzauberung.",
	cinfo_magicrunes
};

/* Heimstein-Zauber */
static struct curse_type ct_magicwalls = { "magicwalls",
	CURSETYP_NORM, 0, NO_MERGE,
	"Die Macht dieses Zaubers ist fast greifbar und tief in die Mauern "
	"gebunden. Starke elementarmagische Kräfte sind zu spüren. "
	"Vieleicht wurde gar ein Erdelementar in diese Mauern gebannt. "
	"Ausser ebenso starkter Antimagie wird nichts je diese Mauern "
	"gefährden können.",
	cinfo_building
};

/* Feste Mauer - Präkampfzauber, wirkt nur 1 Runde */
static struct curse_type ct_strongwall = { "strongwall",
	CURSETYP_NORM, 0, NO_MERGE,
	"",
	NULL
};

/* Ewige Mauern-Zauber */
static struct curse_type ct_nocostbuilding = { "nocostbuilding",
	CURSETYP_NORM, 0, NO_MERGE,
	"Die Macht dieses Zaubers ist fast greifbar und tief in die Mauern "
	"gebunden. Unbeeindruck vom Zahn der Zeit wird dieses Gebäude wohl "
	"auf Ewig stehen.",
	cinfo_building
};


void 
register_buildingcurse(void)
{
	register_function((pf_generic)cinfo_magicrunes, "curseinfo::magicrunes");

	ct_register(&ct_magicwalls);
	ct_register(&ct_strongwall);
	ct_register(&ct_magicrunes);
	ct_register(&ct_nocostbuilding);
}
