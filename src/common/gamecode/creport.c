/* vi: set ts=2:
 *
 *	$Id: creport.c,v 1.9 2001/02/19 14:19:24 corwin Exp $
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
#include <eressea.h>
#include "creport.h"

/* modules include */
#include <modules/score.h>

/* attributes include */
#include <attributes/follow.h>

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
#include <render.h>
#include <reports.h>
#include <ship.h>
#include <skill.h>
#include <teleport.h>
#include <unit.h>

/* util includes */
#include <goodies.h>

/* libc includes */
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define C_REPORT_VERSION 55
#define NEWBLOCKS (C_REPORT_VERSION>=35)

#define ENCODE_SPECIAL 1

extern const char *directions[];
extern const char *spelldata[];
extern int quiet;

void
cr_output_str_list(FILE * F, const char *title, const strlist * S, faction * f)
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

#define CTMAXHASH 511
struct crtype {
	struct crtype * nexthash;
	messagetype * mt;
} * crtypes[CTMAXHASH];

struct crtype * free_crtypes;

#include "objtypes.h"

static void
print_curses(FILE * F, void * obj, typ_t typ, attrib *a, int self)
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
		}
		a = a->next;
	}
}

void
creport_cleanup(void)
{
	while (free_crtypes) {
		struct crtype * ct = free_crtypes->nexthash;
		free_crtypes = ct;
	}
}

static void
render_message(FILE * f, faction * receiver, message * m)
{
	int i;
	messagetype * mt = m->type;
	struct entry * e = mt->entries;

	if (m->receiver && receiver!=m->receiver) return;
	if (m->receiver && get_msglevel(receiver->warnings, receiver->msglevels, m->type) < msg_level(m))
		return;
	fprintf(f, "MESSAGE %u\n", receiver->index++);
	fprintf(f, "%u;type\n", mt->hashkey);
	if (receiver->options & want(O_DEBUG)) {
		fprintf(f, "%d;level\n", mt->level);
	}
#define RENDER_CR
#ifdef RENDER_CR
	fprintf(f, "\"%s\";rendered\n", render(m, receiver->locale));
#endif
	for (i=0;i!=mt->argc;++i) {
		switch (e->type) {
			case IT_RESOURCE:
				fprintf(f, "\"%s\";%s\n", resname((resource_t)m->data[i], 0), e->name);
				break;
			case IT_RESOURCETYPE:
				fprintf(f, "\"%s\";%s\n", locale_string(receiver->locale, resourcename((const resource_type *)m->data[i], 1)), e->name);
				break;
			case IT_STRING:
				fprintf(f, "\"%s\";%s\n", (char*)m->data[i], e->name);
				break;
			case IT_BUILDING: {
				building * b = (building*)m->data[i];
				fprintf(f, "%d;%s\n", b->no, e->name);
				break;
			}
			case IT_SHIP: {
				ship * s = (ship*)m->data[i];
				fprintf(f, "%d;%s\n", s->no, e->name);
				break;
			}
			case IT_UNIT: {
				unit * u = (unit*)m->data[i];
				/* TODO: Wilder Hack */
				if ((int)u > 1) fprintf(f, "%d;%s\n", u->no, e->name);
				break;
			}
			case IT_SKILL: {
				skill_t sk = (skill_t)m->data[i];
				assert(sk!=NOSKILL && sk<MAXSKILLS);
				fprintf(f, "\"%s\";%s\n", skillnames[sk], e->name);
				break;
			}
			case IT_FACTION: {
				faction * x = (faction*)m->data[i];
				fprintf(f, "%d;%s\n", x->no, e->name);
				break;
			}
			case IT_REGION: {
				region * r = (region*)m->data[i];
				if (!r) break;
				fprintf(f, "\"%d,%d\";%s\n", region_x(r, receiver), region_y(r, receiver), e->name);
				break;
			}
			default:
				fprintf(f, "%d;%s\n", (int)m->data[i], e->name);
				break;
		}
		e = e->next;
	}
}

static void
render_messages(FILE * f, void * receiver, message * msgs)
{
	message * m;
	for (m=msgs;m;m=m->next) {
		render_message(f, receiver, m);
	}
}

