/* vi: set ts=2:
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
#include "reports.h"

/* kernel includes */
#include "skill.h"
#include "unit.h"
#include "item.h"
#include "group.h"
#include "faction.h"
#include "building.h"
#include "karma.h"
#include "ship.h"
#include "race.h"
#include "magic.h"
#ifdef USE_UGROUPS
# include "ugroup.h"
#endif

/* util includes */
#include <base36.h>
#include <goodies.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/* attributes includes */
#include <attributes/follow.h>
#include <attributes/otherfaction.h>
#include <attributes/racename.h>

const char * g_reportdir;

const char *neue_gebiete[] = {
	"Kein Magiegebiet",
	"Illaun",
	"Tybied",
	"Cerddor",
	"Gwyrrd",
	"Draig"
};

const char *coasts[MAXDIRECTIONS] =
{
	"Nordwestküste",
	"Nordostküste",
	"Ostküste",
	"Südostküste",
	"Südwestküste",
	"Westküste"
};

const char * 
reportpath(void)
{
	static char zText[MAX_PATH];
	if (g_reportdir) return g_reportdir;
	return strcat(strcpy(zText, basepath()), "/reports");
}

static char *
groupid(const struct group * g, const struct faction * f)
{
	typedef char name[OBJECTIDSIZE + 1];
	static name idbuf[8];
	static int nextbuf = 0;
	char *buf = idbuf[(++nextbuf) % 8];
	sprintf(buf, "%s (%s)", g->name, factionid(f));
	return buf;
}


char *
report_kampfstatus(const unit * u)
{
	static char fsbuf[64];

	switch (u->status) {
	case ST_FIGHT:
		strcpy(fsbuf, ", vorne");
		break;

	case ST_BEHIND:
		strcpy(fsbuf, ", hinten");
		break;

	case ST_AVOID:
		strcpy(fsbuf, ", kämpft nicht");
		break;

	case ST_FLEE:
		strcpy(fsbuf, ", flieht");
		break;

	default:
		strcpy(fsbuf, ", unbekannt");
		break;
	}
#ifdef NOAID
	if(fval(u, FL_NOAID)) strcat(fsbuf, ", bekommt keine Hilfe");
#endif
	return fsbuf;
}

const char *
hp_status(const unit * u)
{
	double p = (double) ((double) u->hp / (double) (u->number * unit_max_hp(u)));

	if (p > 2.00)
		return "sehr stark";
	if (p > 1.50)
		return "stark";
	if (p < 0.50)
		return "schwer verwundet";
	if (p < 0.75)
		return "verwundet";
	if (p < 0.99)
		return "erschöpft";

	return NULL;
}

void
report_item(const unit * owner, const item * i, const faction * viewer, const char ** name, const char ** basename, int * number, boolean singular)
{
	if (owner->faction == viewer) {
		if (name) *name = locale_string(viewer->locale, resourcename(i->type->rtype, ((i->number!=1 && !singular)?GR_PLURAL:0)));
		if (basename) *basename = resourcename(i->type->rtype, 0);
		if (number) *number = i->number;
	} else if (i->type->rtype==r_silver) {
		int pp = i->number/owner->number;
		if (number) *number = 1;
		if (pp > 50000 && dragon(owner)) {
			if (name) *name = locale_string(viewer->locale, "dragonhoard");
			if (basename) *basename = "dragonhoard";
		} else if (pp > 5000) {
			if (name) *name = locale_string(viewer->locale, "moneychest");
			if (basename) *basename = "moneychest";
		} else if (pp > 500) {
			if (name) *name = locale_string(viewer->locale, "moneybag");
			if (basename) *basename = "moneybag";
		} else {
			if (number) *number = 0;
			if (name) *name = NULL;
			if (basename) *basename = NULL;
		}
	} else {
		if (name) *name = locale_string(viewer->locale, resourcename(i->type->rtype, NMF_APPEARANCE|((i->number!=1 && !singular)?GR_PLURAL:0)));
		if (basename) *basename = resourcename(i->type->rtype, NMF_APPEARANCE);
		if (number) {
			const herb_type * htype = resource2herb(i->type->rtype);
			if (htype!=NULL) *number = 1;
			else *number = i->number;
		}
	}
}

