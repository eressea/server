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

#define INDENT 0

#include <config.h>
#include <eressea.h>

/* modules includes */
#include <modules/score.h>
#ifdef ALLIANCES
#include <modules/alliance.h>
#endif

/* attributes includes */
#include <attributes/overrideroads.h>
#include <attributes/viewrange.h>
#include <attributes/otherfaction.h>
#ifdef AT_OPTION
# include <attributes/option.h>
#endif

/* gamecode includes */
#include "creport.h"
#include "economy.h"
#include "monster.h"
#include "laws.h"

/* kernel includes */
#include <alchemy.h>
#include <border.h>
#include <build.h>
#include <building.h>
#include <faction.h>
#include <group.h>
#include <item.h>
#include <karma.h>
#include <magic.h>
#include <message.h>
#include <movement.h>
#include <objtypes.h>
#include <plane.h>
#include <pool.h>
#include <race.h>
#include <region.h>
#include <render.h>
#include <reports.h>
#include <resources.h>
#include <save.h>
#include <ship.h>
#include <skill.h>
#include <teleport.h>
#include <unit.h>
#ifdef USE_UGROUPS
#  include <ugroup.h>
#endif

/* util includes */
#include <functions.h>
#include <goodies.h>
#include <base36.h>
#include <nrmessage.h>
#include <translation.h>
#include <util/message.h>
#include <log.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

extern int quiet;
extern int *storms;
extern int  weeks_per_month;
extern int  months_per_year;

boolean nocr = false;
boolean nonr = false;
boolean nomer = false;
boolean noreports = false;

char **seasonnames;
char **weeknames;
char **weeknames2;
char **monthnames;
int  *month_season;
char *agename;
int  seasons;

static const char*
MailitPath(void)
{
  static const char * value = NULL;
  if (value==NULL) {
    value = get_param(global.parameters, "report.mailit");
  }
  return value;

}

int
read_datenames(const char *filename)
{
	FILE *namesFP;
	char line[256];
	int  i, l;

	if( (namesFP=fopen(filename,"r")) == NULL) {
		log_error(("Kann Datei '%s' nicht öffnen, Abbruch\n", filename));
		return -1;
	}

	fgets(line,255,namesFP);
	l = strlen(line)-1;
	if(line[l] == '\n') line[l] = 0;
	agename = strdup(mkname("calendar", line));

	fgets(line,255,namesFP);
	seasons = strtol(line, NULL, 10);
	seasonnames = malloc(sizeof(char *) * seasons);

	for (i=0;i<seasons;i++) {
		fgets(line,255,namesFP);
		l = strlen(line)-1;
		if(line[l] == '\n') line[l] = 0;
		seasonnames[i] = strdup(mkname("calendar", line));
	}

	fgets(line,255,namesFP);
	weeks_per_month = strtol(line, NULL, 10);
	weeknames = malloc(sizeof(char *) * weeks_per_month);
	weeknames2 = malloc(sizeof(char *) * weeks_per_month);

	for(i=0;i<weeks_per_month;i++) {
		char * np;

		fgets(line,255,namesFP);
		l = strlen(line)-1;
		if(line[l] == '\n') line[l] = 0;

		np = strtok(line,":");
		weeknames[i] = strdup(mkname("calendar", np));
		np = strtok(NULL,":");
		weeknames2[i] = strdup(mkname("calendar", np));
	}

	fgets(line,255,namesFP);
	months_per_year  = strtol(line, NULL, 10);
	monthnames = malloc(sizeof(char *) * months_per_year);
	month_season = malloc(sizeof(int) * months_per_year);
	storms = malloc(sizeof(int) * months_per_year);

	for(i=0;i<months_per_year;i++) {
		char * np;
		fgets(line,255,namesFP);
		l = strlen(line)-1;
		if(line[l] == '\n') line[l] = 0;

		np = strtok(line,":");
		monthnames[i] = strdup(mkname("calendar", np));
		month_season[i] = atoi(strtok(NULL,":"));
		storms[i] = atoi(strtok(NULL,":"));
	}

	fclose(namesFP);
	return 0;
}

int
season(int turn)
{
	int year,month;
	int t = turn - FirstTurn();

	year  = t/(months_per_year * weeks_per_month) + 1;
	month = (t - (year-1) * months_per_year * weeks_per_month)/weeks_per_month;

	assert(month >= 0 && month < months_per_year);

	return month_season[month];
}

#if 0
static char *
gamedate(const struct locale * lang)
{
	int year,month,week,r;
	static char buf[256];
	int t = turn - FirstTurn();

	if (t<0) t = turn;
	assert(lang);

	year  = t/(months_per_year * weeks_per_month) + 1;
	r     = t - (year-1) * months_per_year * weeks_per_month;
	month = r/weeks_per_month;
	week  = r%weeks_per_month;
	sprintf(buf, LOC(lang, "nr_calendar"),
		LOC(lang, weeknames[week]),
		LOC(lang, monthnames[month]),
		LOC(lang, year),
		LOC(lang, agename));

	return buf;
}
#endif

static char *
gamedate_season(const struct locale * lang)
{
	int year,month,week,r;
	static char buf[256];
	int t = turn - FirstTurn();

	if (t<0) t = turn;
	assert(lang);

	year  = t/(months_per_year * weeks_per_month) + 1;
	r     = t - (year-1) * months_per_year * weeks_per_month;
	month = r/weeks_per_month;
	week  = r%weeks_per_month;
	sprintf(buf, LOC(lang, "nr_calendar_season"),
		LOC(lang, weeknames[week]),
		LOC(lang, monthnames[month]),
		year,
		LOC(lang, agename),
		LOC(lang, seasonnames[month_season[month]]));

	return buf;
}

static char *
gamedate2(const struct locale * lang)
{
	int year,month,week,r;
	static char buf[256];
	int t = turn - FirstTurn();

	if (t<0) t = turn;

	year  = t/(months_per_year * weeks_per_month) + 1;
	r     = t - (year-1) * months_per_year * weeks_per_month;
	month = r/weeks_per_month;			/* 0 - months_per_year-1 */
	week  = r%weeks_per_month;			/* 0 - weeks_per_month-1 */
	sprintf(buf, "in %s des Monats %s im Jahre %d %s.",
			LOC(lang, weeknames2[week]),
			LOC(lang, monthnames[month]),
			year,
			LOC(lang, agename));
	return buf;
}

static char *
gamedate_short(const struct locale * lang)
{
	int year,month,week,r;
	static char buf[256];
	int t = turn - FirstTurn();

	if (t<0) t = turn;

	year  = t/(months_per_year * weeks_per_month) + 1;
	r     = t - (year-1) * months_per_year * weeks_per_month;
	month = r/weeks_per_month;			/* 0 - months_per_year-1 */
	week  = r%weeks_per_month;			/* 0 - weeks_per_month-1 */

	sprintf(buf, "%d/%s/%d", week+1, LOC(lang, monthnames[month]), year);

	return buf;
}

static void
rpsnr(FILE * F, const char * s, int offset)
{
	char inset[REPORTWIDTH];
	const char *l, *x = s;
	char ui=0;
	size_t indent = 0, len;

	/* Geht nicht: Privatbeschreibungen der Monster enthalten
	 * \\r-Koordinaten ! */
	/* assert(!strstr(s, "\\r(") || !"error: regionid not translated"); */

	len = strlen(s);
	while (*x++ == ' ');
	indent = x - s - 1;
	if (*(x - 1) && indent && *x == ' ') indent += 2;
	if (!indent) indent = offset;
	x = s;
	memset(inset, 32, indent * sizeof(char));
	inset[indent] = 0;
	while (s <= x+len) {
		size_t line = min(len-(s-x), REPORTWIDTH - indent*ui);
		char   * br = strchr(s, '\n');
		l = s + line;

		if(br && br < l) {
			l = br;
		} else {
			if (*l) {
				while (*l!=' ' && l!=s) --l;
				/* wenn nicht gefunden, harte breaks: */
				if (s == l) l = s + REPORTWIDTH - indent;
			}
		}
		if (*s) {
#if INDENT
			putc(' ', F);
#endif
			if (s!=x) {
				fputs(inset, F);
			}
			fwrite(s, sizeof(char), l-s, F);
			putc('\n', F);
		}
		s = l+1; ui=1;
	}
}

static faction *current_faction = NULL;

int outi;
char outbuf[4096];

void
rpc(FILE * F, char c, size_t num)
{
	while(num > 0) {
		putc(c, F);
		num--;
	}
}

void
rnl(FILE * F)
{
	int i;
	int rc, vc;

	i = outi;
	assert(i < 4096);
	while (i && isspace((int)outbuf[i - 1]))
		i--;
	outbuf[i] = 0;

	i = 0;
	rc = 0;
	vc = 0;

	while (outbuf[i]) {
		switch (outbuf[i]) {
		case ' ':
			vc++;
			break;

		case '\t':
			vc = (vc & ~7) + 8;
			break;

		default:
			/* ER: Tabs in Reports sind Mist. Die versauen die *
			 * Anzeige von Einheiten in Burgen und Schiffen. while
			 * (rc / 8 != vc / 8) { if ((rc & 7) == 7) putc(' ',
			 * F); else putc('\t', F); rc = (rc & ~7) + 8; } */
			while (rc != vc) {
				putc(' ', F);
				rc++;
			}

			putc(outbuf[i], F);
			rc++;
			vc++;
		}

		i++;
		assert(i < 4096);
	}

	putc('\n', F);
	outi = 0;
}

static void
rps(FILE * F, const char * src)
{
	char * s;

	if (strstr(src, "\\r(")) {
		s = replace_global_coords(src, current_faction);
	} else if(src != buf) {
		s = strcpy(buf, src);
	} else {
		s = (char *)src;
	}
	rpsnr(F, s, 0);
}

static void
centre(FILE * F, const char *s, boolean breaking)
{
	/* Bei Namen die genau 80 Zeichen lang sind, kann es hier Probleme
	 * geben. Seltsamerweise wird i dann auf MAXINT oder aehnlich
	 * initialisiert. Deswegen keine Strings die laenger als REPORTWIDTH
	 * sind! */

	if (breaking && REPORTWIDTH < strlen(s)) {
		strlist *T, *SP = 0;
		sparagraph(&SP, s, 0, 0);
		T = SP;
		while (SP) {
			centre(F, SP->s, false);
			SP = SP->next;
		}
		freestrlist(T);
	} else {
		rpc(F, ' ', (REPORTWIDTH - strlen(s)+1) / 2);
		fputs(s, F);
		putc('\n', F);
	}
}

static void
rparagraph(FILE *F, const char *s, int indent, char mark)
{
	static char mbuf[BUFSIZE+1];
	/* static size_t bsize = 0; */
	size_t size;
	char inset[REPORTWIDTH];

	if (indent) {
		memset(inset, ' ', indent);
		inset[indent]=0;
		if (mark)
			inset[indent - 2] = mark;
	} else {
		assert(mark == 0);
		inset[0] = 0;
	}
	inset[indent]=0;
	size = strlen(s)+indent+1;
	if (size==1) return;
	strcpy(mbuf, inset);
	strncpy(mbuf+indent, s, BUFSIZE-indent);
	*(mbuf+size-1)=0;
	rps(F, mbuf);
}

static void
report_spell(FILE * F, spellid_t id, const struct locale * lang)
{
	int k, itemanz, costtyp;
  resource_t res;
	int dh = 0;
	spell *sp = find_spellbyid(id);

	rnl(F);
	centre(F, spell_name(sp, lang), true);
	rnl(F);
	strcpy(buf,"Beschreibung:");
	rps(F, buf);
	rparagraph(F, spell_info(sp, lang), 0, 0);
	rnl(F);


	strcpy(buf, "Art: ");
	if (sp->sptyp & PRECOMBATSPELL) {
		scat("Präkampfzauber");
	} else if (sp->sptyp & COMBATSPELL) {
		scat("Kampfzauber");
	} else if (sp->sptyp & POSTCOMBATSPELL) {
		scat("Postkampfzauber");
	} else {
		scat("Normaler Zauber");
	}
	rps(F, buf);

	strcpy(buf, "Komponenten:");
	rps(F, buf);
	for (k = 0; k < MAXINGREDIENT; k++) {
		res = sp->komponenten[k][0];
		itemanz = sp->komponenten[k][1];
		costtyp = sp->komponenten[k][2];
		if(itemanz > 0){
			sprintf(buf, "  %d %s", itemanz, LOC(lang, resname(res, itemanz!=1)));
			if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR )
				scat(" * Stufe");
			rps(F, buf);
		}
	}

	strcpy(buf, "Modifikationen: ");
	if (sp->sptyp & FARCASTING) {
		scat("Fernzauber");
		dh = 1;
	}
	if (sp->sptyp & OCEANCASTABLE) {
		if (dh == 1){
			scat(", ");
		}
		scat("Seezauber");
		dh = 1;
	}
	if (sp->sptyp & ONSHIPCAST) {
		if (dh == 1){
			scat(", ");
		}
		scat("Schiffszauber");
		dh = 1;
	}
	if (sp->sptyp & NOTFAMILIARCAST) {
		if (dh == 1){
			scat(", k");
		} else {
			scat("K");
		}
		scat("ann nicht vom Vertrauten gezaubert werden");
		dh = 1;
	}
	if(dh == 0) scat("Keine");

	rps(F, buf);

	sprintf(buf, "Stufe: %d", sp->level);
	rps(F, buf);

	sprintf(buf, "Rang: %d", sp->rank);
	rps(F, buf);

	rnl(F);
	strcpy(buf, "Syntax: ");
	rps(F, buf);
	if(!sp->syntax){
		if(sp->sptyp & ISCOMBATSPELL){
			strcpy(buf, "KAMPFZAUBER ");
		}else{
			strcpy(buf, "ZAUBERE ");
		}
		/* Reihenfolge beachten: Erst REGION, dann STUFE! */
		if (sp->sptyp & FARCASTING) {
			scat("[REGION x y] ");
		}
		if (sp->sptyp & SPELLLEVEL) {
			scat("[STUFE n] ");
		}
		scat("\"");
		scat(spell_name(sp, lang));
		scat("\" ");
		if (sp->sptyp & ONETARGET){
			if (sp->sptyp & UNITSPELL) {
				scat("<Einheit-Nr>");
			} else if (sp->sptyp & SHIPSPELL) {
				scat("<Schiff-Nr>");
			} else if (sp->sptyp & BUILDINGSPELL) {
				scat("<Gebäude-Nr>");
			}
		}else {
			if (sp->sptyp & UNITSPELL) {
				scat("<Einheit-Nr> [<Einheit-Nr> ...]");
			} else if (sp->sptyp & SHIPSPELL) {
				scat("<Schiff-Nr> [<Schiff-Nr> ...]");
			} else if (sp->sptyp & BUILDINGSPELL) {
				scat("<Gebäude-Nr> [<Gebäude-Nr> ...]");
			}
		}
	}else{
		strcpy(buf, sp->syntax);
	}
	rps(F, buf);

	rnl(F);
}

