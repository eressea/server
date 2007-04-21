/* vi: set ts=2:
 *
 * Eressea PB(E)M host Copyright (C) 1998-2003
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
	int m = read_unit_reference(&ad->mage, F);
	int t = read_unit_reference(&ad->target, F);
	if (m!=AT_READ_OK || t!=AT_READ_OK) return AT_READ_FAIL;
	return AT_READ_OK;
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
  unit *mage = co->magician.u;
  int cast_level = co->level;
  spellparameter *pa = co->par;

  opfer = pa->param[0]->data.u;

  /* Der Alp gehört den Monstern, darum erhält der Magier auch keine
  * Regionsberichte von ihm.  Er erhält aber später eine Mitteilung,
  * sobald der Alp sein Opfer erreicht hat.
  */
  alp = createunit(r, findfaction(MONSTER_FACTION), 1, new_race[RC_ALP]);
  set_level(alp, SK_STEALTH, 7);
  set_string(&alp->name, "Alp");
  setstatus(alp, ST_FLEE); /* flieht */

  {
    attrib * a = a_add(&alp->attribs, a_new(&at_alp));
    alp_data * ad = (alp_data*) a->data.v;
    ad->mage = mage;
    ad->target = opfer;
  }

  {
    /* Wenn der Alp stirbt, den Magier nachrichtigen */
    add_trigger(&alp->attribs, "destroy", trigger_unitmessage(mage, 
      "Ein Alp starb, ohne sein Ziel zu erreichen.", MSG_EVENT, ML_INFO));
    /* Wenn Opfer oder Magier nicht mehr existieren, dann stirbt der Alp */
    add_trigger(&mage->attribs, "destroy", trigger_killunit(alp));
    add_trigger(&opfer->attribs, "destroy", trigger_killunit(alp));
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
  variant effect;

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
	a_removeall(&alp->attribs, &at_eventhandler);

  /* Alp umwandeln in Curse */
  effect.i = -2;
	c = create_curse(mage, &opfer->attribs, ct_find("worse"), 2, 2, effect, opfer->number);
	/* solange es noch keine spezielle alp-Antimagie gibt, reagiert der
	 * auch auf normale */
	destroy_unit(alp);

	{
		/* wenn der Magier stirbt, wird der Curse wieder vom Opfer genommen */
		add_trigger(&mage->attribs, "destroy", trigger_removecurse(c, opfer));
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
