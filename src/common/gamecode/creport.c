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
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#include <config.h>
#include <eressea.h>
#include "creport.h"

/* tweakable features */
#define ENCODE_SPECIAL 1
#define RENDER_CRMESSAGES 1

/* modules include */
#include <modules/score.h>

/* attributes include */
#include <attributes/follow.h>
#include <attributes/racename.h>
#include <attributes/orcification.h>
#include <attributes/otherfaction.h>
#include <attributes/raceprefix.h>

/* gamecode includes */
#include "laws.h"
#include "economy.h"

/* kernel includes */
#include <alchemy.h>
#include <border.h>
#include <building.h>
#include <faction.h>
#include <group.h>
#include <item.h>
#include <karma.h>
#include <magic.h>
#include <message.h>
#include <movement.h>
#include <plane.h>
#include <race.h>
#include <region.h>
#include <reports.h>
#include <resources.h>
#include <ship.h>
#include <skill.h>
#include <teleport.h>
#include <unit.h>
#include <save.h>

/* util includes */
#include <util/message.h>
#include <goodies.h>
#include <crmessage.h>
#include <nrmessage.h>
#include <language.h>

/* libc includes */
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* imports */
extern const char *directions[];
extern const char *spelldata[];
extern int quiet;

/* globals */
#define C_REPORT_VERSION 64

#define TAG_LOCALE "de"
#ifdef TAG_LOCALE
static const char *
crtag(const char * key)
{
	static const locale * lang = NULL;
	if (!lang) lang = find_locale(TAG_LOCALE);
	return locale_string(lang, key);
}
#else
#define crtag(x) (x)
#endif
/*
 * translation table
 */
typedef struct translation {
	struct translation * next;
	char * key;
	const char * value;
} translation;

#define TRANSMAXHASH 255
static translation * translation_table[TRANSMAXHASH];
static translation * junkyard;

static const char *
add_translation(const char * key, const char * value)
{
	int kk = ((key[0] << 5) + key[0]) % TRANSMAXHASH;
	translation * t = translation_table[kk];
	while (t && strcmp(t->key, key)!=0) t=t->next;
	if (!t) {
		if (junkyard) {
			t = junkyard;
			junkyard = junkyard->next;
		} else t = malloc(sizeof(translation));
		t->key = strdup(key);
		t->value = value;
		t->next = translation_table[kk];
		translation_table[kk] = t;
	}
	return crtag(key);
}

static void
write_translations(FILE * F)
{
	int i;
	fputs("TRANSLATION\n", F);
	for (i=0;i!=TRANSMAXHASH;++i) {
		translation * t = translation_table[i];
		while (t) {
			fprintf(F, "\"%s\";%s\n", t->value, crtag(t->key));
			t = t->next;
		}
	}
}

static void
reset_translations(void)
{
	int i;
	for (i=0;i!=TRANSMAXHASH;++i) {
		translation * t = translation_table[i];
		while (t) {
			translation * c = t->next;
			free(t->key);
			t->next = junkyard;
			junkyard = t;
			t = c;
		}
		translation_table[i] = 0;
	}
}

/* implementation */
void
cr_output_str_list(FILE * F, const char *title, const strlist * S, const faction * f)
{
	char *s;

	if (!S) return;

	fprintf(F, "%s\n", title);
	while (S) {
		s = replace_global_coords(S->s, f);
		fprintf(F, "\"%s\"\n", s);
		S = S->next;
	}
}

#include "objtypes.h"

static void
print_curses(FILE * F, const void * obj, typ_t typ, const attrib *a, int self)
{
	boolean header = false;
	while (a) {
		int dh = 0;
		curse *c;

		if (fval(a->type, ATF_CURSE)) {

			c = (curse *)a->data.v;
			if (c->type->curseinfo)
				dh = c->type->curseinfo(obj, typ, c, self);
			if (dh == 1) {
				if (!header) {
					header = 1;
					fputs("EFFECTS\n", F);
				}
				fprintf(F, "\"%s\"\n", buf);
			}
		} else if (a->type==&at_effect && self) {
			effect_data * data = (effect_data *)a->data.v;
			const char * key = resourcename(data->type->itype->rtype, 0);
			if (!header) {
				header = 1;
				fputs("EFFECTS\n", F);
			}
			fprintf(F, "\"%d %s\"\n", data->value, add_translation(key, locale_string(default_locale, key)));
		}
		a = a->next;
	}
}

static int
cr_unit(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	unit * u = (unit *)v;
	sprintf(buffer, "%d", u?u->no:-1);
	unused(report);
	return 0;
}

static int
cr_ship(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	ship * u = (ship *)v;
	sprintf(buffer, "%d", u?u->no:-1);
	unused(report);
	return 0;
}

static int
cr_building(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	building * u = (building *)v;
	sprintf(buffer, "%d", u?u->no:-1);
	unused(report);
	return 0;
}

static int
cr_faction(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	faction * f = (faction *)v;
	sprintf(buffer, "%d", f?f->no:-1);
	unused(report);
	return 0;
}

static int
cr_region(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	region * r = (region *)v;
	if (r) {
		plane * p = rplane(r);
		if (!p || !(p->flags & PFL_NOCOORDS)) {
			sprintf(buffer, "%d %d %d", region_x(r, report), region_y(r, report), p?p->id:0);
			return 0;
		}
	}
	return -1;
}