static void
cr_output_messages(FILE * F, message * msgs, faction * f)
{
	message * m;
	if (!msgs) return;
/*	fprintf(F, "%s\n", title); */
	for (m=msgs;m;m=m->next) {
		static messagetype * last = NULL;
		messagetype * mt = m->type;
#ifdef MSG_LEVELS
		if (get_msglevel(f->warnings, f->msglevels, mt) < m->level) continue;
#endif
		if (mt!=last && (!m->receiver || f==m->receiver)) {
			unsigned int index = mt->hashkey % CTMAXHASH;
			struct crtype * ct = crtypes[index];
			last = mt;
			while (ct && ct->mt->hashkey!=mt->hashkey) ct=ct->nexthash;
			if (!ct) {
				if (free_crtypes) {
					ct = free_crtypes;
					free_crtypes = free_crtypes->nexthash;
				} else {
					ct = calloc(1, sizeof(struct crtype));
				}
				ct->mt = mt;
				ct->nexthash = crtypes[index];
				crtypes[index] = ct;
			}
		}
	}
	render_messages(F, f, msgs);
}

extern void rendercr(FILE * f, messagetype * mt, const locale * lang);

static void
report_crtypes(FILE * f, locale * lang)
{
	int i;
	fputs("MESSAGETYPES\n", f);
	for (i=0;i!=CTMAXHASH;++i) {
		struct crtype * ct;
		for (ct=crtypes[i];ct;ct=ct->nexthash) {
			rendercr(f, ct->mt, lang);
		}
	}
}

static void
reset_crtypes(void)
{
	int i;
	for (i=0;i!=CTMAXHASH;++i) {
		struct crtype * ct;
		struct crtype * next;
		for (ct=crtypes[i];ct;ct=next) {
			next = ct->nexthash;
			ct->nexthash=free_crtypes;
			free_crtypes=ct;
		}
		crtypes[i] = NULL;
	}
}

