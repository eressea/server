/* vi: set ts=2:
 *
 *	$Id: reports.c,v 1.1 2001/01/25 09:37:57 enno Exp $
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

#include "unit.h"
#include "item.h"
#include "group.h"
#include "faction.h"
#include "building.h"
#include "karma.h"
#include "ship.h"
#include "race.h"
#include "magic.h"

/* util includes */
#include <base36.h>
#include <goodies.h>

/* libc includes */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/* attributes includes */
#include <attributes/follow.h>

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
	static char zText[PATH_MAX];
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

	if (p > 1.25)
		return "magisch gestärkt";
	if (p < 0.50)
		return "schwer verwundet";
	if (p < 0.75)
		return "verwundet";
	if (p < 0.99)
		return "erschöpft";

	return NULL;
}

void
report_item(const unit * owner, const item * i, const faction * viewer, const char ** name, const char ** basename, int * number)
{
	if (owner->faction == viewer) {
		if (name) *name = locale_string(viewer->locale, resourcename(i->type->rtype, ((i->number!=1)?GR_PLURAL:0)));
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
		if (name) *name = locale_string(viewer->locale, resourcename(i->type->rtype, NMF_APPEARANCE|((i->number!=1)?GR_PLURAL:0)));
		if (basename) *basename = resourcename(i->type->rtype, NMF_APPEARANCE);
		if (number) {
			const herb_type * htype = resource2herb(i->type->rtype);
			if (htype!=NULL) *number = 1;
			else *number = i->number;
		}
	}
}

int
bufunit(const faction * f, const unit * u, int indent,
       int mode)
{
	int i, dh;
	skill_t sk;
	int getarnt = fval(u, FL_PARTEITARNUNG);
	const char *c;
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

	if (!getarnt && (u->faction != f)) {
		scat(", ");
		scat(factionname(u->faction));
	} else {
#ifdef GROUPS
		if (u->faction==f) {
			attrib * a = a_find(u->attribs, &at_group);
			if (a) {
				group * g = (group*)a->data.v;
				scat(", ");
				scat(groupid(g, f));
			}
		}
#endif
		if (getarnt) scat(", parteigetarnt");
	}
	scat(", ");
	if(u->faction != f && a_fshidden
			&& a_fshidden->data.ca[0] == 1 && effskill(u, SK_STEALTH) >= 5) {
		scat("? ");
	} else {
		icat(u->number);
		scat(" ");
	}

	if (u->irace != u->race) {
		scat(race[u->irace].name[u->number != 1]);
		if (u->faction == f) {
			scat(" (");
			scat(race[u->race].name[u->number != 1]);
			scat(")");
		}
	} else {
		scat(race[u->race].name[u->number != 1]);
	}

	/* status */

	if (u->number && (u->faction == f || telepath_see || isbattle)) {
		scat(report_kampfstatus(u));
		c = hp_status(u);
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
			unit * u = (unit*)a->data.v;
			if (u) {
				scat(", folgt ");
				scat(itoa36(u->no));
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
			spskill(u, sk, &dh, 1);
		}
	}

	dh = 0;
	if (f == u->faction || telepath_see || u->faction->race==RC_ILLUSION) {
		show = u->items;
	} else if (!itemcloak && mode >= see_unit && !(a_fshidden
			&& a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 2)) {
		show = NULL;
		for (itm=u->items;itm;itm=itm->next) {
			item * ishow;
			const char * ic;
			int in;
			report_item(u, itm, f, NULL, &ic, &in);
			if (ic && *ic && in>0) {
				for (ishow = show; ishow; ishow=ishow->next) {
					const char * sc;
					int sn;
					if (ishow->type==itm->type) sc=ic;
					else report_item(u, ishow, f, NULL, &sc, &sn);
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
		report_item(u, itm, f, &ic, NULL, &in);
		if (in==0 || ic==NULL) continue;
		scat(", ");

		if (!dh) {
			scat("hat: ");
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
						scat(", Zauber: ");
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
				scat(". Kampfzauber: ");
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
						scat("keiner");
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

	c = uprivate(u);
	if (u->faction == f && c) {
		scat(" (Bem: ");
		scat(c);
		scat(")");
	}

	dh=0;
	if (!getarnt && f && f->allies) {
		ally *sf;
		for (sf = f->allies; sf && !dh; sf = sf->next) {
			if (sf->status > 0 && sf->status <= HELP_ALL && sf->faction == u->faction) {
				dh = 1;
			}
		}
	}
	return dh;
}

void
spskill(const struct unit * u, skill_t sk, int *dh, int days)
{
	int i, d;
	if (!u->number)
		return;

	d = get_skill(u, sk);
	if (!d)
		return;

	scat(", ");

	if (!*dh) {
		scat("Talente: ");
		*dh = 1;
	}
	scat(skillnames[sk]);
	scat(" ");

	if (sk == SK_MAGIC){
		if (find_magetype(u) != M_GRAU){
			scat(magietypen[find_magetype(u)]);
			scat(" ");
		}
	}

	i = u_geteffstealth(u);
	if (sk == SK_STEALTH && i>=0) {
		icat(i);
		scat("/");
	}
	icat(effskill(u, sk));

	if (days) {
		assert(u->number);
		scat(" [");
		icat(d / u->number);
		scat("]");
	}
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
	int dh = bufunit(f, u, indent, mode);
	assert(SP);
	lparagraph(SP, buf, indent, (char) ((u->faction == f) ? '*' : (dh ? '+' : '-')));
}