int
bufunit(const faction * f, const unit * u, int indent, int mode)
{
	int i, dh;
	skill_t sk;
	int getarnt = fval(u, FL_PARTEITARNUNG);
	attrib *a_otherfaction = NULL;
	const char *pzTmp;
	spell *sp;
	building * b;
	boolean itemcloak = is_cursed(u->attribs, C_ITEMCLOAK, 0);
	boolean isbattle = (boolean)(mode == see_battle);
	int telepath_see = fspecial(f, FS_TELEPATHY);
	attrib *a_fshidden = NULL;
	item * itm;
	item * show;
	
	if(fspecial(u->faction, FS_HIDDEN))
		a_fshidden = a_find(u->attribs, &at_fshidden);

	strcpy(buf, unitname(u));
	
	a_otherfaction = a_find(u->attribs, &at_otherfaction);

	if (u->faction == f) {
#ifdef GROUPS
		attrib *a = a_find(u->attribs, &at_group);
		if (a) {
			group * g = (group*)a->data.v;
			scat(", ");
			scat(groupid(g, f));
		}
#endif
		if (getarnt) {
			scat(", parteigetarnt");
		}	else if (a_otherfaction) {
			scat(", ");
			scat(factionname(findfaction(a_otherfaction->data.i)));
		}
	} else {
		if (getarnt) {
			scat(", parteigetarnt");
		} else if(a_otherfaction) {
			scat(", ");
			scat(factionname(findfaction(a_otherfaction->data.i)));
		} else {
			scat(", ");
			scat(factionname(u->faction));
		}
	}

	scat(", ");

	if(u->faction != f && a_fshidden
			&& a_fshidden->data.ca[0] == 1 && effskill(u, SK_STEALTH) >= 6) {
		scat("? ");
	} else {
		icat(u->number);
		scat(" ");
	}

	pzTmp = get_racename(u->attribs);
	if (pzTmp || u->irace != u->race) {
		if (pzTmp) scat(pzTmp);
		else scat(locale_string(f->locale, race[u->irace].name[u->number != 1]));
		if (u->faction == f) {
			scat(" (");
			scat(locale_string(f->locale, race[u->race].name[u->number != 1]));
			scat(")");
		}
	} else {
		scat(locale_string(f->locale, race[u->race].name[u->number != 1]));
	}

	/* status */

	if (u->number && (u->faction == f || telepath_see || isbattle)) {
		const char * c = hp_status(u);
		scat(report_kampfstatus(u));
		if (c || fval(u, FL_HUNGER)) {
			scat(" (");
			if(c) scat(c);
			if(fval(u, FL_HUNGER)) {
				if (c) scat(", hungert");
				else  scat("hungert");
			}
			scat(")");
		}
	}
	if (getguard(u)) scat(", bewacht die Region");

#if NEW_FOLLOW
	if (u->faction==f || telepath_see) {
		attrib * a = a_find(u->attribs, &at_follow);
		if (a) {
			unit * uf = (unit*)a->data.v;
			if (uf) {
				scat(", folgt ");
				scat(itoa36(uf->no));
			}
		}
	}
#endif
	if ((b = usiege(u))!=NULL) {
		scat(", belagert ");
		scat(buildingname(b));
	}

	dh = 0;
	if (u->faction == f || telepath_see) {
		for (sk = 0; sk != MAXSKILLS; sk++) {
			spskill(f->locale, u, sk, &dh, 1);
		}
	}

	dh = 0;
	if (f == u->faction || telepath_see || omniscient(u->faction)) {
		show = u->items;
	} else if (!itemcloak && mode >= see_unit && !(a_fshidden
			&& a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 3)) {
		show = NULL;
		for (itm=u->items;itm;itm=itm->next) {
			item * ishow;
			const char * ic;
			int in;
			report_item(u, itm, f, NULL, &ic, &in, false);
			if (ic && *ic && in>0) {
				for (ishow = show; ishow; ishow=ishow->next) {
					const char * sc;
					int sn;
					if (ishow->type==itm->type) sc=ic;
					else report_item(u, ishow, f, NULL, &sc, &sn, false);
					if (sc==ic || strcmp(sc, ic)==0) {
						ishow->number+=itm->number;
						break;
					}
				}
				if (ishow==NULL) {
					ishow = i_add(&show, i_new(itm->type));
					ishow->number = itm->number;
				}
			}
		}
	} else {
		show = NULL;
	}
	for (itm=show; itm; itm=itm->next) {
		const char * ic;
		int in;
		report_item(u, itm, f, &ic, NULL, &in, false);
		if (in==0 || ic==NULL) continue;
		scat(", ");

		if (!dh) {
			sprintf(buf+strlen(buf), "%s: ", LOC(f->locale, "nr_inventory"));
			dh = 1;
		}
		if (in == 1) {
			scat(ic);
		} else {
			icat(in);
			scat(" ");
			scat(ic);
		}
	}
	if (show!=u->items) while (show) i_free(i_remove(&show, show));

	if (u->faction == f || telepath_see) {
		dh = 0;

		if (is_mage(u) == true) {
			scat(". Aura ");
			icat(get_spellpoints(u));
			scat("/");
			icat(max_spellpoints(u->region,u));
			{
				spell_ptr *spt;
				int t = effskill(u, SK_MAGIC);

				for (spt = get_mage(u)->spellptr;spt; spt = spt->next){
					sp = find_spellbyid(spt->spellid);
					if(sp->level > t){
						continue;
					}
					if (!dh){
						sprintf(buf+strlen(buf),", %s: ", LOC(f->locale, "nr_spells"));
						dh = 1;
					}else{
						scat(", ");
					}
					scat(sp->name);
				}
			}
			dh = 0;
			for (i = 0; i < MAXCOMBATSPELLS; i++){
				sp = get_combatspell(u,i);
				if (sp) {
					dh = 1;
				}
			}
			if(dh){
				dh = 0;
				sprintf(buf+strlen(buf),", %s: ", LOC(f->locale, "nr_combatspells"));
				for (i = 0; i < MAXCOMBATSPELLS; i++){
					if (!dh){
						dh = 1;
					}else{
						scat(", ");
					}
					sp = get_combatspell(u,i);
					if (sp) {
						int sl;
						scat(sp->name);
						if((sl = get_combatspelllevel(u,i)) > 0) {
							scat(" (");
							icat(sl);
							scat(")");
						}
					}else{
						scat(LOC(f->locale, "nr_nospells"));
					}
				}
			}
		}
		if (!isbattle && u->lastorder[0]) {
			scat(", \"");
			scat(u->lastorder);
			scat("\"");
		}
	}
	i = 0;

	if (u->display[0]) {
		scat("; ");
		scat(u->display);

		i = u->display[strlen(u->display) - 1];
	}
	if (i != '!' && i != '?' && i != '.')
		scat(".");

	pzTmp = uprivate(u);
	if (u->faction == f && pzTmp) {
		scat(" (Bem: ");
		scat(pzTmp);
		scat(")");
	}

	dh=0;
	if (!getarnt && f && f->allies) {
		ally *sf;
		faction *tf = u->faction; 
		
		/* getarnte Partei bei a_otherfaction */
		if(a_otherfaction) {
			tf = findfaction(a_otherfaction->data.i);
		}
		for (sf = f->allies; sf && !dh; sf = sf->next) {
			if (sf->status > 0 && sf->status <= HELP_ALL && sf->faction == tf) {
				dh = 1;
			}
		}
	}
	return dh;
}