void
nmr_warnings(void)

{
	faction *f,*fa;
#define FRIEND (HELP_GUARD|HELP_MONEY)
	for(f=factions;f;f=f->next) {
		if(f->no != MONSTER_FACTION && (turn-f->lastorders) >= 2) {
			for(fa=factions;fa;fa=fa->next) {
				if (alliedfaction(NULL, f, fa, FRIEND) && alliedfaction(NULL, fa, f, FRIEND)) {
					sprintf(buf, "Achtung: %s hat einige Zeit keine "
						"Züge eingeschickt und könnte dadurch in Kürze aus dem "
						"Spiel ausscheiden.", factionname(f));
					addmessage(0, fa, buf, MSG_EVENT, ML_WARN);
				}
			}
		}
	}
}

void
sparagraph(strlist ** SP, const char *s, int indent, char mark)
{

	/* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
	 * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
	 * Vgl. spunit (). */

	int i, j, width;
	int firstline;
	static char buf[REPORTWIDTH + 1];

	width = REPORTWIDTH - indent;
	firstline = 1;

	for (;;) {
		i = 0;

		do {
			j = i;
			while (s[j] && s[j] != ' ')
				j++;
			if (j > width) {

				/* j zeigt auf das ende der aktuellen zeile, i zeigt auf den anfang der
				 * nächsten zeile. existiert ein wort am anfang der zeile, welches
				 * länger als eine zeile ist, muß dieses hier abgetrennt werden. */

				if (i == 0)
					i = width - 1;
				break;
			}
			i = j + 1;
		}
		while (s[j]);

		for (j = 0; j != indent; j++)
			buf[j] = ' ';

		if (firstline && mark)
			buf[indent - 2] = mark;

		for (j = 0; j != i - 1; j++)
			buf[indent + j] = s[j];
		buf[indent + j] = 0;

		addstrlist(SP, buf);

		if (s[i - 1] == 0)
			break;

		s += i;
		firstline = 0;
	}
}

int
hat_in_region(item_t it, region * r, faction * f)
{
	unit *u;

	for (u = r->units; u; u = u->next) {
		if (u->faction == f && get_item(u, it) > 0) {
			return 1;
		}
	}
	return 0;
}

static void
print_curses(FILE *F, const faction *viewer, const void * obj, typ_t typ, int indent)
{
	attrib *a = NULL;
	int self = 0;
	region *r;

	/* Die Sichtbarkeit eines Zaubers und die Zaubermeldung sind bei
	 * Gebäuden und Schiffen je nach, ob man Besitzer ist, verschieden.
	 * Bei Einheiten sieht man Wirkungen auf eigene Einheiten immer.
	 * Spezialfälle (besonderes Talent, verursachender Magier usw. werde
	 * bei jedem curse gesondert behandelt. */
	if (typ == TYP_SHIP){
		ship * sh = (ship*)obj;
		unit * owner;
		a = sh->attribs;
		r = sh->region;
		if((owner = shipowner(r,sh)) != NULL){
			if (owner->faction == viewer){
				self = 2;
			} else { /* steht eine person der Partei auf dem Schiff? */
				unit *u = NULL;
				for (u = r->units; u; u = u->next) {
					if (u->ship == sh) {
						self = 1;
						break;
					}
				}
			}
		}
	} else if (typ == TYP_BUILDING) {
		building * b = (building*)obj;
		unit * owner;
		a = b->attribs;
		r = b->region;
		if((owner = buildingowner(r,b)) != NULL){
			if (owner->faction == viewer){
				self = 2;
			} else { /* steht eine Person der Partei in der Burg? */
				unit *u = NULL;
				for (u = r->units; u; u = u->next) {
					if (u->building == b) {
						self = 1;
						break;
					}
				}
			}
		}
	} else if (typ == TYP_UNIT) {
		unit *u = (unit *)obj;
		a = u->attribs;
		r = u->region;
		if (u->faction == viewer){
			self = 2;
		}
	} else if (typ == TYP_REGION) {
		r = (region *)obj;
		a = r->attribs;
	} else {
		/* fehler */
	}

	for(;a;a=a->next) {
		int dh = 0;

		if (fval(a->type, ATF_CURSE)) {
			curse *c = (curse *)a->data.v;
			if (c->type->curseinfo) {
				if (c->type->cansee) {
					self = c->type->cansee(viewer, obj, typ, c, self);
				}
				dh = c->type->curseinfo(viewer->locale, obj, typ, c, self);
			}
			if (dh == 1) {
				rnl(F);
				rparagraph(F, buf, indent, 0);
			}
		} else if (a->type==&at_effect && self) {
			effect_data * data = (effect_data *)a->data.v;
			sprintf(buf, "Auf der Einheit lieg%s %d Wirkung%s %s.",
					(data->value==1 ? "t" : "en"),
					data->value,
					(data->value==1 ? "" : "en"),
					LOC(default_locale, resourcename(data->type->itype->rtype, 0)));
			rnl(F);
			rparagraph(F, buf, indent, 0);
		}
	}
}

char *
replace_global_coords(const char *s, const faction * f)
{
	return translate_regions(s, f);
}

static void
rps_nowrap(FILE * F, const char *s)
{
	const char *x = s;
	int indent = 0;

	x = s = replace_global_coords(s, current_faction);

	while (*x++ == ' ');
	indent = x - s - 1;
	if (*(x - 1) && indent && *x == ' ')
		indent += 2;
	x = s;
	while (*s) {
		if (s == x) {
			x = strchr(x + 1, ' ');
			if (!x)
				x = s + strlen(s);
		}
		rpc(F, *s++, 1);
	}
}

static void
rpunit(FILE * F, const faction * f, const unit * u, int indent, int mode)
{
	attrib *a_otherfaction;
	char marker;
	strlist *S;
	int dh;
	boolean isbattle = (boolean)(mode == see_battle);
#ifdef USE_UGROUPS
	ugroup *ug = findugroup(u);
#endif
	if(u->race == new_race[RC_SPELL]) return;

#ifdef USE_UGROUPS
	if(u->faction != f && (isbattle || ug)) {
		if(is_ugroupleader(u, ug)) {
			rnl(F);
			dh = bufunit_ugroupleader(f, u, indent, mode);
		} else {
			return;
		}
	} else
#endif
	{
		rnl(F);
		dh = bufunit(f, u, indent, mode);
	}

	a_otherfaction = a_find(u->attribs, &at_otherfaction);

	if(u->faction == f) {
		marker = '*';
	} else {
		if(a_otherfaction && f != u->faction && get_otherfaction(a_otherfaction) == f
				&& !fval(u, UFL_PARTEITARNUNG)) {
			marker = '!';
		} else {
			if(dh && !fval(u, UFL_PARTEITARNUNG)) {
				marker = '+';
			} else {
				marker = '-';
			}
		}
	}

	rparagraph(F, buf, indent, marker);

	if(!isbattle){
#ifdef USE_UGROUPS
		if(ug) {
			int i;
			for(i=0; i<ug->members; i++) {
				print_curses(F, f, ug->unit_array[i], TYP_UNIT, indent);
			}
		} else
#endif /* USE_UGROUPS */
		print_curses(F, f, u, TYP_UNIT, indent);
	}

#ifdef USE_UGROUPS
	if(ug) {
		int i;
		for(i=0; i<ug->members; i++) {
			if (mode==see_unit && ug->unit_array[i]->faction == f && ug->unit_array[i]->botschaften) {
				for (S =  ug->unit_array[i]->botschaften; S; S = S->next) {
					rnl(F);
					rparagraph(F, S->s, indent, 0);
				}
			}
		}
	} else
#endif
		if (mode==see_unit && u->faction == f && u->botschaften) {
			for (S = u->botschaften; S; S = S->next) {
				rnl(F);
				rparagraph(F, S->s, indent, 0);
			}
		}
}

static void
rp_messages(FILE * F, message_list * msgs, faction * viewer, int indent, boolean centered, boolean categorized)
{
	messageclass * category;
	if (!msgs) return;
	for (category=msgclasses; category; category=category->next) {
		int k = 0;
		struct mlist * m = msgs->begin;
		while (m) {
			boolean debug = viewer->options & want(O_DEBUG);
#ifdef MSG_LEVELS
			if (!debug && get_msglevel(viewer->warnings, viewer->msglevels, m->type) < m->level) continue;
#endif
			/* messagetype * mt = m->type; */
			if (strcmp(nr_section(m->msg), category->name)==0)
			{
				char lbuf[8192], *s = lbuf;
				nr_render(m->msg, viewer->locale, s, sizeof(lbuf), viewer);
				if (!k && categorized) {
					const char * name;
					char cat_identifier[24];

					sprintf(cat_identifier, "section_%s", category->name);
					name = LOC(viewer->locale, cat_identifier);
					if (debug) {
						if (name!=lbuf) strcpy(lbuf, name);
						sprintf(lbuf+strlen(name), " [%s]", cat_identifier);
						name = lbuf;
					}
					k = 1;
					rnl(F);
					if (centered) centre(F, name, true);
					else {
						if (indent>0) strcpy(lbuf, "          ");
						strcpy(lbuf+indent, name);
						rpsnr(F, lbuf, 2);
					}
					rnl(F);
				}
				if (indent>0) {
					strcpy(lbuf, "          ");
					strcpy(lbuf+indent, s);
					s = lbuf;
				}
#ifdef MSG_LEVELS
				if (debug) {
					int mylevel = get_msglevel(viewer->warnings, viewer->msglevels, m->type);
					int level = msg_level(m);
					if (s!=lbuf) strcpy(lbuf, s);
					sprintf(buf+strlen(s), " [%d:%d(%d)]", m->type->hashkey, level, mylevel);
					s = lbuf;
				}
#endif
				rpsnr(F, s, 2);
			}
			m=m->next;
		}
	}
}

static void
rp_battles(FILE * F, faction * f)
{
  if (f->battles!=NULL) {
    struct bmsg * bm;
    rnl(F);
    centre(F, LOC(f->locale, "section_battle"), false);
    rnl(F);
    for (bm=f->battles;bm;bm=bm->next) {
      RENDER(f, buf, 80, ("battle::header", "region", bm->r));
      rnl(F);
      centre(F, buf, true);
      rnl(F);
      rp_messages(F, bm->msgs, f, 0, true, false);
    }
  }
}

char *
f_regionid(const region * r, const faction * f)
{
	static int i = 0;
	static char bufs[4][NAMESIZE + 20];
	char * buf = bufs[(++i)%4];
	plane       *pl = NULL;

	if (!r)
		strcpy(buf, "(Chaos)");
	else {
		pl = getplane(r);
		if(pl && fval(pl,PFL_NOCOORDS)) {
			sprintf(buf, "%s", rname(r, f->locale));
		} else {
			sprintf(buf, "%s (%d,%d%s%s)", rname(r, f->locale), region_x(r,f), region_y(r,f), pl?",":"", pl?pl->name:"");
		}
	}

	return buf;
}

