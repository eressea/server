/* vi: set ts=2:
 *
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2000
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
#include <resolve.h>
#include <base36.h>

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
direction_t
dirmirror(direction_t dir)
{
	return (direction_t)((dir+MAXDIRECTIONS/2) % MAXDIRECTIONS);
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

#define MAXENTITYHASH 8191
curse *cursehash[MAXENTITYHASH];

void
chash(curse *c)
{
	curse *old = cursehash[c->no %MAXENTITYHASH];

	cursehash[c->no %MAXENTITYHASH] = c;
	c->nexthash = old;
}

void
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
/* Spruch identifizieren */

#include "umlaut.h"

typedef struct cursetype_list {
	struct cursetype_list * next;
	const curse_type * type;
} cursetype_list;

cursetype_list * cursetypes;

void
ct_register(const curse_type * ct)
{
	cursetype_list ** ctlp = &cursetypes;
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
	cursetype_list * ctl = cursetypes;
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
boolean
is_normalcurse(const curse_type * ct)
{

	if (!ct) return false;

	if (ct->typ == CURSETYP_NORM)
		return true;

	return false;
}

boolean
is_curseunit(const curse_type * ct)
{
	if (!ct) return false;

	if (ct->typ == CURSETYP_UNIT)
		return true;

	return false;
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
	int effect = (int)data;
	return (c->effect==effect);
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
/* Normalerweise ist alles ausser der Id eines curse und dem
 * verzauberten Objekt nicht bekannt. Um den Zauber eindeutig zu
 * identifizieren benötigt man je nach Typ einen weiteren Identifier.
 */
void
remove_cursetype(attrib **ap, const curse_type *ct)
{
	attrib *a = a_select(*ap, ct, cmp_cursetype);
	if (a) a_remove(ap, a);
}

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
static int
get_cursevigour(const curse *c)
{
	if (c) return c->vigour;
	return 0;
}

/* setzt die Stärke der Verzauberung auf i */
static void
set_cursevigour(curse *c, int vigour)
{
	assert(c && vigour > 0);
	c->vigour = vigour;
}

/* verändert die Stärke der Verzauberung um +i und gibt die neue
 * Stärke zurück. Sollte die Zauberstärke unter Null sinken, löst er
 * sich auf.
 */
int
curse_changevigour(attrib **ap, curse *c, int vigour)
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
	if (c) return c->effect;
	return 0;
}

/* ------------------------------------------------------------- */
void
set_curseingmagician(struct unit *magician, struct attrib *ap_target, const curse_type *ct)
{
	curse * c = get_curse(ap_target, ct);
	if (c) {
		c->magician = magician;
	}
}

unit *
get_cursingmagician(struct attrib *ap, const curse_type *ct)
{
	curse *c = get_curse(ap, ct);
	if( !c )
		return NULL;

	return c->magician;
}

/* Wichtig! Alle struct curse<typ>, die den Verweis auf den zaubenden
 * Magier enthalten, müssen sie hier mit abgeprüft werden
 */
void
remove_cursemagepointer(unit *magician, attrib *ap_target)
{
	const attrib * a;

	a = a_find(ap_target, &at_curse);
	while (a) {
		curse *c = (curse*)a->data.v;
		if(c->magician == magician)
			c->magician = (unit*)NULL;
		a = a->nexttype;
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

int
change_cursedmen(attrib **ap, curse *c, int cursedmen)
{
	if (!c) return 0;

	/* je nach curse_type andere data struct */
	if (c->type->typ == CURSETYP_UNIT) {
		curse_unit * cc = (curse_unit*)c->data;
		cursedmen +=  cc->cursedmen;
	}
	if (cursedmen <= 0) {
		remove_curse(ap, c);
	} else {
		set_cursedmen(c, cursedmen);
	}

	return cursedmen;
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
set_curse(unit *mage, attrib **ap, const curse_type *ct, int vigour,
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
	c->effect = effect;
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
create_curse(unit *magician, attrib **ap, const curse_type *ct, int vigour,
		int duration, int effect, int men)
{
	curse *c;

	/* die Kraft eines Spruchs darf nicht 0 sein*/
	assert(vigour >= 0);

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
			c->effect += effect;
		}
		if(ct->mergeflags & M_MAXEFFECT){
			c->effect = max(c->effect, effect);
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
	int vigour = c->vigour;
	unit *magician = c->magician;
	int effect = c->effect;
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
/* Diese Funktionen werden von reports.c:print_curses() während der
 * Generierung des Normalreports aufgerufen und ersetzen
 * cursedisplay
 */
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

/* C_DISORIENTATION */
static int
cinfo_disorientation(void * obj, typ_t typ, curse *c, int self)
{
	unused(typ);
	unused(obj);
	unused(self);

	assert(typ == TYP_SHIP);

	sprintf(buf, "Der Kompaß kaputt, die Segel zerrissen, der Himmel "
			"wolkenverhangen. Wohin fahren wir? (%s)", curseid(c));

	return 1;
}

static int
read_skill(FILE * F, curse * c)
{
	int skill;
	if (global.data_version<CURSETYPE_VERSION) {
		int men;
		fscanf(F, "%d %d", &skill, &men);
	} else {
		fscanf(F, "%d", &skill);
	}
	c->data = (void*)skill;
	return 0;
}

static int
write_skill(FILE * F, const curse * c)
{
	fprintf(F, "%d ", (int)c->data);
	return 0;
}
/* ------------------------------------------------------------- */
/* cursedata */
/* ------------------------------------------------------------- */
/* typedef struct cursedata {
 *   int id; (altlast für kompatibiliät)
 *   char *name;
 *   int typ;
 *   spread_t spread;
 *   int mergeflags;
 *   char *info;
 *   void (*display)(void*,typ_t, curse*);
 *} cursedata;
 */
/* die Beschreibung wird bei einer gelungenen Zauberanalyse ausgegeben
 * und hat die Form:
 * Magier (xx) gelang es folgendes herauszufinden:
 * Unit (xyz) steht unter dem Einfluss des Zaubers %name,/ Auf
 * Region/Schiff/Burg (xy) liegt der Zauber %name,
 * der wohl noch etwa %s Wochen andauert.
 * %info, "Dieser Zauber blafalsel blub"
 */
static curse_type cursedaten[MAXCURSE] =
{
/* struct's vom typ curse: */
	{ 
		"fogtrap",
		CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
		"",
		(cdesc_fun)cinfo_region
	},
	{ 
		"antimagiczone",
		CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
		"Dieser Zauber scheint magische Energien irgendwie abzuleiten und "
		"so alle in der Region gezauberten Sprüche in ihrer Wirkung zu "
		"schwächen oder ganz zu verhindern.",
		NULL
	},
	{ 
		"farvision",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},
	{ 
		"gbdream",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		(cdesc_fun)cinfo_dreamcurse
	},

	{  /* Verändert die max Aura und Regeneration um effect% */
		"auraboost",
		CURSETYP_NORM, CURSE_SPREADMODULO, (NO_MERGE),
		"Dieser Zauber greift irgendwie in die Verbindung zwischen Magier "
		"und Magischer Essenz ein. Mit positiver Ausrichtung kann er wohl "
		"wie ein Fokus für Aura wirken, jedoch genauso für das Gegenteil "
		"benutzt werden.",
		/* cinfo_auraboost */
	},
	{
		"maelstrom",
		CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
		"Dieser Zauber verursacht einen gigantischen magischen Strudel. Der "
		"Mahlstrom wird alle Schiffe, die in seinen Sog geraten, schwer "
		"beschädigen.",
		NULL
	},
	{
		"blessedharvest",
		CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
		"Dieser Fruchtbarkeitszauber erhöht die Erträge der Felder.",
		(cdesc_fun)cinfo_region
	},
	{
		"drought",
		CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
		"Dieser Zauber strahlt starke negative Energien aus. Warscheinlich "
		"ist er die Ursache der Dürre."	,
		(cdesc_fun)cinfo_region
	},
	{
		"badlearn",
		CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
		"Dieser Zauber scheint die Ursache für die Schlaflosigkeit und "
		"Mattigkeit zu sein, unter der die meisten Leute hier leiden und "
		"die dazu führt, das Lernen weniger Erfolg bringt. ",
		(cdesc_fun)cinfo_region
	},
	{ /* Sturmwind-Zauber, wirkt nur 1 Runde */
		"stormwind",
		CURSETYP_NORM, 0, NO_MERGE,
		"",
		NULL
	},
	{ /* Luftschiff-Zauber, wirkt nur 1 Runde */
		"flyingship",
		CURSETYP_NORM, 0, NO_MERGE,
		"",
		NULL
	},
	{ /* GünstigeWinde-Zauber */
		"nodrift",
		CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
		"Der Zauber auf diesem Schiff ist aus den elementaren Magien der Luft "
		"und des Wassers gebunden. Der dem Wasser verbundene Teil des Zaubers "
		"läßt es leichter durch die Wellen gleiten und der der Luft verbundene "
		"Teil scheint es vor widrigen Winden zu schützen.",
		/* cinfo_shipnodrift */
	},
	{ /*  Trübsal-Zauber */
		"depression",
		CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
		"Wie schon zu vermuten war, sind der ewig graue Himmel und die "
		"depressive Stimmung in der Region nicht natürlich. Dieser Fluch "
		"hat sich wie ein bleiernes Tuch auf die Gemüter der Bevölkerung "
		"gelegt und eh er nicht gebrochen oder verklungen ist, wird keiner "
		"sich an Gaukelleien erfreuen können.",
		(cdesc_fun)cinfo_region
	},
	{ /* Heimstein-Zauber */
		"magicwalls",
		CURSETYP_NORM, 0, NO_MERGE,
		"Die Macht dieses Zaubers ist fast greifbar und tief in die Mauern "
		"gebunden. Starke elementarmagische Kräfte sind zu spüren. "
		"Vieleicht wurde gar ein Erdelementar in diese Mauern gebannt. "
		"Ausser ebenso starkter Antimagie wird nichts je diese Mauern "
		"gefährden können.",
		/* cinfo_magicstone */
	},
	{ /* Feste Mauer - Präkampfzauber, wirkt nur 1 Runde */
		"strongwall",
		CURSETYP_NORM, 0, NO_MERGE,
		"",
		NULL
	},
	{ /* Astralblock, auf Astralregion */
		"astralblock",
		CURSETYP_NORM, 0, NO_MERGE,
		"",
		(cdesc_fun)cinfo_region
	},
	{ /* Unterhaltungsanteil vermehren */
		"generous",
		CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR | M_MAXEFFECT ),
		"Dieser Zauber beeinflusst die allgemeine Stimmung in der Region positiv. "
		"Die gute Laune macht die Leute freigiebiger.",
		(cdesc_fun)cinfo_region
	},
	{ /* verhindert Attackiere regional */
		"peacezone",
		CURSETYP_NORM, 0, NO_MERGE,
		"Dieser machtvoller Beeinflussungszauber erstickt jeden Streit schon im "
		"Keim.",
		(cdesc_fun)cinfo_region
	},
	{  /* erschwert geordnete Bewegungen */
		"disorientationzone",
		CURSETYP_NORM, 0, NO_MERGE,
		"",
		(cdesc_fun)cinfo_region
	},
	{ /*  erzeugt Straßennetz */
		"magicstreet",
		CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
		"Es scheint sich um einen elementarmagischen Zauber zu handeln, der alle "
		"Pfade und Wege so gut festigt, als wären sie gepflastert. Wie auf einer "
		"Straße kommt man so viel besser und schneller vorwärts.",
		/* cinfo_magicstreet */
	},
	{
		"magicrunes",
		CURSETYP_NORM, 0, M_SUMEFFECT,
		"Dieses Zauber verstärkt die natürliche Widerstandskraft gegen eine "
		"Verzauberung.",
		/* cinfo_magicrunes */
	},
	{ /*  erniedigt Magieresistenz von nicht-aliierten Einheiten, wirkt nur
			 1x pro Einheit */
		"badmagicresistancezone",
		CURSETYP_NORM, 0, NO_MERGE,
		"Dieses Lied, das irgendwie in die magische Essenz der Region gewoben "
		"ist, schwächt die natürliche Widerstandskraft gegen eine "
		"Verzauberung. Es scheint jedoch nur auf bestimmte Einheiten zu wirken.",
		NULL
	},
	{ /* erhöht Magieresistenz von aliierten Einheiten, wirkt nur 1x pro
			 Einheit */
		"goodmagicresistancezone",
		CURSETYP_NORM, 0, NO_MERGE,
		"Dieser Lied, das irgendwie in die magische Essenz der Region gewoben "
		"ist, verstärkt die natürliche Widerstandskraft gegen eine "
		"Verzauberung. Es scheint jedoch nur auf bestimmte Einheiten zu wirken.",
		NULL
	},
	{ /* dient fremder Partei. Zählt nicht zu Migranten, attackiert nicht */
		"slavery",
		CURSETYP_NORM, 0, NO_MERGE,
		"Dieser mächtige Bann scheint die Einheit ihres freien Willens "
		"zu berauben. Solange der Zauber wirkt, wird sie nur den Befehlen "
		"ihres neuen Herrn gehorchen.",
		/* cinfo_slave */
	},
	{
		"shipdisorientation",
		CURSETYP_NORM, 0, NO_MERGE,
		"Dieses Schiff hat sich verfahren.",
		/* cinfo_disorientation */
	},
	{
		"calmmonster",
		CURSETYP_NORM, CURSE_SPREADNEVER, NO_MERGE,
		"Dieser Beeinflussungszauber scheint die Einheit einem ganz "
		"bestimmten Volk wohlgesonnen zu machen.",
		/* cinfo_calm */
	},
	{ /* Merkt sich die alte 'richtige' Rasse einer gestalltwandelnden
			 Einheit */
		"oldrace",
		CURSETYP_NORM, CURSE_SPREADALWAYS, NO_MERGE,
		"",
		NULL
	},
	{
		"fumble",
		CURSETYP_NORM, CURSE_SPREADNEVER, NO_MERGE,
		"Eine Wolke negativer Energie umgibt die Einheit.",
		/* cinfo_fumble */
	},
	{
		"riotzone",
		CURSETYP_NORM, 0, (M_DURATION),
		"Eine Wolke negativer Energie liegt über der Region.",
		(cdesc_fun)cinfo_region
	},
	{ /* Ewige Mauern-Zauber */
		"nocostbuilding",
		CURSETYP_NORM, 0, NO_MERGE,
		"Die Macht dieses Zaubers ist fast greifbar und tief in die Mauern "
		"gebunden. Unbeeindruck vom Zahn der Zeit wird dieses Gebäude wohl "
		"auf Ewig stehen.",
		(cdesc_fun)cinfo_region
	},
	{
		"holyground",
		CURSETYP_NORM, 0, (M_VIGOUR_ADD),
		"Verschiedene Naturgeistern sind im Boden der Region gebunden und "
		"beschützen diese vor dem der dunklen Magie des lebenden Todes.",
		(cdesc_fun)cinfo_region
	},
	{
		"godcursezone",
		CURSETYP_NORM, 0, (NO_MERGE),
		"Diese Region wurde von den Göttern verflucht. Stinkende Nebel ziehen "
		"über die tote Erde, furchbare Kreaturen ziehen über das Land. Die Brunnen "
		"sind vergiftet, und die wenigen essbaren Früchte sind von einem rosa Pilz "
		"überzogen. Niemand kann hier lange überleben.",
		/* cinfo_cursed_by_the_gods, */
	},
	{
		"",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},
	{ 
		"",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},
	{
		"",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},
	{
		"",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},
	{
		"",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},
	{
		"",
		CURSETYP_NORM, 0, (NO_MERGE),
		"",
		NULL
	},

	/* struct's vom typ curse_unit: */
	{
		"speed",
		CURSETYP_UNIT, CURSE_SPREADNEVER, M_MEN,
		"Diese Einheit bewegt sich doppelt so schnell.",
		/* cinfo_speed */
	},
	{
		"orcish",
		CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
		"Dieser Zauber scheint die Einheit zu 'orkisieren'. Wie bei Orks "
		"ist eine deutliche Neigung zur Fortpflanzung zu beobachten.",
		/* cinfo_orc */
	},
	{
		"magicboost",
		CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
		"",
		NULL
	},
	{
		"insectfur",
		CURSETYP_UNIT, CURSE_SPREADMODULO, ( M_MEN | M_DURATION ),
		"Dieser Zauber schützt vor den Auswirkungen der Kälte.",
		/* cinfo_kaelteschutz */
	},
	{ /* */
		"strength",
		CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
		"Dieser Zauber vermehrt die Stärke der verzauberten Personen um ein "
		"vielfaches.",
		/* cinfo_strength */
	},
	{ /* Alp */
		"worse",
		CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
		"",
		/* cinfo_allskills */
	},
	{ /* */
		"magicresistance",
		CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
		"Dieser Zauber verstärkt die natürliche Widerstandskraft gegen eine "
		"Verzauberung.",
		NULL
	},
	{ /* */
		"itemcloak",
		CURSETYP_UNIT, CURSE_SPREADNEVER, M_DURATION,
		"Dieser Zauber macht die Ausrüstung unsichtbar.",
		/* cinfo_itemcloak */
	},
	{ /* */
		"sparkle",
		CURSETYP_UNIT, CURSE_SPREADMODULO, ( M_MEN | M_DURATION ),
		"Dieser Zauber ist einer der ersten, den junge Magier in der Schule lernen.",
		/* cinfo_sparkle */
	},
	{
		"",
		CURSETYP_UNIT, 0, (NO_MERGE),
		"",
		NULL
	},
	{
		"",
		CURSETYP_UNIT, 0, (NO_MERGE),
		"",
		NULL
	},
	{
		"",
		CURSETYP_UNIT, 0, (NO_MERGE),
		"",
		NULL
	},

/* struct's vom typ curse_skill: */
	{
		"skillmod",
		CURSETYP_NORM, CURSE_SPREADMODULO, M_MEN,
		"",
		/* cinfo_skill, */
		NULL, NULL, /* void (*change_vigour)(curse*, int); */
		read_skill, write_skill
	}
};

const char * 
oldcursename(int id)
{
	return cursedaten[id].cname;
}

void *
resolve_curse(void * id)
{
   return cfindhash((int)id);
}

void
register_curses(void)
{
	int i;
	for (i=0;i!=MAXCURSE;++i) {
		if (strlen(cursedaten[i].cname)) ct_register(&cursedaten[i]);
	}
}
