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
 *  based on:
 *
 * Atlantis v1.0  13 September 1993 Copyright 1993 by Russell Wallace
 * Atlantis v1.7                    Copyright 1996 by Alex Schröder
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 * This program may not be sold or used commercially without prior written
 * permission from the authors.
 */

#include <config.h>
#include "eressea.h"
#include "curse.h"

/* kernel includes */
#include "magic.h"
#include "skill.h"
#include "unit.h"
#include "region.h"
#include "race.h"
#include "faction.h"
#include "building.h"
#include "ship.h"
#include "objtypes.h"

/* util includes */
#include <util/resolve.h>
#include <util/base36.h>
#include <util/goodies.h>

/* libc includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <math.h>

/* spells includes */
#include <spells/regioncurse.h>
#include <spells/unitcurse.h>
#include <spells/shipcurse.h>
#include <spells/buildingcurse.h>


/* ------------------------------------------------------------- */

#define MAXENTITYHASH 8191
curse *cursehash[MAXENTITYHASH];

void
chash(curse *c)
{
	curse *old = cursehash[c->no %MAXENTITYHASH];

	cursehash[c->no %MAXENTITYHASH] = c;
	c->nexthash = old;
}

static void
cunhash(curse *c)
{
	curse **show;

	for (show = &cursehash[c->no % MAXENTITYHASH]; *show; show = &(*show)->nexthash) {
		if ((*show)->no == c->no)
			break;
	}
	if (*show) {
		assert(*show == c);
		*show = (*show)->nexthash;
		c->nexthash = 0;
	}
}

curse *
cfindhash(int i)
{
	curse *old;

	for (old = cursehash[i % MAXENTITYHASH]; old; old = old->nexthash)
		if (old->no == i)
			return old;
	return NULL;
}

/* ------------------------------------------------------------- */
/* at_curse */
void
curse_init(attrib * a) {
	a->data.v = calloc(1, sizeof(curse));
}

int
curse_age(attrib * a)
{
	curse * c = (curse*)a->data.v;

	if (c->flag & CURSE_NOAGE) {
		c->duration = 1;
	} else {
		c->duration = max(0, c->duration-1);
	}
	return (c->duration);
}

void
curse_done(attrib * a) {
	curse *c = (curse *)a->data.v;

	cunhash(c);

	if( c->data && c->type->typ == CURSETYP_UNIT)
			free(c->data);
	free(c);
}

/* ------------------------------------------------------------- */

int
curse_read(attrib * a, FILE * f) {
	int mageid;
	curse * c = (curse*)a->data.v;
	const curse_type * ct;

	char cursename[64];

	if(global.data_version >= CURSEVIGOURISFLOAT_VERSION) {
		fscanf(f, "%d %s %d %d %lf %d %d ", &c->no, cursename, &c->flag,
			&c->duration, &c->vigour, &mageid, &c->effect.i);
	} else {
		int vigour;
		fscanf(f, "%d %s %d %d %d %d %d ", &c->no, cursename, &c->flag,
			&c->duration, &vigour, &mageid, &c->effect.i);
		c->vigour = vigour;
	}
	ct = ct_find(cursename);

	assert(ct!=NULL);

#ifdef CONVERT_DBLINK
	if (global.data_version<DBLINK_VERSION) {
		static const curse_type * cmonster = NULL;
		if (!cmonster) cmonster=ct_find("calmmonster");
		if (ct==cmonster) {
			c->effect.v = uniquefaction(c->effect.i);
		}
	}
#endif
	c->type = ct;

	/* beim Einlesen sind noch nicht alle units da, muss also
	 * zwischengespeichert werden. */
	if (mageid == -1){
		c->magician = (unit *)NULL;
	} else {
		ur_add((void*)mageid, (void**)&c->magician, resolve_unit);
	}

	if (c->type->read) c->type->read(f, c);
	else if (c->type->typ==CURSETYP_UNIT) {
		curse_unit * cc = calloc(1, sizeof(curse_unit));

		c->data = cc;
		fscanf(f, "%d ", &cc->cursedmen);
	}
	chash(c);

	return AT_READ_OK;
}

void
curse_write(const attrib * a, FILE * f) {
	int flag;
	int mage_no;
	curse * c = (curse*)a->data.v;
	const curse_type * ct = c->type;

	flag = (c->flag & ~(CURSE_ISNEW));

	if (c->magician){
		mage_no = c->magician->no;
	} else {
		mage_no = -1;
	}

	fprintf(f, "%d %s %d %d %f %d %d ", c->no, ct->cname, flag,
			c->duration, c->vigour, mage_no, c->effect.i);

	if (c->type->write) c->type->write(f, c);
	else if (c->type->typ == CURSETYP_UNIT) {
		curse_unit * cc = (curse_unit*)c->data;
		fprintf(f, "%d ", cc->cursedmen);
	}
}