static void
prices(FILE * F, const region * r, const faction * f)
{
	const luxury_type *sale=NULL;
	struct demand * dmd;
	message * m;
	int n = 0;

	if (r->land==NULL || r->land->demands==NULL) return;
	for (dmd=r->land->demands;dmd;dmd=dmd->next) {
		if (dmd->value==0) sale = dmd->type;
		else if (dmd->value > 0) n++;
	}
	assert(sale!=NULL);

	m = msg_message("nr_market_sale", "product price",
		sale->itype->rtype, sale->price);
	nr_render(m, f->locale, buf, sizeof(buf), f);
	msg_release(m);

	if(n > 0) {
		scat(" ");
		scat(LOC(f->locale, "nr_trade_intro"));
		scat(" ");

		for (dmd=r->land->demands;dmd;dmd=dmd->next) if(dmd->value > 0) {
			char sbuf[80];
			m = msg_message("nr_market_price", "product price",
				dmd->type->itype->rtype, dmd->value * dmd->type->price);
			nr_render(m, f->locale, sbuf, sizeof(sbuf), f);
			msg_release(m);
			scat(sbuf);
			n--;
			if (n == 0) scat(LOC(f->locale, "nr_trade_end"));
			else if (n == 1) {
				scat(" ");
				scat(LOC(f->locale, "nr_trade_final"));
				scat(" ");
			} else {
				scat(LOC(f->locale, "nr_trade_next"));
				scat(" ");
			}
		}
	}
	/* Schreibe Paragraphen */
	rparagraph(F, buf, 0, 0);

}

boolean
see_border(const border * b, const faction * f, const region * r)
{
	boolean cs = b->type->fvisible(b, f, r);
	if (!cs) {
		cs = b->type->rvisible(b, r);
		if (!cs) {
			const unit * us = r->units;
			while (us && !cs) {
				if (us->faction==f) {
					cs = b->type->uvisible(b, us);
					if (cs) break;
				}
				us=us->next;
			}
		}
	}
	return cs;
}

const char *
trailinto(const region * r, const struct locale * lang)
{
	char ref[32];
	const char * s;
	if (r) {
		terrain_t t = r->terrain;
		if (is_cursed(r->attribs, C_MAELSTROM, 0)) {
			/* das kostet. evtl. wäre ein FL_CURSED gut? */
			s = locale_string(lang, "maelstrom_trail");
		}
		else if (terrain[t].trailname==NULL) {
			strcat(strcpy(ref, terrain[t].name), "_trail");
			s = locale_string(lang, ref);
		}
		else s = locale_string(lang, terrain[t].trailname(r));
		if (s && *s) {
			if (strstr(s, "%s"))	return s;
		}
	}
	return "%s";
}

static void
eval_trail(struct opstack ** stack, const void * userdata) /* (int, int) -> int */
{
	const struct faction * f = (const struct faction *)userdata;
	const struct locale * lang = opop(stack, const struct locale*);
	const struct region * r = opop(stack, const struct region*);
	const char * trail = trailinto(r, lang);
	const char * rn = regionname(r, f);
	char * x = balloc(strlen(trail)+strlen(rn));
	sprintf(x, trail, rn);
	opush(stack, x);
}

static void
describe(FILE * F, const region * r, int partial, faction * f)
{
	char dbuf[512];
	int n;
	boolean dh;
	direction_t d;
	int trees;
#if GROWING_TREES
	int ytrees;
#endif
	attrib *a;
	const char *tname;
	struct edge {
		struct edge * next;
		char * name;
		boolean transparent;
		boolean block;
		boolean exist[MAXDIRECTIONS];
		direction_t lastd;
	} * edges = NULL, * e;
	boolean see[MAXDIRECTIONS];

	for (d = 0; d != MAXDIRECTIONS; d++)
	{ /* Nachbarregionen, die gesehen werden, ermitteln */
		region *r2 = rconnect(r, d);
		border *b;
		see[d] = true;
		if (!r2) continue;
		for (b=get_borders(r, r2);b;) {
			struct edge * e = edges;
			boolean transparent = b->type->transparent(b, f);
			const char * name = b->type->name(b, r, f, GF_DETAILED|GF_ARTICLE);

			if (!transparent) see[d] = false;
			if (!see_border(b, f, r)) {
				b = b->next;
				continue;
			}
			while (e && (e->transparent != transparent || strcmp(name,e->name))) e = e->next;
			if (!e) {
				e = calloc(sizeof(struct edge), 1);
				e->name = strdup(name);
				e->transparent = transparent;
				e->next = edges;
				edges = e;
			}
			e->lastd=d;
			e->exist[d] = true;
			b = b->next;
		}
	}

	strcpy(buf, f_regionid(r, f));

	if (partial == 1) {
		scat(" (durchgereist)");
	}
	else if (partial == 3) {
		scat(" (benachbart)");
	}
	else if (partial == 2) {
		scat(" (vom Turm erblickt)");
	}
	/* Terrain */

	scat(", ");
	if(is_cursed(r->attribs,C_MAELSTROM, 0))
		tname = "maelstrom";
	else {
		if (r_isforest(r)) tname = "forest";
		else tname = terrain[rterrain(r)].name;
	}
	scat(LOC(f->locale, tname));

	/* Bäume */

#if GROWING_TREES
	trees  = rtrees(r,2);
	ytrees = rtrees(r,1);
	if (production(r)) {
		if(trees > 0 || ytrees > 0) {
			scat(", ");
			icat(trees);
			scat("/");
			icat(ytrees);
			scat(" ");
			if (fval(r, RF_MALLORN)) {
				if (trees == 1)
					scat(LOC(f->locale, "nr_mallorntree"));
				else
					scat(LOC(f->locale, "nr_mallorntree_p"));
			}
			else if (trees == 1)
				scat(LOC(f->locale, "nr_tree"));
			else
				scat(LOC(f->locale, "nr_tree_p"));
		}
	}
#else
	trees = rtrees(r);
	if (production(r) && trees > 0) {
		scat(", ");
		icat(trees);
		scat(" ");
		if (fval(r, RF_MALLORN)) {
			if (trees == 1)
				scat(LOC(f->locale, "nr_mallorntree"));
			else
				scat(LOC(f->locale, "nr_mallorntree_p"));
		}
		else if (trees == 1)
			scat(LOC(f->locale, "nr_tree"));
		else
			scat(LOC(f->locale, "nr_tree_p"));
	}
#endif

	/* iron & stone */
#if NEW_RESOURCEGROWTH
	if (partial == 0 && f != (faction *) NULL) {
		struct rawmaterial * res;
		for (res=r->resources;res;res=res->next) {
			int level = -1;
			int visible = -1;
			int maxskill = 0;
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
								level = res->level + itype->construction->minskill - 1;
							}
							maxskill = s;
							visible = res->type->visible(res, maxskill);
						}
					}
				}
			}
			if (level>=0 && visible >= 0) {
				char * pos = buf+strlen(buf);
				snprintf(pos, sizeof(buf)-(pos-buf), ", %d %s/%d",
					visible, LOC(f->locale, res->type->name), res->level + itype->construction->minskill - 1);
			}
		}
#else
	if (partial == 0 && f != (faction *) NULL) {
		int maxmining = 0;
		const unit * u;
		for (u = r->units; u; u = u->next) {
			if (u->faction == f) {
				int s = eff_skill(u, SK_MINING, r);
				maxmining = max(maxmining, s);
			}
		}
		if (riron(r) > 0 && maxmining >= 4) {
			scat(", ");
			icat(riron(r));
			scat(" Eisen");
		}
		if (rlaen(r)>=0 && maxmining >= 7) {
			scat(", ");
			icat(rlaen(r));
			scat(" Laen");
		}
#endif
	}

	/* peasants & silver */
	if (rpeasants(r)) {
		scat(", ");
		icat(rpeasants(r));

		if(fval(r, RF_ORCIFIED)) {
			scat(" ");
			scat(LOC(f->locale, rpeasants(r)==1?"rc_orc":"rc_orc_p"));
		} else {
			scat(" ");
			scat(LOC(f->locale, resourcename(oldresourcetype[R_PEASANTS], rpeasants(r)!=1)));
		}

		if (rmoney(r) && partial == 0) {
			scat(", ");
			icat(rmoney(r));
			scat(" ");
			scat(LOC(f->locale, resourcename(oldresourcetype[R_SILVER], rmoney(r)!=1)));
		}
	}
	/* Pferde */

	if (rhorses(r)) {
		scat(", ");
		icat(rhorses(r));
		scat(" ");
#ifdef NEW_ITEMS
		scat(LOC(f->locale, resourcename(oldresourcetype[R_HORSE], (rhorses(r)>1)?GR_PLURAL:0)));
#else
		scat(itemdata[I_HORSE].name[rhorses(r) > 1]);
#endif
	}
	scat(".");

	if (r->display[0]) {
		scat(" ");
		scat(r->display);

		n = r->display[strlen(r->display) - 1];
		if (n != '!' && n != '?' && n != '.')
			scat(".");
	}

  {
    const unit * u = region_owner(r);
    if (u) {
      scat(" Die Region ist im Besitz von ");
      scat(factionname(u->faction));
      scat(".");
    }
  }

  if (!is_cursed(r->attribs, C_REGCONF, 0)) {
		attrib *a_do = a_find(r->attribs, &at_overrideroads);
		if(a_do) {
			scat(" ");
			scat((char *)a_do->data.v);
		} else {
			int nrd = 0;

			/* Nachbarregionen, die gesehen werden, ermitteln */
			for (d = 0; d != MAXDIRECTIONS; d++)
				if (see[d] && rconnect(r, d)) nrd++;

			/* Richtungen aufzählen */

			dh = false;
			for (d = 0; d != MAXDIRECTIONS; d++) if (see[d]) {
				region * r2 = rconnect(r, d);
				if(!r2) continue;
				nrd--;
				if (dh) {
					if (nrd == 0) {
						scat(" ");
						scat(LOC(f->locale, "nr_nb_final"));
					} else {
						scat(LOC(f->locale, "nr_nb_next"));
					}
					scat(LOC(f->locale, directions[d]));
					scat(" ");
					sprintf(dbuf, trailinto(r2, f->locale),
							f_regionid(r2, f));
					scat(dbuf);
				}
				else {
					scat(" ");
					MSG(("nr_vicinitystart", "dir region", d, r2), dbuf, sizeof(dbuf), f->locale, f);
					scat(dbuf);
					dh = true;
				}
			}
			/* Spezielle Richtungen */
			for (a = a_find(r->attribs, &at_direction);a;a = a->nexttype) {
				spec_direction * d = (spec_direction *)(a->data.v);
				scat(" ");
				scat(d->desc);
				scat(" (\"");
				scat(d->keyword);
				scat("\")");
				scat(".");
				dh = 1;
			}
			if (dh) scat(".");
		}
		rnl(F);
		rparagraph(F, buf, 0, 0);
	} else {
		scat(" Große Verwirrung befällt alle Reisenden in dieser Region.");
		rnl(F);
		rparagraph(F, buf, 0, 0);
	}

	if (partial==0 && r->planep && r->planep->id == 1 &&
			!is_cursed(r->attribs, C_ASTRALBLOCK, 0))	{
		/* Sonderbehandlung Teleport-Ebene */
		region_list *rl = allinhab_in_range(r_astral_to_standard(r), TP_RADIUS);
		region_list *rl2;

		if (rl) {
			strcpy(buf, "Schemen der Regionen ");
			rl2 = rl;
			while(rl2) {
				scat(f_regionid(rl2->data, f));
				rl2 = rl2->next;
				if(rl2) scat(", ");
			}
			scat(" sind erkennbar.");
			free_regionlist(rl);
			/* Schreibe Paragraphen */
			rnl(F);
			rparagraph(F, buf, 0, 0);
		}
	}

	n = 0;

	/* Wirkungen permanenter Sprüche */
	print_curses(F, f, r, TYP_REGION,0);

	/* Produktionsreduktion */
	a = a_find(r->attribs, &at_reduceproduction);
	if(a) {
		sprintf(buf, "Die Region ist verwüstet, der Boden karg.");
		rparagraph(F, buf, 0, 0);
	}

	if (edges) rnl(F);
	for (e=edges;e;e=e->next) {
		boolean first = true;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			if (!e->exist[d]) continue;
			if (first) strcpy(buf, "Im ");
			else {
				if (e->lastd==d) strcat(buf, " und im ");
				else strcat(buf, ", im ");
			}
			strcat(buf, LOC(f->locale, directions[d]));
			first = false;
		}
		if (!e->transparent) strcat(buf, " versperrt ");
		else strcat(buf, " befindet sich ");
		strcat(buf, e->name);
		if (!e->transparent) strcat(buf, " die Sicht.");
		else strcat(buf, ".");
		rparagraph(F, buf, 0, 0);
	}
	if (edges) {
		while (edges) {
			e = edges->next;
			free(edges->name);
			free(edges);
			edges = e;
		}
	}
}

