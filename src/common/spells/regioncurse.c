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
cinfo_region(const locale * lang, void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(typ);
	unused(self);
	unused(obj);

	assert(typ == TYP_REGION);

	msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);
	return 1;
}

/* CurseInfo mit Spezialabfragen */

static int
cinfo_cursed_by_the_gods(const locale * lang,void * obj, typ_t typ, curse *c, int self)
{
	region *r;
	message * msg;

	unused(typ);
	unused(self);

	assert(typ == TYP_REGION);
	r = (region *)obj;
	if (rterrain(r)!=T_OCEAN){
		msg = msg_message("curseinfo::godcurse", "id", c->no);
	} else {
		msg = msg_message("curseinfo::godcurseocean", "id", c->no);
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);

	return 1;
}
/* C_GBDREAM, */
static int
cinfo_dreamcurse(const locale * lang,void * obj, typ_t typ, curse *c, int self)
{
	unused(self);
	unused(typ);
	unused(obj);
	assert(typ == TYP_REGION);

	if (c->effect > 0){
		msg = msg_message("curseinfo::gooddream", "id", c->no);
		sprintf(buf, "Die Leute haben schöne Träume. (%s)", curseid(c));
	}else{
		msg = msg_message("curseinfo::baddream", "id", c->no);
		sprintf(buf, "Albträume plagen die Leute. (%s)", curseid(c));
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);

	return 1;
}
/* C_MAGICSTREET */
static int
cinfo_magicstreet(const locale * lang,void * obj, typ_t typ, curse *c, int self)
{
	unused(typ);
	unused(self);
	unused(obj);

	assert(typ == TYP_REGION);

	sprintf(buf, "Die Straßen sind erstaunlich trocken und gut begehbar");
	/* Warnung vor auflösung!*/
	if (c->duration <= 2){
		scat(", doch an manchen Stellen bilden sich wieder die erste Schlammlöcher");
	}
	scat(". (");
	scat(itoa36(c->no));
	scat(")");

	return 1;
}