attrib_type at_curse =
{
	"curse",
	curse_init,
	curse_done,
	curse_age,
	curse_write,
	curse_read,
	ATF_CURSE
};
/* ------------------------------------------------------------- */
/* Spruch identifizieren */

#include "umlaut.h"

typedef struct cursetype_list {
	struct cursetype_list * next;
	const curse_type * type;
} cursetype_list;

#define CMAXHASH 63
cursetype_list * cursetypes[CMAXHASH];

void
ct_register(const curse_type * ct)
{
  unsigned int hash = hashstring(ct->cname);
	cursetype_list ** ctlp = &cursetypes[hash];
	while (*ctlp) {
		cursetype_list * ctl = *ctlp;
		if (ctl->type==ct) return;
		ctlp=&ctl->next;
	}
	*ctlp = calloc(1, sizeof(cursetype_list));
	(*ctlp)->type = ct;
}

const curse_type *
ct_find(const char *c)
{
  unsigned int hash = hashstring(c);
	cursetype_list * ctl = cursetypes[hash];
	while (ctl) {
		int k = min(strlen(c), strlen(ctl->type->cname));
		if (!strncasecmp(c, ctl->type->cname, k)) return ctl->type;
		ctl = ctl->next;
	}
	/* disable this assert to be able to remoce certain curses from the game
	 * make sure that all locations using that curse can deal with a NULL
	 * return value.
	 */
	assert(!"unknown cursetype");
	return NULL;
}

/* ------------------------------------------------------------- */
/* get_curse identifiziert eine Verzauberung über die ID und gibt
 * einen pointer auf die struct zurück.
 */

typedef struct cid {
	int id;
	int id2;
} twoids;

boolean
cmp_curseeffect(const curse * c, const void * data)
{
	return (c->effect.v==data);
}

boolean
cmp_cursedata(const curse * c, const void * data)
{
	return (c->data==data);
}

boolean
cmp_curse(const attrib * a, const void * data) {
	const curse * c = (const curse*)data;
	if (a->type->flags & ATF_CURSE) {
		if (!data || c == (curse*)a->data.v) return true;
	}
	return false;
}

boolean
cmp_cursetype(const attrib * a, const void * data)
{
	const curse_type * ct = (const curse_type *)data;
	if (a->type->flags & ATF_CURSE) {
		if (!data || ct == ((curse*)a->data.v)->type) return true;
	}
	return false;
}

curse *
get_cursex(attrib *ap, const curse_type * ctype, void * data, boolean(*compare)(const curse *, const void *))
{
	attrib * a = a_select(ap, ctype, cmp_cursetype);
	while (a) {
		curse * c = (curse*)a->data.v;
		if (compare(c, data)) return c;
		a = a_select(a->next, ctype, cmp_cursetype);
	}
	return NULL;
}

curse *
get_curse(attrib *ap, const curse_type * ctype)
{
	attrib * a = a_select(ap, ctype, cmp_cursetype);

	if (!a) return NULL;
	return (curse*)a->data.v;
}

/* ------------------------------------------------------------- */
/* findet einen curse global anhand seiner 'curse-Einheitnummer' */

curse *
findcurse(int cid)
{
	return cfindhash(cid);
}

/* ------------------------------------------------------------- */
void
remove_curse(attrib **ap, const curse *c)
{
	attrib *a = a_select(*ap, c, cmp_curse);
	if (a) a_remove(ap, a);
}

void
remove_allcurse(attrib **ap, const void * data, boolean(*compare)(const attrib *, const void *))
{
	attrib * a = a_select(*ap, data, compare);
	while (a) {
		attrib * next = a->nexttype;
		a_remove(ap, a);
		a = a_select(next, data, compare);
	}
}

/* gibt die allgemeine Stärke der Verzauberung zurück. id2 wird wie
 * oben benutzt. Dies ist nicht die Wirkung, sondern die Kraft und
 * damit der gegen Antimagie wirkende Widerstand einer Verzauberung */
static double
get_cursevigour(const curse *c)
{
	if (c) return c->vigour;
	return 0;
}

/* setzt die Stärke der Verzauberung auf i */
static void
set_cursevigour(curse *c, double vigour)
{
	assert(c && vigour > 0);
	c->vigour = vigour;
}

/* verändert die Stärke der Verzauberung um +i und gibt die neue
 * Stärke zurück. Sollte die Zauberstärke unter Null sinken, löst er
 * sich auf.
 */
