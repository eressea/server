/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
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

/* misc includes */
#include <attributes/key.h>
#include <items/questkeys.h>
#include <items/catapultammo.h>
#include <modules/xecmd.h>
#ifdef ALLIANCES
#include <modules/alliance.h>
#endif

/* gamecode includes */
#include <economy.h>

/* kernel includes */
#include <border.h>
#include <building.h>
#include <ship.h>
#include <faction.h>
#include <item.h>
#include <magic.h>
#include <message.h>
#include <monster.h>
#include <movement.h>
#include <names.h>
#include <pathfinder.h>
#include <plane.h>
#include <pool.h>
#include <race.h>
#include <region.h>
#include <reports.h>
#include <resources.h>
#include <skill.h>
#include <teleport.h>
#include <unit.h>
#include <spell.h>
#include <alchemy.h>
#include <study.h>

/* util includes */
#include <attrib.h>
#include <language.h>
#include <base36.h>
#include <log.h>
#include <cvector.h>
#include <event.h>
#include <goodies.h>
#include <resolve.h>
#include <sql.h>
#include <vset.h>

/* libc includes */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/key.h>

#undef XMAS1999
#undef XMAS2000
#undef XMAS2001
#undef XMAS2002

extern void reorder_owners(struct region * r);

static int
curse_emptiness(void)
{
	const curse_type * ct = ct_find("godcursezone");
	region * r;
	for (r=regions;r!=NULL;r=r->next) {
		unit * u = r->units;
		if (r->land==NULL) continue;
		if (fval(r, RF_CHAOTIC)) continue;
		if (r->terrain==T_GLACIER) continue;
		if (r->age<=200) continue;
		if (get_curse(r->attribs, ct)) continue;
		while (u && u->faction->no==MONSTER_FACTION) u=u->next;
		if (u==NULL) fset(r, FL_MARK);
	}
	for (r=regions;r!=NULL;r=r->next) {
		if (fval(r, FL_MARK)) {
			direction_t d;
			for (d=0;d!=MAXDIRECTIONS;++d) {
				region * rn = rconnect(r,d);
				if (rn==NULL) continue;
				if (fval(rn, FL_MARK) || get_curse(rn->attribs, ct)) {
					break;
				}
			}
			if (d!=MAXDIRECTIONS) {
				curse * c = create_curse(NULL, &r->attribs, ct,
										 100, 100, 0, 0);
				curse_setflag(c, CURSE_ISNEW|CURSE_IMMUNE);
			}
			freset(r, FL_MARK);
		}
	}
	return 0;
}

void
french_testers(void)
{
	faction * f = factions;
	const struct locale * french = find_locale("fr");
	while (f!=NULL) {
		if (f->locale==french) fset(f, FFL_NOTIMEOUT);
		f = f->next;
	}
}