/* TODO: telepath_see wird nicht berücksichtigt: Parteien mit
 * telepath_see sollten immer einzelne Einheiten zu sehen
 * bekommen, alles andere ist darstellungsteschnisch kompliziert.
 */

#ifdef USE_UGROUPS
 int
bufunit_ugroupleader(const faction * f, const unit * u, int indent, int mode)
{
	int i, dh;
	int getarnt = fval(u, FL_PARTEITARNUNG);
	attrib *a_otherfaction = NULL;
	const char *pzTmp;
	boolean itemcloak = is_cursed(u->attribs, C_ITEMCLOAK, 0);
	attrib *a_fshidden = NULL;
	item * itm;
	item * show;
	ugroup *ug = findugroup(u);
	boolean guards = false;
	boolean sieges = false;
	
	if(fspecial(u->faction, FS_HIDDEN))
		a_fshidden = a_find(u->attribs, &at_fshidden);

	strcpy(buf, u->name);
	
	a_otherfaction = a_find(u->attribs, &at_otherfaction);

	if (getarnt) {
		scat(", parteigetarnt");
	} else if(a_otherfaction) {
		scat(", ");
		scat(factionname(findfaction(a_otherfaction->data.i)));
	} else {
		scat(", ");
		scat(factionname(u->faction));
	}

	scat(", ");

	for(i = 0; i < ug->members; i++) {
		unit *uc = ug->unit_array[i];
		if(uc->faction != f && a_fshidden
				&& a_fshidden->data.ca[0] == 1 && effskill(uc, SK_STEALTH) >= 6) {
			scat("? ");
		}	 else {
			icat(uc->number);
			scat(" ");
		}
		pzTmp = get_racename(uc->attribs);
		if (pzTmp || u->irace != uc->race) {
			if (pzTmp)
				scat(pzTmp);
			else
				scat(locale_string(f->locale, race[uc->irace].name[uc->number != 1]));
			scat(" (");
			scat(itoa36(uc->no));
			scat("), ");
		} else {
			scat(locale_string(f->locale, race[uc->race].name[uc->number != 1]));
			scat(" (");
			scat(itoa36(uc->no));
			scat("), ");
		}
		if(getguard(uc)) guards = true;
		if(usiege(uc)) sieges = true;
	}

	if(guards == true) scat(", bewacht die Region");

	if(sieges == true) {
		scat(", belagert ");
		for(i = 0; i < ug->members; i++) {
			building *b = usiege(ug->unit_array[i]);
			if(b) {
				scat(buildingname(b));
				scat(", ");
			}
		}
	}

	dh = 0;
	show = NULL;
	for(i = 0; i < ug->members; i++) {
		unit *uc = ug->unit_array[i];
		if(!itemcloak && mode >= see_unit && !(a_fshidden
				 && a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 3)) {
			for (itm=uc->items;itm;itm=itm->next) {
				item *ishow;
				const char * ic;
				int in;
				report_item(u, itm, f, NULL, &ic, &in, false);
				if (ic && *ic && in>0) {
					for (ishow = show; ishow; ishow=ishow->next) {
						const char * sc;
						int sn;
						if (ishow->type==itm->type) sc=ic;
						else report_item(u, ishow, f, NULL, &sc, &sn, false);
						if (sc==ic || strcmp(sc, ic)==0) {
							ishow->number+=itm->number;
							break;
						}
					}
					if (ishow==NULL) {
						ishow = i_add(&show, i_new(itm->type));
						ishow->number = itm->number;
					}
				}
			}
		}
	}
	for (itm=show; itm; itm=itm->next) {
		const char * ic;
		int in;
		report_item(u, itm, f, &ic, NULL, &in, false);
		if (in==0 || ic==NULL) continue;
		scat(", ");

		if (!dh) {
			sprintf(buf+strlen(buf), "%s: ", LOC(f->locale, "nr_inventory"));
			dh = 1;
		}
		if (in == 1) {
			scat(ic);
		} else {
			icat(in);
			scat(" ");
			scat(ic);
		}
	}
	if(show) while(show) i_free(i_remove(&show, show));

	i = 0;
	if (u->display[0]) {
		scat("; ");
		scat(u->display);

		i = u->display[strlen(u->display) - 1];
	}
	if (i != '!' && i != '?' && i != '.')
		scat(".");

	dh=0;
	if (!getarnt && f && f->allies) {
		ally *sf;
		faction *tf = u->faction; 
		
		/* getarnte Partei bei a_otherfaction */
		if(a_otherfaction) {
			tf = findfaction(a_otherfaction->data.i);
		}
		for (sf = f->allies; sf && !dh; sf = sf->next) {
			if (sf->status > 0 && sf->status <= HELP_ALL && sf->faction == tf) {
				dh = 1;
			}
		}
	}
	return dh;
}
#endif