static void
statistics(FILE * F, const region * r, const faction * f)
{
	const unit *u;
	int number, p;
	item *itm, *items = NULL;
	p = rpeasants(r);
	number = 0;

	/* zählen */
	for (u = r->units; u; u = u->next)
		if (u->faction == f && u->race != new_race[RC_SPELL]) {
			for (itm=u->items;itm;itm=itm->next) {
				i_change(&items, itm->type, itm->number);
			}
			number += u->number;
		}

	/* Ausgabe */
	rnl(F);
	sprintf(buf, "Statistik für %s:", f_regionid(r, f));
	rps(F, buf);
	rnl(F);

	/* Region */
	if (landregion(rterrain(r)) && rmoney(r)) {
		sprintf(buf, "Unterhaltung: max. %d Silber", entertainmoney(r));
		rps(F, buf);
	}
	if (production(r) && (!rterrain(r) == T_OCEAN || f->race == new_race[RC_AQUARIAN])) {
		sprintf(buf, "Lohn für Arbeit: %d Silber", fwage(r, f, true));
		rps(F, buf);
	}
	if (p) {
		sprintf(buf, "Rekrutieren: max. %d Bauern",
				p / RECRUITFRACTION);
		rps(F, buf);

		if (!TradeDisabled()) {
			if (buildingtype_exists(r, bt_find("caravan"))) {
				sprintf(buf, "Luxusgüter zum angegebenen Preis: %d",
					(p * 2) / TRADE_FRACTION);
			} else {
				sprintf(buf, "Luxusgüter zum angegebenen Preis: %d",
					p / TRADE_FRACTION);
			}
			rps(F, buf);
		}
	}
	/* Info über Einheiten */

	sprintf(buf, "Personen: %d", number);
	rps(F, buf);
	for (itm = items; itm; itm=itm->next) {
		sprintf(buf, "%s: %d", LOC(f->locale, resourcename(itm->type->rtype, GR_PLURAL)), itm->number);
		rps(F, buf);
	}
	while (items) i_free(i_remove(&items, items));
}

static void
durchreisende(FILE * F, const region * r, const faction * f)
{
	attrib *ru;
	int wieviele;
	int counter;

	wieviele = counter = 0;

	/* Wieviele sind aufzulisten? Für die Grammatik. */

	for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
		unit * u = (unit*)ru->data.v;
		if (cansee_durchgezogen(f, r, u, 0) > 0 && r!=u->region) {
			if (u->ship && !fval(u, UFL_OWNER))
				continue;
			wieviele++;
		}
	}

	if (!wieviele)
		return;

	/* Auflisten. */

	buf[0] = 0;
	rnl(F);

	for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
		unit * u = (unit*)ru->data.v;
		if (cansee_durchgezogen(f, r, u, 0) > 0 && r!=u->region) {
			if (u->ship && !fval(u, UFL_OWNER))
				continue;
			counter++;
			if (u->ship != (ship *) NULL) {
				if (counter == 1) {
					scat("Die ");
				} else {
					scat("die ");
				}
				scat(shipname(u->ship));
			} else {
				scat(unitname(u));
			}
			if (counter + 1 < wieviele) {
				scat(", ");
			} else if (counter + 1 == wieviele) {
				scat(" und ");
			}
		}
	}

	if (wieviele == 1) {
		scat(" hat die Region durchquert.");
		rparagraph(F, buf, 0, 0);
	} else {
		scat(" haben die Region durchquert.");
		rparagraph(F, buf, 0, 0);
	}
}

static void
order_template(FILE * F, faction * f)
{
	strlist *S2;
	region *r;
	plane *pl;
	unit *u;
	int dh;
	region *last = f->last?f->last:lastregion(f);

  if (quiet) {
    printf(" ZV");
    fflush(stdout);
  }
  else
    printf(" - Schreibe Zugvorlage\n");
	rps_nowrap(F, "");
	rnl(F);
	rps_nowrap(F, LOC(f->locale, "nr_template"));
	rnl(F);
	rps_nowrap(F, "");
	rnl(F);

	sprintf(buf, "%s %s \"%s\"", LOC(f->locale, "ERESSEA"), factionid(f), LOC(f->locale, "enterpasswd"));
	rps_nowrap(F, buf);
	rnl(F);

	dh = (f->options & Pow(O_SILBERPOOL));

	rps_nowrap(F, "");
	rnl(F);
	sprintf(buf, "; ECHECK %s-w4 -r%d -v%s", dh ? "-l " : "",
			f->race->recruitcost, ECHECK_VERSION);
					/* -v3.4: ECheck Version 3.4.x */
	rps_nowrap(F, buf);
	rnl(F);

	for (r = f->first?f->first:firstregion(f); r != last; r = r->next) {
		dh = 0;
		for (u = r->units; u; u = u->next)
			if (u->faction == f && u->race != new_race[RC_SPELL]) {
				if (!dh) {
					rps_nowrap(F, "");
					rnl(F);
					pl = getplane(r);
					if (pl && fval(pl, PFL_NOCOORDS)) {
						sprintf(buf, "%s; %s", LOC(f->locale, parameters[P_REGION]), rname(r, f->locale));
					} else if (pl && pl->id != 0) {
						sprintf(buf, "%s %d,%d,%d ; %s", LOC(f->locale, parameters[P_REGION]), region_x(r,f),
								region_y(r,f), pl->id, rname(r, f->locale));
					} else {
						sprintf(buf, "%s %d,%d ; %s", LOC(f->locale, parameters[P_REGION]), region_x(r,f),
								region_y(r,f), rname(r, f->locale));
					}
					rps_nowrap(F, buf);
					rnl(F);
					sprintf(buf,"; ECheck Lohn %d", fwage(r,f,true));
					rps_nowrap(F, buf);
					rnl(F);
					rps_nowrap(F, "");
					rnl(F);
				}
				dh = 1;

				sprintf(buf, "%s %s;		%s [%d,%d$", LOC(u->faction->locale, parameters[P_UNIT]),
						unitid(u), u->name, u->number, get_money(u));
				if (u->building != NULL && fval(u, UFL_OWNER)) {
					building * b = u->building;
					int cost = buildingmaintenance(b, r_silver);

					if (cost > 0) {
						scat(",U");
						icat(cost);
					}
#if TODO
					if (buildingdaten[u->building->typ].spezial != 0) {
						scat("+");
					}
#endif
				} else if (u->ship) {
					if (fval(u, UFL_OWNER))
						scat(",S");
					else
						scat(",s");
					scat(shipid(u->ship));
				}
				if (lifestyle(u) == 0)
					scat(",I");
				scat("]");

				rps_nowrap(F, buf);
				rnl(F);

				for (S2 = u->orders; S2; S2 = S2->next) {
					if(is_persistent(S2->s, u->faction->locale)) {
						sprintf(buf, "   %s", S2->s);
						rps_nowrap(F, buf);
						rnl(F);
					}
				}

				/* If the lastorder begins with an @ it should have
				 * been printed in the loop before. */
				if(u->lastorder[0] != 0 && u->lastorder[0] != '@') {
					sprintf(buf, "   %s", u->lastorder);
					rps_nowrap(F, buf);
					rnl(F);
				}
			}
	}
	rps_nowrap(F, "");
	rnl(F);
	sprintf(buf, LOC(f->locale, parameters[P_NEXT]));
	rps_nowrap(F, buf);
	rnl(F);
}

static void
show_allies(const faction * f, const ally * allies)
{
	int allierte = 0;
	int i=0, h, hh = 0;
	int dh = 0;
	const ally * sf;
	for (sf = allies; sf; sf = sf->next) {
		int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
#ifdef ALLIANCES
		if (f->alliance!=sf->faction->alliance) continue;
#endif
		if (mode > 0) ++allierte;
	}

	for (sf = allies; sf; sf = sf->next) {
		int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
#ifdef ALLIANCES
		if (f->alliance!=sf->faction->alliance) continue;
#endif
		if (mode <= 0) continue;
		i++;
		if (dh) {
			if (i == allierte)
				scat(" und ");
			else
				scat(", ");
		}
		dh = 1;
		hh = 0;
		scat(factionname(sf->faction));
		scat(" (");
		if (mode == HELP_ALL) {
			scat("Alles");
		} else
			for (h = 1; h < HELP_ALL; h *= 2) {
				if ((mode & h) == h)
					switch (h) {
					case HELP_TRAVEL:
						scat("Durchreise");
						hh = 1;
						break;
					case HELP_MONEY:
						scat("Silber");
						hh = 1;
						break;
					case HELP_FIGHT:
						if (hh)
							scat(", ");
						scat("Kämpfe");
						hh = 1;
						break;
					case HELP_OBSERVE:
						if (hh)
							scat(", ");
						scat("Wahrnehmung");
						hh = 1;
						break;
					case HELP_GIVE:
						if (hh)
							scat(", ");
						scat("Gib");
						hh = 1;
						break;
					case HELP_GUARD:
						if (hh)
							scat(", ");
						scat("Bewache");
						hh = 1;
						break;
					case HELP_FSTEALTH:
						if (hh)
							scat(", ");
						scat("Parteitarnung");
						hh = 1;
						break;
					}
			}
		scat(")");
	}
}

static void
allies(FILE * F, const faction * f)
{
	const group * g = f->groups;
	if (f->allies) {
		if (!f->allies->next) {
			strcpy(buf, "Wir helfen der Partei ");
		} else {
			strcpy(buf, "Wir helfen den Parteien ");
		}
		show_allies(f, f->allies);
		scat(".");
		rparagraph(F, buf, 0, 0);
		rnl(F);
	}

	while (g) {
		if (g->allies) {
			if (!g->allies->next) {
				sprintf(buf, "%s hilft der Partei ", g->name);
			} else {
				sprintf(buf, "%s hilft den Parteien ", g->name);
			}
			show_allies(f, g->allies);
			scat(".");
			rparagraph(F, buf, 0, 0);
			rnl(F);
		}
		g = g->next;
	}
}

#ifdef REGIONOWNERS
static void
enemies(FILE * F, const faction * f)
{
  faction_list * flist = f->enemies;
  if (flist!=NULL) {
    strcpy(buf, "Wir liegen im Krieg mit ");
    for (;flist!=NULL;flist = flist->next) {
      const faction * enemy = flist->data;
      scat(factionname(enemy));
      if (flist->next) {
        scat(", ");
      } else {
        scat(".");
      }
    }
		rparagraph(F, buf, 0, 0);
		rnl(F);
  }
}
#endif

static void
guards(FILE * F, const region * r, const faction * see)
{				/* die Partei  see  sieht dies; wegen
                                * "unbekannte Partei", wenn man es selbst ist... */

  faction* guardians[512];

  int nextguard = 0;

  unit *u;
  int i;

  boolean tarned = false;
  /* Bewachung */

  for (u = r->units; u; u = u->next) {
    if (getguard(u)) {
      faction *f  = u->faction;
      faction *fv = visible_faction(see, u);

      if(fv != f && see != fv) {
        f = fv;
      }

      if (f != see && fval(u, UFL_PARTEITARNUNG)) {
        tarned=true;
      } else {
        for (i=0;i!=nextguard;++i) if (guardians[i]==f) break;
        if (i==nextguard) {
          guardians[nextguard++] = f;
        }
      }
    }
  }

  if (nextguard || tarned) {
    strcpy(buf, "Die Region wird von ");
  } else {
    return;
  }

  for (i = 0; i!=nextguard+(tarned?1:0); ++i) {
    if (i!=0) {
      if (i == nextguard-(tarned?0:1))
        scat(" und ");
      else
        scat(", ");
    }
    if (i<nextguard) scat(factionname(guardians[i]));

    else scat("unbekannten Einheiten");
  }
  scat(" bewacht.");
  rnl(F);
  rparagraph(F, buf, 0, 0);
}

static void
rpline(FILE * F)
{
	rpc(F, ' ', 1);
	rpc(F, '-', REPORTWIDTH);
	rnl(F);
}

static void
list_address(FILE * F, const faction * uf, const faction_list * seenfactions)
{
	const faction_list *flist = seenfactions;

	centre(F, LOC(uf->locale, "nr_addresses"), false);
	rnl(F);

	while (flist!=NULL) {
		const faction * f = flist->data;
		if (f->no!=MONSTER_FACTION) {
			sprintf(buf, "%s: %s; %s", factionname(f), f->email, f->banner);
			rparagraph(F, buf, 4, (char)(ALLIED(uf, f)?'+':'*'));
#ifdef SHORTPWDS
      if (f->shortpwds) {
        shortpwd * spwd = f->shortpwds;
        while (spwd) {
          if (spwd->used) {
            sprintf(buf, "Vertretung: %s", spwd->email);
            rparagraph(F, buf, 6, '-');
          }
          spwd=spwd->next;
        }
      }
#endif

		}
		flist = flist->next;
	}
	rnl(F);
	rpline(F);
}

void
report_building(FILE *F, const region * r, const building * b, const faction * f, int mode)
{
	int i;
	unit *u;
	attrib * a = a_find(b->attribs, &at_icastle);
	const struct locale * lang = NULL;
	const building_type * type = b->type;

	if (f) lang = f->locale;

	if (a!=NULL) {
		type = ((icastle_data*)a->data.v)->type;
	} else {
		type = b->type;
	}

	sprintf(buf, "%s, %s %d, %s", buildingname(b), LOC(f->locale, "nr_size"),
		b->size, LOC(lang, buildingtype(b, b->size)));
	if (b->size < type->maxsize) {
		scat(" (im Bau)");
	}

	if (b->besieged > 0 && mode>=see_lighthouse) {
		scat(", belagert von ");
		icat(b->besieged);
		scat(" Personen ");
		if (b->besieged >= b->size * SIEGEFACTOR) {
			scat("(abgeschnitten)");
		} else {
			scat("(unvollständig belagert)");
		}
	}
	i = 0;
	if (b->display[0]) {
		scat("; ");
		scat(b->display);
		i = b->display[strlen(b->display) - 1];
	}
	if (i != '!' && i != '?' && i != '.')
		scat(".");

	rparagraph(F, buf, 2, 0);

	if (mode<see_lighthouse) return;

	print_curses(F, f, b, TYP_BUILDING, 4);

	for (u = r->units; u; u = u->next)
		if (u->building == b && fval(u, UFL_OWNER)) {
			rpunit(F, f, u, 6, mode);
			break;
		}
	for (u = r->units; u; u = u->next)
		if (u->building == b && !fval(u, UFL_OWNER))
			rpunit(F, f, u, 6, mode);
}