static void
verify_owners(boolean bOnce)
{
	region * r;

	for (r=regions;r;r=r->next) {
		unit * u;
		boolean bFail = false;

		for (u=r->units;u;u=u->next) {
			if (u->building) {
				unit * bo = buildingowner(r, u->building);
				if (!fval(bo, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist Besitzer von %s, hat aber UFL_OWNER nicht.\n", unitname(bo), buildingname(u->building)));
					bFail = true;
					if (bOnce) break;
				}
				if (bo!=u && fval(u, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist NICHT Besitzer von %s, hat aber UFL_OWNER.\n", unitname(u), buildingname(u->building)));
					bFail = true;
					if (bOnce) break;
				}
			}
			if (u->ship) {
				unit * bo = shipowner(r, u->ship);
				if (!fval(bo, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist Besitzer von %s, hat aber UFL_OWNER nicht.\n", unitname(bo), shipname(u->ship)));
					bFail = true;
					if (bOnce) break;
				}
				if (bo!=u && fval(u, UFL_OWNER)) {
					log_error(("[verify_owners] %u ist NICHT Besitzer von %s, hat aber UFL_OWNER.\n", unitname(u), shipname(u->ship)));
					bFail = true;
					if (bOnce) break;
				}
			}
		}
		if (bFail) reorder_owners(r);
	}
}

/* make sure that this is done only once! */
#define do_once(magic, fun) \
{ \
	attrib * a = find_key(global.attribs, atoi36(magic)); \
	if (!a) { \
		log_warning(("[do_once] a unique fix %d=\"%s\" was applied.\n", atoi36(magic), magic)); \
		if (fun == 0) a_add(&global.attribs, make_key(atoi36(magic))); \
	} \
}

int
warn_items(void)
{
	boolean found = 0;
	region * r;
	const item_type * it_money = it_find("money");
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			item * itm;
			for (itm=u->items;itm;itm=itm->next) {
				if (itm->number>100000 && itm->type!=it_money) {
					found = 1;
					log_error(("Einheit %s hat %u %s\n",
						unitid(u), itm->number,
						resourcename(itm->type->rtype, 0)));
				}
			}
		}
	}
	return found;
}

static boolean
kor_teure_talente(unit *u)
{
	if(effskill(u, SK_TACTICS) >= 1 ||
		 effskill(u, SK_MAGIC) >= 1 ||
		 effskill(u, SK_ALCHEMY) >= 1 ||
		 effskill(u, SK_HERBALISM) >= 1 ||
		 effskill(u, SK_SPY) >= 1) {
				return true;
	}
	return false;
}

static void
no_teurefremde(boolean convert)
{
	region *r;
	unit *u;

	for(r=regions;r;r=r->next) {
		for(u=r->units;u;u=u->next) {
			if (u->faction->no != MONSTER_FACTION
				&& playerrace(u->faction->race)
					&& is_migrant(u)
					&& kor_teure_talente(u))
			{
				log_printf("* Warnung, teurer Migrant: %s %s\n",
						unitname(u), factionname(u->faction));
				if(convert) {
					u->race = u->faction->race;
					u->irace = u->faction->race;
					u->faction->num_migrants -= u->number;
					sprintf(buf, "Die Götter segnen %s mit der richtigen Rasse",
							unitname(u));
					addmessage(0, u->faction, buf, MSG_MESSAGE, ML_IMPORTANT);
				}
			}
		}
	}
}

attrib_type at_roadfix = {
	"roadfix",
	DEFAULT_INIT,
	DEFAULT_FINALIZE,
	DEFAULT_AGE,
#if RELEASE_VERSION<DISABLE_ROADFIX
	DEFAULT_WRITE,
#else
	NULL, /* disable writing them so they will disappear */
#endif
	DEFAULT_READ,
	ATF_UNIQUE
};

/* ************************************************************ */
/* GANZ WICHTIG! ALLE GEÄNDERTEN SPRÜCHE NEU ANZEIGEN */
/* GANZ WICHTIG! FÜGT AUCH NEUE ZAUBER IN DIE LISTE DER BEKANNTEN EIN */
/* ************************************************************ */
static void
show_newspells(void)
{
  region *r;
  /* Alle geänderten Zauber in das array newspellids[]. mit SPL_NOSPELL
  * terminieren */

  spellid_t newspellids[] = {
    SPL_NOSPELL
  };

  /* die id's der neuen oder veränderten Sprüche werden in newspellids[]
  * abgelegt */

  for(r=regions; r; r=r->next) {
    unit *u;
    for(u=r->units;u;u=u->next) {
      sc_mage *m = get_mage(u);
      if (u->faction->no == MONSTER_FACTION) continue;
      if (m != NULL) {
        int i;

        if (m->magietyp == M_GRAU) continue;

        for (i = 0; newspellids[i] != SPL_NOSPELL; i++) {
          spell *sp = find_spellbyid(newspellids[i]);

          if (!sp) continue;

          if (m->magietyp == sp->magietyp || has_spell(u, sp)) {
            attrib * a = a_find(u->faction->attribs, &at_reportspell);
            while (a && a->data.i != sp->id) a = a->nexttype;
            if (!a) {
              /* spell is not being shown yet. if seen before, remove to show again */
              a = a_find(u->faction->attribs, &at_seenspell);
              while (a && a->data.i != sp->id) a = a->nexttype;
              if (a) a_remove(&u->faction->attribs, a);
            }
          }
        }
      }
    }
  }
}

static int
fix_foreign(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		struct locale * lang;
		for (lang=locales;lang;lang=nextlocale(lang)) {
			const char * udefault = LOC(lang, "unitdefault");
			size_t udlen = strlen(udefault);
			unit * u;
			for (u=r->units;u;u=u->next) {
				size_t unlen = strlen(u->name);
				if (unlen<udlen) continue;
				if (strncmp(u->name, udefault, udlen)==0) {
					fset(u, FL_UNNAMED);
				}
			}
		}
	}
	return 0;
}

