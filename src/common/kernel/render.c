/* vi: set ts=2:
 *
 *	$Id: render.c,v 1.3 2001/01/30 23:16:17 enno Exp $
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
#include "render.h"

#include "item.h"
#include "faction.h"
#include "message.h"
#include "language.h"
#include "goodies.h"
#include "region.h"
#include "unit.h"
#include "building.h"
#include "ship.h"
#include "reports.h"
#include "karma.h"
#include "unit.h"


#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#define RMAXHASH 64

int lastint = 0;

typedef struct localizer {
	struct localizer * nexthash;
	int hashkey;
	const locale * lang;
	struct renderer * renderers[RMAXHASH];
	struct eval * evaluators[RMAXHASH];
} localizer;

typedef const char* (*eval_fun)(const locale * lang, void *);
typedef char * (*render_fun)(const message * m, const locale * lang);

typedef struct renderer {
	int hashkey;
	render_fun fun;
	char * name;
	struct renderer * nexthash;
} renderer;

typedef struct eval {
	int hashkey;
	eval_fun fun;
	const char * name;
	struct eval * nexthash;
} eval;

#define LMAXHASH 32
localizer * localizers[LMAXHASH];

extern int hashstring(const char * name);

static localizer *
get_localizer(const locale * lang)
{
	int hkey = locale_hashkey(lang);
	int id = hkey % LMAXHASH;
	localizer * find = localizers[id];
	while (find && find->lang!=lang) find = find->nexthash;
	if (!find) {
		find = calloc(sizeof(localizer), 1);
		find->nexthash = localizers[id];
		localizers[id] = find;
		find->hashkey = hkey;
		find->lang = lang;
	}
	return find;
}

void
add_renderfun(const char * name, localizer * l, render_fun fun)
{
	int hkey = hashstring(name);
	int id = hkey % RMAXHASH;
	renderer * find = l->renderers[id];
	while (find && find->hashkey!=hkey) find=find->nexthash;
	if (!find) {
		find = calloc(1, sizeof(renderer));
		find->nexthash = l->renderers[id];
		l->renderers[id] = find;
		find->hashkey = hkey;
		find->fun = fun;
		find->name = strdup(name);
	}
	else assert(!strcmp(find->name, name));
}

void
add_evalfun(const char * name, localizer * l, eval_fun fun)
{
	int hkey = hashstring(name);
	int id = hkey % RMAXHASH;
	eval * find = l->evaluators[id];
	while (find && find->hashkey!=hkey) find=find->nexthash;
	if (!find) {
		find = calloc(1, sizeof(eval));
		find->nexthash = l->evaluators[id];
		l->evaluators[id] = find;
		find->hashkey = hkey;
		find->fun = fun;
		find->name = strdup(name);
	}
	else assert(!strcmp(find->name, name));
}

char rbuf[8192];

void
read_strings(FILE * F)
{
	while (fgets(rbuf, 8192, F)) {
		char * b = rbuf;
		locale * lang;
		char * key = b;
		char * language;

		if (rbuf[0]=='#') continue;
		rbuf[strlen(rbuf)-1] = 0; /* \n weg */
		while (*b && *b!=';') ++b;
		if (!*b) continue;
		*b++ = 0;
		language = b;
		while (*b && *b!=';') ++b;
		*b++ = 0;
		lang = find_locale(language);
		if (!lang) lang = make_locale(language);
		locale_setstring(lang, key, b);
	}
}

void
render_cleanup(void)
{
	int i;
	for (i=0;i!=LMAXHASH;++i) {
		while (localizers[i]) {
			int s;
			localizer * l = localizers[i]->nexthash;
#if 0
			for (s=0;s!=SMAXHASH;++s) {
				while (localizers[i]->strings[s]) {
					struct locale_string * ls = localizers[i]->strings[s]->nexthash;
					free(localizers[i]->strings[s]);
					localizers[i]->strings[s]=ls;
				}
			}
#endif
			for (s=0;s!=RMAXHASH;++s) {
				while (localizers[i]->evaluators[s]) {
					eval * e = localizers[i]->evaluators[s]->nexthash;
					free(localizers[i]->evaluators[s]);
					localizers[i]->evaluators[s] = e;
				}
			}

			for (s=0;s!=RMAXHASH;++s) {
				while (localizers[i]->renderers[s]) {
					renderer * e = localizers[i]->renderers[s]->nexthash;
					free(localizers[i]->renderers[s]);
					localizers[i]->renderers[s] = e;
				}
			}
			free(localizers[i]);
			localizers[i] = l;
		}
	}
}

