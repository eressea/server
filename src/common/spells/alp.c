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
#include <eressea.h>
#include "alp.h"

#include <unit.h>
#include <region.h>
#include <skill.h>
#include <magic.h>

/* util includes */
#include <event.h>
#include <triggers/createcurse.h>
#include <triggers/killunit.h>
#include <triggers/removecurse.h>
#include <triggers/unitmessage.h>

/* libc includes */
#include <stdlib.h>
#include <string.h>

extern const char *directions[];

typedef struct alp_data {
	unit * mage;
	unit * target;
} alp_data;

static void
alp_init(attrib * a)
{
	a->data.v = calloc(sizeof(alp_data), 1);
}

static void 
alp_done(attrib * a) 
{
	free(a->data.v);
}

static int
alp_verify(attrib * a)
{
	alp_data * ad = (alp_data*)a->data.v;
	if (ad->mage && ad->target) return 1;
	return 0; /* kaputt */
}

static void
alp_write(const attrib * a, FILE * F)
{
	alp_data * ad = (alp_data*)a->data.v;
	write_unit_reference(ad->mage, F);
	write_unit_reference(ad->target, F);
}

static int
alp_read(attrib * a, FILE * F)
{
	alp_data * ad = (alp_data*)a->data.v;
	read_unit_reference(&ad->mage, F);
	read_unit_reference(&ad->target, F);
	return 1;
}

static attrib_type at_alp = { 
	"alp", 
	alp_init, 
	alp_done, 
	alp_verify, 
	alp_write, 
	alp_read,
	ATF_UNIQUE
};

int
sp_summon_alp(struct castorder *co)
{
	unit *alp, *opfer;
	region *r = co->rt;
	unit *mage = (unit *)co->magician;
	int cast_level = co->level;
	spellparameter *pa = co->par;

	opfer = pa->param[0]->data.u;

	/* Der Alp gehört den Monstern, darum erhält der Magier auch keine
	 * Regionsberichte von ihm.  Er erhält aber später eine Mitteilung,
	 * sobald der Alp sein Opfer erreicht hat.
	 */
	alp = createunit(r, findfaction(MONSTER_FACTION), 1, new_race[RC_ALP]);
	if (r==mage->region) {
		alp->building = mage->building;
		alp->ship = mage->ship;
	}
	set_level(alp, SK_STEALTH, 7);
	set_string(&alp->name, "Alp");
	alp->status = ST_FLEE;	/* flieht */

	{
		attrib * a = a_add(&alp->attribs, a_new(&at_alp));
		alp_data * ad = (alp_data*) a->data.v;
		ad->mage = mage;
		ad->target = opfer;
	}

	strcpy(buf, "Ein Alp starb, ohne sein Ziel zu erreichen.");
	{
#ifdef NEW_TRIGGER
		/* Wenn der Alp stirbt, den Magier nachrichtigen */
		add_trigger(&alp->attribs, "destroy", trigger_unitmessage(mage, buf, MSG_EVENT, ML_INFO));
		/* Wenn Opfer oder Magier nicht mehr existieren, dann stirbt der Alp */
		add_trigger(&mage->attribs, "destroy", trigger_killunit(alp));
		add_trigger(&opfer->attribs, "destroy", trigger_killunit(alp));
#else
		/* da der Alp niemals GIB PERSONEN ausfuehrt, koennen alle seine
		 * spread-Angaben bei Relations und Actions vom Typ SPREAD_NEVER sein.
		 */

		old_trigger *tr1, *tr2;
		action *ac;
		/* Wenn der Alp stirbt, den Magier nachrichtigen */
		tr1 = create_trigger(alp, TYP_UNIT, SPREAD_NEVER, TR_DESTRUCT);
		ac = action_sendmessage(mage, TYP_UNIT, SPREAD_TRANSFER, buf, MSG_EVENT, ML_INFO);
		link_action_trigger(ac, tr1);

		/* Wenn Opfer oder Magier nicht mehr existieren, dann stirbt der Alp */
		tr1 = create_trigger(mage, TYP_UNIT, SPREAD_TRANSFER, TR_DESTRUCT);
		tr2 = create_trigger(opfer, TYP_UNIT, SPREAD_TRANSFER, TR_DESTRUCT);
		ac = action_destroy(alp, TYP_UNIT, SPREAD_NEVER);
		link_action_trigger(ac, tr1);
		link_action_trigger(ac, tr2);
#endif
	}
	sprintf(buf, "%s beschwört den Alp %s für %s.", unitname(mage),
		unitname(alp), unitname(opfer));
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	return cast_level;
}


void
alp_findet_opfer(unit *alp, region *r)
{
	curse * c;
	attrib * a = a_find(alp->attribs, &at_alp);
	alp_data * ad = (alp_data*)a->data.v;
	unit *mage = ad->mage;
	unit *opfer = ad->target;

	assert(opfer);
	assert(mage);

	/* Magier und Opfer Bescheid geben */
	strcpy(buf, "Ein Alp hat sein Opfer gefunden!");
	addmessage(r, mage->faction, buf, MSG_MAGIC, ML_INFO);

	sprintf(buf, "Ein Alp springt auf den Rücken von %s.",
					unitname(opfer));
	addmessage(r, opfer->faction, buf, MSG_EVENT, ML_IMPORTANT);

	/* Relations werden in destroy_unit(alp) automatisch gelöscht.
	 * Die Aktionen, die beim Tod des Alps ausgelöst werden sollen,
	 * müssen jetzt aber deaktiviert werden, sonst werden sie gleich
	 * beim destroy_unit(alp) ausgelöst.
	 */
#ifdef NEW_TRIGGER
	a_removeall(&alp->attribs, &at_eventhandler);
#else
	remove_all_actions(alp, TYP_UNIT);
#endif
	/* Alp umwandeln in Curse */
	c = create_curse(mage, &opfer->attribs, C_ALLSKILLS, 0, 0, 0, -2, opfer->number);
	/* solange es noch keine spezielle alp-Antimagie gibt, reagiert der
	 * auch auf normale */
	/* set_curseflag(opfer->attribs, C_ALLSKILLS, 0, CURSE_NOAGE+CURSE_IMMUN); */
	set_curseflag(opfer->attribs, C_ALLSKILLS, 0, CURSE_NOAGE);
	destroy_unit(alp);

	{
		/* wenn der Magier stirbt, wird der Curse wieder vom Opfer genommen */
#ifdef NEW_TRIGGER
		add_trigger(&mage->attribs, "destroy", trigger_removecurse(c, opfer));
#else
		old_trigger * tr = create_trigger(mage, TYP_UNIT, SPREAD_TRANSFER, TR_DESTRUCT);
		action * ac = action_removecurse(opfer, TYP_UNIT, SPREAD_MODULO, C_ALLSKILLS, 0);
		/* TODO: der Spread des Curses muß dem Spread der Action angepaßt werden! */
		link_action_trigger(ac, tr);
#endif
	}
}

void
register_alp(void)
{
	at_register(&at_alp);
}

unit *
alp_target(unit *alp)
{
	alp_data* ad;
	unit * target = NULL;

	attrib * a = a_find(alp->attribs, &at_alp);
	
	if (a) {
		ad = (alp_data*) a->data.v;
		target = ad->target;
	}
	return target;

}