extern plane * arena;

static void
fix_age(void)
{
	faction * f;
	const race * oldorc = rc_find("orc");
	const race * uruk = rc_find("uruk");
	for (f=factions;f;f=f->next) {
		if (f->no!=MONSTER_FACTION && playerrace(f->race)) continue;
		if (f->race==oldorc) f->race= uruk;
		else if (f->age!=turn) {
			log_printf("Alter von Partei %s auf %d angepasst.\n", factionid(f), turn);
			f->age = turn;
		}
	}
}

#if 1
static int
count_demand(const region *r)
{
	struct demand *dmd;
	int c = 0;
	if (r->land) {
		for (dmd=r->land->demands;dmd;dmd=dmd->next) c++;
	}
	return c;
}
#endif

static int
recurse_regions(region * r, region_list **rlist, boolean(*fun)(const region * r))
{
	if (!fun(r)) return 0;
	else {
		int len = 0;
		direction_t d;
		region_list * rl = calloc(sizeof(region_list), 1);
		rl->next = *rlist;
		rl->data = r;
		(*rlist) = rl;
		fset(r, FL_MARK);
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * nr = rconnect(r, d);
			if (nr && !fval(nr, FL_MARK)) len += recurse_regions(nr, rlist, fun);
		}
		return len+1;
	}
}


#if 1
static int maxluxuries = 0;

static boolean
f_nolux(const region * r)
{
	if (r->land && count_demand(r) != maxluxuries) return true;
	return false;
}

static int
fix_demand_region(region *r)
{
	region_list *rl, *rlist = NULL;
	static const luxury_type **mlux = 0, ** ltypes;
	const luxury_type *sale = NULL;
	int maxlux = 0;

	recurse_regions(r, &rlist, f_nolux);
	if (mlux==0) {
		int i = 0;
		if (maxluxuries==0) for (sale=luxurytypes;sale;sale=sale->next) {
			maxluxuries++;
		}
		mlux = (const luxury_type **)gc_add(calloc(maxluxuries, sizeof(const luxury_type *)));
		ltypes = (const luxury_type **)gc_add(calloc(maxluxuries, sizeof(const luxury_type *)));
		for (sale=luxurytypes;sale;sale=sale->next) {
			ltypes[i++] = sale;
		}
	}
	else {
		int i;
		for (i=0;i!=maxluxuries;++i) mlux[i] = 0;
	}
	for (rl=rlist;rl;rl=rl->next) {
		region * r = rl->data;
		direction_t d;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * nr = rconnect(r, d);
			if (nr && nr->land && nr->land->demands) {
				struct demand * dmd;
				for (dmd = nr->land->demands;dmd;dmd=dmd->next) {
					if (dmd->value == 0) {
						int i;
						for (i=0;i!=maxluxuries;++i) {
							if (mlux[i]==NULL) {
								maxlux = i;
								mlux[i] = dmd->type;
								break;
							} else if (mlux[i]==dmd->type) {
								break;
							}
						}
						break;
					}
				}
			}
		}
		freset(r, FL_MARK); /* undo recursive marker */
	}
	if (maxlux<2) {
		int i;
		for (i=maxlux;i!=2;++i) {
			int j;
			do {
				int k = rand() % maxluxuries;
				mlux[i] = ltypes[k];
				for (j=0;j!=i;++j) {
					if (mlux[j]==mlux[i]) break;
				}
			} while (j!=i);
		}
		maxlux=2;
	}
	for (rl=rlist;rl;rl=rl->next) {
		region * r = rl->data;
		if (!fval(r, RF_CHAOTIC)) log_warning(("fixing demand in %s\n", regionname(r, NULL)));
		setluxuries(r, mlux[rand() % maxlux]);
	}
	while (rlist) {
		rl = rlist->next;
		free(rlist);
		rlist = rl;
	}
	return 0;
}
#endif