double
curse_changevigour(attrib **ap, curse *c, double vigour)
{
	vigour += get_cursevigour(c);

	if (vigour <= 0) {
		remove_curse(ap, c);
		vigour = 0;
	} else {
		set_cursevigour(c, vigour);
	}
	return vigour;
}

/* ------------------------------------------------------------- */

int
curse_geteffect(const curse *c)
{
	if (c) return c->effect.i;
	return 0;
}

/* ------------------------------------------------------------- */
static void
set_curseingmagician(struct unit *magician, struct attrib *ap_target, const curse_type *ct)
{
	curse * c = get_curse(ap_target, ct);
	if (c) {
		c->magician = magician;
	}
}

/* ------------------------------------------------------------- */
/* gibt bei Personenbeschränkten Verzauberungen die Anzahl der
 * betroffenen Personen zurück. Ansonsten wird 0 zurückgegeben. */
int
get_cursedmen(unit *u, curse *c)
{
	int cursedmen = u->number;

	if (!c) return 0;

	/* je nach curse_type andere data struct */
	if (c->type->typ == CURSETYP_UNIT){
		curse_unit * cc = (curse_unit*)c->data;
		cursedmen = cc->cursedmen;
	}

	return min(u->number, cursedmen);
}

/* setzt die Anzahl der betroffenen Personen auf cursedmen */
static void
set_cursedmen(curse *c, int cursedmen)
{
	if (!c) return;

	/* je nach curse_type andere data struct */
	if (c->type->typ == CURSETYP_UNIT) {
		curse_unit * cc = (curse_unit*)c->data;
		cc->cursedmen = cursedmen;
	}
}

/* ------------------------------------------------------------- */
void
curse_setflag(curse *c, int flag)
{
	if (c) c->flag = (c->flag | flag);
}

/* ------------------------------------------------------------- */
/* Legt eine neue Verzauberung an. Sollte es schon einen Zauber
 * dieses Typs geben, gibt es den bestehenden zurück.
 */
curse *
set_curse(unit *mage, attrib **ap, const curse_type *ct, double vigour,
		int duration, int effect, int men)
{
	curse *c;
	attrib * a;

	a = a_new(&at_curse);
	a_add(ap, a);
	c = (curse*)a->data.v;

	c->type = ct;
	c->flag = 0;
	c->vigour = vigour;
	c->duration = duration;
	c->effect.i = effect;
	c->magician = mage;

	c->no = newunitid();
	chash(c);

	switch (c->type->typ) {
		case CURSETYP_NORM:
			break;

		case CURSETYP_UNIT:
		{
			curse_unit *cc = calloc(1, sizeof(curse_unit));
			cc->cursedmen += men;
			c->data = cc;
			break;
		}

	}
	return c;
}


/* Mapperfunktion für das Anlegen neuer curse. Automatisch wird zum
 * passenden Typ verzweigt und die relevanten Variablen weitergegeben.
 */
curse *
create_curse(unit *magician, attrib **ap, const curse_type *ct, double vigour,
		int duration, int effect, int men)
{
	curse *c;

	/* die Kraft eines Spruchs darf nicht 0 sein*/
	assert(vigour > 0);

	c = get_curse(*ap, ct);

	if(c && (c->flag & CURSE_ONLYONE)){
		return NULL;
	}
	assert(c==NULL || ct==c->type);

	/* es gibt schon eins diese Typs */
	if (c && ct->mergeflags != NO_MERGE) {
		if(ct->mergeflags & M_DURATION){
			c->duration = max(c->duration, duration);
		}
		if(ct->mergeflags & M_SUMDURATION){
			c->duration += duration;
		}
		if(ct->mergeflags & M_SUMEFFECT){
			c->effect.i += effect;
		}
		if(ct->mergeflags & M_MAXEFFECT){
			c->effect.i = max(c->effect.i, effect);
		}
		if(ct->mergeflags & M_VIGOUR){
			c->vigour = max(vigour, c->vigour);
		}
		if(ct->mergeflags & M_VIGOUR_ADD){
			c->vigour = vigour + c->vigour;
		}
		if(ct->mergeflags & M_MEN){
			switch (ct->typ) {
				case CURSETYP_UNIT:
				{
					curse_unit * cc = (curse_unit*)c->data;
					cc->cursedmen += men;
				}
			}
		}
		set_curseingmagician(magician, *ap, ct);
	} else {
		c = set_curse(magician, ap, ct, vigour, duration, effect, men);
	}
	return c;
}

/* ------------------------------------------------------------- */
/* hier müssen alle c-typen, die auf Einheiten gezaubert werden können,
 * berücksichtigt werden */

