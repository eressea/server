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
#include "unitcurse.h"

/* kernel includes */
#include <message.h>
#include <nrmessage.h>
#include <race.h>
#include <skill.h>
#include <unit.h>
#include <faction.h>
#include <objtypes.h>
#include <curse.h>

/* util includes */
#include <message.h>
#include <base36.h>
#include <functions.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>


/* ------------------------------------------------------------- */
int
cinfo_unit(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	message * msg;

	unused(typ);
	unused(self);
	unused(obj);

	assert(typ == TYP_UNIT);

	msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);
	return 1;
}

static int
cinfo_unit_onlyowner(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	message * msg;
	struct unit *u;

	unused(typ);

	assert(typ == TYP_UNIT);
	u = (struct unit *)obj;

	if (self != 0){
		msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}
	return 0;
}

/* ------------------------------------------------------------- */
/* 
 * C_AURA 
 */
/* erhöht/senkt regeneration und maxaura um effect% */
static int
cinfo_auraboost(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	struct unit *u;
	unused(typ);
	assert(typ == TYP_UNIT);
	u = (struct unit *)obj;

	if (self != 0){
		if (curse_geteffect(c) > 100){
			sprintf(buf, "%s fühlt sich von starken magischen Energien "
				"durchströmt. (%s)", u->name, curseid(c));
		}else{
			sprintf(buf, "%s hat Schwierigkeiten seine magischen Energien "
					"zu sammeln. (%s)", u->name, curseid(c));
		}
		return 1;
	}
	return 0;
}
static struct curse_type ct_auraboost = { 
	"auraboost",
	CURSETYP_NORM, CURSE_SPREADMODULO, (NO_MERGE),
	"Dieser Zauber greift irgendwie in die Verbindung zwischen Magier "
	"und Magischer Essenz ein. Mit positiver Ausrichtung kann er wohl "
	"wie ein Fokus für Aura wirken, jedoch genauso für das Gegenteil "
	"benutzt werden.",
	cinfo_auraboost
};
/* Magic Boost - Gabe des Chaos */
static struct curse_type ct_magicboost = { 
	"magicboost",
	CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
	"",
	NULL
};

/* ------------------------------------------------------------- */
/* 
 * C_SLAVE 
 */