static void
fix_firewalls(void)
{
	region * r = regions;
	while (r) {
		direction_t d;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * r2 = rconnect(r, d);
			if (r2) {
				border * b = get_borders(r, r2);
				while (b) {
					if (b->type==&bt_firewall) {
						attrib * a = a_find(b->attribs, &at_countdown);
						if (a==NULL || a->data.i <= 0) {
							erase_border(b);
							log_warning(("firewall between regions %s and %s was bugged. removed.\n",
								regionid(r), regionid(r2)));
							b = get_borders(r, r2);
						} else {
							b = b->next;
						}
					} else {
						b = b->next;
					}
				}
			}
		}
		r = r->next;
	}
}

extern attrib * make_atgmcreate(const struct item_type * itype);
extern attrib * make_atpermissions(void);
extern struct attrib_type at_permissions;
extern struct attrib_type at_gmcreate;


static void
update_gms(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		attrib * permissions = a_find(f->attribs, &at_permissions);
		if (permissions) {
			const char * keys[] = { "gmgate", "gmmsgr", "gmkill", "gmmsgu", NULL };
			int k;
			item_t i;
			for (k=0;keys[k];++k) {
				add_key((attrib**)&permissions->data.v, atoi36(keys[k]));
			}
			for (i=I_GREATSWORD;i!=I_KEKS;++i) {
				attrib * a = a_find((attrib*)permissions->data.v, &at_gmcreate);
				while (a && a->data.v!=(void*)olditemtype[i]) a=a->nexttype;
				if (!a) a_add((attrib**)&permissions->data.v, make_atgmcreate(olditemtype[i]));
			}
		}
	}
}

#if 1
static int
fix_demand(void)
{
	region *r;
	const luxury_type *sale = NULL;

	if (maxluxuries==0) for (sale=luxurytypes;sale;sale=sale->next) ++maxluxuries;

	for (r=regions; r; r=r->next) {
		if (r->land!=NULL && r->land->peasants>=100 && count_demand(r) != maxluxuries) {
			fix_demand_region(r);
		}
	}
	return 0;
}
#endif

#include "group.h"
static void
fix_allies(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		group * g;
		for (g=f->groups;g;g=g->next) {
			ally ** ap=&g->allies;
			while (*ap) {
				ally * an, * a = *ap;
				for (an = a->next;an;an=an->next) {
					if (a->faction==an->faction) {
						*ap = a->next;
						free(a);
						break;
					}
				}
				if (an==NULL) ap = &(*ap)->next;
			}
		}
	}
}

#ifdef FUZZY_BASE36
extern boolean enable_fuzzy;
#endif

static void
fix_icastles(void)
{
	region * r;
	for (r=regions; r; r=r->next) {
		building * b;
		for (b=r->buildings; b; b=b->next) {
			attrib * a;
			const building_type * btype = bt_find("castle");
			icastle_data * data;
			a = a_find(b->attribs, &at_icastle);
			if (b->type!=bt_find("illusion") && !a) continue;

			if (!a) {
				/* attribut hat gefehle */
				a = a_add(&b->attribs, a_new(&at_icastle));
			}
			if (b->type!=bt_find("illusion")) {
				/* gebäudetyp war falsch */
				btype = b->type;
				b->type = bt_find("illusion");
			}
			data = (icastle_data*)a->data.v;
			if (data->time<=0) {
				/* zeit war kaputt oder abgelaufen */
				data->time = 1;
			}
			if (!data->type) {
				/* typ muss gesetzt werden, weil er nicht geladen wurde */
				data->type = btype;
			}
			if (data->building!=b) {
				/* rückwärtszeiger auf das gebäude reparieren */
				data->building=b;
			}
		}
	}
}

