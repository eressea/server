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

int
cinfo_unit(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
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
cinfo_unit_onlyowner(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	message * msg;
	struct unit *u;

	unused(typ);

	assert(typ == TYP_UNIT);
	u = (struct unit *)obj;

	if (self){
		msg = msg_message(mkname("curseinfo", c->type->cname), "id", c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}
	return 0;
}

/* CurseInfo mit Spezialabfragen */

/* C_AURA */
/* erhöht/senkt regeneration und maxaura um effect% */
static int
cinfo_auraboost(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	struct unit *u;
	unused(typ);
	assert(typ == TYP_UNIT);
	u = (struct unit *)obj;

	if (self){
		if (c->effect > 100){
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

/* C_SLAVE */
static int
cinfo_slave(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit *u;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if (self){
		sprintf(buf, "%s wird noch %d Woche%s unter unserem Bann stehen. (%s)",
				u->name, c->duration, (c->duration == 1)? "":"n", curseid(c));
		return 1;
	}
	return 0;
}

/* C_CALM */
static int
cinfo_calm(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
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
		if (self) {
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
/* C_SPEED */
static int
cinfo_speed(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit *u;
	curse_unit * cu;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;
	cu = (curse_unit *)c->data;

	if (self){
		sprintf(buf, "%d Person%s von %s %s noch %d Woche%s beschleunigt. (%s)",
				cu->cursedmen, (cu->cursedmen == 1)? "":"en", u->name,
				(cu->cursedmen == 1)? "ist":"sind", c->duration,
				(c->duration == 1)? "":"n",
				curseid(c));
		return 1;
	}
	return 0;
}
/* C_ORC */
static int
cinfo_orc(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit *u;
	message * msg;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if (self){
		msg = msg_message(mkname("curseinfo", c->type->cname), "unit id", u, c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}
	return 0;
}

/* C_KAELTESCHUTZ */
static int
cinfo_kaelteschutz(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit *u;
	curse_unit * cu;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;
	cu = (curse_unit *)c->data;

	if (self){
		sprintf(buf, "%d Person%s von %s %s sich vor Kälte geschützt. (%s)",
				cu->cursedmen, (cu->cursedmen == 1)? "":"en", u->name,
				(cu->cursedmen == 1)? "fühlt":"fühlen",
				curseid(c));
		return 1;
	}
	return 0;
}

/* C_SPARKLE */
static int
cinfo_sparkle(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
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
	else sprintf(buf, effects[begin + c->effect % (end-begin)], u->name);
	scat(" (");
	scat(itoa36(c->no));
	scat(")");

	return 1;
}

/* C_STRENGTH */
static int
cinfo_strength(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unused(c);
	unused(typ);

	assert(typ == TYP_UNIT);
	unused(obj);

	if (self){
		sprintf(buf, "Die Leute strotzen nur so vor Kraft. (%s)",
				curseid(c));
		return 1;
	}
	return 0;
}
/* C_ALLSKILLS */
static int
cinfo_allskills(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unused(obj);
	unused(typ);
	unused(c);

	assert(typ == TYP_UNIT);

	if (self){
		sprintf(buf, "Wird von einem Alp geritten. (%s)", curseid(c));
		return 1;
	}
	return 0;
}
/* C_SKILL */
static int
cinfo_skill(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit *u = (unit *)obj;
	int sk = (int)c->data;

	unused(typ);

	if (self){
		sprintf(buf, "%s ist in %s ungewöhnlich ungeschickt. (%s)", u->name,
				skillname((skill_t)sk, u->faction->locale), curseid(c));
		return 1;
	}
	return 0;
}
/* C_ITEMCLOAK */
static int
cinfo_itemcloak(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit *u;
	unused(typ);

	assert(typ == TYP_UNIT);
	u = (unit *)obj;

	if (self) {
		sprintf(buf, "Die Ausrüstung von %s scheint unsichtbar. (%s)",
				u->name, curseid(c));
		return 1;
	}
	return 0;
}

static int
cinfo_fumble(const struct locale * lang, void * obj, enum typ_t typ, struct curse *c, int self)
{
	unit * u = (unit*)obj;
	unused(typ);

	assert(typ == TYP_UNIT);

	if (self){
		sprintf(buf, "%s kann sich kaum konzentrieren.i (%s)",
				u->name, curseid(c));
		return 1;
	}
	return 0;
}


void
register_unitcurse(void)
{
	register_function((pf_generic)register_function((pf_generic)cinfo_unit, "unit");, "cinfo::unit, "unit");");
	register_function((pf_generic)register_function((pf_generic)cinfo_unit_onlyowner, "unit_onlyowner");, "cinfo::unit_onlyowner, "unit_onlyowner");");
	register_function((pf_generic)register_function((pf_generic)cinfo_auraboost, "auraboost");, "cinfo::auraboost, "auraboost");");
	register_function((pf_generic)register_function((pf_generic)cinfo_slave, "slave");, "cinfo::slave, "slave");");
	register_function((pf_generic)register_function((pf_generic)cinfo_calm, "calm");, "cinfo::calm, "calm");");
	register_function((pf_generic)register_function((pf_generic)cinfo_speed, "speed");, "cinfo::speed, "speed");");
	register_function((pf_generic)register_function((pf_generic)cinfo_orc, "orc");, "cinfo::orc, "orc");");
	register_function((pf_generic)register_function((pf_generic)cinfo_kaelteschutz, "kaelteschutz");, "cinfo::kaelteschutz, "kaelteschutz");");
	register_function((pf_generic)register_function((pf_generic)cinfo_sparkle, "sparkle");, "cinfo::sparkle, "sparkle");");
	register_function((pf_generic)register_function((pf_generic)cinfo_strength, "strength");, "cinfo::strength, "strength");");
	register_function((pf_generic)register_function((pf_generic)cinfo_allskills, "allskills");, "cinfo::allskills, "allskills");");
	register_function((pf_generic)register_function((pf_generic)cinfo_skill, "skill");, "cinfo::skill, "skill");");
	register_function((pf_generic)register_function((pf_generic)cinfo_itemcloak, "itemcloak");, "cinfo::itemcloak, "itemcloak");");
	register_function((pf_generic)register_function((pf_generic)cinfo_fumble, "fumble");, "cinfo::fumble, "fumble");");
}