static int
cr_resource(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	const resource_type * r = (const resource_type *)v;
	if (r) {
		const char * key = resourcename(r, 0);
		sprintf(buffer, "\"%s\"",
			add_translation(key, locale_string(report->locale, key)));
		return 0;
	}
	return -1;
}

static int
cr_race(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	const struct race * rc = (const race *)v;
	const char * key = rc_name(rc, 0);
	sprintf(buffer, "\"%s\"",
		add_translation(key, locale_string(report->locale, key)));
	return 0;
}

static int
cr_skill(const void * v, char * buffer, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	skill_t sk = (skill_t)(int)v;
	if (sk!=NOSKILL) sprintf(buffer, "\"%s\"",
		add_translation(skillname(sk, NULL), skillname(sk, report->locale)));
	else strcpy(buffer, "\"\"");
	return 0;
}

void
creport_init(void)
{
	tsf_register("report", &cr_ignore);
	tsf_register("string", &cr_string);
	tsf_register("int", &cr_int);
	tsf_register("unit", &cr_unit);
	tsf_register("region", &cr_region);
	tsf_register("faction", &cr_faction);
	tsf_register("ship", &cr_ship);
	tsf_register("building", &cr_building);
	tsf_register("skill", &cr_skill);
	tsf_register("resource", &cr_resource);
	tsf_register("race", &cr_race);
	tsf_register("direction", &cr_int);
}

void
creport_cleanup(void)
{
	while (junkyard) {
		translation * t = junkyard;
		junkyard = junkyard->next;
		free(t);
	}
	junkyard = 0;
}

static int msgno;

#define MTMAXHASH 1023

static struct known_mtype {
	const struct message_type * mtype;
	struct known_mtype * nexthash;
} * mtypehash[MTMAXHASH];

static void
report_crtypes(FILE * F, const struct locale* lang)
{
	int i;
#ifdef OLD_MESSAGETYPES
	fputs("MESSAGETYPES\n", F);
	for (i=0;i!=MTMAXHASH;++i) {
		struct known_mtype * kmt;
		for (kmt=mtypehash[i];kmt;kmt=kmt->nexthash) {
			const struct nrmessage_type * nrt = nrt_find(lang, kmt->mtype);
			if (nrt) {
				unsigned int hash = hashstring(mt_name(kmt->mtype));
				fputc('\"', F);
				fputs(escape_string(nrt_string(nrt), NULL, 0), F);
				fprintf(F, "\";%d\n", hash);
			}
		}
		while (mtypehash[i]) {
			kmt = mtypehash[i];
			mtypehash[i] = mtypehash[i]->nexthash;
			free(kmt);
		}
	}
#else
	for (i=0;i!=MTMAXHASH;++i) {
		struct known_mtype * kmt;
		for (kmt=mtypehash[i];kmt;kmt=kmt->nexthash) {
			const struct nrmessage_type * nrt = nrt_find(lang, kmt->mtype);
			if (nrt) {
				unsigned int hash = hashstring(mt_name(kmt->mtype));
				fprintf(F, "MESSAGETYPE %d\n", hash);
				fputc('\"', F);
				fputs(escape_string(nrt_string(nrt), NULL, 0), F);
				fputs("\";text\n", F);
				fprintf(F, "\"%s\";section\n", nrt_section(nrt));
			}
		}
		while (mtypehash[i]) {
			kmt = mtypehash[i];
			mtypehash[i] = mtypehash[i]->nexthash;
			free(kmt);
		}
	}
#endif
}

static void
render_messages(FILE * F, faction * f, message_list *msgs)
{
	struct mlist* m = msgs->begin;
	while (m) {
		char crbuffer[1024*32]; /* gross, wegen spionage-messages :-( */
		boolean printed = false;
		const struct message_type * mtype = m->msg->type;
		unsigned int hash = hashstring(mtype->name);
#if RENDER_CRMESSAGES
		char nrbuffer[1024*32];
		nrbuffer[0] = '\0';
		if (nr_render(m->msg, f->locale, nrbuffer, sizeof(nrbuffer), f)==0 && nrbuffer[0]) {
			fprintf(F, "MESSAGE %d\n", ++msgno);
			fprintf(F, "%d;type\n", hash);
			fputs("\"", F);
			fputs(nrbuffer, F);
			fputs("\";rendered\n", F);
			printed = true;
		}
#endif
		crbuffer[0] = '\0';
		if (cr_render(m->msg, crbuffer, (const void*)f)==0) {
			if (!printed) fprintf(F, "MESSAGE %d\n", ++msgno);
			if (crbuffer[0]) fputs(crbuffer, F);
		} else {
			log_error(("could not render cr-message %p: %s\n", m->msg, m->msg->type->name));
		}
		if (printed) {
			unsigned int ihash = hash % MTMAXHASH;
			struct known_mtype * kmt = mtypehash[ihash];
			while (kmt && kmt->mtype != mtype) kmt = kmt->nexthash;
			if (kmt==NULL) {
				kmt = (struct known_mtype*)malloc(sizeof(struct known_mtype));
				kmt->nexthash = mtypehash[ihash];
				kmt->mtype = mtype;
				mtypehash[ihash] = kmt;
			}
		}
		m = m->next;
	}
}

static void
cr_output_messages(FILE * F, message_list *msgs, faction * f)
{
	if (msgs) render_messages(F, f, msgs);
}

