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
#include "regioncurse.h"

/* kernel includes */
#include "message.h"
#include "nrmessage.h"
#include "objtypes.h"
#include "curse.h"

/* util includes */
#include <message.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>


static int
cinfo_ship(const locale * lang, void * obj, typ_t typ, curse *c, int self)
{
	message * msg;
	ship * sh;

	unused(typ);

	assert(typ == TYP_SHIP);
	sh = (ship*)obj;

	if (self){
		msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
	} else {
		msg = msg_message(mkname("curseinfo", "shipunknown"), "id", c->no);
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);
	return 1;
}

static int
cinfo_ship_onlyowner(const locale * lang, void * obj, typ_t typ, curse *c, int self)
{
	message * msg;
	ship * sh;

	unused(typ);

	assert(typ == TYP_SHIP);
	sh = (ship*)obj;

	if (self){
		msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}
	return 0;
}

/* CurseInfo mit Spezialabfragen */

/* C_SHIP_NODRIFT */
static int
cinfo_shipnodrift(void * obj, typ_t typ, curse *c, int self)
{
	ship * sh;
	unused(typ);

	assert(typ == TYP_SHIP);
	sh = (ship*)obj;

	if (self){
		sprintf(buf, "%s ist mit guten Wind gesegnet", sh->name);
		if (c->duration <= 2){
			scat(", doch der Zauber beginnt sich bereits aufzulösen");
		}
		scat(".");
	} else {
		sprintf(buf, "Ein silberner Schimmer umgibt das Schiff.");
	}
	scat(" (");
	scat(itoa36(c->no));
	scat(")");
	return 1;
}