void
do_transfer_curse(curse *c, unit * u, unit * u2, int n)
{
	int flag = c->flag;
	int duration = c->duration;
	double vigour = c->vigour;
	unit *magician = c->magician;
	int effect = c->effect.i;
	int cursedmen = 0;
	int men = 0;
	boolean dogive = false;
	const curse_type *ct = c->type;

	switch (ct->typ) {
		case CURSETYP_UNIT:
		{
			curse_unit * cc = (curse_unit*)c->data;
			men = cc->cursedmen;
			break;
		}
		default:
			cursedmen = u->number;
	}

	switch (ct->spread){
		case CURSE_SPREADALWAYS:
			dogive = true;
			men = u2->number + n;
			break;

		case CURSE_SPREADMODULO:
		{
			int i;
			int u_number = u->number;
			for (i=0;i<n+1 && u_number>0;i++){
				if (rand()%u_number < cursedmen){
					++men;
					--cursedmen;
					dogive = true;
				}
				--u_number;
			}
			break;
		}
		case CURSE_SPREADCHANCE:
		{
			int chance = u2->number * 100 / (u2->number + n);
			if (rand()%100 > chance ){
				men = u2->number + n;
				dogive = true;
			}
			break;
		}
		case CURSE_SPREADNEVER:
			break;
	}

	if (dogive == true) {
		curse * cnew = set_curse(magician, &u2->attribs, c->type, vigour, duration,
				effect, men);
		curse_setflag(cnew, flag);

		if (ct->typ == CURSETYP_UNIT) set_cursedmen(cnew, men);
	}
}

void
transfer_curse(unit * u, unit * u2, int n)
{
	attrib * a;

	a = a_find(u->attribs, &at_curse);
	while (a) {
		curse *c = (curse*)a->data.v;
		do_transfer_curse(c, u, u2, n);
		a = a->nexttype;
	}
}

/* ------------------------------------------------------------- */

boolean
curse_active(const curse *c)
{
	if (!c) return false;
	if (c->flag & CURSE_ISNEW) return false;
	if (c->vigour <= 0) return false;

	return true;
}

boolean
is_cursed_internal(attrib *ap, const curse_type *ct)
{
	curse *c = get_curse(ap, ct);

	if (!c)
		return false;

	return true;
}


boolean
is_cursed_with(attrib *ap, curse *c)
{
	attrib *a = ap;

	while (a) {
		if ((a->type->flags & ATF_CURSE) && (c == (curse *)a->data.v)) {
			return true;
		}
		a = a->next;
	}

	return false;
}
/* ------------------------------------------------------------- */
/* cursedata */
/* ------------------------------------------------------------- */
/*
 * typedef struct curse_type {
 * 	const char *cname; (Name der Zauberwirkung, Identifizierung des curse)
 * 	int typ;
 * 	spread_t spread;
 * 	unsigned int mergeflags;
 * 	const char *info_str;  Wirkung des curse, wird bei einer gelungenen Zauberanalyse angezeigt
 * 	int (*curseinfo)(const struct locale*, const void*, int, curse*, int);
 * 	void (*change_vigour)(curse*, double);
 * 	int (*read)(FILE * F, curse * c);
 * 	int (*write)(FILE * F, const curse * c);
 * } curse_type;
 */

void *
resolve_curse(void * id)
{
   return cfindhash((int)id);
}

void
register_curses(void)
{
	register_unitcurse();
	register_regioncurse();
	register_shipcurse();
	register_buildingcurse();
}


static const char * oldnames[MAXCURSE] = {
	"fogtrap",
	"antimagiczone",
	"farvision",
	"gbdream",
	"auraboost",
	"maelstrom",
	"blessedharvest",
	"drought",
	"badlearn",
	"stormwind",
	"flyingship",
	"nodrift",
	"depression",
	"magicwalls",
	"strongwall",
	"astralblock",
	"generous",
	"peacezone",
	"disorientationzone",
	"magicstreet",
	"magicrunes",
	"badmagicresistancezone",
	"goodmagicresistancezone",
	"slavery",
	"shipdisorientation",
	"calmmonster",
	"oldrace",
	"fumble",
	"riotzone",
	"nocostbuilding",
	"holyground",
	"godcursezone",
	"",
	"",
	"",
	"",
	"",
	"",
	"speed",
	"orcish",
	"magicboost",
	"insectfur",
	"strength",
	"worse",
	"magicresistance",
	"itemcloak",
	"sparkle",
	"",
	"",
	"",
	"skillmod"
};

const char *
oldcursename(int id)
{
	return oldnames[id];
}