/* prints a building */
static void
cr_output_buildings(FILE * F, building * b, unit * u, int fno, faction *f)
{
	const building_type * type = b->type;
	unit * bo = buildingowner(b->region, b);

	assert(b);
	fprintf(F, "BURG %d\n", b->no);
	if (!u || u->faction != f) {
		const attrib * a = a_find(b->attribs, &at_icastle);
		if (a) type = ((icastle_data*)a->data.v)->type;
	}
	fprintf(F, "\"%s\";Typ\n", buildingtype(b, b->size, f->locale));
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
cr_output_ship(FILE * F, ship * s, unit * u, int fcaptain, faction * f, region * r)
{
	unit *u2;
	int w = 0;
	assert(s);
	fprintf(F, "SCHIFF %d\n", s->no);
	fprintf(F, "\"%s\";Name\n", s->name);
	if (strlen(s->display))
		fprintf(F, "\"%s\";Beschr\n", s->display);
	fprintf(F, "\"%s\";Typ\n", s->type->name[0]);
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
	if (u && (u->faction == f || f->race == RC_ILLUSION)) {
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
cr_output_unit(FILE * F, region * r,
	       faction * f,	/* observers faction */
	       unit * u, int mode)
{
	/* Race attributes are always plural and item attributes always
	 * singular */
	const item_type * lasttype;
	int pr;
	item *itm, *show;
	boolean itemcloak = is_cursed(u->attribs, C_ITEMCLOAK, 0);
	building * b;
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

	if ((u->faction == f || f->race == RC_ILLUSION) || !fval(u, FL_PARTEITARNUNG)) {
#ifdef GROUPS
		if (u->faction == f) {
			const attrib * a = a_find(u->attribs, &at_group);
			if (a) {
				const group * g = (const group*)a->data.v;
				fprintf(F, "%d;gruppe\n", g->gid);
			}
		}
#endif
		fprintf(F, "%d;Partei\n", u->faction->no);
	}

	if(u->faction != f && a_fshidden
			&& a_fshidden->data.ca[0] == 1 && effskill(u, SK_STEALTH) >= 5) {
		fprintf(F, "-1;Anzahl\n");
	} else {
		fprintf(F, "%d;Anzahl\n", u->number);
	}

	fprintf(F, "\"%s\";Typ\n", race[u->irace].name[1]);
	if (u->irace != u->race && u->faction==f) {
		fprintf(F, "\"%s\";wahrerTyp\n", race[u->race].name[1]);
	}

	if (u->building)
		fprintf(F, "%d;Burg\n", u->building->no);
	if (u->ship)
		fprintf(F, "%d;Schiff\n", u->ship->no);
	if (getguard(u))
		fprintf(F, "%d;bewacht\n", getguard(u)?1:0);
	if ((b=usiege(u))!=NULL)
		fprintf(F, "%d;belagert\n", b->no);
	if (fval(u, FL_PARTEITARNUNG))
		fprintf(F, "%d;Parteitarnung\n", fval(u, FL_PARTEITARNUNG));

	/* additional information for own units */
	if (u->faction == f || f->race == RC_ILLUSION) {
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
#ifdef OLD_SILBER
		if (i)
			fprintf(F, "%d;Silber\n", i);
#endif
		if (u->status)
			fprintf(F, "%d;Kampfstatus\n", u->status);
		i = u_geteffstealth(u);
		if (i >= 0)
			fprintf(F, "%d;Tarnung\n", i);
		c = uprivate(u);
		if (c)
			fprintf(F, "\"%s\";privat\n", c);
		c = hp_status(u);
		if (c && *c && (u->faction == f || f->race == RC_ILLUSION))
			fprintf(F, "\"%s\";hp\n", c);
		if (fval(u, FL_HUNGER) && (u->faction == f))
			fputs("1;hunger\n", F);
		if (is_mage(u)) {
			fprintf(F, "%d;Aura\n", get_spellpoints(u));
			fprintf(F, "%d;Auramax\n", max_spellpoints(u->region,u));
		}

		/* default commands */
		fprintf(F, "COMMANDS\n");
		if(u->lastorder[0]) fprintf(F, "\"%s\"\n", u->lastorder);
		for (S = u->orders; S; S = S->next) {
			switch (igetkeyword(S->s)) {
			case K_LIEFERE:
			case K_DEFAULT:
			case K_RESERVE:
			case K_KOMMENTAR:
			case K_BUY:
			case K_SELL:
				fprintf(F, "\"%s\"\n", S->s);
			}
		}

		/* talents */
		pr = 0;
		for (sk = 0; sk != MAXSKILLS; ++sk)
			if (get_skill(u, sk)) {
				if (!pr) {
					pr = 1;
					fprintf(F, "TALENTE\n");
				}
#ifdef NOVISIBLESKILLPOINTS
				/* 0 ist nur der Kompatibilität wegen drin, rausnehmen */
				fprintf(F, "0 %d;%s\n", eff_skill(u, sk, r), skillnames[sk]);
#else
				fprintf(F, "%d %d;%s\n", get_skill(u, sk), eff_skill(u, sk, r), skillnames[sk]);
#endif
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
#ifdef OLD_SILBER
	/* silver information for other units */
	else {
		int i = get_money(u);
		if (i / u->number >= 5000)
			fprintf(F, "%d;Silber\n", 5000 * u->number);
		else if (i / u->number >= 500)
			fprintf(F, "%d;Silber\n", 500 * u->number);
	}
#endif
	/* items */
	pr = 0;
	if (f == u->faction || u->faction->race==RC_ILLUSION) {
		show = u->items;
	} else if (itemcloak==false && mode>=see_unit && !(a_fshidden
			&& a_fshidden->data.ca[1] == 1 && effskill(u, SK_STEALTH) >= 2)) {
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
					ishow = i_add(&show, i_new(itm->type));
					ishow->number = itm->number;
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
		fprintf(F, "%d;%s\n", in, locale_string(NULL, ic));
	}

	if ((u->faction == f || f->race == RC_ILLUSION) && u->botschaften)
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
show_active_spells(region * r)
{
	char fogwall[MAXDIRECTIONS];
#ifdef TODO /* alte Regionszauberanzeigen umstellen */
	unit *u;
	int env = 0;
#endif
	memset(fogwall, 0, sizeof(char) * MAXDIRECTIONS);

}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

extern int fcompare(const void *a, const void *b);


/* this is a copy of laws.c->find_address output changed. */
static void
cr_find_address(FILE * F, faction * uf)
{
	faction *f;
	void **fp;
	cvector *fcts = malloc(sizeof(cvector));

#if NEWBLOCKS
	cv_init(fcts);
	for (f = factions; f; f = f->next) {
		if (f->no != 0 && kann_finden(uf, f) != 0) {
			cv_pushback(fcts, f);
		}
	}
	v_sort(fcts->begin, fcts->end, fcompare);

	/* FIND orders - diese duerfen erst ablaufen, nachdem alle ihre Adresse
	 * gesetzt haben! */

	if (!quiet)
		puts(" - gebe Adressen heraus (CR)");
	for (fp = fcts->begin; fp != fcts->end; ++fp) {
		f = *fp;
		if (uf!=f && f->no != 0 && kann_finden(uf, f) != 0) {
			fprintf(F, "PARTEI %d\n", f->no);
			fprintf(F, "\"%s\";Parteiname\n", f->name);
			fprintf(F, "\"%s\";email\n", f->email);
			fprintf(F, "\"%s\";banner\n", f->banner);
		}
	}
	free(cv_kill(fcts));
#endif
}
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =  */

static void
cr_reportspell(FILE * F, spellid_t id)
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
			fprintf(F, "%d %d;%s\n", itemanz, costtyp == SPC_LEVEL || costtyp == SPC_LINEAR, resname(res, 0));
		}
	}
}

static unsigned int
encode_region(faction * f, region * r) {
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

/* main function of the creport. creates the header and traverses all regions */
void
report_computer(FILE * F, faction * f)
{
	int i;
	region *r;
	building *b;
	ship *sh;
	unit *u;
	seen_region * sd;
	const attrib * a;

	int maxmining;
	/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
	/* initialisations, header and lists */

	if (quiet) {
		printf(" CR");
		fflush(stdout);
	}
	else printf(" - schreibe Computerreport\n");
	assert(crtypes[0]==NULL);
	fprintf(F, "VERSION %d\n", C_REPORT_VERSION);
	fprintf(F, "\"%s\";Spiel\n", global.gamename);
	fprintf(F, "\"%s\";Konfiguration\n", "Standard");
	fprintf(F, "\"%s\";Koordinaten\n", "Hex");
	fprintf(F, "%d;Basis\n", 36);
	fprintf(F, "%d;Umlaute\n", 1);
	fprintf(F, "%d;Runde\n", turn);
	fputs("2;Zeitalter\n", F);
	fprintf(F, "PARTEI %d\n", f->no);
/*	fprintf(F, "\"%s\";Passwort\n", f->passw); */
	fprintf(F, "\"%s\";locale\n", locale_name(f->locale));
	fprintf(F, "%d;Optionen\n", f->options);
	if (f->options & want(O_SCORE) && f->age>DISPLAYSCORE) {
		fprintf(F, "%d;Punkte\n", f->score);
		fprintf(F, "%d;Punktedurchschnitt\n", average_score_of_age(f->age, f->age / 24 + 1));
	}
	fprintf(F, "\"%s\";Typ\n", race[f->race].name[1]);
	fprintf(F, "%d;Rekrutierungskosten\n", race[f->race].rekrutieren);
	fprintf(F, "%d;Anzahl Personen\n", count_all(f));
	fprintf(F, "\"%s\";Magiegebiet\n", neue_gebiete[f->magiegebiet]);

	if (f->race == RC_HUMAN) {
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
#ifdef GROUPS
	{
		group * g;
		for (g=f->groups;g;g=g->next) {
			fprintf(F, "GRUPPE %d\n", g->gid);
			fprintf(F, "\"%s\";name\n", g->name);
			show_allies(F, g->allies);
		}
	}
#endif

	f->index = 0;
	cr_output_str_list(F, "FEHLER", f->mistakes, f);
	cr_output_messages(F, f->msgs, f);
	{
		struct bmsg * bm;
		for (bm=f->battles;bm;bm=bm->next) {
			fprintf(F, "BATTLE %d %d\n", region_x(bm->r, f), region_y(bm->r, f));
			cr_output_messages(F, bm->msgs, f);
		}
	}

	cr_find_address(F, f);
	a = a_find(f->attribs, &at_reportspell);
	while (a) {
		cr_reportspell(F, (spellid_t)a->data.i);
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
		fprintf(F, "\"%s\";Name\n", locale_string(f->locale, ch));
		fprintf(F, "%d;Stufe\n", ptype->level);
		fprintf(F, "\"%s\";Beschr\n", ptype->text);
		fprintf(F, "ZUTATEN\n");

		while (m->number) {
			fprintf(F, "\"%s\"\n", locale_string(f->locale, resourcename(oldresourcetype[m->type], 0)));
			m++;
		}
	}
	/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
	 * = = = */
	/* traverse all regions */
	for (sd = seen; sd!=NULL; sd = sd->next) {
		int modifier = 0;
		const char * tname;

		r = sd->r;

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

		fprintf(F, "\"%s\";Terrain\n", locale_string(f->locale, tname));
		if (sd->mode != see_neighbour)
		{
			int g = 0;
			direction_t d;
			if (r->display && strlen(r->display))
				fprintf(F, "\"%s\";Beschr\n", r->display);
			if (landregion(rterrain(r))) {
				fprintf(F, "%d;Bauern\n", rpeasants(r));
				fprintf(F, "%d;Pferde\n", rhorses(r));
				fprintf(F, "%d;Baeume\n", rtrees(r));
				if (fval(r, RF_MALLORN) && rtrees(r))
					fprintf(F, "1;Mallorn\n");

				if (sd->mode>=see_unit) {
					struct demand * dmd = r->land->demands;
					if (rterrain(r) == T_MOUNTAIN || rterrain(r) == T_GLACIER) {
						maxmining = 0;
						for (u = r->units; u; u = u->next) {
							int s = eff_skill(u, SK_MINING, r);
							if (u->faction == f || f->race == RC_ILLUSION)
								maxmining = max(maxmining, s);
						}
						if (maxmining >= 4)
							fprintf(F, "%d;Eisen\n", riron(r));
						if (maxmining >= 7)
							fprintf(F, "%d;Laen\n", rlaen(r));
					}
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
					/* trade */
					if(rpeasants(r)/TRADE_FRACTION > 0) {
						fputs("PREISE\n", F);
						while (dmd) {
								fprintf(F, "%d;%s\n", (dmd->value
										  ? dmd->value*dmd->type->price
										  : -dmd->type->price),
										  locale_string(f->locale, resourcename(dmd->type->itype->rtype, 0)));
							dmd=dmd->next;
						}
					}
				}
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
			if (sd->mode==see_unit && r->planep && r->planep->id == 1 && !is_cursed(r->attribs, C_ASTRALBLOCK, 0))
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
				boolean see = false;
				const attrib * ru;
				/* show units pulled throuth region */
				for (ru = a_find(r->attribs, &at_travelunit); ru; ru = ru->nexttype) {
					unit * u = (unit*)ru->data.v;
					if (cansee_durchgezogen(f, r, u, 0) && in_region(r, u) == 0) {
						if (u->ship && !fval(u, FL_OWNER))
							continue;
						if (!see) fprintf(F, "DURCHREISE\n");
						see = true;
						if (u->ship)
							fprintf(F, "\"%s\"\n", shipname(u->ship));
						else
							fprintf(F, "\"%s\"\n", unitname(u));
					}
				}
			}
			cr_output_messages(F, r->msgs, f);
			/* buildings */
			for (b = rbuildings(r); b; b = b->next) {
				int fno = -1;
				u = buildingowner(r, b);
				if (u && !fval(u, FL_PARTEITARNUNG))
					fno = u->faction->no;

				cr_output_buildings(F, b, u, fno, f);
			}

			/* ships */
			for (sh = r->ships; sh; sh = sh->next) {
				int fno = -1;
				u = shipowner(r, sh);
				if (u && !fval(u, FL_PARTEITARNUNG))
					fno = u->faction->no;

				cr_output_ship(F, sh, u, fno, f, r);
			}

			/* visible units */
			for (u = r->units; u; u = u->next) {
				boolean visible = true;
				switch (sd->mode) {
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
					cr_output_unit(F, r, f, u, sd->mode);
			}
		}			/* region traversal */
	}
	report_crtypes(F, f->locale);
	reset_crtypes();
}