static const char *
v_unit(const locale * l, void * data) {
	unit * u = (void*)data;
	if (u) return unitname(u);
	return locale_string(l, "unknownunit");
}

static const char *
v_of_de(const locale * l, void * data) {
	int i = (int)data;
	if (!i || i==INT_MAX || i==lastint) return "";
	sprintf(buf, "von %d ", i);
	return buf;
}

static const char *
v_sink_de(const locale * l, void * data) {
	int i = (int)data;
	unused(l);
	return i?" und sank":"";
}

static const char *
v_mage_de(const locale * l, void * data) {
	int i = (int)data;
	unused(l);
	/* TODO: i==1 getarnt, wilder Hack */
	return i>1?unitname((unit*)i):"Jemand";
}

static const char *
v_unknown(const locale * l, void * data) {
	unused(l);
	unused(data);
	/* TODO: i==1 getarnt, wilder Hack */
	return "[an error occured]";
}

const char *
v_dir(const locale * l, void * data) {
	int i = (int)data;
	static const char* dres[6] = { "northwest", "northeast", "east",
		"southeast", "southwest", "west" };
	/* static char buffer[10]; */
	return locale_string(l, dres[i]);
}

static char *
render_immediate(const message * m, const char * find, localizer * l)
{
	char * b = rbuf;
	const char * p = find;
	while (*p) {
		while (*p && *p!='{') *b++ = *p++;
		if (*p=='{') {
			static char token[128];
			static char function[128];
			char * t = token;
			char * f = function;
			eval_fun fun = NULL;
			const char * var = NULL;
			int i = 0;
			struct entry * e = m->type->entries;
			++p;
			if (*p=='$') {
				int key;
				eval * e;
				++p;
				while (*p!=' ') *f++ = *p++;
				*f = 0;
				key = hashstring(function);
				for (e = l->evaluators[key % RMAXHASH];e;e=e->nexthash) {
					if (!strcmp(e->name, function)) break;
				}
				if (e==NULL) {
					/* in default gucken */
					localizer * l = get_localizer(find_locale("de"));
					for (e = l->evaluators[key % RMAXHASH];e;e=e->nexthash) {
						if (!strcmp(e->name, function)) break;
					}
					if (!e) {
						fun = v_unknown;
						fprintf(stderr, "WARNING: unknown function for rendering %s\n", function);
					}
					else fun = e->fun;
				}
				else fun = e->fun;
				++p;
			}
			while (*p!='}') {
				*t++ = *p++;
			}
			*t = 0;
			++p;
			while (e && strcmp(e->name, token)) {
				e = e->next;
				++i;
			}
			if (fun) var = fun(l->lang, m->data[i]);
			else if (e) switch (e->type) {
			case IT_FACTION:
				var = factionname((faction*)m->data[i]);
				break;
			case IT_UNIT:
				/* TODO: Grammatik falsch. Groß/Kleinschreibung */
				if (m->data[i] == (void *)&u_peasants) {
					if (b == rbuf) {
						var = "Die Bevölkerung";
					} else {
						var = "die Bevölkerung";
					}
				} else if(m->data[i] == NULL) {
					if (b == rbuf) {
						var = "Eine unsichtbare Einheit";
					} else {
						var = "eine unsichtbare Einheit";
					}
				} else {
					var = unitname((unit*)m->data[i]);
				}
				break;
			case IT_REGION:
				if (m->data[i]) var = tregionid((region*)m->data[i], m->receiver);
				else var="eine unbekannte Region";
				break;
			case IT_SHIP:
				var = shipname((ship*)m->data[i]);
				break;
			case IT_BUILDING:
				var = buildingname((building*)m->data[i]);
				break;
			case IT_RESOURCETYPE:
				var = locale_string(l->lang, resourcename((const resource_type *)m->data[i], (lastint==1)?0:GR_PLURAL));
				break;
			case IT_RESOURCE:
				var = resname((resource_t)m->data[i], lastint!=1);
				break;
			case IT_SKILL:
				var = skillnames[(int)m->data[i]];
				break;
			case IT_INT:
				sprintf(token, "%d", (int)m->data[i]);
				var = token;
				lastint = (int)m->data[i];
				break;
			case IT_STRING:
				var = (char*)m->data[i];
				break;
			case IT_DIRECTION:
				var = directions[(int)m->data[i]];
				break;
			case IT_FSPECIAL:
				var = fspecials[(int)m->data[i]].name;
				break;
			} else {
				assert(!"unknown token");
			}
			while (*var) *b++ = *var++;
		}
	}
	*b = 0;
	return rbuf;
}

