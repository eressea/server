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
#include <region.h>
#include <message.h>
#include <nrmessage.h>
#include <objtypes.h>
#include <curse.h>

/* util includes */
#include <message.h>
#include <functions.h>

/* libc includes */
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* --------------------------------------------------------------------- */

int
cinfo_region(const struct locale * lang, const void * obj, typ_t typ, struct curse *c, int self)
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

/* 
 * CurseInfo mit Spezialabfragen 
 */

/* godcursezone */
static int
cinfo_cursed_by_the_gods(const locale * lang, const void * obj, typ_t typ, curse *c, int self)
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
cinfo_dreamcurse(const locale * lang, const void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(self);
	unused(typ);
	unused(obj);
	assert(typ == TYP_REGION);

	if (c->effect > 0){
		msg = msg_message("curseinfo::gooddream", "id", c->no);
	}else{
		msg = msg_message("curseinfo::baddream", "id", c->no);
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);

	return 1;
}
/* C_MAGICSTREET */
static int
cinfo_magicstreet(const locale * lang, const void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(typ);
	unused(self);
	unused(obj);

	assert(typ == TYP_REGION);

	/* Warnung vor Auflösung */
	if (c->duration <= 2){
		msg = msg_message("curseinfo::magicstreet", "id", c->no);
	} else {
		msg = msg_message("curseinfo::magicstreetwarn", "id", c->no);
	}
	nr_render(msg, lang, buf, sizeof(buf), NULL);
	msg_release(msg);

	return 1;
}
static int
cinfo_antimagiczone(const locale * lang, const void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(typ);
	unused(self);
	unused(obj);

	assert(typ == TYP_REGION);

	/* Tybied Magier spüren eine Antimagiezone */
	if (self != 0){
		msg = msg_message("curseinfo::antimagiczone", "id", c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}

	return 0;
}
static int
cinfo_farvision(const locale * lang, const void * obj, typ_t typ, curse *c, int self)
{
	message * msg;

	unused(typ);
	unused(obj);

	assert(typ == TYP_REGION);

	/* Magier spüren eine farvision */
	if (self != 0){
		msg = msg_message("curseinfo::farvision", "id", c->no);
		nr_render(msg, lang, buf, sizeof(buf), NULL);
		msg_release(msg);
		return 1;
	}

	return 0;
}

/* --------------------------------------------------------------------- */

/* --------------------------------------------------------------------- */

static struct curse_type ct_fogtrap = {
	"fogtrap",
	CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
	"",
	cinfo_region
};

static struct curse_type ct_antimagiczone = { 
	"antimagiczone",
	CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
	"Dieser Zauber scheint magische Energien irgendwie abzuleiten und "
	"so alle in der Region gezauberten Sprüche in ihrer Wirkung zu "
	"schwächen oder ganz zu verhindern.",
	cinfo_antimagiczone
};
static struct curse_type ct_farvision = { 
	"farvision",
	CURSETYP_NORM, 0, (NO_MERGE),
	"",
	cinfo_farvision
};
static struct curse_type ct_gbdream = { 
	"gbdream",
	CURSETYP_NORM, 0, (NO_MERGE),
	"",
	cinfo_region
};
static struct curse_type ct_maelstrom = {
	"maelstrom",
	CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
	"Dieser Zauber verursacht einen gigantischen magischen Strudel. Der "
	"Mahlstrom wird alle Schiffe, die in seinen Sog geraten, schwer "
	"beschädigen.",
	NULL
};
static struct curse_type ct_blessedharvest = {
	"blessedharvest",
	CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
	"Dieser Fruchtbarkeitszauber erhöht die Erträge der Felder.",
	cinfo_region
};
static struct curse_type ct_drought = {
	"drought",
	CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
	"Dieser Zauber strahlt starke negative Energien aus. Warscheinlich "
	"ist er die Ursache der Dürre."	,
	cinfo_region
};
static struct curse_type ct_badlearn = {
	"badlearn",
	CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
	"Dieser Zauber scheint die Ursache für die Schlaflosigkeit und "
	"Mattigkeit zu sein, unter der die meisten Leute hier leiden und "
	"die dazu führt, das Lernen weniger Erfolg bringt. ",
	cinfo_region
};
/*  Trübsal-Zauber */
static struct curse_type ct_depression = {
	"depression",
	CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR ),
	"Wie schon zu vermuten war, sind der ewig graue Himmel und die "
	"depressive Stimmung in der Region nicht natürlich. Dieser Fluch "
	"hat sich wie ein bleiernes Tuch auf die Gemüter der Bevölkerung "
	"gelegt und eh er nicht gebrochen oder verklungen ist, wird keiner "
	"sich an Gaukelleien erfreuen können.",
	cinfo_region
};