static void
report(FILE *F, faction * f, const faction_list * addresses,
	const char * pzTime)
{
#ifndef NEW_ITEMS
	potion_t potion;
#endif
	int flag = 0;
	char ch;
	int dh;
	int anyunits;
	const struct region *r;
  region * last = f->last?f->last:lastregion(f);
	building *b;
	ship *sh;
	unit *u;
	attrib *a;
	message * m;
	int wants_stats;
	int ix;
	unsigned char op;
	char buf2[80];
	ix = Pow(O_STATISTICS);
	wants_stats = (f->options & ix);

	if (quiet) {
		printf(" NR");
		fflush(stdout);
	}
	else
		printf(" - Schreibe Report\n");

	m = msg_message("nr_header_date", "game date", global.gamename, pzTime);
	nr_render(m, f->locale, buf, sizeof(buf), f);
	msg_release(m);
	centre(F, buf, true);

	centre(F, gamedate_season(f->locale), true);
	rnl(F);
	sprintf(buf, "%s, %s/%s (%s)", factionname(f),
			LOC(f->locale, rc_name(f->race, 1)), neue_gebiete[f->magiegebiet],
			f->email);
	centre(F, buf, true);
#ifdef ALLIANCES
	if (f->alliance!=NULL) {
		centre(F, alliancename(f->alliance), true);
	}
#endif

	buf[0] = 0;
	dh = 0;
	for(a=a_find(f->attribs, &at_faction_special); a; a=a->nexttype) {
		dh++;
		if(fspecials[a->data.sa[0]].maxlevel != 100) {
			sprintf(buf2, "%s (%d)", fspecials[a->data.sa[0]].name, a->data.sa[1]);
		} else {
			sprintf(buf2, "%s", fspecials[a->data.sa[0]].name);
		}
		if(dh > 1) strcat(buf, ", ");
		strcat(buf, buf2);
	}
	if(dh > 0) centre(F, buf, true);
	dh = 0;

	if(f->karma > 0) {
		sprintf(buf, "Deine Partei hat %d Karma.", f->karma);
		centre(F, buf, true);
	}

	if (f->age <= 2) {
		if (f->age <= 1) {
			ADDMSG(&f->msgs, msg_message("changepasswd",
				"value", gc_add(strdup(f->passw))));
		}
		RENDER(f, buf, sizeof(buf), ("newbie_password", "password", f->passw));
		rnl(F);
		centre(F, buf, true);
		rnl(F);
		centre(F, LOC(f->locale, "newbie_info_1"), true);
		rnl(F);
		centre(F, LOC(f->locale, "newbie_info_2"), true);
		if ((f->options & want(O_COMPUTER)) == 0) {
			f->options |= want(O_COMPUTER);
			rnl(F);
			centre(F, LOC(f->locale, "newbie_info_3"), true);
		}
	}
	rnl(F);
	if (f->options & want(O_SCORE) && f->age > DISPLAYSCORE) {
		RENDER(f, buf, sizeof(buf), ("nr_score", "score average", f->score, average_score_of_age(f->age, f->age / 24 + 1)));
		centre(F, buf, true);
	}
	m = msg_message("nr_population", "population units", count_all(f), f->no_units);
	nr_render(m, f->locale, buf, sizeof(buf), f);
	msg_release(m);
	centre(F, buf, true);
	if (f->race == new_race[RC_HUMAN]) {
		int maxmig = count_maxmigrants(f);
		sprintf(buf, "Deine Partei hat %d Migranten und kann maximal %d Migranten aufnehmen.",
			count_migrants(f), maxmig);
		centre(F, buf, true);
	}

	if (f->age > 1 && f->lastorders != turn) {
		rnl(F);
		if (turn - f->lastorders == 1) {
			centre(F, LOC(f->locale, "nr_nmr"), true);
		} else {
			sprintf(buf,
				"Deine Partei hat seit %d Runden keinen Zug abgegeben! Wenn du"
				" drei Runden nacheinander keinen Zug abgibst, wird sie"
				" automatisch gelöscht.",
				turn - f->lastorders);
			centre(F, buf, true);
		}
		rnl(F);
	}

	/* Insekten-Winter-Warnung */
	if(f->race == new_race[RC_INSECT]) {
		if(month_season[month(1)] == 0) {
			strcpy(buf, "Es ist Winter, und Insekten können nur in Wüsten oder mit "
				"Hilfe des Nestwärme-Tranks Personen rekrutieren.");
			centre(F, buf, true);
			rnl(F);
		} else if(month_season[month(2)] == 0) {
			strcpy(buf, "Es ist Spätherbst, und diese Woche ist die letzte vor dem "
				"Winter, in der Insekten rekrutieren können.");
			centre(F, buf, true);
			rnl(F);
		}
	}

	sprintf(buf, "%s:", LOC(f->locale, "nr_options"));
	for (op = 0; op != MAXOPTIONS; op++) {
		if (f->options & (int) pow(2, op)) {
			scat(" ");
			scat(LOC(f->locale, options[op]));
#ifdef AT_OPTION
			if(op == O_NEWS) {
				attrib *a = a_find(f->attribs, &at_option_news);
				if(!a) {
					/* Zur Altlastenbeseitigung */
					f->options = f->options & ~op;
				} else {
					int sec = a->data.i;
					int i;
					scat("(");
					for(i=1; sec != 0; i *= 2) {
						if(sec & i) {
							icat(i);
							sec = sec & ~i;
							if(sec) scat(",");
						}
					}
					scat(")");
				}
			}
#endif
			flag++;
		}
	}
	if (flag > 0) {
		rnl(F);
		centre(F, buf, true);
	}
	/* Der Report soll später einmal durch einen Makroprozessor laufen.
	 * (enno) was? wer hat das geschrieben?
	 * Momentan ist das wegen der der Mailverschickmimik schwierig. */
	rp_messages(F, f->msgs, f, 0, true, true);
	rp_battles(F, f);
	a = a_find(f->attribs, &at_reportspell);
	if (a) {
		rnl(F);
		centre(F, LOC(f->locale, "section_newspells"), true);
		while (a) {
			report_spell(F, (spellid_t)a->data.i, f->locale);
			a = a->nexttype;
		}
	}

	ch = 0;
	for (a=a_find(f->attribs, &at_showitem);a;a=a->nexttype) {
		const potion_type * ptype = resource2potion(((const item_type*)a->data.v)->rtype);
		const char * description = NULL;
		requirement * m;
    if (ptype!=NULL) {
      const char * pname = resourcename(ptype->itype->rtype, 0);
			const char * potiontext = mkname("potion", pname);
		  m = ptype->itype->construction->materials;
		  if (ch==0) {
			  rnl(F);
			  centre(F, LOC(f->locale, "section_newpotions"), true);
			  ch = 1;
		  }

		  rnl(F);
		  centre(F, LOC(f->locale, pname), true);
		  sprintf(buf, "%s %d", LOC(f->locale, "nr_level"), ptype->level);
		  centre(F, buf, true);
		  rnl(F);
		  sprintf(buf, "%s: ", LOC(f->locale, "nr_herbsrequired"));
		  while (m->number) {
			  scat(LOC(f->locale, resourcename(oldresourcetype[m->type], 0)));
			  ++m;
			  if (m->number) scat(", ");
		  }
		  centre(F, buf, true);
		  rnl(F);
		  description = LOC(f->locale, potiontext);
			if (strcmp(description, potiontext)==0) {
				/* string not found */
				description = ptype->text;
			}
		  centre(F, description, true);
    }
	}
	rnl(F);
	centre(F, LOC(f->locale, "nr_alliances"), false);
	rnl(F);

#ifdef REGIONOWNERS
  enemies(F, f);
#endif
	allies(F, f);

	rpline(F);

	anyunits = 0;

  for (r=f->first?f->first:firstregion(f);r!=last;r=r->next) {
		boolean unit_in_region = false;
		boolean durchgezogen_in_region = false;
		int turm_sieht_region = false;
    seen_region * sd = find_seen(r);
    if (sd==NULL) continue;

		switch (sd->mode) {
		case see_lighthouse:
			turm_sieht_region = true;
			break;
		case see_far:
			break;
		case see_travel:
			durchgezogen_in_region = true;
			break;
		case see_unit:
			unit_in_region = true;
			anyunits = true;
			break;
		default:
			continue;
		}
		r = sd->r;
		/* Beschreibung */

		if (unit_in_region) {
			describe(F, r, 0, f);
			if (!TradeDisabled() && rterrain(r) != T_OCEAN && rpeasants(r)/TRADE_FRACTION > 0) {
				rnl(F);
				prices(F, r, f);
			}
			guards(F, r, f);
			durchreisende(F, r, f);
		}
    else {
      if (sd->mode==see_far) {
			  describe(F, r, 3, f);
			  guards(F, r, f);
			  durchreisende(F, r, f);
		  }
		  else if (turm_sieht_region) {
			  describe(F, r, 2, f);
			  durchreisende(F, r, f);
		  } else {
			  describe(F, r, 1, f);
			  durchreisende(F, r, f);
		  }
    }
		/* Statistik */

		if (wants_stats && unit_in_region == 1)
			statistics(F, r, f);

		/* Nachrichten an REGION in der Region */

		if (unit_in_region || durchgezogen_in_region) {
			message_list * mlist = r_getmessages(r, f);
			rp_messages(F, r->msgs, f, 0, true, true);
			if (mlist) rp_messages(F, mlist, f, 0, true, true);
		}
		/* Burgen und ihre Einheiten */

		for (b = rbuildings(r); b; b = b->next) {
			rnl(F);
			report_building(F, r, b, f, sd->mode);
		}

		/* Restliche Einheiten */

		if (sd->mode>=see_lighthouse) {
			for (u = r->units; u; u = u->next) {
				if (!u->building && !u->ship) {
					if ((u->faction == f) ||
					    (unit_in_region && cansee(f, r, u, 0)) ||
					    (durchgezogen_in_region && cansee(f, r, u, -1)) ||
					    (sd->mode==see_far && cansee(f, r, u, -2)) ||
					    (turm_sieht_region && cansee(f, r, u, -2)))
					{
						if (dh == 0 && !(rbuildings(r) || r->ships)) {
							dh = 1;
							/* rnl(F); */
						}
						rpunit(F, f, u, 4, sd->mode);
					}
				}
			}
		}

		/* Schiffe und ihre Einheiten */

		for (sh = r->ships; sh; sh = sh->next) {
			int w = 0;
			faction *of = NULL;

			rnl(F);

			/* Gewicht feststellen */

			for (u = r->units; u; u = u->next)
				if (u->ship == sh && fval(u, UFL_OWNER)) {
					of = u->faction;
					break;
				}
			if (of == f) {
				for (u = r->units; u; u = u->next) {
					if (u->ship == sh) {
						w += weight(u);
					}
				}
				sprintf(buf, "%s, %s, (%d/%d)",
						shipname(sh), LOC(f->locale, sh->type->name[0]),
						(w + 99) / 100, shipcapacity(sh) / 100);
			} else {
				sprintf(buf, "%s, %s", shipname(sh), LOC(f->locale, sh->type->name[0]));
			}

			assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
			if (sh->size!=sh->type->construction->maxsize) {
				sprintf(buf+strlen(buf), ", %s (%d/%d)",
					LOC(f->locale, "nr_undercons"), sh->size,
					sh->type->construction->maxsize);
			}
			if (sh->damage) {
				sprintf(buf+strlen(buf), ", %d%% %s",
					sh->damage*100/(sh->size*DAMAGE_SCALE),
					LOC(f->locale, "nr_damaged"));
			}
			if (rterrain(r) != T_OCEAN) {
				if (sh->coast != NODIRECTION) {
					scat(", ");
					scat(LOC(f->locale, coasts[sh->coast]));
				}
			}
			ch = 0;
			if (sh->display[0]) {
				scat("; ");
				scat(sh->display);

				ch = sh->display[strlen(sh->display) - 1];
			}
			if (ch != '!' && ch != '?' && ch != '.')
				scat(".");

			rparagraph(F, buf, 2, 0);

			print_curses(F,f,sh,TYP_SHIP,4);

			for (u = r->units; u; u = u->next)
				if (u->ship == sh && fval(u, UFL_OWNER)) {
					rpunit(F, f, u, 6, sd->mode);
					break;
				}
			for (u = r->units; u; u = u->next)
				if (u->ship == sh && !fval(u, UFL_OWNER))
					rpunit(F, f, u, 6, sd->mode);
		}

		rnl(F);
		rpline(F);
	}
	if (f->no != MONSTER_FACTION) {
		if (!anyunits) {
			rnl(F);
			rparagraph(F, LOC(f->locale, "nr_youaredead"), 0, 0);
		} else {
			list_address(F, f, addresses);
		}
	}
}