typedef struct stats_t {
	struct stats_t * next;
	const struct item_type * type;
	double number;
} stats_t;

static void 
s_change(stats_t** s, const struct item_type * type, int count)
{
	while (*s && (*s)->type!=type) s=&(*s)->next;
	if (*s==NULL) {
		*s = calloc(1, sizeof(stats_t));
		(*s)->type = type;
	}
	(*s)->number += count;
}

static stats_t * 
s_find(stats_t * s, const struct item_type * type)
{
	while (s && s->type!=type) s=s->next;
	return s;
}

static void
stats(void)
{
	FILE * F;
	stats_t * items = NULL;
	char zText[MAX_PATH];

	strcat(strcpy(zText, resourcepath()), "/stats");
	F = fopen(zText, "wt");
	if (F) {
		region * r;
		const item_type * itype;
		for (r=regions;r;r=r->next) {
			unit * u;
			item * itm;
			for (u=r->units;u;u=u->next) {

				for (itm=u->items;itm;itm=itm->next) {
					if (itm->number>10000000) {
						log_error(("unit %s has %d %s\n", unitname(u), itm->number, resourcename(itm->type->rtype, 0)));
						/* itm->number=1; */
					}
					s_change(&items, itm->type, itm->number);
				}
			}
		}
		for (itype=itemtypes;itype;itype=itype->next) {
			stats_t * itm = s_find(items, itype);
			if (itm && itm->number>0.0)
				fprintf(F, "%4.0f %s\n", itm->number, locale_string(NULL, resourcename(itype->rtype, 0)));
			else
				fprintf(F, "%4.0f %s\n", 0.0, locale_string(NULL, resourcename(itype->rtype, 0)));
		}
		fclose(F);
	} else {
		perror(zText);
	}

}

#if 0
static void
fix_herbs(void)
{
	const char * plain_herbs[] = {"Flachwurz", "Würziger Wagemut", "Eulenauge", "Grüner Spinnerich", "Blauer Baumringel", "Elfenlieb"};
	const herb_type * htypes[6];
	int herbs[6];
	int hneed[6];
	region *r;
	int i, hsum = 0, left = 0;

	for (i=0;i!=6;++i) {
		htypes[i] = resource2herb(finditemtype(plain_herbs[i], NULL)->rtype);
		herbs[i] = 0;
	}

	for (r=regions; r; r=r->next) if (rterrain(r) == T_PLAIN) {
		const herb_type *htype = rherbtype(r);
		if (htype==NULL) {
			htype = htypes[i];
			rsetherbtype(r, htype);
		}
		for (i=0;i!=6;++i) if (htypes[i]==htype) break;
		assert(i!=6);
		herbs[i]++;
		hsum++;
	}

	for (i=0;i!=6;++i) {
		int hwant = hsum / (6-i);
		hneed[i] = hwant - herbs[i];
		if (hneed[i]>0) left += hneed[i];
		hsum -= hwant;
	}

	for (r=regions; r; r=r->next) if (rterrain(r) == T_PLAIN) {
		const herb_type *htype = rherbtype(r);
		assert(htype);
		for (i=0;i!=6;++i) if (htypes[i]==htype) break;
		assert(i!=6);
		if (hneed[i]<0) {
			int p;
			int k = rand() % left;
			for (p=0;p!=6;++p) if (hneed[p]>0) {
				k-=hneed[p];
				if (k<0) break;
			}
			assert(p!=6);
			hneed[p]--;
			hneed[i]++;
			left--;
			rsetherbtype(r, htypes[p]);
		}
		hsum++;
	}

	for (i=0;i!=6;++i) herbs[i] = 0;

	for (r=regions; r; r=r->next) if (rterrain(r) == T_PLAIN) {
		const herb_type *htype = rherbtype(r);
		assert(htype);
		for (i=0;i!=6;++i) if (htypes[i]==htype) break;
		assert(i!=6);
		herbs[i]++;
	}
	for (i=0;i!=6;++i) {
		fprintf(stderr, "%s : %d\n", locale_string(NULL, resourcename(htypes[i]->itype->rtype, 0)), herbs[i]);
	}
}
#endif