/* Astralblock, auf Astralregion */
static struct curse_type ct_astralblock = {
	"astralblock",
	CURSETYP_NORM, 0, NO_MERGE,
	"",
	cinfo_region
};
/* Unterhaltungsanteil vermehren */
static struct curse_type ct_generous = {
	"generous",
	CURSETYP_NORM, 0, ( M_DURATION | M_VIGOUR | M_MAXEFFECT ),
	"Dieser Zauber beeinflusst die allgemeine Stimmung in der Region positiv. "
	"Die gute Laune macht die Leute freigiebiger.",
	cinfo_region
};
/* verhindert Attackiere regional */
static struct curse_type ct_peacezone = {
	"peacezone",
	CURSETYP_NORM, 0, NO_MERGE,
	"Dieser machtvoller Beeinflussungszauber erstickt jeden Streit schon im "
	"Keim.",
	cinfo_region
};
/* erschwert geordnete Bewegungen */
static struct curse_type ct_disorientationzone = {
	"disorientationzone",
	CURSETYP_NORM, 0, NO_MERGE,
	"",
	cinfo_region
};
/*  erzeugt Straßennetz */
static struct curse_type ct_magicstreet = {
	"magicstreet",
	CURSETYP_NORM, 0, (M_DURATION | M_VIGOUR),
	"Es scheint sich um einen elementarmagischen Zauber zu handeln, der alle "
	"Pfade und Wege so gut festigt, als wären sie gepflastert. Wie auf einer "
	"Straße kommt man so viel besser und schneller vorwärts.",
	cinfo_magicstreet
};
/*  erniedigt Magieresistenz von nicht-aliierten Einheiten, wirkt nur 1x
 *  pro Einheit */
static struct curse_type ct_badmagicresistancezone = {
	"badmagicresistancezone",
	CURSETYP_NORM, 0, NO_MERGE,
	"Dieses Lied, das irgendwie in die magische Essenz der Region gewoben "
	"ist, schwächt die natürliche Widerstandskraft gegen eine "
	"Verzauberung. Es scheint jedoch nur auf bestimmte Einheiten zu wirken.",
	cinfo_region
};
/* erhöht Magieresistenz von aliierten Einheiten, wirkt nur 1x pro
 * Einheit */
static struct curse_type ct_goodmagicresistancezone = {
	"goodmagicresistancezone",
	CURSETYP_NORM, 0, NO_MERGE,
	"Dieser Lied, das irgendwie in die magische Essenz der Region gewoben "
	"ist, verstärkt die natürliche Widerstandskraft gegen eine "
	"Verzauberung. Es scheint jedoch nur auf bestimmte Einheiten zu wirken.",
	cinfo_region
};
static struct curse_type ct_riotzone = {
	"riotzone",
	CURSETYP_NORM, 0, (M_DURATION),
	"Eine Wolke negativer Energie liegt über der Region.",
	cinfo_region
};
static struct curse_type ct_holyground = {
	"holyground",
	CURSETYP_NORM, 0, (M_VIGOUR_ADD),
	"Verschiedene Naturgeistern sind im Boden der Region gebunden und "
	"beschützen diese vor dem der dunklen Magie des lebenden Todes.",
	cinfo_region
};
static struct curse_type ct_godcursezone = {
	"godcursezone",
	CURSETYP_NORM, 0, (NO_MERGE),
	"Diese Region wurde von den Göttern verflucht. Stinkende Nebel ziehen "
	"über die tote Erde, furchbare Kreaturen ziehen über das Land. Die Brunnen "
	"sind vergiftet, und die wenigen essbaren Früchte sind von einem rosa Pilz "
	"überzogen. Niemand kann hier lange überleben.",
	cinfo_cursed_by_the_gods,
};


void 
register_regioncurse(void)
{
	register_function((pf_generic)cinfo_cursed_by_the_gods, "curseinfo::cursed_by_the_gods");
	register_function((pf_generic)cinfo_dreamcurse, "curseinfo::dreamcurse");
	register_function((pf_generic)cinfo_magicstreet, "curseinfo::magicstreet");

	ct_register(&ct_fogtrap);
	ct_register(&ct_antimagiczone);
	ct_register(&ct_farvision);
	ct_register(&ct_gbdream);
	ct_register(&ct_maelstrom);
	ct_register(&ct_blessedharvest);
	ct_register(&ct_drought);
	ct_register(&ct_badlearn);
	ct_register(&ct_depression);
	ct_register(&ct_astralblock);
	ct_register(&ct_generous);
	ct_register(&ct_peacezone);
	ct_register(&ct_disorientationzone);
	ct_register(&ct_magicstreet);
	ct_register(&ct_badmagicresistancezone);
	ct_register(&ct_goodmagicresistancezone);
	ct_register(&ct_riotzone);
	ct_register(&ct_godcursezone);
	ct_register(&ct_holyground);
}


