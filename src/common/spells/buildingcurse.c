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
cinfo_building(const locale * lang, void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(typ);
	assert(typ == TYP_BUILDING);

	if (self){
		msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
	} else {
		msg = msg_message(mkname("curseinfo", "buildingunknown"), "id", c->no);
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);
	return 1;
}

/* CurseInfo mit Spezialabfragen */


