/* vi: set ts=2:
 *
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
#include "lmsreward.h"

/* kernel includes */
#include <item.h>
#include <region.h>
#include <unit.h>
#include <curse.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

static int
info_lmsstory(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unused(lang);
	unused(obj);
	unused(typ);
	unused(c);
	unused(self);
	strcpy(buf, "Die Bauern der Region erzählen sich die Geschichte von den glorreichen Siegern des Last-Man-Standing Turniers.");
	return 1;
}

static curse_type ct_lmsstory = {
	"lmsstory", 0, 0, 0, "xxx", &info_lmsstory
};


static int
age_lmsstory(attrib * a)
{
	return --a->data.i;
}

static attrib_type at_lmsstory = {
	"lms_story",
	NULL, NULL,
	age_lmsstory,
	a_writedefault, 
	a_readdefault, 
	ATF_UNIQUE /* | ATF_CURSE */ /* Das geht nicht, weil der Inhalt
																	des Attributs kein curse * ist */
};

#if 0
static int
use_lmsreward(struct unit * u, const struct item_type * itype, const char * cmd)
{
	region * r = u->region;
	attrib * a = a_find(r->attribs, &at_lmsstory);
	if (!a) a = a_add(&r->attribs, a_new(&at_lmsstory));

	a->data.i = 4 + (rand() % 8);
	sprintf(buf, "%s erzählt in allen Schenken die Geschichte vom glorreichen Sieg im Last-Man-Standing Turnier. Noch wochenlang werden die Bauern in %s sich diese Geschichte erzählen.", unitname(u), regionid(r));
	addmessage(r, NULL, buf, MSG_MESSAGE, ML_IMPORTANT);

	unused(itype);
	unused(cmd);
	return -1;
}
#endif

static resource_type rt_lmsreward = {
	{ "lmsreward", "lmsreward_p" },
	{ "lmsreward", "lmsreward_p" },
	RTF_ITEM,
	&res_changeitem
};

item_type it_lmsreward = {
	&rt_lmsreward,           /* resourcetype */
	ITF_NOTLOST|ITF_CURSED, 0, 0,       /* flags, weight, capacity */
	NULL,                    /* construction */
	NULL,	/* anstelle von use_lmsreward */
/*	&use_lmsreward, */
	NULL
};

void
register_lmsreward(void)
{
	it_register(&it_lmsreward);
	at_register(&at_lmsstory);
}