/* prints a building */
static void
cr_output_buildings(FILE * F, building * b, unit * u, int fno, faction *f)
{
	const building_type * type = b->type;
	unit * bo = buildingowner(b->region, b);
	const char * bname = buildingtype(b, b->size);

	fprintf(F, "BURG %d\n", b->no);
	if (!u || u->faction != f) {
		const attrib * a = a_find(b->attribs, &at_icastle);
		if (a) type = ((icastle_data*)a->data.v)->type;
	}
	fprintf(F, "\"%s\";Typ\n", add_translation(bname, LOC(f->locale, bname)));
	fprintf(F, "\"%s\";Name\n", b->name);
	if (strlen(b->display))
		fprintf(F, "\"%s\";Beschr\n", b->display);
	if (b->size)
		fprintf(F, "%d;Groesse\n", b->size);
	if (u)
		fprintf(F, "%d;Besitzer\n", u ? u->no : -1);
	if (fno >= 0)
		fprintf(F, "%d;Partei\n", fno);
#ifdef TODO
	int cost = buildingdaten[b->type].per_size * b->size + buildingdaten[b->type].unterhalt;
	if (u && u->faction == f && cost)
		fprintf(F, "%d;Unterhalt\n", cost);
#endif
	if (b->besieged)
		fprintf(F, "%d;Belagerer\n", b->besieged);
	if(bo != NULL){
		print_curses(F, b, TYP_BUILDING, b->attribs,
				(bo->faction == f)? 1 : 0);
	} else {
		print_curses(F, b, TYP_BUILDING, b->attribs, 0);
	}
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints a ship */
static void
cr_output_ship(FILE * F, const ship * s, const unit * u, int fcaptain, const faction * f, const region * r)
{
	unit *u2;
	int w = 0;
	assert(s);
	fprintf(F, "SCHIFF %d\n", s->no);
	fprintf(F, "\"%s\";Name\n", s->name);
	if (strlen(s->display))
		fprintf(F, "\"%s\";Beschr\n", s->display);
	fprintf(F, "\"%s\";Typ\n", add_translation(s->type->name[0], locale_string(f->locale, s->type->name[0])));
	fprintf(F, "%d;Groesse\n", s->size);
	if (s->damage) {
		int percent = s->damage*100/(s->size*DAMAGE_SCALE);
		fprintf(F, "%d;Schaden\n", percent);
	}
	if (u)
		fprintf(F, "%d;Kapitaen\n", u ? u->no : -1);
	if (fcaptain >= 0)
		fprintf(F, "%d;Partei\n", fcaptain);

	/* calculate cargo */
	if (u && (u->faction == f || omniscient(f))) {
		for (u2 = r->units; u2; u2 = u2->next)
			if (u2->ship == s)
				w += weight(u2);

		fprintf(F, "%d;Ladung\n", (w + 99) / 100);
		fprintf(F, "%d;MaxLadung\n", shipcapacity(s) / 100);
	}
	/* shore */
	w = NODIRECTION;
	if (rterrain(r) != T_OCEAN) w = s->coast;
	if (w != NODIRECTION)
		fprintf(F, "%d;Kueste\n", w);

	if(shipowner(r, s) != NULL){
		print_curses(F, s, TYP_SHIP, s->attribs,
				(shipowner(r, s)->faction == f)? 1 : 0);
	} else {
		print_curses(F, s, TYP_SHIP, s->attribs, 0);
	}
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints all that belongs to a unit */
static void
cr_output_unit(FILE * F, const region * r,
	       const faction * f,	/* observers faction */
	       const unit * u, int mode)
{
	/* Race attributes are always plural and item attributes always
	 * singular */
	const item_type * lasttype;
	int pr;
	item *itm, *show;
	boolean itemcloak = is_cursed(u->attribs, C_ITEMCLOAK, 0);
	building * b;
	const char * pzTmp;
	skill_t sk;
	strlist *S;
	const attrib *a_fshidden = NULL;

	assert(u);

	if(fspecial(u->faction, FS_HIDDEN))
		a_fshidden = a_find(u->attribs, &at_fshidden);

	fprintf(F, "EINHEIT %d\n", u->no);
	fprintf(F, "\"%s\";Name\n", u->name);
	if (strlen(u->display))
		fprintf(F, "\"%s\";Beschr\n", u->display);

	{
		/* print faction information */
		const faction * sf = visible_faction(f,u);
		if (u->faction == f || omniscient(f)) {
			const attrib *a_otherfaction = a_find(u->attribs, &at_otherfaction);
			/* my own faction, full info */
			const attrib * a = a_find(u->attribs, &at_group);
			const attrib * ap = 0;
			if (a) {
				const group * g = (const group*)a->data.v;
				ap = a_find(g->attribs, &at_raceprefix);
				fprintf(F, "%d;gruppe\n", g->gid);
			}
			if (ap==NULL) {
				ap = a_find(u->faction->attribs, &at_raceprefix);
			}
			if (ap) {
				const char * name = (const char*)ap->data.v;
				fprintf(F, "\"%s\";typprefix\n", add_translation(name, LOC(f->locale, name)));
			}
			fprintf(F, "%d;Partei\n", u->faction->no);
			if (sf!=u->faction) fprintf(F, "%d;Verkleidung\n", sf->no);
			if (fval(u, FL_PARTEITARNUNG))
				fprintf(F, "%d;Parteitarnung\n", i2b(fval(u, FL_PARTEITARNUNG)));
			if (a_otherfaction)
				fprintf(F, "%d;Anderepartei\n", a_otherfaction->data.i);
		} else {
			const attrib * a = a_find(u->attribs, &at_group);
			if (fval(u, FL_PARTEITARNUNG)) {
				/* faction info is hidden */
				fprintf(F, "%d;Parteitarnung\n", i2b(fval(u, FL_PARTEITARNUNG)));
			} else {
				/* other unit. show visible faction, not u->faction */
				fprintf(F, "%d;Partei\n", sf->no);
				if (sf == f) {
					fprintf(F, "1;Verraeter\n");
				}
			}
			if (a) {
				const attrib *agrp = a_find(((const group*)a->data.v)->attribs, &at_raceprefix);
				if (agrp==NULL) {
					agrp = a_find(u->faction->attribs, &at_raceprefix);
				}
				if (agrp) {
					const char * name = (const char*)agrp->data.v;
					fprintf(F, "\"%s\";typprefix\n", add_translation(name, LOC(f->locale, name)));
				}
			}
		}
	}
	if(u->faction != f && a_fshidden
			&& a_fshidden->data.ca[0] == 1 && effskill(u, SK_STEALTH) >= 6) {
		fprintf(F, "-1;Anzahl\n");
	} else {
		fprintf(F, "%d;Anzahl\n", u->number);
	}


	pzTmp = get_racename(u->attribs);
	if (pzTmp==NULL) {
		const char * zRace = rc_name(u->irace, 1);
		fprintf(F, "\"%s\";Typ\n", 
			add_translation(zRace, locale_string(f->locale, zRace)));
	}
	else fprintf(F, "\"%s\";Typ\n", pzTmp);
	if ((pzTmp || u->irace != u->race) && u->faction==f) {
		const char * zRace = rc_name(u->race, 1);
		fprintf(F, "\"%s\";wahrerTyp\n",
			add_translation(zRace, locale_string(f->locale, zRace)));
	}

	if (u->building)
		fprintf(F, "%d;Burg\n", u->building->no);
	if (u->ship)
		fprintf(F, "%d;Schiff\n", u->ship->no);
	if (getguard(u))
		fprintf(F, "%d;bewacht\n", getguard(u)?1:0);
	if ((b=usiege(u))!=NULL)
		fprintf(F, "%d;belagert\n", b->no);

	/* additional information for own units */
	if (u->faction == f || omniscient(f)) {
		const char *c;
		int i;
		const attrib * a;

		a = a_find(u->attribs, &at_follow);
		if (a) {
			unit * u = (unit*)a->data.v;
			if (u) fprintf(F, "%d;folgt\n", u->no);
		}
		i = ualias(u);
		if (i>0)
			fprintf(F, "%d;temp\n", i);
		else if (i<0)
			fprintf(F, "%d;alias\n", -i);
		i = get_money(u);
		fprintf(F, "%d;Kampfstatus\n", u->status);
		if(fval(u, FL_NOAID)) {
			fputs("1;unaided\n", F);
		}
		i = u_geteffstealth(u);
		if (i >= 0)
			fprintf(F, "%d;Tarnung\n", i);
		c = uprivate(u);
		if (c)
			fprintf(F, "\"%s\";privat\n", c);
		c = hp_status(u);
		if (c && *c && (u->faction == f || omniscient(f)))
			fprintf(F, "\"%s\";hp\n", add_translation(c, locale_string(u->faction->locale, c)));
		if (fval(u, FL_HUNGER) && (u->faction == f))
			fputs("1;hunger\n", F);
		if (is_mage(u)) {
			fprintf(F, "%d;Aura\n", get_spellpoints(u));
			fprintf(F, "%d;Auramax\n", max_spellpoints(u->region,u));
		}

		/* default commands */
		fprintf(F, "COMMANDS\n");
		if(u->lastorder[0] && u->lastorder[0] != '@') fprintf(F, "\"%s\"\n", u->lastorder);
		for (S = u->orders; S; S = S->next) {
			if(is_persistent(S->s, u->faction->locale)) {
				fprintf(F, "\"%s\"\n", S->s);
			}
		}

		/* talents */
		pr = 0;
		for (sk = 0; sk != MAXSKILLS; ++sk) {
			if (has_skill(u, sk)) {
				int esk = eff_skill(u, sk, r);
				if (!pr) {
					pr = 1;
					fprintf(F, "TALENTE\n");
				}
				fprintf(F, "%d %d;%s\n", u->number*level_days(get_level(u, sk)), esk,
					add_translation(skillname(sk, NULL), skillname(sk, f->locale)));
			}
		}
		/* spells */
		if (is_mage(u)) {
			sc_mage * mage = get_mage(u);
			spell_ptr *spt = mage->spellptr;
			if (spt) {
				spell *sp;
				int i;
				fprintf(F, "SPRUECHE\n");
				for (;spt; spt = spt->next) {
					sp = find_spellbyid(spt->spellid);
					fprintf(F, "\"%s\"\n", sp->name);
				}
				for (i=0;i!=MAXCOMBATSPELLS;++i) {
					sp = find_spellbyid(mage->combatspell[i]);
					if (sp) {
						fprintf(F, "KAMPFZAUBER %d\n", i);
						fprintf(F, "\"%s\";name\n", sp->name);
						fprintf(F, "%d;level\n", mage->combatspelllevel[i]);
					}
				}
			}
		}
	}
	/* items */
	pr = 0;
	if (f == u->faction || omniscient(u->faction)) {
		show = u->items;
	} else if (itemcloak==false && mode>=see_unit && !(a_fshidden
			&& a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 3)) {
		show = NULL;
		for (itm=u->items;itm;itm=itm->next) {
			item * ishow;
			const char * ic;
			int in;
			report_item(u, itm, f, NULL, &ic, &in, true);
			if (in>0 && ic && *ic) {
				for (ishow = show; ishow; ishow=ishow->next) {
					const char * sc;
					int sn;
					if (ishow->type==itm->type) sc=ic;
					else report_item(u, ishow, f, NULL, &sc, &sn, true);
					if (sc==ic || strcmp(sc, ic)==0) {
						ishow->number+=itm->number;
						break;
					}
				}
				if (ishow==NULL) {
					ishow = i_add(&show, i_new(itm->type, itm->number));
				}
			}
		}
	} else {
		show = NULL;
	}
	lasttype = NULL;
	for (itm=show; itm; itm=itm->next) {
		const char * ic;
		int in;
		assert(itm->type!=lasttype || !"error: list contains two objects of the same item");
		report_item(u, itm, f, NULL, &ic, &in, true);
		if (in==0) continue;
		if (!pr) {
			pr = 1;
			fputs("GEGENSTAENDE\n", F);
		}
		fprintf(F, "%d;%s\n", in, add_translation(ic, locale_string(f->locale, ic)));
	}

	if ((u->faction == f || omniscient(f)) && u->botschaften)
		cr_output_str_list(F, "EINHEITSBOTSCHAFTEN", u->botschaften, f);

	print_curses(F, u, TYP_UNIT, u->attribs, (u->faction == f)? 1 : 0);
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints allies */
static void
show_allies(FILE * F, ally * sf)
{
	for (; sf; sf = sf->next) if(sf->faction) {
		fprintf(F, "ALLIANZ %d\n", sf->faction->no);
		fprintf(F, "\"%s\";Parteiname\n", sf->faction->name);
		fprintf(F, "%d;Status\n", sf->status);
	}
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* prints all visible spells in a region */
static void
show_active_spells(const region * r)
{
	char fogwall[MAXDIRECTIONS];
#ifdef TODO /* alte Regionszauberanzeigen umstellen */
	unit *u;
	int env = 0;
#endif
	memset(fogwall, 0, sizeof(char) * MAXDIRECTIONS);

}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

/* this is a copy of laws.c->find_address output changed. */
static void
cr_find_address(FILE * F, const faction * uf, const faction_list * addresses)
{
	const faction_list * flist = addresses;
	if (!quiet)
		puts(" - gebe Adressen heraus (CR)");
	while (flist!=NULL) {
		const faction * f = flist->data;
		if (uf!=f && f->no != MONSTER_FACTION) {
			fprintf(F, "PARTEI %d\n", f->no);
			fprintf(F, "\"%s\";Parteiname\n", f->name);
			fprintf(F, "\"%s\";email\n", f->email);
			fprintf(F, "\"%s\";banner\n", f->banner);
		}
		flist = flist->next;
	}
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

static void
cr_reportspell(FILE * F, spellid_t id, const struct locale * lang)
{
	int k, itemanz, res, costtyp;
	spell *sp = find_spellbyid(id);

	fprintf(F, "ZAUBER %d\n", hashstring(sp->name));
	fprintf(F, "\"%s\";name\n", sp->name);
	fprintf(F, "%d;level\n", sp->level);
	fprintf(F, "%d;rank\n", sp->rank);
	fprintf(F, "\"%s\";info\n", sp->beschreibung);

	if (sp->sptyp & PRECOMBATSPELL) fputs("\"precombat\";class\n", F);
	else if (sp->sptyp & COMBATSPELL) fputs("\"combat\";class\n", F);
	else if (sp->sptyp & POSTCOMBATSPELL) fputs("\"postcombat\";class\n", F);
	else fputs("\"normal\";class\n", F);

	if (sp->sptyp & FARCASTING) fputs("1;far\n", F);
	if (sp->sptyp & OCEANCASTABLE) fputs("1;ocean\n", F);
	if (sp->sptyp & ONSHIPCAST) fputs("1;ship\n", F);
	if (!(sp->sptyp & NOTFAMILIARCAST)) fputs("1;familiar\n", F);
	fputs("KOMPONENTEN\n", F);

	for (k = 0; k < MAXINGREDIENT; k++) {
		res = sp->komponenten[k][0];
		itemanz = sp->komponenten[k][1];
		costtyp = sp->komponenten[k][2];
		if(itemanz > 0) {
			const char * name = resname(res, 0);
			fprintf(F, "%d %d;%s\n", itemanz, costtyp == SPC_LEVEL || costtyp == SPC_LINEAR,
				add_translation(name, LOC(lang, name)));
		}
	}
}

static unsigned int
encode_region(const faction * f, const region * r) {
	unsigned int id;
	char *cp, c;
	/* obfuscation */
	assert(sizeof(int)==sizeof(char)*4);
	id = (((((r->x ^ f->no) % 1024) << 20) | ((r->y ^ f->no) % 1024)));
	cp = (char*)&id;
	c = cp[0];
	cp[0] = cp[2];
	cp[2] = cp[1];
	cp[1] = cp[3];
	cp[3] = c;
	return id;
}

static char *
report_resource(char * buf, const char * name, const locale * loc, int amount, int level)
{
	buf += sprintf(buf, "RESOURCE %u\n", hashstring(name));
	buf += sprintf(buf, "\"%s\";type\n", add_translation(name, LOC(loc, name)));
	if (amount>=0) {
		if (level>=0) buf += sprintf(buf, "%d;skill\n", level);
		buf += sprintf(buf, "%d;number\n", amount);
	}
	return buf;
}

/* main function of the creport. creates the header and traverses all regions */
void
report_computer(FILE * F, faction * f, const seen_region * seen,
	const faction_list * addresses, const time_t report_time)
{
	int i;
	building *b;
	ship *sh;
	unit *u;
	const seen_region * sd = seen;
	const attrib * a;

	/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
	/* initialisations, header and lists */

	if (quiet) {
		printf(" CR");
		fflush(stdout);
	}
	else printf(" - schreibe Computerreport\n");

	fprintf(F, "VERSION %d\n", C_REPORT_VERSION);
	fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
	fprintf(F, "%d;noskillpoints\n", !SKILLPOINTS);
	fprintf(F, "%ld;date\n", report_time);
	fprintf(F, "\"%s\";Spiel\n", global.gamename);
	fprintf(F, "\"%s\";Konfiguration\n", "Standard");
	fprintf(F, "\"%s\";Koordinaten\n", "Hex");
	fprintf(F, "%d;Basis\n", 36);
	fprintf(F, "%d;Runde\n", turn);
	fputs("2;Zeitalter\n", F);
	fprintf(F, "PARTEI %d\n", f->no);
	fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
	fprintf(F, "%d;Optionen\n", f->options);
	if (f->options & want(O_SCORE) && f->age>DISPLAYSCORE) {
		fprintf(F, "%d;Punkte\n", f->score);
		fprintf(F, "%d;Punktedurchschnitt\n", average_score_of_age(f->age, f->age / 24 + 1));
	}
	{
		const char * zRace = rc_name(f->race, 1);
		fprintf(F, "\"%s\";Typ\n", add_translation(zRace, LOC(f->locale, zRace)));
	}
	a = a_find(f->attribs, &at_raceprefix);
	if (a) {
		const char * name = (const char*)a->data.v;
		fprintf(F, "\"%s\";typprefix\n", add_translation(name, LOC(f->locale, name)));
	}
	fprintf(F, "%d;Rekrutierungskosten\n", f->race->recruitcost);
	fprintf(F, "%d;Anzahl Personen\n", count_all(f));
	fprintf(F, "\"%s\";Magiegebiet\n", neue_gebiete[f->magiegebiet]);

	if (f->race == new_race[RC_HUMAN]) {
		fprintf(F, "%d;Anzahl Immigranten\n", count_migrants(f));
		fprintf(F, "%d;Max. Immigranten\n", count_maxmigrants(f));
	}
	fprintf(F, "\"%s\";Parteiname\n", f->name);
	fprintf(F, "\"%s\";email\n", f->email);
	fprintf(F, "\"%s\";banner\n", f->banner);
	fputs("OPTIONEN\n", F);
	for (i=0;i!=MAXOPTIONS;++i) {
		fprintf(F, "%d;%s\n", (f->options&want(i))?1:0, options[i]);
	}
	show_allies(F, f->allies);
	{
		group * g;
		for (g=f->groups;g;g=g->next) {
			const attrib *a = a_find(g->attribs, &at_raceprefix);

			fprintf(F, "GRUPPE %d\n", g->gid);
			fprintf(F, "\"%s\";name\n", g->name);
			if(a) {
				const char * name = (const char*)a->data.v;
				fprintf(F, "\"%s\";typprefix\n", add_translation(name, LOC(f->locale, name)));
			}
			show_allies(F, g->allies);
		}
	}

	cr_output_str_list(F, "FEHLER", f->mistakes, f);
	cr_output_messages(F, f->msgs, f);
	{
		struct bmsg * bm;
		for (bm=f->battles;bm;bm=bm->next) {
			if (!rplane(bm->r)) fprintf(F, "BATTLE %d %d\n", region_x(bm->r, f), region_y(bm->r, f));
			else {
				if (rplane(bm->r)->flags & PFL_NOCOORDS) fprintf(F, "BATTLESPEC %d %d\n", encode_region(f, bm->r), rplane(bm->r)->id);
				else fprintf(F, "BATTLE %d %d %d\n", region_x(bm->r, f), region_y(bm->r, f), rplane(bm->r)->id);
			}
			cr_output_messages(F, bm->msgs, f);
		}
	}

	cr_find_address(F, f, addresses);
	a = a_find(f->attribs, &at_reportspell);
	while (a) {
		cr_reportspell(F, (spellid_t)a->data.i, f->locale);
		a = a->nexttype;
	}
	for (a=a_find(f->attribs, &at_showitem);a;a=a->nexttype) {
		const potion_type * ptype = resource2potion(((const item_type*)a->data.v)->rtype);
		requirement * m;
		const char * ch;
		if (ptype==NULL) continue;
		m = ptype->itype->construction->materials;
		ch = resourcename(ptype->itype->rtype, 0);
		fprintf(F, "TRANK %d\n", hashstring(ch));
		fprintf(F, "\"%s\";Name\n", add_translation(ch, locale_string(f->locale, ch)));
		fprintf(F, "%d;Stufe\n", ptype->level);
		fprintf(F, "\"%s\";Beschr\n", ptype->text);
		fprintf(F, "ZUTATEN\n");

		while (m->number) {
			ch = resourcename(oldresourcetype[m->type], 0);
			fprintf(F, "\"%s\"\n", add_translation(ch, locale_string(f->locale, ch)));
			m++;
		}
	}

	/* traverse all regions */
	while (sd!=NULL) {
		int modifier = 0;
		const char * tname;
		unsigned char seemode = sd->mode;
		const region * r = sd->r;
		sd = sd->next;

		if (!rplane(r)) fprintf(F, "REGION %d %d\n", region_x(r, f), region_y(r, f));
		else {
#if ENCODE_SPECIAL
			if (rplane(r)->flags & PFL_NOCOORDS) fprintf(F, "SPEZIALREGION %d %d\n", encode_region(f, r), rplane(r)->id);
#else
			if (rplane(r)->flags & PFL_NOCOORDS) continue;
#endif
			else fprintf(F, "REGION %d %d %d\n", region_x(r, f), region_y(r, f), rplane(r)->id);
		}
		if (r->land && strlen(rname(r, f->locale))) fprintf(F, "\"%s\";Name\n", rname(r, f->locale));
		if (is_cursed(r->attribs,C_MAELSTROM, 0))
			tname = "maelstrom";
		else {
			if (r_isforest(r)) tname = "forest";
			else tname = terrain[rterrain(r)].name;
		}

		fprintf(F, "\"%s\";Terrain\n", add_translation(tname, locale_string(f->locale, tname)));
		switch (seemode) {
#ifdef SEE_FAR
		case see_far:
			fputs("\"neighbourhood\";visibility\n", F);
			break;
#endif
		case see_lighthouse:
			fputs("\"lighthouse\";visibility\n", F);
			break;
		case see_travel:
			fputs("\"travel\";visibility\n", F);
			break;
		}
		if (seemode != see_neighbour)
		{
#define RESOURCECOMPAT
			char cbuf[8192], *pos = cbuf;
			int g = 0;
			direction_t d;
#ifdef RESOURCECOMPAT
			if (r->display && strlen(r->display))
				fprintf(F, "\"%s\";Beschr\n", r->display);
#endif
			if (landregion(rterrain(r))) {
#if GROWING_TREES
				int trees = rtrees(r,2);
				int ytrees = rtrees(r,1);
# ifdef RESOURCECOMPAT
				if (trees > 0) fprintf(F, "%d;Baeume\n", trees);
				if (ytrees > 0) fprintf(F, "%d;Schoesslinge\n", ytrees);
				if (fval(r, RF_MALLORN) && (trees > 0 || ytrees > 0))
					fprintf(F, "1;Mallorn\n");
# endif
				if (!fval(r, RF_MALLORN)) {
					if (ytrees) pos = report_resource(pos, "rm_youngtrees", f->locale, ytrees, -1);
					if (trees) pos = report_resource(pos, "rm_trees", f->locale, trees, -1);
				} else {
					if (ytrees) pos = report_resource(pos, "rm_youngmallorn", f->locale, ytrees, -1);
					if (trees) pos = report_resource(pos, "rm_mallorn", f->locale, trees, -1);
				}
#else
				int trees = rtrees(r);
# ifdef RESOURCECOMPAT
				fprintf(F, "%d;Baeume\n", trees);
				if (fval(r, RF_MALLORN) && trees)
					fprintf(F, "1;Mallorn\n");
# endif
				if (!fval(r, RF_MALLORN)) {
					if (trees) pos = report_resource(pos, "rm_trees", f->locale, trees, -1);
				} else {
					if (trees) pos = report_resource(pos, "rm_mallorn", f->locale, trees, -1);
				}
#endif
				fprintf(F, "%d;Bauern\n", rpeasants(r));
				if(fval(r, RF_ORCIFIED)) {
					fprintf(F, "1;Verorkt\n");
				}
				fprintf(F, "%d;Pferde\n", rhorses(r));

				if (seemode>=see_unit) {
					struct demand * dmd = r->land->demands;
#if NEW_RESOURCEGROWTH
					struct rawmaterial * res = r->resources;
#endif
					fprintf(F, "%d;Silber\n", rmoney(r));
					fprintf(F, "%d;Unterh\n", entertainmoney(r));

					if (is_cursed(r->attribs, C_RIOT, 0)){
						fprintf(F, "0;Rekruten\n");
					} else {
						fprintf(F, "%d;Rekruten\n", rpeasants(r) / RECRUITFRACTION);
					}
					if (production(r)) {
						fprintf(F, "%d;Lohn\n", fwage(r, f, true));
					}

#if NEW_RESOURCEGROWTH
					while (res) {
						int maxskill = 0;
						int level = -1;
						int visible = -1;
						const item_type * itype = resource2item(res->type->rtype);
						if (res->type->visible==NULL) {
							visible = res->amount;
							level = res->level + itype->construction->minskill - 1;
						} else {
							const unit * u;
							for (u=r->units; visible!=res->amount && u!=NULL; u=u->next) {
								if (u->faction == f) {
									int s = eff_skill(u, itype->construction->skill, r);
									if (s>maxskill) {
										if (s>=itype->construction->minskill) {
											assert(itype->construction->minskill>0);
											level = res->level + itype->construction->minskill - 1;
										}
										maxskill = s;
										visible = res->type->visible(res, maxskill);
									}
								}
							}
						}
						if (level>=0) {
							pos = report_resource(pos, res->type->name, f->locale, visible, level);
# ifdef RESOURCECOMPAT
							if (visible>=0) fprintf(F, "%d;%s\n", visible, crtag(res->type->name));
#endif
						}
						res = res->next;
					}
#else
					const unit * u;
					int maxmining = 0;
					for (u = r->units; u; u = u->next) {
						if (u->faction == f) {
							int s = eff_skill(u, SK_MINING, r);
							maxmining = max(maxmining, s);
						}
					}
					for (u = r->units; u; u = u->next) {
						if (u->faction == f) {
							int s = eff_skill(u, SK_MINING, r);
							maxmining = max(maxmining, s);
						}
					}
					if (maxmining >= 4 && riron(r) > 0)
						fprintf(F, "%d;Eisen\n", riron(r));
					if (maxmining >= 7 && rlaen(r) > 0)
						fprintf(F, "%d;Laen\n", rlaen(r));
#endif
					/* trade */
					if(rpeasants(r)/TRADE_FRACTION > 0) {
						fputs("PREISE\n", F);
						while (dmd) {
							const char * ch = resourcename(dmd->type->itype->rtype, 0);
							fprintf(F, "%d;%s\n", (dmd->value
									  ? dmd->value*dmd->type->price
									  : -dmd->type->price),
									  add_translation(ch, locale_string(f->locale, ch)));
							dmd=dmd->next;
						}
					}
				}
				if (pos!=cbuf) fputs(cbuf, F);
			}
			for (d = 0; d != MAXDIRECTIONS; d++)
			{ /* Nachbarregionen, die gesehen werden, ermitteln */
				region * r2 = rconnect(r, d);
				border * b;
				if (!r2) continue;
				b  = get_borders(r, r2);
				while (b) {
					boolean cs = b->type->fvisible(b, f, r);

					if (!cs) {
						cs = b->type->rvisible(b, r);
						if (!cs) {
							unit * us = r->units;
							while (us && !cs) {
								if (us->faction==f) {
									cs = b->type->uvisible(b, us);
									if (cs) break;
								}
								us=us->next;
							}
						}
					}
					if (cs) {
						fprintf(F, "GRENZE %d\n", ++g);
						fprintf(F, "\"%s\";typ\n", b->type->name(b, r, f, GF_NONE));
						fprintf(F, "%d;richtung\n", d);
						if (!b->type->transparent(b, f)) fputs("1;opaque\n", F);
						/* pfusch: */
						if (b->type==&bt_road) {
							int p = rroad(r, d)*100/terrain[rterrain(r)].roadreq;
							fprintf(F, "%d;prozent\n", p);
						}
					}
					b = b->next;
				}
			}
			if (seemode==see_unit && r->planep && r->planep->id == 1 && !is_cursed(r->attribs, C_ASTRALBLOCK, 0))
			{
				/* Sonderbehandlung Teleport-Ebene */
				regionlist *rl = allinhab_in_range(r_astral_to_standard(r),TP_RADIUS);

				if (rl) {
					regionlist *rl2 = rl;
					while(rl2) {
						region * r = rl2->region;
						fprintf(F, "SCHEMEN %d %d\n", region_x(r, f), region_y(r, f));
						fprintf(F, "\"%s\";Name\n", rname(r, f->locale));
						rl2 = rl2->next;
						if(rl2) scat(", ");
					}
					free_regionlist(rl);
				}
			}

			print_curses(F, r, TYP_REGION, r->attribs, 0);
			/* describe both passed and inhabited regions */
			show_active_spells(r);
			{
				boolean seeunits = false, seeships = false;
				const attrib * ru;
				/* show units pulled through region */
				for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
					unit * u = (unit*)ru->data.v;
					if (cansee_durchgezogen(f, r, u, 0) && r!=u->region) {
						if (!u->ship || !fval(u, FL_OWNER)) continue;
						if (!seeships) fprintf(F, "DURCHSCHIFFUNG\n");
						seeships = true;
						fprintf(F, "\"%s\"\n", shipname(u->ship));
					}
				}
				for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
					unit * u = (unit*)ru->data.v;
					if (cansee_durchgezogen(f, r, u, 0) && r!=u->region) {
						if (u->ship) continue;
						if (!seeunits) fprintf(F, "DURCHREISE\n");
						seeunits = true;
						fprintf(F, "\"%s\"\n", unitname(u));
					}
				}
			}
			cr_output_messages(F, r->msgs, f);
			{
				message_list * mlist = r_getmessages(r, f);
				if (mlist) cr_output_messages(F, mlist, f);
			}
			/* buildings */
			for (b = rbuildings(r); b; b = b->next) {
				int fno = -1;
				u = buildingowner(r, b);
				if (u && !fval(u, FL_PARTEITARNUNG)) {
					const faction * sf = visible_faction(f,u);
					fno = sf->no;
				}
				cr_output_buildings(F, b, u, fno, f);
			}

			/* ships */
			for (sh = r->ships; sh; sh = sh->next) {
				int fno = -1;
				u = shipowner(r, sh);
				if (u && !fval(u, FL_PARTEITARNUNG)) {
					const faction * sf = visible_faction(f,u);
					fno = sf->no;
				}

				cr_output_ship(F, sh, u, fno, f, r);
			}

			/* visible units */
			for (u = r->units; u; u = u->next) {
				boolean visible = true;
				switch (seemode) {
				case see_unit:
					modifier=0;
					break;
#ifdef SEE_FAR
				case see_far:
#endif
				case see_lighthouse:
					modifier = -2;
					break;
				case see_travel:
					modifier = -1;
					break;
				default:
					visible=false;
				}
				if (u->building || u->ship || (visible && cansee(f, r, u, modifier)))
					cr_output_unit(F, r, f, u, seemode);
			}
		}			/* region traversal */
	}
	report_crtypes(F, f->locale);
	write_translations(F);
	reset_translations();
}