void
rendercr(FILE * f, messagetype * mt, const locale * lang)
{
	fprintf(f, "\"%s\";%d\n", locale_string(lang, mt->name), mt->hashkey);
}

static char *
render_default(const message * m, const locale * lang)
{
	const char * str = locale_string(lang, m->type->name);
	if (!str) {
		fprintf(stderr, "WARNUNG: fehlende message %s\n", m->type->name);
		return m->type->name;
	}
	else return render_immediate(m, str, get_localizer(lang));
}

static render_fun
get_renderfun(const char * name, const localizer * l)
{
	int hkey = hashstring(name);
	int id = hkey % RMAXHASH;
	renderer * find = l->renderers[id];
	while (find && find->hashkey!=hkey) find=find->nexthash;
	if (find && !strcmp(find->name, name)) return find->fun;
	return &render_default;
}

const char *
render(const message * m, const locale * lang)
{
	localizer * loc = get_localizer(lang);
	char * s;
	render_fun f;
	f = get_renderfun(m->type->name, loc);
	s = f(m, lang);
	return s;
}

static char *
render_income(const message * m, const locale * lang)
{
	static const char * special[] = {"income_work","income_entertainment","income_tax","income_trade","income_tradetax", "income_steal", "income_magic"};
	static const char * special2[] = {"income_work_reduced","income_entertainment_reduced","income_tax_reduced","income_trade","income_tradetax", "income_steal_reduced", "income_magic_reduced"};
	int want=0, have=0, type = -1;
	const char * key;
	localizer * l = get_localizer(lang);
	struct entry * e = m->type->entries;
	const char * ls;
	int i = 0;
	if (!l) return render_default(m, lang);
	while (e) {
		switch(e->type) {
		case IT_INT:
			if (!strcmp(e->name, "amount")) have = (int)m->data[i];
			else if (!strcmp(e->name, "wanted")) want = (int)m->data[i];
			else if (!strcmp(e->name, "mode")) type = (int)m->data[i];
			else assert(0);
			break;
		case IT_REGION:
		case IT_UNIT:
			/* part of the message, but safely ignored here */
			break;
		default:
			assert(0);
		}
		++i;
		e = e->next;
	}
	if (type<0) return render_default(m, lang);
	if (want!=have) key = special2[type];
	else key = special[type];
	ls = locale_string(lang, key);
	return render_immediate(m, ls, l);
}