FILE *
openbatch(void)
{
  faction *f;
  FILE * BAT = NULL;

  /* falls und mind. ein internet spieler gefunden wird, schreibe den
  * header des batch files. ab nun kann BAT verwendet werden, um zu
  * pruefen, ob netspieler vorhanden sind und ins mailit batch
  * geschrieben werden darf. */

  for (f = factions; f; f = f->next) {

    /* bei "internet:" verschicken wir die mail per batchfile. für
    * unix kann man alles in EIN batchfile schreiben, als
    * sogenanntes "herefile". Konnte der batchfile nicht geoeffnet
    * werden, schreiben wir die reports einzeln. der BAT file wird
    * nur gemacht, wenn es auch internet benutzer gibt. */

    if (f->email) {
      sprintf(buf, "%s/mailit", reportpath());
      if ((BAT = fopen(buf, "w")) == NULL)
        log_error(("mailit konnte nicht geöffnet werden!\n"));
      else
        fprintf(BAT,
        "#!/bin/sh\n\n"
        "# MAILIT shell file, vom Eressea Host generiert\n#\n"
        "# Verwendung: nohup mailit &\n#\n\n"
        "PATH=%s\n\n"
        "chmod 755 *.sh\n"
        "\n", MailitPath());
      break;

    }
  }
  return BAT;
}

void
closebatch(FILE * BAT)
{
  if (BAT) {
    fputs("\n", BAT);
    fclose(BAT);
  }
}


void
base36conversion(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			if (forbiddenid(u->no)) {
				uunhash(u);
				u->no = newunitid();
				uhash(u);
			}
		}
	}
}

extern void init_intervals(void);

#define DBG_CACHE 1
#if DBG_CACHE
int chits = 0;
int ctries = 0;
#endif
static void
view_default(region *r, faction *f)
{
	direction_t dir;
	for (dir=0;dir!=MAXDIRECTIONS;++dir) {
		region * r2 = rconnect(r, dir);
		if (r2) {
			border * b = get_borders(r, r2);
			while (b) {
				if (!b->type->transparent(b, f)) break;
				b = b->next;
			}
			if (!b) add_seen(r2, see_neighbour, false);
		}
	}
}

static void
view_neighbours(region * r, faction * f)
{
	direction_t dir;
	for (dir=0;dir!=MAXDIRECTIONS;++dir) {
		region * r2 = rconnect(r, dir);
		if (r2) {
			border * b = get_borders(r, r2);
			while (b) {
				if (!b->type->transparent(b, f)) break;
				b = b->next;
			}
			if (!b) {
				if (add_seen(r2, see_far, false)) {
					if (!(terrain[rterrain(r2)].flags & FORBIDDEN_LAND)) {
						direction_t dir;
						for (dir=0;dir!=MAXDIRECTIONS;++dir) {
							region * r3 = rconnect(r2, dir);
							if (r3) {
								border * b = get_borders(r2, r3);
								while (b) {
									if (!b->type->transparent(b, f)) break;
									b = b->next;
								}
								if (!b) add_seen(r3, see_neighbour, false);
							}
						}
					}
				}
			}
		}
	}
}

static void
recurse_regatta(region *center, region *r, faction *f, int maxdist)
{
	direction_t dir;
	int dist = distance(center, r);
	for (dir=0;dir!=MAXDIRECTIONS;++dir) {
		region * r2 = rconnect(r, dir);
		if (r2) {
			int ndist = distance(center, r2);
			if (ndist>dist && r2->terrain==T_OCEAN) {
				border * b = get_borders(r, r2);
				while (b) {
					if (!b->type->transparent(b, f)) break;
					b = b->next;
				}
				if (!b) {
					if (ndist<maxdist) {
						if (add_seen(r2, see_far, false)) {
							recurse_regatta(center, r2, f, maxdist);
						}
					} else add_seen(r2, see_neighbour, false);
				}
			}
		}
	}
}

static void
view_regatta(region * r, faction * f)
{
	unit *u;
	int skill = 0;
	for (u=r->units; u; u=u->next) {
		if (u->faction==f) {
			int es = effskill(u, SK_OBSERVATION);
			if (es>skill) skill=es;
		}
	}
	recurse_regatta(r, r, f, skill/2);
}

static void
prepare_report(faction * f)
{
  region * r;
  region * end = f->last?f->last:lastregion(f);
  seen_init();
  for (r = f->first?f->first:firstregion(f); r != end; r = r->next) {
		attrib *ru;
		unit * u;
		plane * p = rplane(r);
		unsigned char mode = see_none;
		boolean dis = false;
#if DBG_CACHE
		++ctries;
#endif
		if (p) {
			watcher * w = p->watchers;
			while (w) {
				if (f==w->faction) {
					mode = w->mode;
					break;
				}
				w = w->next;
			}
		}
		if (mode<see_unit) for (u = r->units; u; u = u->next) {
			if (u->faction == f) {
				if (u->race != new_race[RC_SPELL] || u->number == RS_FARVISION) {
					mode = see_unit;
					if (fval(u, UFL_DISBELIEVES)) {
						dis = true;
						break;
					}
				}
			}
		}

		if (mode<see_travel) for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
			unit * u = (unit*)ru->data.v;
			if (u->faction == f) {
				mode = see_travel;
				break;
			}
		}

		if (mode<see_lighthouse && rterrain(r) == T_OCEAN) {
			if (check_leuchtturm(r, f)) mode = see_lighthouse;
		}

		if (mode == see_none)
			continue;
#if DBG_CACHE
		else ++chits;
#endif

		add_seen(r, mode, dis);
		/* nicht, wenn Verwirrung herrscht: */
		if (!is_cursed(r->attribs, C_REGCONF, 0)) {
			void (*view)(region * r, faction * f) = view_default;
			if (p && fval(p, PFL_SEESPECIAL)) {
				attrib * a = a_find(p->attribs, &at_viewrange);
				if (a) view = (void (*)(region * r, faction * f))a->data.f;
			}
			view(r, f);
		}
	}
#if DBG_CACHE
	if (!quiet)
		printf(" - cache hits: %.2f%%\n", 100*chits/(double)ctries);
#endif
}

#define FMAXHASH 1023

struct fsee {
	struct fsee * nexthash;
	faction * f;
	struct see {
		struct see * next;
		faction * seen;
		unit * proof;
	} * see;
} * fsee[FMAXHASH];

void
reports(void)
{
	faction *f;
	boolean gotit;
	FILE *shfp, *F, *BAT;
	int wants_report, wants_computer_report,
		wants_compressed, wants_bzip2;
	time_t ltime = time(NULL);
	char pzTime[64];

#ifdef _GNU_SOURCE
	strftime(pzTime, 64, "%A, %-e. %B %Y, %-k:%M", localtime(&ltime));
#else
	strftime(pzTime, 64, "%A, %d. %B %Y, %H:%M", localtime(&ltime));
#endif

	nmr_warnings();
#ifdef DMALLOC
	assert(dmalloc_verify ( NULL ));
#endif
	makedir(reportpath(), 0700);

	if (global.data_version<BASE36_VERSION) base36conversion();
	/* öffnet file BAT (mailit batch file) */

	BAT = openbatch();

	wants_report = 1 << O_REPORT;
	wants_computer_report = 1 << O_COMPUTER;
	wants_compressed = 1 << O_COMPRESS;
	wants_bzip2 = 1 << O_BZIP2;
	printf("\n");

	report_donations();
	remove_empty_units();
  log_printf("Updating region intervals\n");
  update_intervals();
	log_printf("Report timestamp - %s\n", pzTime);
	for (f = factions; f; f = f->next) {
		faction_list * addresses;
		attrib * a = a_find(f->attribs, &at_reportspell);
		current_faction = f;
		gotit = false;

		if (quiet) {
			printf("Reports für %s: ", factionname(f));
			fflush(stdout);
		}
		else printf("%s\n", factionname(f));
		prepare_report(f);
		addresses = get_addresses(f);

    /* NR schreiben: */
		if (!nonr && (f->options & wants_report)) {
			sprintf(buf, "%s/%d-%s.nr", reportpath(), turn, factionid(f));
			F = cfopen(buf, "wt");
			if (F) {
				report(F, f, addresses, pzTime);
				fclose(F);
				gotit = true;
			}
		}
    /* CR schreiben: */
		if (!nocr && (f->options & wants_computer_report || f->age<3)) {
			sprintf(buf, "%s/%d-%s.cr", reportpath(), turn, factionid(f));
			F = cfopen(buf, "wt");
			if (F) {
				report_computer(F, f, addresses, ltime);
				fclose(F);
				gotit = true;
			}
		}
    /* ZV schreiben: */
    if (f->options & (1 << O_ZUGVORLAGE)) {
      sprintf(buf, "%s/%d-%s.txt", reportpath(), turn, factionid(f));
      F = cfopen(buf, "wt");
      if (F) {
        order_template(F, f);
        fclose(F);
      }
    }
		if (f->no > 0 && f->email && BAT) {
			sprintf(buf, "%s/%s.sh", reportpath(), factionid(f));
			shfp = fopen(buf, "w");
			fprintf(shfp,"#!/bin/sh\n\nPATH=%s\n\n",MailitPath());
			fprintf(shfp,"if [ $# -ge 1 ]; then\n");
			fprintf(shfp,"\taddr=$1\n");
			fprintf(shfp,"else\n");
			fprintf(shfp,"\taddr=%s\n", f->email);
			fprintf(shfp,"fi\n\n");

			fprintf(BAT, "\n\ndate;echo %s\n", f->email);

			if (f->no > 0 && f->options & wants_compressed) {

				if(f->age == 1) {
					fprintf(BAT, "ls %d-%s.nr %d-%s.txt %d-%s.cr | zip -m -j -9 -@ %d-%s.zip\n",
						turn, factionid(f), 
            turn, factionid(f), 
            turn, factionid(f), 
            turn, factionid(f));
					fprintf(BAT, "zip -j -9 %d-%s.zip ../res/%s/%s/welcome.txt\n", 
            turn, factionid(f), global.welcomepath, locale_name(f->locale));
				} else {
					fprintf(BAT, "ls %d-%s.nr %d-%s.txt %d-%s.cr | zip -m -j -9 -@ %d-%s.zip\n",
						turn, factionid(f), 
            turn, factionid(f), 
            turn, factionid(f), 
            turn, factionid(f));
				}

				fprintf(shfp, "eresseamail.zipped $addr \"%s %s\" \"%d-%s.zip\" "
				    "%d-%s.zip\n", global.gamename, gamedate_short(f->locale), 
            turn, factionid(f), 
            turn, factionid(f));

			} else if(f->options & wants_bzip2) {

				if (f->age == 1) {
					fprintf(shfp,
						" \\\n\t\"text/plain\" \"Willkommen\" ../res/%s/%s/welcome.txt", global.welcomepath, locale_name(f->locale));
				}

				fprintf(BAT, "bzip2 -9v `ls %d-%s.nr %d-%s.txt %d-%s.cr`\n",
					turn, factionid(f), 
          turn, factionid(f), 
          turn, factionid(f));

				fprintf(shfp, "eresseamail.bzip2 $addr \"%s %s\"", global.gamename, gamedate_short(f->locale));

				if (!nonr && f->options & wants_report)
					fprintf(shfp,
						" \\\n\t\"application/x-bzip2\" \"Report\" %d-%s.nr.bz2",
						turn,factionid(f));

        if (f->options & (1 << O_ZUGVORLAGE))
          fprintf(shfp,
          " \\\n\t\"application/x-bzip2\" \"Zugvorlage\" %d-%s.txt.bz2",
          turn,factionid(f));

				if (!nocr && (f->options & wants_computer_report || f->age<3))
					fprintf(shfp,
						" \\\n\t\"application/x-bzip2\" \"Computer-Report\" %d-%s.cr.bz2",
						turn, factionid(f));
			} else {

				fprintf(shfp, MAIL " $addr \"%s %s\"", global.gamename, gamedate_short(f->locale));

				if (f->age == 1) {
					fprintf(shfp,
						" \\\n\t\"text/plain\" \"Willkommen\" ../res/%s/%s/welcome.txt", global.welcomepath, locale_name(f->locale));
				}

        if (!nonr && f->options & wants_report)
          fprintf(shfp,
          " \\\n\t\"text/plain\" \"Report\" %d-%s.nr",
          turn, factionid(f));

        if (f->options & (1 << O_ZUGVORLAGE))
          fprintf(shfp,
          " \\\n\t\"text/plain\" \"Zugvorlage\" %d-%s.txt",
          turn, factionid(f));

				if (!nocr && (f->options & wants_computer_report || f->age<3))
					fprintf(shfp,
						" \\\n\t\"text/x-eressea-cr\" \"Computer-Report\" %d-%s.cr",
						turn, factionid(f));
			}

			fprintf(BAT, ". %s.sh %s\n", factionid(f), f->email);
			fclose(shfp);
		}

		if (!gotit)
			printf("* Fehler: Kein Report für Partei Nr. %s!\n",
			       factionid(f));
		while (a) {
			attrib * a_old = a;
			a = a->nexttype;
			a_remove(&f->attribs, a_old);
		}
		if (quiet)
			printf("\n");
		freelist(addresses);
	}
	/* schliesst BAT und verschickt Zeitungen und Kommentare */
	current_faction = NULL;
	seen_done();
	closebatch(BAT);
}