#include <event.h>
#include <triggers/timeout.h>
#include <triggers/changerace.h>
#include <triggers/changefaction.h>
#include <triggers/createcurse.h>
#include <triggers/createunit.h>
#include <triggers/killunit.h>
#include <triggers/giveitem.h>

typedef struct handler_info {
	char * event;
	trigger * triggers;
} handler_info;


typedef struct timeout_data {
	trigger * triggers;
	int timer;
	variant trigger_data;
} timeout_data;

trigger *
get_timeout(trigger * td, trigger * tfind)
{
	trigger * t = td;
	while (t) {
		if (t->type==&tt_timeout) {
			timeout_data * tdata = (timeout_data *)t->data.v;
			trigger * tr = tdata->triggers;
			while (tr) {
				if (tr==tfind) break;
				tr=tr->next;
			}
			if (tr==tfind) break;
		}
		t=t->next;
	}
	return t;
}

#include <modules/gmcmd.h>

static int
secondfaction(faction * pf)
{
	unit * u = findunit(atoi36("5q9w"));
	if (u!=NULL) {
		plane * p = rplane(u->region);
		if (p!=NULL) {
			region * center = findregion((p->maxx-p->minx)/2, (p->maxy-p->miny)/2);
			if (center!=NULL) {
				gm_addfaction(pf->email, p, center);
				return 0;
			}
		}
	}
	return -1;
}

static void
update_gmquests(void)
{
	faction * f = findfaction(atoi36("gm04"));
	if (f) {
		unit * u = f->units;
		potion_t p;
		attrib * permissions = a_find(f->attribs, &at_permissions);

		if (u!=NULL) {
			plane * p = rplane(u->region);
			/* gm04 will keine Orks */
			if (p!=NULL) p->flags |= PFL_NOORCGROWTH;
			/* gm04 will keine Monster */
			if (p!=NULL) p->flags |= PFL_NOMONSTERS;
		}
		for (p=0;p!=MAX_POTIONS;++p) {
			attrib * a = a_find((attrib*)permissions->data.v, &at_gmcreate);
			while (a && a->data.v!=(void*)oldpotiontype[p]->itype) a=a->nexttype;
			if (!a) a_add((attrib**)&permissions->data.v, make_atgmcreate(oldpotiontype[p]->itype));
		}
		do_once("et02", secondfaction(f));
	}
	do_once("renm", fix_foreign());
}

#define TEST_LOCALES 0
#if TEST_LOCALES
static void
setup_locales(void)
{
	locale * lang = find_locale("en");
	faction * f = factions;
	while (f) {
		f->locale = lang;
		f = f->next;
	}
}
#endif

#include <triggers/shock.h>
#include <triggers/killunit.h>


#if RESOURCE_CONVERSION
extern struct attrib_type at_resources;
void
init_resourcefix(void)
{
	at_register(&at_resources);
}

#endif

#if GROWING_TREES
int
growing_trees(void)
{
	region *r;

	for(r=regions; r; r=r->next) {
		if(rtrees(r, 2)) {
			rsettrees(r, 1, rtrees(r, 2)/4);
			rsettrees(r, 0, rtrees(r, 2)/2);
		}
	}
	return 0;
}
#endif

#include <triggers/gate.h>
#include <triggers/unguard.h>
typedef struct gate_data {
	struct building * gate;
	struct region * target;
} gate_data;