static char *
de_render_casualties(const message * m, const locale * lang)
{
/*
 * casualties;de;{unit} verlor {fallen} Personen, {alive} überlebten und {run} flohen nach {runto}.
 */
	int i, fallen=0, alive=0, run=0;
	region * runto = NULL;
	unit * u = NULL;
	char * sbuf;
	struct entry * e = m->type->entries;
	for(i=0;e;e=e->next, ++i) {
		switch(e->type) {
		case IT_INT:
			if (!strcmp(e->name, "fallen")) fallen = (int)m->data[i];
			else if (!strcmp(e->name, "run")) run = (int)m->data[i];
			else if (!strcmp(e->name, "alive")) alive = (int)m->data[i];
			else assert(0);
			break;
		case IT_REGION:
			if (!strcmp(e->name, "runto")) runto = (region*)m->data[i];
			else assert(0);
			break;
		case IT_UNIT:
			if (!strcmp(e->name, "unit")) u = (unit*)m->data[i];
			else assert(0);
			break;
		default:
			assert(!"message format of 'casualties' is broken");
		}
	}
	sbuf = rbuf;
	if (fallen) {
		sbuf = strcpy(sbuf, unitname(u)) + strlen(sbuf);
		sbuf += sprintf(sbuf, " verlor %d Person%s", fallen, fallen==1?"":"en");
		if (alive) sbuf += sprintf(sbuf, ", %d überlebt%s", alive, alive==1?"e":"en");
		if (run) sbuf += sprintf(sbuf, " und %d floh%s nach %s", run, run==1?"":"en", tregionid(runto, m->receiver));
	} else {
		if (alive && run) {
			sprintf(sbuf, "%d Person%s aus %s überlebt%s und %d floh%s nach %s",
				alive, alive==1?"":"en",
				unitname(u), alive==1?"e":"en",
				run, run==1?"":"en",
				tregionid(runto, m->receiver));
		} else if (run) {
			sprintf(sbuf, "%d Person%s aus %s floh%s nach %s",
			run, run==1?"":"en", unitname(u), run==1?"":"en",
			tregionid(runto, m->receiver));
		}
	}
	strcat(sbuf, ".");
	return rbuf;
}

static const char *
v_travel(const locale * l, void * data) 
{
	int i = (int)data;
	unused(l);
	switch(i) {
	case 0: return "flieht";
	case 1: return "reitet";
	default: return "wandert";
	}
}

static const char *
v_travelthru_de(const locale * l, void * data) {
	char * c = (char*)data;
	unused(l);
	if (c && strlen(c)) sprintf(buf," Dabei wurde %s durchquert.", c);
	else return "";
	return buf;
}

void
render_init(void)
{
	localizer * loc;
	locale * lang = find_locale("de");

	if (lang==NULL) lang = make_locale("de");
	loc = get_localizer(lang);
	add_renderfun("income", loc, render_income);
	add_renderfun("casualties", loc, de_render_casualties);
	add_evalfun("dir", loc, v_dir);
	add_evalfun("dir", loc, v_dir);
	add_evalfun("travel", loc, v_travel);
	add_evalfun("unit", loc, v_unit);
	add_evalfun("of", loc, v_of_de);
	add_evalfun("sink", loc, v_sink_de);
	add_evalfun("mage", loc, v_mage_de);
	add_evalfun("travelthru", loc, v_travelthru_de);

	lang = find_locale("en");
	if (lang==NULL) lang = make_locale("en");
	loc = get_localizer(lang);
	add_renderfun("income", loc, render_income);
}

void
read_messages(FILE * F)
{
	while (fgets(buf, 8192, F)) {
		char * b = buf;
		char * name = b;
		char * language;
		static locale * lang;
		char * section = NULL;
		int level = 0;
		messagetype * mtype;

		buf[strlen(buf)-1] = 0; /* \n weg */
		if (buf[0]=='#' || buf[0]==0) continue;
		while (*b && *b!=';') ++b;
		if (!*b) continue;
		*b++ = 0;
		section = b;
		while (*b && *b!=';' && *b!=':') ++b;
		if (!strcmp(section, "none")) section=NULL;
		if (*b==':') {
			char * x;
			*b++ = 0;
			x = b;
			while (*b && *b!=';') ++b;
			*b++ = 0;
			level=atoi(x);
		} else {
			level = 0;
			*b++ = 0;
		}
		language = b;
		while (*b && *b!=';') ++b;
		*b++ = 0;
		lang = find_locale(language);
		if (!lang) lang = make_locale(language);
		mtype = find_messagetype(name);
		if (!mtype) mtype = new_messagetype(name, level, section);
		else if (section && !mtype->section) {
			mtype->section = mc_add(section);
			mtype->level = level;
		}
		locale_setstring(lang, name, b);
	}
}

