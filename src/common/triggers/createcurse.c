/* vi: set ts=2:
 *
 *	
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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
#include "createcurse.h"

/* kernel includes */
#include <curse.h>
#include <unit.h>

/* util includes */
#include <event.h>
#include <resolve.h>
#include <base36.h>

/* ansi includes */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


#include <stdio.h>
/***
 ** restore a mage that was turned into a toad
 **/

typedef struct createcurse_data {
	struct unit * mage;
	struct unit * target;
	const curse_type * type;
	double vigour;
	int duration;
	int effect;
	int men;
} createcurse_data;

static void
createcurse_init(trigger * t)
{
	t->data.v = calloc(sizeof(createcurse_data), 1);
}

static void
createcurse_free(trigger * t)
{
	free(t->data.v);
}

static int
createcurse_handle(trigger * t, void * data)
{
	/* call an event handler on createcurse.
	 * data.v -> ( variant event, int timer )
	 */
	createcurse_data * td = (createcurse_data*)t->data.v;
	if (td->mage!=NULL && td->target!=NULL) {
		create_curse(td->mage, &td->target->attribs,
			td->type, td->vigour, td->duration, td->effect, td->men);
	} else {
		log_error(("could not perform createcurse::handle()\n"));
	}
	unused(data);
	return 0;
}

static void
createcurse_write(const trigger * t, FILE * F)
{
	createcurse_data * td = (createcurse_data*)t->data.v;
	fprintf(F, "%s ", itoa36(td->mage->no));
	fprintf(F, "%s ", itoa36(td->target->no));
	fprintf(F, "%s %lf %d %d %d ", td->type->cname, td->vigour, td->duration, td->effect, td->men);
}

static int
createcurse_read(trigger * t, FILE * F)
{
	createcurse_data * td = (createcurse_data*)t->data.v;
	char zText[128];
	int i;

	fscanf(F, "%s", zText);
	i = atoi36(zText);
	td->mage = findunit(i);
	if (td->mage==NULL) ur_add((void*)i, (void**)&td->mage, resolve_unit);

	fscanf(F, "%s", zText);
	i = atoi36(zText);
	td->target = findunit(i);
	if (td->target==NULL) ur_add((void*)i, (void**)&td->target, resolve_unit);

	if (global.data_version<CURSETYPE_VERSION) {
		int id1, id2;
		fscanf(F, "%d %d %lf %d %d %d ", &id1, &id2, &td->vigour, &td->duration, &td->effect, &td->men);
		assert(id2==0);
		td->type = ct_find(oldcursename(id1));
	} else {
		fscanf(F, "%s %lf %d %d %d ", zText, &td->vigour, &td->duration, &td->effect, &td->men);
		td->type = ct_find(zText);
	}
	return AT_READ_OK;
}

trigger_type tt_createcurse = {
	"createcurse",
	createcurse_init,
	createcurse_free,
	createcurse_handle,
	createcurse_write,
	createcurse_read
};

trigger *
trigger_createcurse(struct unit * mage, struct unit * target,
					const curse_type * ct, double vigour, int duration,
					int effect, int men)
{
	trigger * t = t_new(&tt_createcurse);
	createcurse_data * td = (createcurse_data*)t->data.v;
	td->mage = mage;
	td->target = target;
	td->type = ct;
	td->vigour = vigour;
	td->duration = duration;
	td->effect = effect;
	td->men = men;
	return t;
}
