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

	if (self == 1){ /* owner or inside */
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
		if (self){
			sprintf(buf, "Auf den Mauern von %s erkennt man seltsame Runen. (%s)",
					b->name, curseid(c));
			return 1;
		}
	} else if (typ == TYP_SHIP) {
		ship *sh;
		sh = (ship*)obj;
		if (self){
			sprintf(buf, "Auf den Planken von %s erkennt man seltsame Runen. (%s)",
					sh->name, curseid(c));
			return 1;
		}
	}

	return 0;
}


void 
register_buildingcurse(void)
{
	register_function((pf_generic)cinfo_magicrunes, "curseinfo::magicrunes");
}