#include "skill.h"
#include "magic.h"
void
spy_message(int spy, unit *u, unit *target)
{
	const char *c;
	region * r = u->region;

	if (spy < 0) {
		sprintf(buf, "%s konnte nichts über ", unitname(u));
		scat(unitname(target));
		scat(" herausbekommen.");
		addmessage(r, u->faction, buf, MSG_EVENT, ML_WARN);
	} else if (spy == 0) {
		sprintf(buf, "%s gelang es, Informationen über ", unitname(u));
		scat(unitname(target));
		scat(" herausbekommen: Partei '");
		scat(factionname(target->faction));
		scat("', Talente: ");

		change_skill(u, SK_SPY, PRODUCEEXP / 2);

		{
			int first = 1;
			int found = 0;
			skill_t sk;

			for (sk = 0; sk != MAXSKILLS; sk++) {
				if (get_skill(target, sk)) {
					found++;
					if (first == 1) {
						first = 0;
					} else {
						scat(", ");
					}
					scat(skillnames[sk]);
				}
			}
			if (found == 0) {
				scat("Keine");
			}
		}
		scat(".");

		addmessage(0, u->faction, buf, MSG_EVENT, ML_IMPORTANT);

	/* Spionage > Wahrnehmung:
	 * Talente mit Werten, Gegenstände und Kampfstatus */
	} else if (spy > 0) {
		sprintf(buf, "%s gelang es, Informationen über ", unitname(u));
		scat(unitname(target));
		scat(" herauszubekommen: Partei '");
		scat(factionname(target->faction));
		scat("', Talente: ");

		change_skill(u, SK_SPY, PRODUCEEXP);

		{
			int first = 1;
			int found = 0;
			skill_t sk;

			for (sk = 0; sk != MAXSKILLS; sk++) {
				if (get_skill(target, sk)) {
					found++;
					if (first == 1) {
						first = 0;
					} else {
						scat(", ");
					}
					scat(skillnames[sk]);
					scat(" ");
					icat(eff_skill(target, sk, target->region));
				}
			}
			if (found == 0) {
				scat("Keine");
			}
		}
		scat("; Kampfstatus: ");
		scat(report_kampfstatus(target) + 2);
		c = hp_status(target);
		if (c && strlen(c))
			sprintf(buf, "%s (%s)", buf, c);
		scat("; ");

		icat(get_money(target));
		scat(" Silber;");

		scat(" Im Gepäck sind");
		{
			boolean first = true;
			int found = 0;
			item * itm;
			for (itm=target->items;itm;itm=itm->next) {
				if (itm->number>0) {
					resource_type * rtype = itm->type->rtype;
					++found;
					if (first) {
						first = false;
						scat(": ");
					} else {
						scat(", ");
					}

					if (itm->number == 1) {
						scat("1 ");
						scat(locale_string(u->faction->locale, resourcename(rtype, 0)));
					} else {
						icat(itm->number);
						scat(" ");
						scat(locale_string(u->faction->locale, resourcename(rtype, NMF_PLURAL)));
					}
				}
			}
			if (found == 0) {
				scat(" keine verborgenen Gegenstände");
			}
			scat(".");
		}

		/* magische Spionage:
		 * zusätzlich Magiegebiet und Zauber */
		if (spy > 1){
			if (eff_skill(target, SK_MAGIC, target->region) > 0){
				spell_ptr *spt;
				spell *sp;
				int first = 1;
				int found = 0;

				scat(" Magiegebiet: ");
				scat(magietypen[find_magetype(target)]);
				if (get_mage(target)) {
					scat(", Sprüche: ");

					for (spt = get_mage(target)->spellptr;spt; spt = spt->next){
						sp = find_spellbyid(spt->spellid);
						found++;
						if (first == 1){
							first = 0;
						} else {
							scat(", ");
						}
						scat(sp->name);
					}
					if (found == 0) {
						scat("Keine");
					}
				}
			}
		}
		addmessage(0, u->faction, buf, MSG_EVENT, ML_IMPORTANT);
	}
}