static void
fix_gates(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		building * b;
		for (u=r->units;u;u=u->next) {
			trigger ** triggers = get_triggers(u->attribs, "timer");
			if (triggers) {
				trigger * t = *triggers;
				while (t && t->type!= &tt_gate) t=t->next;
				if (t!=NULL) {
					gate_data * gd = (gate_data*)t->data.v;
					struct building * b = gd->gate;
					struct region * rtarget = gd->target;
					if (r!=b->region) {
						add_trigger(&b->attribs, "timer", trigger_gate(b, rtarget));
						add_trigger(&b->attribs, "create", trigger_unguard(b));
						fset(b, BLD_UNGUARDED);
					}
				}
				remove_triggers(&u->attribs, "timer", &tt_gate);
				remove_triggers(&u->attribs, "create", &tt_unguard);
			}
		}
		for (b=r->buildings;b;b=b->next) {
			trigger ** triggers = get_triggers(b->attribs, "timer");
			if (triggers) {
				trigger * t = *triggers;
				while (t && t->type!= &tt_gate) t=t->next;
				if (t!=NULL) {
					gate_data * gd = (gate_data*)t->data.v;
					struct building * b = gd->gate;
					struct region * rtarget = gd->target;
					remove_triggers(&b->attribs, "timer", &tt_gate);
					remove_triggers(&b->attribs, "create", &tt_unguard);
					if (r!=b->region) {
						add_trigger(&b->attribs, "timer", trigger_gate(b, rtarget));
						add_trigger(&b->attribs, "create", trigger_unguard(b));
						fset(b, BLD_UNGUARDED);
					}
				}
			}
		}
	}
}

static int
fix_astralplane(void)
{
  plane * astralplane = get_astralplane();
  region * r;
  region_list * rlist = NULL;
  faction * monsters = findfaction(MONSTER_FACTION);

  if (astralplane==NULL || monsters==NULL) return 0;

  freset(astralplane, PFL_NOCOORDS);
  freset(astralplane, PFL_NOFEED);
  set_ursprung(monsters, astralplane->id, 0, 0);

  for (r=regions;r;r=r->next) if (rplane(r)==astralplane) {
    region * ra = r_standard_to_astral(r);
    if (ra==NULL) continue;
    if (r->terrain!=T_FIREWALL) continue;
    if (ra->terrain==T_ASTRALB) continue;
    if (ra->units!=NULL) {
      add_regionlist(&rlist, ra);
    }
    log_printf("protecting firewall in %s by blocking astral space in %s.\n", regionname(r, NULL), regionname(ra, NULL));
    terraform(ra, T_ASTRALB);
  }
  while (rlist!=NULL) {
    region_list * rnew = rlist;
    region * r = rnew->data;
    direction_t dir;
    rlist = rlist->next;
    for (dir=0;dir!=MAXDIRECTIONS;++dir) {
      region * rnext = rconnect(r, dir);
      if (rnext==NULL) continue;
      if (rnext->terrain!=T_ASTRAL) continue;
      while (r->units) {
        unit * u = r->units;
        move_unit(u, rnext, NULL);
        if (u->faction->no==MONSTER_FACTION) {
          set_number(u, 0);
        }
      }
      break;
    }
    if (r->units!=NULL) {
      unit * u;
      for (u=r->units;u;u=u->next) {
        if (u->faction->no!=MONSTER_FACTION) {
          log_error(("Einheit %s in %u, %u steckt im Nebel fest.\n", 
            unitname(u), r->x, r->y));
        }
      }
    }
    free(rnew);
  }
  return 0;
}

static int
warn_password(void)
{
	faction * f = factions;
	while (f) {
		boolean pwok = true;
		const char * c = f->passw;
		while (*c) {
			if (!isalnum(*c)) pwok = false;
			c++;
		}
		if (pwok == false) {
			ADDMSG(&f->msgs, msg_message("msg_errors", "string",
				"Dein Passwort enthält Zeichen, die bei der Nachsendung "
				"von Reports Probleme bereiten können. Bitte wähle ein neues "
				"Passwort, bevorzugt nur aus Buchstaben und Zahlen bestehend."));
		}
		f = f->next;
	}
	return 0;
}

extern border *borders[];

static void
fix_road_borders(void)
{
	border *delete[10000];
	int hash;
	int i = 0;

	for(hash=0; hash<BMAXHASH; hash++) {
		border * b;
    for (b=borders[hash];b;b=b->next) {
			if(b->type == &bt_road) {
				int x1, x2, y1, y2;
				region *r1, *r2;

				x1 = b->from->x;
				y1 = b->from->y;
				x2 = b->to->x;
				y2 = b->to->y;

				r1 = findregion(x1, y1);
				r2 = findregion(x2, y2);

				if(r1->land == NULL || r2->land == NULL
						|| terrain[r1->terrain].roadreq == 0
						|| terrain[r2->terrain].roadreq == 0) {
					delete[i] = b;
					i++;
				}
			}
		}
	}

	while(i>0) {
		i--;
		erase_border(delete[i]);
	}
}