static int
cinfo_slave(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if (self != 0){
		sprintf(buf, "%s wird noch %d Woche%s unter unserem Bann stehen. (%s)",
				u->name, c->duration, (c->duration == 1)? "":"n", curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_slavery = { "slavery",
	CURSETYP_NORM, 0, NO_MERGE,
	"Dieser mächtige Bann scheint die Einheit ihres freien Willens "
	"zu berauben. Solange der Zauber wirkt, wird sie nur den Befehlen "
	"ihres neuen Herrn gehorchen.",
	cinfo_slave
};

/* ------------------------------------------------------------- */
/* 
 * C_CALM
 */
static int
cinfo_calm(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u;
	const struct race * rc;
	faction *f;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;
	if (c->magician){
		rc = c->magician->irace;
		f = c->magician->faction;
		if (self != 0) {
			sprintf(buf, "%s mag %s", u->name, factionname(f));
		} else {
			sprintf(buf, "%s scheint %s zu mögen", u->name, LOC(f->locale, rc_name(rc, 1)));
		}
		scat(". (");
		scat(itoa36(c->no));
		scat(")");

		return 1;
	}
	return 0;
}
static struct curse_type ct_calmmonster = { "calmmonster",
	CURSETYP_NORM, CURSE_SPREADNEVER, NO_MERGE,
	"Dieser Beeinflussungszauber scheint die Einheit einem ganz "
	"bestimmten Volk wohlgesonnen zu machen.",
	cinfo_calm
};

/* ------------------------------------------------------------- */
/* 
 * C_SPEED
 */
static int
cinfo_speed(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u;
	curse_unit * cu;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;
	cu = (curse_unit *)c->data;

	if (self != 0){
		sprintf(buf, "%d Person%s von %s %s noch %d Woche%s beschleunigt. (%s)",
				cu->cursedmen, (cu->cursedmen == 1)? "":"en", u->name,
				(cu->cursedmen == 1)? "ist":"sind", c->duration,
				(c->duration == 1)? "":"n",
				curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_speed = {
	"speed",
	CURSETYP_UNIT, CURSE_SPREADNEVER, M_MEN,
	"Diese Einheit bewegt sich doppelt so schnell.",
	cinfo_speed
};

/* ------------------------------------------------------------- */
/*
 * C_ORC
 */
static int
cinfo_orc(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u;
	message * msg;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if (self != 0){
		msg = msg_message(mkname("curseinfo", c->type->cname), "unit id", u, c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}
	return 0;
}
static struct curse_type ct_orcish = {
	"orcish",
	CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
	"Dieser Zauber scheint die Einheit zu 'orkisieren'. Wie bei Orks "
	"ist eine deutliche Neigung zur Fortpflanzung zu beobachten.",
	cinfo_orc
};

/* ------------------------------------------------------------- */
/* 
 * C_KAELTESCHUTZ
 */
static int
cinfo_kaelteschutz(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u;
	curse_unit * cu;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;
	cu = (curse_unit *)c->data;

	if (self != 0){
		sprintf(buf, "%d Person%s von %s %s sich vor Kälte geschützt. (%s)",
				cu->cursedmen, (cu->cursedmen == 1)? "":"en", u->name,
				(cu->cursedmen == 1)? "fühlt":"fühlen",
				curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_insectfur = {
	"insectfur",
	CURSETYP_UNIT, CURSE_SPREADMODULO, ( M_MEN | M_DURATION ),
	"Dieser Zauber schützt vor den Auswirkungen der Kälte.",
	cinfo_kaelteschutz
};

/* ------------------------------------------------------------- */
/*
 * C_SPARKLE
 */
static int
cinfo_sparkle(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	const char * effects[] = {
		NULL, /* end grau*/
		"%s ist im Traum eine Fee erschienen.",
		"%s wird von bösen Alpträumen geplagt.",
		NULL, /* end traum */
		"%s wird von einem glitzernden Funkenregen umgeben.",
		"Ein schimmernder Lichterkranz umgibt %s.",
		NULL, /* end tybied */
		"Eine Melodie erklingt, und %s tanzt bis spät in die Nacht hinein.",
		"%s findet eine kleine Flöte, die eine wundersame Melodie spielt.",
		"Die Frauen des nahegelegenen Dorfes bewundern %s verstohlen.",
		"Eine Gruppe vorbeiziehender Bergarbeiter rufen %s eindeutig Zweideutiges nach.",
		NULL, /* end cerrdor */
		"%s bekommt von einer Schlange einen Apfel angeboten.",
		"Ein Einhorn berührt %s mit seinem Horn und verschwindet kurz darauf im Unterholz.",
		"Vogelzwitschern begleitet %s auf all seinen Wegen.",
		"Leuchtende Blumen erblühen rund um das Lager von %s.",
		NULL, /* end gwyrrd */
		"Über %s zieht eine Gruppe Geier ihre Kreise.",
		"Der Kopf von %s hat sich in einen grinsenden Totenschädel verwandelt.",
		"Ratten folgen %s auf Schritt und Tritt.",
		"Pestbeulen befallen den Körper von %s.",
		"Eine dunkle Fee erscheint %s im Schlaf. Sie ist von schauriger Schönheit.",
		"Fäulnisgeruch dringt %s aus allen Körperöffnungen.",
		NULL, /* end draig */
	};
	int m, begin=0, end=0;
	unit *u;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if(!c->magician || !c->magician->faction) return 0;

	for(m=0;m!=c->magician->faction->magiegebiet;++m) {
		while (effects[end]!=NULL) ++end;
		begin = end+1;
		end = begin;
	}

	while (effects[end]!=NULL) ++end;
	if (end==begin) return 0;
	else sprintf(buf, effects[begin + curse_geteffect(c) % (end-begin)], u->name);
	scat(" (");
	scat(itoa36(c->no));
	scat(")");

	return 1;
}
static struct curse_type ct_sparkle = { "sparkle",
	CURSETYP_UNIT, CURSE_SPREADMODULO, ( M_MEN | M_DURATION ),
	"Dieser Zauber ist einer der ersten, den junge Magier in der Schule lernen.",
	cinfo_sparkle
};

/* ------------------------------------------------------------- */
/*
 * C_STRENGTH
 */
static int
cinfo_strength(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unused(c);
	unused(typ);

	assert(typ == TYP_UNIT);
	unused(obj);

	if (self != 0){
		sprintf(buf, "Die Leute strotzen nur so vor Kraft. (%s)",
				curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_strength = { "strength",
	CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
	"Dieser Zauber vermehrt die Stärke der verzauberten Personen um ein "
	"vielfaches.",
	cinfo_strength
};

/* ------------------------------------------------------------- */
/*
 * C_ALLSKILLS (Alp)
 */
static int
cinfo_allskills(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unused(obj);
	unused(typ);
	unused(c);

	assert(typ == TYP_UNIT);

	if (self != 0){
		sprintf(buf, "Wird von einem Alp geritten. (%s)", curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_worse = { 
	"worse",
	CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
	"",
	cinfo_allskills
};

/* ------------------------------------------------------------- */

/*
 * C_ITEMCLOAK
 */
static int
cinfo_itemcloak(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if (self != 0) {
		sprintf(buf, "Die Ausrüstung von %s scheint unsichtbar. (%s)",
				u->name, curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_itemcloak = {
	"itemcloak",
	CURSETYP_UNIT, CURSE_SPREADNEVER, M_DURATION,
	"Dieser Zauber macht die Ausrüstung unsichtbar.",
	cinfo_itemcloak
};
/* ------------------------------------------------------------- */

static int
cinfo_fumble(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit * u = (unit*)obj;
	unused(typ);

	assert(typ == TYP_UNIT);

	if (self != 0){
		sprintf(buf, "%s kann sich kaum konzentrieren. (%s)",
				u->name, curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_fumble = {
	"fumble",
	CURSETYP_NORM, CURSE_SPREADNEVER, NO_MERGE,
	"Eine Wolke negativer Energie umgibt die Einheit.",
	cinfo_fumble
};
/* ------------------------------------------------------------- */


static struct curse_type ct_oldrace = { "oldrace",
	CURSETYP_NORM, CURSE_SPREADALWAYS, NO_MERGE,
	"",
	NULL
};
static struct curse_type ct_magicresistance = { "magicresistance",
	CURSETYP_UNIT, CURSE_SPREADMODULO, M_MEN,
	"Dieser Zauber verstärkt die natürliche Widerstandskraft gegen eine "
	"Verzauberung.",
	NULL
};


/* ------------------------------------------------------------- */
/*
 * C_SKILL
 */

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

static int
cinfo_skill(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
{
	unit *u = (unit *)obj;
	int sk = (int)c->data;

	unused(typ);

	if (self != 0){
		sprintf(buf, "%s ist in %s ungewöhnlich ungeschickt. (%s)", u->name,
				skillname((skill_t)sk, u->faction->locale), curseid(c));
		return 1;
	}
	return 0;
}
static struct curse_type ct_skillmod = {
	"skillmod",
	CURSETYP_NORM, CURSE_SPREADMODULO, M_MEN,
	"",
	cinfo_skill,
	NULL,
	read_skill, write_skill
};

/* ------------------------------------------------------------- */
void
register_unitcurse(void)
{
	register_function((pf_generic)cinfo_unit_onlyowner, "curseinfo::unit_onlyowner");
	register_function((pf_generic)cinfo_auraboost, "curseinfo::auraboost");
	register_function((pf_generic)cinfo_slave, "curseinfo::slave");
	register_function((pf_generic)cinfo_calm, "curseinfo::calm");
	register_function((pf_generic)cinfo_speed, "curseinfo::speed");
	register_function((pf_generic)cinfo_orc, "curseinfo::orc");
	register_function((pf_generic)cinfo_kaelteschutz, "curseinfo::kaelteschutz");
	register_function((pf_generic)cinfo_sparkle, "curseinfo::sparkle");
	register_function((pf_generic)cinfo_strength, "curseinfo::strength");
	register_function((pf_generic)cinfo_allskills, "curseinfo::allskills");
	register_function((pf_generic)cinfo_skill, "curseinfo::skill");
	register_function((pf_generic)cinfo_itemcloak, "curseinfo::itemcloak");
	register_function((pf_generic)cinfo_fumble, "curseinfo::fumble");

	ct_register(&ct_auraboost);
	ct_register(&ct_magicboost);
	ct_register(&ct_slavery);
	ct_register(&ct_calmmonster);
	ct_register(&ct_speed);
	ct_register(&ct_orcish);
	ct_register(&ct_insectfur);
	ct_register(&ct_sparkle);
	ct_register(&ct_strength);
	ct_register(&ct_worse);
	ct_register(&ct_skillmod);
	ct_register(&ct_itemcloak);
	ct_register(&ct_fumble);
	ct_register(&ct_oldrace);
	ct_register(&ct_magicresistance);
}