void
spskill(const struct locale * lang, const struct unit * u, skill_t sk, int *dh, int days)
{
	char * sbuf = buf+strlen(buf);
	int i, d;
	if (!u->number)
		return;

	d = get_skill(u, sk);
	if (!d)
		return;

	strcat(sbuf, ", "); sbuf+=2;

	if (!*dh) {
		sbuf += sprintf(sbuf, "%s: ", LOC(lang, "nr_skills"));
		*dh = 1;
	}
	sbuf += sprintf(sbuf, "%s ", skillname(sk, lang));

	if (sk == SK_MAGIC){
		if (find_magetype(u) != M_GRAU){
			sbuf += sprintf(sbuf, "%s ", magietypen[find_magetype(u)]);
		}
	}

	if (sk == SK_STEALTH) {
		i = u_geteffstealth(u);
		if(i>=0) {
			sbuf += sprintf(sbuf, "%d/", i);
		}
	}
	sbuf += sprintf(sbuf, "%d", effskill(u, sk));

#ifndef NOVISIBLESKILLPOINTS
	if (days) {
		assert(u->number);
		sbuf += sprintf(sbuf, " [%d]", d / u->number);
	}
#endif
}

void
lparagraph(struct strlist ** SP, char *s, int indent, char mark)
{

	/* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
	 * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
	 * Vgl. spunit (). */

	char *buflocal = calloc(strlen(s) + indent + 1, sizeof(char));

	if (indent) {
		memset(buflocal, ' ', indent);
		if (mark)
			buflocal[indent - 2] = mark;
	}
	strcpy(buflocal + indent, s);
	addstrlist(SP, buflocal);
	free(buflocal);
}

void
spunit(struct strlist ** SP, const struct faction * f, const unit * u, int indent,
       int mode)
{
	int dh;
#ifdef USE_UGROUPS
	ugroup *ug = findugroup(u);

	if(ug) {
		if(is_ugroupleader(u, ug)) {
			dh = bufunit_ugroupleader(f, u, indent, mode);
		} else {
			return;
		}
	} else
#endif
		dh = bufunit(f, u, indent, mode);
	lparagraph(SP, buf, indent, (char) ((u->faction == f) ? '*' : (dh ? '+' : '-')));
}

void
spy_message(int spy, unit *u, unit *target)
{
	const char *c;
	struct region * r = u->region;

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
					scat(skillname(sk, u->faction->locale));
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
					scat(skillname(sk, u->faction->locale));
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