void
report_cleanup(void)
{
	int i;
	for (i=0;i!=FMAXHASH;++i) {
		while (fsee[i]) {
			struct fsee * fs = fsee[i]->nexthash;
			free(fsee[i]);
			fsee[i] = fs;
		}
	}
}

unit *
can_find(faction * f, faction * f2)
{
	int key = f->no % FMAXHASH;
	struct fsee * fs = fsee[key];
	struct see * ss;
	if (f==f2) return f->units;
	while (fs && fs->f!=f) fs=fs->nexthash;
	if (!fs) return NULL;
	ss=fs->see;
	while (ss && ss->seen!=f2) ss=ss->next;
	if (ss) {
		/* bei TARNE PARTEI yxz muss die Partei von unit proof nicht
		 * wirklich Partei f2 sein! */
		/* assert(ss->proof->faction==f2); */
		return ss->proof;
	}
	return NULL;
}

static void
add_find(faction * f, unit * u, faction *f2)
{
	/* faction f sees f2 through u */
	int key = f->no % FMAXHASH;
	struct fsee ** fp = &fsee[key];
	struct fsee * fs;
	struct see ** sp;
	struct see * ss;
	while (*fp && (*fp)->f!=f) fp=&(*fp)->nexthash;
	if (!*fp) {
		fs = *fp = calloc(sizeof(struct fsee), 1);
		fs->f = f;
	} else fs = *fp;
	sp = &fs->see;
	while (*sp && (*sp)->seen!=f2) sp=&(*sp)->next;
	if (!*sp) {
		ss = *sp = calloc(sizeof(struct see), 1);
		ss->proof = u;
		ss->seen = f2;
	} else ss = *sp;
	ss->proof = u;
}

static void
update_find(void)
{
	region * r;
	static boolean initial = true;

	if (initial) for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			faction * lastf = u->faction;
			unit * u2;
			for (u2=r->units;u2;u2=u2->next) {
				if (u2->faction==lastf || u2->faction==u->faction)
					continue;
				if (seefaction(u->faction, r, u2, 0)) {
					faction *fv = visible_faction(u->faction, u2);
					lastf = fv;
					add_find(u->faction, u2, fv);
				}
			}
		}
	}
	initial = false;
}

boolean
kann_finden(faction * f1, faction * f2)
{
	update_find();
	return (boolean)(can_find(f1, f2)!=NULL);
}

/******* summary *******/
int dropouts[2];
int * age = NULL;

typedef struct summary {
	int waffen;
	int factions;
	int artefakte;
	int ruestungen;
	int schiffe;
	int gebaeude;
	int maxskill;
	int inhabitedregions;
	int peasants;
	int nunits;
	int playerpop;
	double playermoney;
	double peasantmoney;
	int armed_men;
	int poprace[MAXRACES];
	int factionrace[MAXRACES];
	int landregionen;
	int regionen_mit_spielern;
	int landregionen_mit_spielern;
	int orkifizierte_regionen;
	int inactive_volcanos;
	int active_volcanos;
	int spielerpferde;
	int pferde;
	struct language {
		struct language * next;
		int number;
		const struct locale * locale;
	} * languages;
} summary;

summary *
make_summary(boolean count_new)
{
	faction *f;
	region *r;
	unit *u;
	summary * s = calloc(1, sizeof(summary));

	for (f = factions; f; f = f->next) {
		const struct locale * lang = f->locale;
		struct language * plang = s->languages;
		while (plang && plang->locale != lang) plang=plang->next;
		if (!plang) {
			plang = calloc(sizeof(struct language), 1);
			plang->next = s->languages;
			s->languages = plang;
			plang->locale = lang;
		}
		++plang->number;
		f->nregions = 0;
		f->number = 0;
		f->money = 0;
		if (f->alive && (count_new || f->age > 0)) s->factions++;
	}

	/* Alles zählen */

	for (r = regions; r; r = r->next) {
		s->pferde += rhorses(r);
		s->schiffe += listlen(r->ships);
		s->gebaeude += listlen(r->buildings);
		if (rterrain(r) != T_OCEAN) {
			s->landregionen++;
			if (r->units) {
				s->landregionen_mit_spielern++;
			}
			if (fval(r, RF_ORCIFIED)) {
				s->orkifizierte_regionen++;
			}
			if (rterrain(r) == T_VOLCANO) {
				s->inactive_volcanos++;
			} else if(rterrain(r) == T_VOLCANO_SMOKING) {
				s->active_volcanos++;
			}
		}
		if (r->units) {
			s->regionen_mit_spielern++;
		}
		if (rpeasants(r) || r->units) {
			s->inhabitedregions++;
			s->peasants += rpeasants(r);
			s->peasantmoney += rmoney(r);

			/* Einheiten Info. nregions darf nur einmal pro Partei
			 * incrementiert werden. */

			for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
			for (u = r->units; u; u = u->next) {
				f = u->faction;
				if (u->faction->no != MONSTER_FACTION) {
          skill * sv;

					s->nunits++;
					s->playerpop += u->number;
					s->spielerpferde += get_item(u, I_HORSE);
					s->playermoney += get_money(u);
					s->armed_men += armedmen(u);
					s->waffen += get_item(u, I_SWORD);
					s->waffen += get_item(u, I_SPEAR);
					s->waffen += get_item(u, I_CATAPULT);
					s->waffen += get_item(u, I_CROSSBOW);
					s->waffen += get_item(u, I_LONGBOW);
					s->waffen += get_item(u, I_RUNESWORD);

					s->ruestungen += get_item(u, I_CHAIN_MAIL);
					s->ruestungen += get_item(u, I_PLATE_ARMOR);

#ifdef COMPATIBILITY
					s->ruestungen += get_item(u, I_CLOAK_OF_INVULNERABILITY);

					s->artefakte += get_item(u, I_AMULET_OF_DARKNESS);
					s->artefakte += get_item(u, I_AMULET_OF_HEALING);
					s->artefakte += get_item(u, I_CLOAK_OF_INVULNERABILITY);
					s->artefakte += get_item(u, I_SHIELDSTONE);
					s->artefakte += get_item(u, I_STAFF_OF_FIRE);
					s->artefakte += get_item(u, I_STAFF_OF_LIGHTNING);
					s->artefakte += get_item(u, I_WAND_OF_TELEPORTATION);
#endif

					s->artefakte += get_item(u, I_AMULET_OF_TRUE_SEEING);
					s->artefakte += get_item(u, I_RING_OF_INVISIBILITY);
					s->artefakte += get_item(u, I_SPHERE_OF_INVISIBILITY);
					s->artefakte += get_item(u, I_SACK_OF_CONSERVATION);
					s->artefakte += get_item(u, I_RING_OF_POWER);
					s->artefakte += get_item(u, I_RUNESWORD);

					s->spielerpferde += get_item(u, I_HORSE);

          for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
						skill_t sk = sv->id;
            int aktskill = eff_skill(u, sk, r);
						if (aktskill > s->maxskill) s->maxskill = aktskill;
					}
					if (!fval(f, FL_DH)) {
						f->nregions++;
						fset(f, FL_DH);
					}
				}

				f->number += u->number;
				f->money += get_money(u);
				s->poprace[old_race(u->race)] += u->number;
			}
		}
	}

	/* jetzt noch parteienweise zählen */
	/* Problem mit Monsterpartei ... */
	for (f = factions; f; f = f->next) if (f->no!=MONSTER_FACTION) {
		if (f->alive && (count_new || f->age > 0)) {
			s->factionrace[old_race(f->race)]++;
		}
	}
	return s;
}

static char *
pcomp(double i, double j)
{
   sprintf(buf, "%.0f (%s%.0f)", i, (i>=j)?"+":"", i-j);
   return buf;
}

static char *
rcomp(int i, int j)
{
	sprintf(buf, "%d (%s%d,%s%d%%)",
			i, (i>=j)?"+":"", i-j, (i>=j)?"+":"",j?((i-j)*100)/j:0);
	return buf;
}

static void
writemonument(void)
{
	FILE * F;
	region *r;
	building *b;
	building *buildings[7] = {NULL, NULL, NULL, NULL, NULL,  NULL, NULL};
	int size[7] = {0,0,0,0,0,0,0};
	int i, j, ra;
	int count = 0;
	unit *owner;

	for (r = regions; r; r = r->next) {
		for (b = r->buildings; b; b = b->next) {
			if (b->type == bt_find("monument") && b->display && *b->display) {
				freset(b, FL_DH);
				count++;
				if(b->size > size[6]) {
					for(i=0; i <= 6; i++) if(b->size >= size[i]) {
						for(j=5;j >= i; j--) {
							size[j+1] = size[j];
							buildings[j+1] = buildings[j];
						}
						buildings[i] = b;
						size[i] = b->size;
						break;
					}
				}
			}
		}
	}

	for(i=0; i <= 6; i++) if(buildings[i]) {
		fset(buildings[i], FL_DH);
	}
	{
		char zText[MAX_PATH];
		sprintf(zText, "%s/news-monument", basepath());
		F = cfopen(zText, "w");
		if (!F) return;
	}
	fprintf(F, "\n--- maintitle ---\n\n");
	for(i = 0; i<=6; i++) {
		if (buildings[i] != NULL) {
			fprintf(F, "In %s", rname(buildings[i]->region, NULL));
			if ((owner=buildingowner(buildings[i]->region,buildings[i]))!=NULL && !fval(owner,UFL_PARTEITARNUNG)) {
				fprintf(F, ", Eigentümer: %s", factionname(owner->faction));
			}
			fprintf(F, "\n\n");
			report_building(F, buildings[i]->region, buildings[i], findfaction(0), see_neighbour);
			fprintf(F, "\n\n");
		}
	}

	fprintf(F, "\n--- newcomer ---\n\n");

	if(count > 7) {
		ra = rand()%(count-7);
		j = 0;
		for (r = regions; r; r = r->next) {
			for (b = r->buildings; b; b = b->next) {
				if (b->type == bt_find("monument") && b->display && *b->display && !fval(b, FL_DH)) {
					j++;
					if(j == ra) {
						fprintf(F, "In %s", rname(b->region, NULL));
						if ((owner=buildingowner(b->region,b))!=NULL && !fval(owner,UFL_PARTEITARNUNG)) {
							fprintf(F, ", Eigentümer: %s", factionname(owner->faction));
						}
						fprintf(F, "\n\n");
						report_building(F, b->region, b, findfaction(0), see_neighbour);
						fprintf(F, "\n\n");
					}
				}
			}
		}
	}

	fclose(F);

#ifdef OLD_ITEMS
	{
		char zText[MAX_PATH];
		sprintf(zText, "%s/news-silly", basepath());
		F = cfopen(zText, "w");
		if (!F) return;
	}
	for (f=factions;f;f=f->next) {
		unit * u;
		int k;
		int count = 0;
		for (u=f->units;u;u=u->nextF) {
			count += get_herb(u, h);
		}
		for (i=0;i!=3;++i) if (count<fjord[i].size) break;
		if (i) {
			for (k=0;k<i-1;++k) fjord[k] = fjord[k+1];
			fjord[i-1].size=count;
			fjord[i-1].f=f;
		}
	}
	fprintf(F, "\n\nBesitzer der größten %shaufen:\n", herbdata[0][h]);
	for (i=0;i!=3;++i) {
		fprintf(F, "  %s (%d Stück)\n", factionname(fjord[2-i].f), fjord[2-i].size);
	}
	fclose(F);
#else
	printf("!! news-silly not implemented\n");
#endif
}

static void
writeadresses(void)
{
	faction *f;
	FILE *F;
	char zText[MAX_PATH];
	sprintf(zText, "%s/adressen", basepath());
	F = cfopen(zText, "w");
	if (!F) return;

	for (f = factions; f; f = f->next) {
		if (f->no != MONSTER_FACTION && playerrace(f->race)) {
			fprintf(F, "%s:%s:%s\n", factionname(f), f->email, f->banner);
		}
	}
	fclose(F);
}

static void
writenewssubscriptions(void)
{
	char zText[MAX_PATH];
	FILE *F;

	sprintf(zText, "%s/news-subscriptions", basepath());
	F = cfopen(zText, "w");
	if (!F) return;
#ifdef AT_OPTION
	{
		faction *f;
		for(f=factions; f; f=f->next) {
			attrib *a = a_find(f->attribs, &at_option_news);
			if(!a) {
				fprintf(F, "%s:0\n", f->email);
			} else {
				fprintf(F, "%s:%d\n", f->email, a->data.i);
			}
		}
	}
#endif
	fclose(F);
}

static void
writeforward(void)
{
	FILE *forwardFile;
	region *r;
	faction *f;
	unit *u;

	{
		char zText[MAX_PATH];
		sprintf(zText, "%s/aliases", basepath());
		forwardFile = cfopen(zText, "w");
		if (!forwardFile) return;
	}

	for (f = factions; f; f = f->next) {
		if (f->no != MONSTER_FACTION) {
			fprintf(forwardFile,"partei-%s: %s\n", factionid(f), f->email);
		}
	}

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			if (u->faction->no != MONSTER_FACTION) {
				fprintf(forwardFile,"einheit-%s: %s\n", unitid(u), u->faction->email);
			}
		}
	}

	fclose(forwardFile);
}