#ifdef WDW_PHOENIX
static region *
random_land_region(void)
{
	region *r;
	int c = 0;

	/* count the available regions */
	for(r=regions; r; r=r->next) {
		if (r->land && rplane(r)==NULL) ++c;
	}

	/* choose one */
	c = rand()%c;

	/* this is a bit obfuscated, but should be correct */
	for(r=regions; c > 0; r=r->next) {
		if (r->land && rplane(r)==NULL) --c;
	}

	return(r);
}

static void
check_phoenix(void)
{
	const race * phoenix_race = rc_find("phoenix");
	unit * phoenix;
	region * r;
	faction *f;

	/* check if there is a phoenix in the world */
	for(f=factions; f; f=f->next) {
		unit *u;

		for(u=f->units; u; u=u->nextF) {
			if(u->race == phoenix_race) {
				return;
			}
		}
	}

	/* it is not, so we create it */
	r = random_land_region();
	phoenix = createunit(r, findfaction(MONSTER_FACTION), 1, phoenix_race);
	phoenix->name = strdup("Der Phönix");

	/* TODO: generate an appropriate region message */
}
#endif

static void
fix_dissolve(unit * u, int value, char mode)
{
  attrib * a = a_find(u->attribs, &at_unitdissolve);

  if (a!=NULL) return;
  a = a_add(&u->attribs, a_new(&at_unitdissolve));
  a->data.ca[0] = mode;
  a->data.ca[1] = (char)value;
  log_warning(("unit %s has race %s and no dissolve-attrib\n", unitname(u), rc_name(u->race, 0)));
}

static void
check_dissolve(void)
{
  region * r;
  for (r=regions;r!=NULL;r=r->next) {
    unit * u;
    for (u=r->units;u!=NULL;u=u->next) if (u->faction->no!=MONSTER_FACTION) {
      if (u->race==new_race[RC_STONEGOLEM]) {
        fix_dissolve(u, STONEGOLEM_CRUMBLE, 0);
        continue;
      } 
      if (u->race==new_race[RC_IRONGOLEM]) {
        fix_dissolve(u, IRONGOLEM_CRUMBLE, 0);
        continue;
      } 
      if (u->race==new_race[RC_PEASANT]) {
        fix_dissolve(u, 15, 1);
        continue;
      }
      if (u->race==new_race[RC_TREEMAN]) {
        fix_dissolve(u, 5, 2);
        continue;
      }
    }
  }
}

void
korrektur(void)
{
  check_dissolve();
	french_testers();
#if TEST_LOCALES
	setup_locales();
#endif
	fix_astralplane();
	fix_firewalls();
	fix_gates();
	update_gms();
	verify_owners(false);
	/* fix_herbtypes(); */
	/* In Vin3 können Parteien komplett übergeben werden. */
#ifdef ENHANCED_QUIT
	no_teurefremde(0);
#else
	no_teurefremde(1);
#endif
	fix_allies();
	update_gmquests(); /* test gm quests */
	/* fix_unitrefs(); */
	stats();
	warn_password();
	fix_road_borders();
	if (turn>1000) curse_emptiness(); /*** disabled ***/
#ifdef ALLIANCES
	/* init_alliances(); */
#endif
	/* seems something fishy is going on, do this just
	 * to be on the safe side:
	 */
	fix_demand();
	/* trade_orders(); */

	/* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
	 * Beschreibungen geändert haben */
	show_newspells();
	fix_age();

	/* Immer ausführen! Erschafft neue Teleport-Regionen, wenn nötig */
	create_teleport_plane();

#ifdef WDW_PHOENIX
	check_phoenix();
#endif

	if (global.data_version<TYPES_VERSION) fix_icastles();
#ifdef FUZZY_BASE36
	enable_fuzzy = true;
#endif
}

void
korrektur_end(void)
{
}

void
init_conversion(void)
{
}