static void
writeturn(void)
{
	char zText[MAX_PATH];
	FILE *f;

	sprintf(zText, "%s/datum", basepath());
	f = cfopen(zText, "w");
	if (!f) return;
	fputs(gamedate2(default_locale), f);
	fclose(f);
	sprintf(zText, "%s/turn", basepath());
	f = cfopen(zText, "w");
	if (!f) return;
	fprintf(f,"%d",turn);
	fclose(f);
}

static void
out_faction(FILE *file, faction *f)
{
#ifdef ALLIANCES
 	fprintf(file, "%s (%s/%d) (%.3s/%.3s), %d Einh., %d Pers., $%d, %d %s\n",
		f->name, itoa36(f->no), f->alliance?f->alliance->id:0,
		LOC(default_locale, rc_name(f->race, 0)), neue_gebiete[f->magiegebiet],
		f->no_units, f->number, f->money, turn - f->lastorders,
		turn - f->lastorders != 1 ? "NMRs" : "NMR ");
#else
 	fprintf(file, "%s (%.3s/%.3s), %d Einh., %d Pers., $%d, %d %s\n",
		factionname(f), LOC(default_locale, rc_name(f->race, 0)),
		neue_gebiete[f->magiegebiet], f->no_units, f->number, f->money,
		turn - f->lastorders, turn - f->lastorders != 1 ? "NMRs" : "NMR ");
#endif
}

void
report_summary(summary * s, summary * o, boolean full)
{
	FILE * F = NULL;
	int i, newplayers = 0;
	faction * f;
	int * nmrs = malloc(sizeof(int)*(NMRTimeout()+1));

	{
		char zText[MAX_PATH];
		if (full) {
			sprintf(zText, "%s/parteien.full", basepath());
		} else {
			sprintf(zText, "%s/parteien", basepath());
		}
		F = cfopen(zText, "w");
		if (!F) return;
	}
	printf("Schreibe Zusammenfassung (parteien)...\n");
	fprintf(F,   "%s\n%s\n\n", global.gamename, gamedate2(default_locale));
	fprintf(F,   "Auswertung Nr:         %d\n\n", turn);
	fprintf(F,   "Parteien:              %s\n", pcomp(s->factions, o->factions));
	fprintf(F,   "Einheiten:             %s\n", pcomp(s->nunits, o->nunits));
	fprintf(F,   "Spielerpopulation:     %s\n",
			pcomp(s->playerpop, o->playerpop));
	fprintf(F,   " davon bewaffnet:      %s\n",
			pcomp(s->armed_men, o->armed_men));

	if (full) {
		fprintf(F, "Regionen:              %d\n", listlen(regions));
		fprintf(F, "Bewohnte Regionen:     %d\n", s->inhabitedregions);
		fprintf(F, "Landregionen:          %d\n", s->landregionen);
		fprintf(F, "Spielerregionen:       %d\n", s->regionen_mit_spielern);
		fprintf(F, "Landspielerregionen:   %d\n", s->landregionen_mit_spielern);
		fprintf(F, "Orkifizierte Regionen: %d\n", s->orkifizierte_regionen);
		fprintf(F, "Inaktive Vulkane:      %d\n", s->inactive_volcanos);
		fprintf(F, "Aktive Vulkane:        %d\n\n", s->active_volcanos);
	}

	for (i = 0; i < MAXRACES; i++) if (s->factionrace[i] && playerrace(new_race[i])
			&& i != RC_TEMPLATE && i != RC_CLONE) {
		fprintf(F, "%14svölker: %s\n",
		LOC(default_locale, rc_name(new_race[i], 3)), pcomp(s->factionrace[i], o->factionrace[i]));
	}

	if(full) {
		fprintf(F, "\n");
		{
			struct language * plang = s->languages;
			while (plang!=NULL) {
				struct language * olang = o->languages;
				int nold = 0;
				while (olang && olang->locale!=plang->locale) olang=olang->next;
				if (olang) nold = olang->number;
				fprintf(F, "Sprache %12s: %s\n", locale_name(plang->locale),
					rcomp(plang->number, nold));
				plang=plang->next;
			}
		}
	}

	fprintf(F, "\n");
	if (full) {
		for (i = 0; i < MAXRACES; i++) if (s->poprace[i]) {
			fprintf(F, "%20s: %s\n", LOC(default_locale, rc_name(new_race[i], 1)),
				rcomp(s->poprace[i],o->poprace[i]));
		}
	} else {
		for (i = 0; i < MAXRACES; i++) if (s->poprace[i] && playerrace(new_race[i])
				&& i != RC_TEMPLATE && i != RC_CLONE) {
			fprintf(F, "%20s: %s\n", LOC(default_locale, rc_name(new_race[i], 1)),
				rcomp(s->poprace[i],o->poprace[i]));
		}
	}

	if (full) {
		fprintf(F, "\nWaffen:               %s\n", pcomp(s->waffen,o->waffen));
		fprintf(F, "Rüstungen:            %s\n",
				pcomp(s->ruestungen,o->ruestungen));
		fprintf(F, "ungezähmte Pferde:    %s\n", pcomp(s->pferde, o->pferde));
		fprintf(F, "gezähmte Pferde:      %s\n",
				pcomp(s->spielerpferde,o->spielerpferde));
		fprintf(F, "Artefakte:            %s\n", pcomp(s->artefakte,o->artefakte));
		fprintf(F, "Schiffe:              %s\n", pcomp(s->schiffe, o->schiffe));
		fprintf(F, "Gebäude:              %s\n", pcomp(s->gebaeude, o->gebaeude));

		fprintf(F, "\nBauernpopulation:     %s\n", pcomp(s->peasants,o->peasants));

		fprintf(F, "Population gesamt:    %d\n\n", s->playerpop+s->peasants);

		fprintf(F, "Reichtum Spieler:     %s Silber\n",
				pcomp(s->playermoney,o->playermoney));
		fprintf(F, "Reichtum Bauern:      %s Silber\n",
				pcomp(s->peasantmoney, o->peasantmoney));
		fprintf(F, "Reichtum gesamt:      %s Silber\n\n",
				pcomp(s->playermoney+s->peasantmoney, o->playermoney+o->peasantmoney));
	}

	fprintf(F, "\n\n");

	for (i = 0; i <= NMRTimeout(); ++i) {
		nmrs[i] = 0;
	}

	for (f = factions; f; f = f->next) {
		if (f->age <= 1 && turn - f->lastorders == 1) {
			newplayers++;
		} else if (f->no != MONSTER_FACTION) {
      int nmr = turn-f->lastorders;
      if (nmr<0 || nmr>NMRTimeout()) {
        log_error(("faction %s has %d NMRS\n", factionid(f), nmr));
        nmr = max(0, nmr);
        nmr = min(nmr, NMRTimeout());
      }
			nmrs[nmr]++;
		}
	}

	for (i = 0; i <= NMRTimeout(); ++i) {
		if (i == NMRTimeout()) {
			fprintf(F, "+ NMRs:\t\t %d\n", nmrs[i]);
		} else {
			fprintf(F, "%d %s:\t\t %d\n", i,
				i != 1 ? "NMRs" : "NMR ", nmrs[i]);
		}
	}
	if (age) {
		if (age[2] != 0) {
			fprintf(F, "Erstabgaben:\t %d%%\n", 100 - (dropouts[0] * 100 / age[2]));
		}
		if (age[3] != 0) {
			fprintf(F, "Zweitabgaben:\t %d%%\n", 100 - (dropouts[1] * 100 / age[3]));
		}
	}
	fprintf(F, "Neue Spieler:\t %d\n", newplayers);

	if (factions)
   		fprintf(F, "\nParteien:\n\n");

	for (f = factions; f; f = f->next) {
		out_faction(F, f);
	}

	if (NMRTimeout() && full) {
		fprintf(F, "\n\nFactions with NMRs:\n");
		for (i = NMRTimeout(); i > 0; --i) {
			for(f=factions; f; f=f->next) {
				if(i == NMRTimeout()) {
					if(turn - f->lastorders >= i) {
						out_faction(F, f);
					}
				} else {
					if(turn - f->lastorders == i) {
						out_faction(F, f);
					}
				}
			}
		}
	}

	fclose(F);

	if (full) {
		printf("Schreibe Liste der Adressen (adressen)...\n");
		writeadresses();
		writenewssubscriptions();
		writeforward();

		printf("writing date & turn\n");
		writeturn();

		writemonument();
	}
	free(nmrs);
}
/******* end summary ******/

static void
eval_unit(struct opstack ** stack, const void * userdata) /* unit -> string */
{
	const struct unit * u = opop(stack, const struct unit *);
	const char * c = u?unitname(u):"nobody";
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}

static void
eval_spell(struct opstack ** stack, const void * userdata) /* unit -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct spell * sp = opop(stack, const struct spell *);
  const char * c = sp?spell_name(sp, f->locale):"an unknown spell";
  size_t len = strlen(c);
  opush(stack, strcpy(balloc(len+1), c));
}

static void
eval_unitname(struct opstack ** stack, const void * userdata) /* unit -> string */
{
	const struct unit * u = opop(stack, const struct unit *);
	const char * c = u?u->name:"nobody";
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}


static void
eval_unitid(struct opstack ** stack, const void * userdata) /* unit -> int */
{
	const struct unit * u = opop(stack, const struct unit *);
	const char * c = u?u->name:"nobody";
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}

static void
eval_faction(struct opstack ** stack, const void * userdata) /* faction -> string */
{
	const struct faction * f = opop(stack, const struct faction *);
	const char * c = factionname(f);
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}

#ifdef ALLIANCES
static void
eval_alliance(struct opstack ** stack, const void * userdata) /* faction -> string */
{
	const struct alliance * al = opop(stack, const struct alliance *);
	const char * c = alliancename(al);
	if (c!=NULL) {
		size_t len = strlen(c);
		opush(stack, strcpy(balloc(len+1), c));
	}
	else opush(stack, NULL);
}
#endif

static void
eval_region(struct opstack ** stack, const void * userdata) /* region -> string */
{
	const struct faction * f = (const struct faction *)userdata;
	const struct region * r = opop(stack, const struct region *);
	const char * c = regionname(r, f);
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}

static void
eval_ship(struct opstack ** stack, const void * userdata) /* ship -> string */
{
	const struct ship * u = opop(stack, const struct ship *);
	const char * c = u?shipname(u):"nobody";
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}

static void
eval_building(struct opstack ** stack, const void * userdata) /* building -> string */
{
	const struct building * u = opop(stack, const struct building *);
	const char * c = u?buildingname(u):"nobody";
	size_t len = strlen(c);
	opush(stack, strcpy(balloc(len+1), c));
}

static void
eval_resource(struct opstack ** stack, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	int j = opop(stack, int);
	struct resource_type * res = opop(stack, struct resource_type *);

	const char * c = LOC(report->locale, resourcename(res, j!=1));
	opush(stack, strcpy(balloc(strlen(c)+1), c));
}

static void
eval_race(struct opstack ** stack, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	int j = opop(stack, int);
	const race * r = opop(stack, const race *);

	const char * c = LOC(report->locale, rc_name(r, j!=1));
	opush(stack, strcpy(balloc(strlen(c)+1), c));
}

static void
eval_direction(struct opstack ** stack, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	int i = opop(stack, int);
	const char * c;
	if (i>=0) {
		c = LOC(report->locale, directions[i]);
	} else {
		c = LOC(report->locale, "unknown_direction");
	}
	opush(stack, strcpy(balloc(strlen(c)+1), c));
}

static void
eval_skill(struct opstack ** stack, const void * userdata)
{
	const faction * report = (const faction*)userdata;
	skill_t sk = (skill_t)opop(stack, int);
	const char * c = skillname(sk, report->locale);
	opush(stack, strcpy(balloc(strlen(c)+1), c));
}

static void
eval_int36(struct opstack ** stack, const void * userdata)
{
	int i = opop(stack, int);
	const char * c = itoa36(i);
	opush(stack, strcpy(balloc(strlen(c)+1), c));
	unused(userdata);
}

static void
eval_string(struct opstack ** stack, const void * userdata)
{
  const char * c = opop(stack, const char*);
  opush(stack, strcpy(balloc(strlen(c)+1), c));
  unused(userdata);
}

void
report_init(void)
{
#ifdef ALLIANCES
	add_function("alliance", &eval_alliance);
#endif
	add_function("region", &eval_region);
	add_function("resource", &eval_resource);
	add_function("race", &eval_race);
	add_function("faction", &eval_faction);
	add_function("ship", &eval_ship);
	add_function("unit", &eval_unit);
        add_function("order", &eval_string);
        add_function("spell", &eval_spell);
	add_function("unit.name", &eval_unitname);
	add_function("unit.id", &eval_unitid);
	add_function("building", &eval_building);
	add_function("skill", &eval_skill);
	add_function("direction", &eval_direction);
	add_function("int36", &eval_int36);
	add_function("trail", &eval_trail);
	register_function((pf_generic)view_neighbours, "view_neighbours");
	register_function((pf_generic)view_regatta, "view_regatta");
}
