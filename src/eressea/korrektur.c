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
#include <eressea.h>

/* misc includes */
#include <attributes/key.h>
#include <modules/xmas2000.h>
#include <modules/xmas2001.h>
#include <modules/museum.h>

/* gamecode includes */
#include <creation.h>
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
#include <base36.h>
#include <cvector.h>
#include <event.h>
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

#undef  XMAS1999
#undef  XMAS2000
#undef  XMAS2001

extern void reorder_owners(struct region * r);

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
				if (!fval(bo, FL_OWNER)) {
					log_error(("[verify_owners] %u ist Besitzer von %s, hat aber FL_OWNER nicht.\n", unitname(bo), buildingname(u->building)));
					bFail = true;
					if (bOnce) break;
				}
				if (bo!=u && fval(u, FL_OWNER)) {
					log_error(("[verify_owners] %u ist NICHT Besitzer von %s, hat aber FL_OWNER.\n", unitname(u), buildingname(u->building)));
					bFail = true;
					if (bOnce) break;
				}
			}
			if (u->ship) {
				unit * bo = shipowner(r, u->ship);
				if (!fval(bo, FL_OWNER)) {
					log_error(("[verify_owners] %u ist Besitzer von %s, hat aber FL_OWNER nicht.\n", unitname(bo), shipname(u->ship)));
					bFail = true;
					if (bOnce) break;
				}
				if (bo!=u && fval(u, FL_OWNER)) {
					log_error(("[verify_owners] %u ist NICHT Besitzer von %s, hat aber FL_OWNER.\n", unitname(u), shipname(u->ship)));
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
	if (a) { \
		log_warning(("[do_once] a unique fix %d=\"%s\" was called a second time\n", atoi36(magic), magic)); \
	} else { \
		if (fun == 0) a_add(&global.attribs, make_key(atoi36(magic))); \
	} \
}

int
warn_items(void)
{
	boolean found = 0;
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			item * itm;
			for (itm=u->items;itm;itm=itm->next) {
				if (itm->number>1000000) {
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

#ifdef XMAS1999
#include "race.h"
#include "movement.h"

static void
santa_comes_to_town(void)
{
	unit * santa = ufindhash(atoi36("xmas"));
	unit * tans = ufindhash(atoi36("tans"));
	faction * f;
	region * r;

	if (!tans) r = findregion(0,0);
	else r = tans->region;
	if (!r) return;

	while (santa && santa->race!=RC_ILLUSION) {
		uunhash(santa);
		santa->no = newunitid();
		uhash(santa);
		santa = ufindhash(atoi36("xmas"));
	}
	if (!santa) {
		santa = createunit(r, findfaction(MONSTER_FACTION), 1, new_race[RC_ILLUSION]);
		uunhash(santa);
		santa->no = atoi36("xmas");
		uhash(santa);
	}
	santa->age = 2;
	santa->irace = new_race[RC_DAEMON];
	set_string(&santa->name, "Ein dicker Gnom mit einem Rentierschlitten");
	set_string(&santa->display, "hat: 6 Rentiere, Schlitten, Sack mit Geschenken, Kekse für Khorne");
	fset(santa, FL_TRAVELTHRU);

	for (f=factions;f;f=f->next) {
		unit * u;
		runit *ru;
		region * r;
		item_t i = (item_t) (rand() % 4 + I_KEKS);
		unit * senior = f->units;
		for (u = f->units; u; u=u->nextF) {
			if (senior->age < u->age) senior = u;
		}
		if (!senior) continue;
		r = senior->region;
		sprintf(buf, "%s gibt %d %s an %s.", unitname(santa), 1, itemdata[i].name[0], unitname(senior));
		change_item(senior, i, 1);
		addmessage(r, senior->faction, buf, MSG_COMMERCE, ML_INFO);

		sprintf(buf, "von %s: 'Ho ho ho. Frohe Weihnachten, und %s für %s.'", unitname(santa), itemdata[i].name[1], unitname(senior));
		addmessage(r, 0, buf, MSG_MESSAGE, ML_IMPORTANT);

		for (ru = r->durchgezogen; ru ; ru=ru->next)
			if (ru->unit==santa) break;
		if (!ru) {
			ru = calloc(1, sizeof(runit));
			ru->next = r->durchgezogen;
			ru->unit = santa;
			r->durchgezogen = ru;
		}
	}
}
#endif

#if 0
static void
init_hp(void)
{
	region *r;
	unit *u;

	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			if (u->hp == -1) {
				u->hp = unit_max_hp(u) * u->number;
			}
		}
	}
}

static void
convert(int x, int y, int *nx, int *ny)
{
	*ny = -y;
	*nx = x + (y + ((y>=0)?1:0))/2;
}
#endif

#if 0
static void
loesche_coasts(void)
{
	region *r;
	ship *sh;

	for(r=regions;r;r=r->next) {
		for(sh=r->ships;sh;sh=sh->next) {
			sh->coast = NODIRECTION;
		}
	}
}
#endif

#if 0
static void
create_underworld(void)

{
	int x, y;
	region *r;
	attrib *a;
	plane * hell = create_new_plane(-1, "Hölle", 100000, 100000, 100020, 100020, PFL_NONE);

	for(x=0;x<=20;x++) {
		for(y=0;y<=20;y++) {
			r = new_region(x+100000,y);
			r->planep = hell;
			if(x==0 || x == 20 || y == 0 || y == 20) {
				rsetterrain(r, T_FIREWALL);
			} else {
				rsetterrain(r, T_HELL);
			}
		}
	}

	/* Verbindungen schaffen */

	a = create_special_direction(findregion(4,4),100005,5,-1,
		"Ein dunkler Schlund fuehrt in die Tiefe der Erde",
		"Schlund");

	a = create_special_direction(findregion(100005,5),4,4,-1,
		"Ein hell erleuchteter Tunnel fuehrt nach oben",
		"Tunnel");
}
#endif

#if 0
static void
bename_dracoide(void)
{
	region *r;
	unit *u;

	for(r=regions;r;r=r->next) {
		for(u=r->units;u;u=u->next) {
			if(u->race == new_race[RC_DRACOID]) name_unit(u);
		}
	}
}
#endif

static void
fix_migrants(void) {
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			if (u->race == new_race[RC_HUMAN]) u->irace = u->race;
			if (u->irace!=u->race && (u->race->flags & RCF_SHAPESHIFT)==0) {
				log_warning(("[fix_migrants] %s ist ein %s, als %s getarnt\n", unitname(u), LOC(default_locale, rc_name(u->race, 0)), LOC(default_locale, rc_name(u->irace, 0))));
			}
		}
	}
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
			if(u->faction->no != MONSTER_FACTION
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

#if 0
static void
give_arena_gates(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		unit * u;
		region * r;
		unit * senior = f->units;
		if (f->age>2) return;
		for (u = f->units; u; u=u->nextF) {
			if (senior->age < u->age) senior = u;
			if (f->no==MONSTER_FACTION && get_item(u, I_ARENA_GATE)) return;
		}
		if (!senior) continue;
		r = senior->region;
		sprintf(buf, "Der Himmel über %s reißt auf und ein Licht aus den Wolken scheint auf %s. Ein Gesang wie von tausend Walküren ertönt, dazu Schlachtenlärm. Nach einer Weile endet die Vision, und %s hält ein %s in den Händen.", regionid(senior->region), unitname(senior), unitname(senior), itemdata[I_ARENA_GATE].name[0]);
		change_item(senior, I_ARENA_GATE, 1);
		addmessage(NULL, senior->faction, buf, MSG_MESSAGE, ML_IMPORTANT);
	}
}

static void
remove_impossible_dragontargets(void)
{
	region *r;
	for (r=regions;r;r=r->next) {
		unit *u;
		for (u=r->units;u;u=u->next) {
			attrib *a;
			if(u->faction->no != MONSTER_FACTION) continue;

			a = a_find(u->attribs, &at_targetregion);
			if (a!=NULL) {
				boolean cango = false;
				region * r2 = a->data.v;
				if(r2!=NULL) {
#ifdef NEW_PATH
					cango = path_exists(r, r2, DRAGON_RANGE, allowed_dragon);
#else
					cango = path_exists(r, r2, FLY|DRAGON_LIMIT);
#endif
				}
				if(!cango) {
					printf("Lösche Ziel von %s\n", unitname(u));
					a_remove(&u->attribs, a);
				}
			}
		}
	}
}

static void
fix_score_option(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		f->options &= ((1<<O_SCORE)-1);
		f->options &= ~(1<<O_MERIAN);
		f->options |= (1<<O_SCORE);
	}
}
#endif

#if 0
static void
fix_wounds(void)
{
	region *r;
	unit *u;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			int wounds = unit_max_hp(u)*u->number - u->hp;
			u->hp = max(1, u->hp - wounds);
		}
	}
}
#endif

#if 0
static void
fix_beached(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		if (r->land && rterrain(r)!=T_PLAIN) {
			ship * s;
			building *b;

			for(b=r->buildings; b; b=b->next) {
				if(b->typ == BT_HAFEN) break;
			}
			if(b == NULL) {
				for (s=r->ships;s;s=s->next) {
					if (s->coast!=NODIRECTION && s->type>SH_BOAT)
					{
						region * r2 = rconnect(r, s->coast);
						assert(rterrain(r2)==T_OCEAN || fval(r2, RF_CHAOTIC));
						move_ship(s, r, r2, NULL);
					}
				}
			}
		}
	}
}
#endif

#if 0
static void
fix_hungrydead(void)
{
	region *r;
	unit *u;

	for (r=regions;r;r=r->next) {
		for (u=r->units;u;u=u->next) {
			if(u->hp == 0) u->hp = 1;
		}
	}
}

static void
name_seaserpents(void)
{
	region *r;
	unit *u;

	for (r=regions;r;r=r->next) {
		for (u=r->units;u;u=u->next) {
			if(u->race == new_race[RC_SEASERPENT] && strncmp(u->name, "Nummer ", 7)) {
				set_string(&u->name, "Seeschlange");
			}
		}
	}
}
#endif

static int
old_rroad(region * r)
{
	attrib * a = a_find(r->attribs, &at_road);
	if (!a) return 0;
	return a->data.i;
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

static void
newroads(void)
{
	region *r;
	for(r=regions;r;r=r->next) if (terrain[rterrain(r)].roadreq>0) {
		direction_t d;
		int connections = 0;
		int maxcon = MAXDIRECTIONS;
		int stones = old_rroad(r);
		if (stones<=0) continue;
		for (d=0;d!=MAXDIRECTIONS;++d) {
			region * r2 = rconnect(r, d);
			if (r2 && old_rroad(r2)) ++connections;
			else if (terrain[rterrain(r2)].roadreq<=0) --maxcon;
		}
		if (!connections) connections=maxcon;
		else maxcon=0;
		for (d=0;d!=MAXDIRECTIONS && connections;++d) {
			region * r2 = rconnect(r, d);
			int use = stones/connections;
			if (!r2 ||
				terrain[rterrain(r2)].roadreq<=0 ||
				(old_rroad(r2)<=0 && !maxcon))
				continue;
			if (use>terrain[rterrain(r)].roadreq) {
				/* wenn etwas übrig bleibt (connections==1) kriegt es der regionserste */
				int give = use-terrain[rterrain(r)].roadreq;
				unit * u = r->units;
				use = terrain[rterrain(r)].roadreq;
				if (u) change_item(u, I_STONE, give);
			}
			rsetroad(r, d, use);
			stones = stones - use;
			--connections;
			if (old_rroad(r)==terrain[rterrain(r)].roadreq*2 && old_rroad(r2)==terrain[rterrain(r2)].roadreq*2) {
				border * b = get_borders(r, r2);
				attrib * a = a_find(b->attribs, &at_roadfix);
				if (a) continue;
				while (b && b->type!=&bt_road) b = b->next;
				assert(b);
				a = a_add(&b->attribs, a_new(&at_roadfix));
			}
		}
	}
}

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
		SPL_NOSPELL };

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

					if (m->magietyp == sp->magietyp || getspell(u, sp->id)) {
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

#if 0
static void
fix_feuerwand_orks(void)
{
	unit *u;

	if((u=findunitg(atoi36("1j1L"), NULL))==NULL) {
	  printf("Korrektur: kann Orks nicht finden!\n");
	  return;
	}

	if(u->number < 1000) {
	  printf("Korrektur: Orks sind schon korrigiert!\n");
	  return;
	}
	scale_number(u,170);
	u->hp=2337; /* Buggy Anzahl Personen = Normale HP */
}

static void
fix_buildings(void)
{
	building *burg;
	unit *u, *uf = NULL;
	const requirement *req;
	const construction *con;
	FILE *statfile;
	vset done;
	int s1, s2;
	char buf[256];
	int a,b,c,d;
	int first;

	if((statfile=fopen("building.txt","r"))==NULL) {
		log_warning(("fix_buildings: cannot open building.txt!\n"));
		return;
	}

	vset_init(&done);
	s1=s2=-1;

	while(fgets(buf, 256, statfile)!=NULL) {
		if(*buf=='#')
			continue;

		a=b=c=d=0;
		first=0;

		sscanf(buf,"%d;%d;%d;%d",&a,&b,&c,&d);

		if((u=findunitg(b,NULL))==NULL) {
			printf("fix_buildings: Einheit %d nicht gefunden.\n",b);
			continue;
		}

		if((burg=findbuilding(a))==NULL) {
			printf("fix_buildings: Burg %d nicht gefunden.\n", a);
			if((burg=u->building)==NULL) {
				printf("Einheit %d steht auch in keinem Gebäude. (gebe auf)\n",u->no);
				continue;
			}
			if(burg->type->construction->maxsize > 0 &&
			   burg->type->construction->reqsize > 0)
				printf("Ersatz: Burg %d\n",burg->no);
			else {
				printf("Ersatzgebäude ist nicht vom betroffenen Typ.\n");
				continue;
			}
		}

		printf("Burg %d; Einheit %d; vorher %d; + %d\n",burg->no,u->no,c,d);

		s2=vset_add(&done, (void *)a);
		if(s2>s1) {
			printf("ERSTER !\n");
			first=1;
			uf=u;
		}
		s1=s2;

		con=burg->type->construction;

		if(con->maxsize != -1 || con->reqsize == 1) continue;

		for(req=con->materials; req->number!=0; req++) {
			int diff=burg->size-c;

			if(req->type==R_SILVER)
				change_resource(uf, R_SILVER, diff*req->number-d*req->number/con->reqsize);
			else {
				if(first)
					change_resource(uf, req->type, diff*req->number);
				assert(uf!=NULL);
				change_resource(uf, req->type, -d*req->number/con->reqsize);
			}
		}
		if(first)
			burg->size=c+d;
		else
			burg->size+=d;
	}
	vset_destroy(&done);
	fclose(statfile);
}
#endif

extern plane * arena;

static void
fix_age(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		if (f->no!=MONSTER_FACTION && playerrace(f->race)) continue;
		if (f->age!=turn) {
			log_printf("Alter von Partei %s auf %d angepasst.\n", factionid(f), turn);
			f->age = turn;
		}
	}
}

#if 0
static boolean
balsambug(const region *r)
{
	direction_t dir;

	for (dir=0; dir<MAXDIRECTIONS; dir++) {
		if (landregion(rterrain(rconnect(r, dir))) && rdemand(r,0) > 0)
			return false;
	}
	return true;
}

static void
fix_balsamfiasko(void)
{
	region *r, *rc;
	direction_t dir;
	int i;

	for (r=regions; r; r=r->next) freset(r, RF_DH);

	/* Zufällig verteilen */
	for (r=regions; r; r=r->next) {
		if (r->x < -36 && r->y < -45 && landregion(rterrain(r))) {
			if(rdemand(r,0) <= 0 && balsambug(r)) {
				rsetdemand(r,0,(char)(1+rand()%5));
				rsetdemand(r,rand()%7,0);
				fset(r, RF_DH);
			}
		}
	}

	/* Smoothing */
	for(i=0; i<3; i++) {
		for (r=regions; r; r=r->next) if(fval(r, RF_DH)) {
			for (dir=0; dir < MAXDIRECTIONS; dir++) {
				rc = rconnect(r, dir);
				if(rc && fval(rc, RF_DH) && rand()%100 < 20) {
					int p1, p2;
					for (p1 = 0; p1 != MAXLUXURIES; p1++)
						if (rdemand(r, p1) == 0) break;
					for (p2 = 0; p2 != MAXLUXURIES; p2++)
						if (rdemand(rc, p2) == 0) break;

					if(p1 != p2) {
						rsetdemand(rc, p2, (char)(1+rand()%5));
						rsetdemand(rc, p1, 0);
					}
				}
			}
		}
	}
}
#endif

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
recurse_regions(region * r, regionlist **rlist, boolean(*fun)(const region * r))
{
	if (!fun(r)) return 0;
	else {
		int len = 0;
		direction_t d;
		regionlist * rl = calloc(sizeof(regionlist), 1);
		rl->next = *rlist;
		rl->region = r;
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
	regionlist *rl, *rlist = NULL;
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
		region * r = rl->region;
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
		region * r = rl->region;
		log_warning(("fixing demand in %s\n", regionname(r, NULL)));
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
				attrib * a = a_find(permissions, &at_gmcreate);
				while (a && a->data.v!=(void*)olditemtype[i]) a=a->nexttype;
				if (!a) a_add((attrib**)&permissions->data.v, make_atgmcreate(olditemtype[i]));
			}
		}
	}
}

static int
fix_gms(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		attrib * permissions = a_find(f->attribs, &at_permissions);
		if (permissions) {
			const char * keys[] = { "gmgate", "gmmsgr", "gmmsgr", "gmmsgu", NULL };
			int k;
			item_t i;
			for (k=0;keys[k];++k) {
				attrib * a = find_key((attrib*)permissions->data.v, atoi36("gmgate"));
				if (a) {
					attrib ** ap = &a->nexttype;
					while (*ap) {
						attrib * an = *ap;
						if (a->data.i==an->data.i) a_remove(ap, an);
						else ap = &an->next;
					}
				}
			}
			for (i=I_GREATSWORD;i!=I_KEKS;++i) {
				attrib * a = a_find(permissions, &at_gmcreate);
				while (a && a->data.v!=(void*)olditemtype[i]) a=a->nexttype;
				if (a) {
					attrib ** ap = &a->nexttype;
					while (*ap) {
						attrib * an = *ap;
						if (a->data.v==an->data.v) a_remove(ap, an);
						else ap = &an->next;
					}
				}
			}
		}
	}
	return 0;
}

#if 1
static int
fix_demand(void)
{
	region *r;
	const luxury_type *sale = NULL;

	if (maxluxuries==0) for (sale=luxurytypes;sale;sale=sale->next) ++maxluxuries;

	for (r=regions; r; r=r->next) {
		if ((r->land) && count_demand(r) != maxluxuries) {
			fix_demand_region(r);
		}
	}
	return 0;
}
#endif

#if 0
static void
read_laenrepair(boolean active)
{
	FILE * f = fopen("repair.laen", "rt");
	FILE * log = fopen("report.laen", "wt");
	if (!f || !log) return;
	while (!feof(f)) {
		int n, x, y;
		region * r;
		fscanf(f, "%d %d %d", &x, &y, &n);
		r = findregion(x, y);
		if (!r) fprintf(log, "%d,%d:\tnot found\n", x, y);
		else if (fval(r, RF_CHAOTIC)) continue;
		else {
			int laen = rlaen(r);
			if (n==-1 && laen>=0) {
				fprintf(log, "%d,%d:\tillegal laen (%d)\n", x, y, laen);
				if (active) rsetlaen(r, n);
			} else if (laen==-1 && n!=-1) {
				fprintf(log, "%d,%d:\tlaen was lost (%d)\n", x, y, n);
				if (active) rsetlaen(r, n);
			}
		}
	}
	fclose(f);
}

static void
write_laenrepair(void) {
	FILE * f = fopen("repair.laen", "wt");
	region * r;
	for (r=regions;r;r=r->next) if (!fval(r, RF_CHAOTIC)) {
		int laen = rlaen(r);
		fprintf(f, "%d %d %d\n", r->x, r->y, laen);
	}
	fclose(f);
}
#endif

#include "group.h"
static void
fix_allies(void) {
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
			const building_type * btype = &bt_castle;
			icastle_data * data;
			a = a_find(b->attribs, &at_icastle);
			if (b->type!=&bt_illusion && !a) continue;

			if (!a) {
				/* attribut hat gefehle */
				a = a_add(&b->attribs, a_new(&at_icastle));
			}
			if (b->type!=&bt_illusion) {
				/* gebäudetyp war falsch */
				btype = b->type;
				b->type = &bt_illusion;
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

#if 0
static void
fix_ponnukis(void)
		/* Es kann nur einen geben! */
{
	unit * ponn = findunit(atoi36("ponn"));
	if (ponn) {
		region * mag = ponn->region;
		unit * u;
		for (u=mag->units;u;u=u->next) {
			if (u!=ponn && u->faction==ponn->faction && u->race==ponn->race && !strcmp(ponn->name, u->name)) {
				set_number(u, 0);
			}
		}
	}
}
#endif

#if 0
static void
fix_traveldir(void)
{
	region *r;
	attrib *a, *a2;
	traveldir *t;

	for(r=regions; r; r=r->next) {
		a = a_find(r->attribs, &at_traveldir);
		while (a) {
			a2 = a_add(&r->attribs, a_new(&at_traveldir_new));
			t = (traveldir *)(a2->data.v);

			t->no  = a->data.sa[0];
			t->dir = a->data.ca[2];
			t->age = a->data.ca[3];

			a = a->nexttype;
		}
	}
	for(r=regions; r; r=r->next) {
		a = a_find(r->attribs, &at_traveldir);
		while(a) {
			a_remove(&r->attribs, a);
			a = a->nexttype;
		}
	}
}
#endif

static void
stats(void)
{
	FILE * F;
	item * items = NULL;
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
						itm->number=1;
					}
					i_change(&items, itm->type, itm->number);
				}
			}
		}
		for (itype=itemtypes;itype;itype=itype->next) {
			item * itm = *i_find(&items, itype);
			if (itm && itm->number)
				fprintf(F, "%4d %s\n", itm->number, locale_string(NULL, resourcename(itype->rtype, 0)));
			else
				fprintf(F, "%4d %s\n", 0, locale_string(NULL, resourcename(itype->rtype, 0)));
		}
		fclose(F);
	} else {
		perror(zText);
	}

}

#if 0
static void
fix_prices(void)
{
	region *r;

	puts(" - Korrigiere Handelsgüterpreise");

	for(r=regions; r; r=r->next) if(r->land) {
		int sales = 0, buys = 0;
		const luxury_type *sale = NULL, *ltype;
		struct demand *dmd;
		for (dmd=r->land->demands;dmd;dmd=dmd->next) {
			if(dmd->value == 0) {
				sales++;
				sale = dmd->type;
			} else {
				buys++;
			}
		}
		if(sales == 0) {
			int c = 0;
			for(ltype=luxurytypes; ltype; ltype=ltype->next) c++;
			c = rand()%c;
			for(ltype=luxurytypes; c>0; c--) ltype=ltype->next;
			r_setdemand(r, ltype, 0);
			sale = ltype;
		}
		assert(sale);
		if(buys == 0) {
			for(ltype=luxurytypes; ltype; ltype=ltype->next) {
				if(ltype != sale) r_setdemand(r, ltype, 1 + rand() % 5);
			}
		}
	}
}

static void
fix_options(void)
{
	faction *f;

	puts(" - Korrigiere Parteioptionen");

	for(f=factions;f;f=f->next) {
		f->options = f->options | Pow(O_REPORT);
		f->options = f->options | Pow(O_ZUGVORLAGE);
	}
}

static void
init_region_age(void)
{
	region *r;

	puts(" - Initialisiere r->age");

	/* Initialisieren mit dem Alter der ältesten Partei in 2 Feldern
	 * Umkreis */

	for(r=regions; r; r=r->next) {
		regionlist *rl = all_in_range(r,2);
		regionlist *rl2;
		unit *u;
		int maxage = 0;

		for(rl2 = rl; rl2; rl2 = rl2->next) {
			for(u=rl2->region->units; u; u=u->next) {
				if(u->faction->age > maxage) maxage = u->faction->age;
			}
		}
		r->age = (unsigned short)maxage;
	}
}

static void
fix_herbtypes(void)
{
	region * r;
	for (r=regions;r!=NULL;r=r->next) if (r->land) {
		const herb_type * htype = r->land->herbtype;
		if (htype==NULL) {
			if (terrain[r->terrain].herbs!=NULL) {
				const item_type * itype;
				int i=0;
				while (terrain[r->terrain].herbs[i]!=NULL) ++i;

				itype = finditemtype(terrain[r->terrain].herbs[rand()%i], NULL);
				r->land->herbtype = resource2herb(itype->rtype);
			}
		} else {
			if (r->terrain==T_PLAIN) {
				if (r_isforest(r)) {
					int i = rand()%3+3;
					const char * name = terrain[T_PLAIN].herbs[i];
					const item_type * itype = finditemtype(name, NULL);
					r->land->herbtype = resource2herb(itype->rtype);
				}
			} else {
				/* Das geht so nicht, wegen der Eisberg-Terrains */
				/* assert(htype->terrain==r->terrain); */
			}
		}
	}
}
#endif

#ifdef SKILLFIX_SAVE
typedef struct skillfix_data {
	unit * u;
	skill_t skill;
	int value;
	int number;
	int self;
	int teach;
} skillfix_data;

static void
init_skillfix(attrib * a)
{
	a->data.v = calloc(sizeof(skillfix_data), 1);
}

static void
free_skillfix(attrib * a)
{
	free(a->data.v);
}

attrib_type at_skillfix = { "skillfix", init_skillfix, free_skillfix };

void
skillfix(struct unit * u, skill_t skill, int from, int self, int teach)
{
	attrib * a = a_add(&u->attribs, a_new(&at_skillfix));
	skillfix_data * data = (skillfix_data*)a->data.v;
	data->u = u;
	data->skill = skill;
	data->number = u->number;
	data->value = from;
	data->self = self;
	data->teach = teach;
}

void
write_skillfix(void)
{
	FILE * F = fopen("skills.fix", "w+");
	region * r;
	fprintf(F, "S%d\n", turn);
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			attrib * a = a_find(u->attribs, &at_skillfix);
			while (a) {
				skillfix_data * data = (skillfix_data*)a->data.v;
				fprintf(F, "%s %d %d %d %d %d\n",
					itoa36(data->u->no),
					data->skill,
					data->number,
					data->value,
					data->self,
					data->teach);
				a = a->nexttype;
			}
		}
	}
	fclose(F);
}
#endif

#if 0
static void
init_mwarden(void)
{
	unit *u = findunitg(atoi36("mwar"), NULL);
	attrib *a;

	if(!u) return;

	a = a_add(&u->attribs, a_new(&at_warden));
	a->data.i = 0;
}
#endif

#ifdef CONVERT_TRIGGER
#include "old/relation.h"
#include "old/trigger.h"
#include "old/trigger_internal.h"

#include <event.h>

#include <triggers/killunit.h>
#include <triggers/timeout.h>
#include <triggers/changerace.h>
#include <triggers/changefaction.h>
#include <triggers/createunit.h>
#include <triggers/giveitem.h>
#include <triggers/createcurse.h>
#include <triggers/shock.h>

typedef struct {
	void *obj2;
	typ_t typ2;
	relation_t id;
	spread_t spread;
} reldata;

extern timeout *all_timeouts;

static void
convert_triggers(void)
{
	region * r=regions;
	timeout * t;
	while (r) {
		unit * u = r->units;
		while (u) {
			attrib * a = a_find(u->attribs, &at_relation);
			while (a) {
				reldata *rel = (reldata *)a->data.v;
				unit * u2 = (unit*)rel->obj2;
				switch (rel->id) {
				case REL_FAMILIAR:
					if (u && u2) {
						if (!playerrace(u->race) || (playerrace(u2->race) && u->race==RC_GOBLIN))
							set_familiar(u2, u);
						else
							set_familiar(u, u2);
					} else {
						if (u2) fprintf(stderr, "WARNING: FAMILIAR info for %s may be broken!\n", unitname(u2));
						if (u) fprintf(stderr, "WARNING: FAMILIAR info for %s may be broken!\n", unitname(u));
					}
					break;
				case REL_CREATOR:
					break;
				case REL_TARGET:
					fprintf(stderr, "WARNING: TARGET relation between %s and 0x%p not converted\n", unitname(u), u2);
					break;
				default:
					fprintf(stderr, "WARNING: unknown relation %d between %s and 0x%p not converted\n", rel->id, unitname(u), u2);
					break;
				}
				a = a->nexttype;
			}
			u = u->next;
		}
		r=r->next;
	}
	for (t=all_timeouts;t;t=t->next) {
		actionlist * al = t->acts;
		int time = t->ticks;
		for (;al;al=al->next) {
			action * act = al->act;
			if (act) switch( act->atype ) {
			case AC_DESTROY: {
				/* conversion keeper */
				unit * u = (unit*)act->obj;
				trigger * tkill = trigger_killunit(u);
				add_trigger(&u->attribs, "timer", trigger_timeout(time, tkill));
			}
			case AC_CHANGERACE: {
				/* conversion for toads */
				unit *u = (unit*)act->obj;
				race_t race = (race_t)act->i[0];
				race_t irace = (race_t)act->i[1];
				trigger * trestore = trigger_changerace(u, race, irace);
				if (rand()%10>2) t_add(&trestore, trigger_giveitem(u, olditemtype[I_TOADSLIME], 1));
				add_trigger(&u->attribs, "timer", trigger_timeout(time, trestore));
				break;
			}
			case AC_CHANGEIRACE: {
				/* conversion for shapeshift */
				unit *u = (unit*)act->obj;
				race_t irace = (race_t)act->i[0];
				trigger * trestore = trigger_changerace(u, NORACE, irace);
				add_trigger(&u->attribs, "timer", trigger_timeout(time, trestore));
				break;
			}
			case AC_CHANGEFACTION: {
				/* charmingsong */
				faction *f = findfaction_unique_id(act->i[0]);
				unit    *u = (unit*)act->obj;
				trigger *trestore = trigger_changefaction(u, f);
				add_trigger(&u->attribs, "timer", trigger_timeout(time, trestore));
				add_trigger(&u->faction->attribs, "destroy", trigger_killunit(u));
				add_trigger(&f->attribs, "destroy", trigger_killunit(u));
				break;
			}
			case AC_CREATEUNIT: {
				/* conversion summon_dragon */
				faction *f  = findfaction_unique_id(act->i[0]);
				region  *r  = (region *)act->obj;
				int      number  = act->i[1];
				race_t   race    = (race_t)act->i[2];
				trigger *tsummon = trigger_createunit(r, f, race, number);
				add_trigger(&r->attribs, "timer", trigger_timeout(time, tsummon));
				break;
			}
			case AC_CREATEMAGICBOOSTCURSE:{
				/* delayed magic boost curse */
				unit    *mage    = (unit*)act->obj;
				trigger *tsummon = trigger_createcurse(mage, mage, C_AURA, 0, act->i[0], 6, 50, 1);
				add_trigger(&mage->attribs, "timer", trigger_timeout(5, tsummon));
				break;
			}
			default:
				fprintf(stderr, "WARNING: timeout not converted\n");
			}
		}
	}
}
#endif

#if 0
#include <items/lmsreward.h>
static void
lms_special(unit * u)
{
	if (u) i_change(&u->items, &it_lmsreward, 1);
}
#endif

#if 0
#define LIFEEXPECTANCY (27*40)
static void
undo_deadpeasants(void)
{
	region * r = regions;
	while (r) {
		int dead = rpeasants(r) / LIFEEXPECTANCY;
		deathcounts(r, -dead);
		r = r->next;
	}
}

static void
fix_undead3percent(void)
{
	region * r = regions;
	while (r) {
		int dead = deathcount(r);
		deathcounts(r, -(dead/2));
		r = r->next;
	}
}

static void
fix_targetregion_resolve(void)
{
	region *r;
	unit *u;
	attrib *a;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			a = a_find(u->attribs, &at_targetregion);
			if(a) a->data.v = findregion(a->data.sa[0], a->data.sa[1]);
		}
	}
}

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

#if 0
static void
fix_timeouts(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			attrib * a = a_find(u->attribs, &at_eventhandler);
			boolean toad = (u->race==RC_TOAD);
			boolean keeper = (u->race==RC_IRONKEEPER);
			while (a!=NULL) {
				trigger * t;
				handler_info * td = (handler_info *)a->data.v;
				for (t=td->triggers;t;t=t->next) {
					trigger ** tptr=&t->next;
					while (*tptr) {
						/* remove duplicates */
						if ((*tptr)->type==t->type) {
							*tptr = (*tptr)->next;
						} else tptr = &(*tptr)->next;
					}
					if (t->type == &tt_changerace ||
						t->type == &tt_changefaction ||
						t->type == &tt_createcurse ||
						t->type == &tt_createunit)
					{
						trigger * timer = get_timeout(td->triggers, t);
						if (toad && t->type == &tt_changerace) {
							toad = false;
						}
						if (timer==NULL) {
							add_trigger(&u->attribs, "timer", trigger_timeout(1+(rand()%2), t));
						}
					}
					else if (t->type == &tt_killunit) {
						if (u->race==RC_IRONKEEPER) {
							trigger * timer = get_timeout(td->triggers, t);
							keeper = false;
							if (timer==NULL) {
								add_trigger(&u->attribs, "timer", trigger_timeout(1+(rand()%2), t));
							}
						}
					}
				}
				a = a->nexttype;
			}
			if (keeper) {
				int duration = 1+ (rand() % 2);
				trigger * tkill = trigger_killunit(u);
				add_trigger(&u->attribs, "timer", trigger_timeout(duration, tkill));
			}
			if (toad) {
				/* repair toad-only mage */
				int duration = 1+ (rand() % 2);
				trigger * trestore = trigger_changerace(u, u->faction->race, u->faction->race);
				if (rand()%10>2) t_add(&trestore, trigger_giveitem(u, olditemtype[I_TOADSLIME], 1));
				add_trigger(&u->attribs, "timer", trigger_timeout(duration, trestore));
			}
		}
	}
}
#endif

#include <modules/gmcmd.h>

static int
resize_plane(struct plane * p, int radius)
{
	region * center = findregion(p->minx+(p->maxx-p->minx)/2, p->miny+(p->maxy-p->miny)/2);
	int minx = center->x - radius;
	int maxx = center->x + radius;
	int miny = center->y - radius;
	int maxy = center->y + radius;
	int x, y;

	for (x=minx;x<=maxx;++x) {
		for (y=miny;y<=maxy;++y) {
			region * r = findregion(x, y);
			if (r && rplane(r)!=p) break;
		}
	}
	if (x<=maxx) return -1;
	p->maxx=maxx;
	p->maxy=maxy;
	p->minx=minx;
	p->miny=miny;
	for (x=minx;x<=maxx;++x) {
		for (y=miny;y<=maxy;++y) {
			region * r = findregion(x, y);
			if (r==NULL) {
				r = new_region(minx+x, miny+y);
				freset(r, RF_ENCOUNTER);
				r->planep = p;
				if (distance(r, center)==radius) {
					terraform(r, T_FIREWALL);
				} else if (distance(r, center)<radius) {
					terraform(r, T_OCEAN);
				}
			}
		}
	}
	return 0;
}

static int
regatta_quest(void)
{
	plane * p = getplanebyname("Regatta");
	if (p) {
		return resize_plane(p, 40);
	} else {
		region * center;
		p = gm_addplane(40, PFL_NORECRUITS, "Regatta");
		center = findregion(p->minx+(p->maxx-p->minx)/2, p->miny+(p->maxy-p->miny)/2);
		gm_addfaction("gregorjochmann@gmx.de", p, center);
		return 0;
	}
}

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
			attrib * a = a_find(permissions, &at_gmcreate);
			while (a && a->data.v!=(void*)oldpotiontype[p]->itype) a=a->nexttype;
			if (!a) a_add((attrib**)&permissions->data.v, make_atgmcreate(oldpotiontype[p]->itype));
		}
		do_once("et02", secondfaction(f));
	}
	do_once("rq02", regatta_quest());
}

#if 0
static void
test_gmquest(void)
{
	 const struct faction * f;
	 /* enno's world */
	 f = gm_addquest("enno@eressea.upb.de", "GM Zone", 1, PFL_NOATTACK|PFL_NOALLIANCES|PFL_NOFEED|PFL_FRIENDLY);
	 log_printf("Neue Questenpartei %s\n", factionname(f));

	 f = gm_addquest("xandril@att.net", "Mardallas Welt", 40, 0);
	 log_printf("Neue Questenpartei %s\n", factionname(f));

	 f = gm_addquest("moritzsalinger@web.de", "Laen-Kaiser", 7, /*PFL_NORECRUITS |*/ PFL_NOMAGIC /*| PFL_NOBUILD*/);
	 log_printf("Neue Questenpartei %s\n", factionname(f));

	 f = gm_addquest("Denise.Muenstermann@home.gelsen-net.de", "Mochikas Queste", 7, PFL_NOMAGIC);
	 log_printf("Neue Questenpartei %s\n", factionname(f));

	 f = gm_addquest("feeron@aol.com", "Eternath", 11, 0);
	 log_printf("Neue Questenpartei %s\n", factionname(f));

	 f = gm_addquest("BigBear@nord-com.net", "Leonidas Vermächtnis", 15, PFL_NOMAGIC|PFL_NOSTEALTH);
	 log_printf("Neue Questenpartei %s\n", factionname(f));

}
#endif

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

#if 0
#error "this is broken, it duplicates triggers"
static void
fix_unitrefs(void)
{
	region * r=regions;
	while (r) {
		unit * u = r->units;
		while (u) {
			attrib * a;

			a = a_find(u->attribs, &at_familiar);
			if (a) {
				/* magier, hat einen familiar */
				unit * ufamiliar = get_familiar(u);
				if (ufamiliar) {
					attrib * ae;
					/* killunit attribut am magier */
					ae = a_find(u->attribs, &at_eventhandler);
					if (ae) {
						trigger * tkillunit = NULL;
						trigger ** tlist = get_triggers(u->attribs, "destroy");
						if (tlist!=NULL) {
							tkillunit = *tlist;
							while (tkillunit) {
								if (strcmp(tkillunit->type->name, "killunit")==0) break;
								tkillunit = tkillunit->next;
							}
							if (tkillunit && !tkillunit->data.v) {
								log_warning(("killunit-trigger für Magier %s und Vertrauten %s restauriert.\n",
									itoa36(u->no), itoa36(ufamiliar->no)));
								tkillunit->data.v = ufamiliar;
							}
							else ae = NULL;
						}
					}
					if (ae==NULL) {
						/* Wenn der Magier stirbt, dann auch der Vertraute */
						add_trigger(&u->attribs, "destroy", trigger_killunit(ufamiliar));
					}

					/* killunit attribut am magier */
					ae = a_find(ufamiliar->attribs, &at_eventhandler);
					if (ae) {
						trigger * tshockunit = NULL;
						trigger ** tlist = get_triggers(ufamiliar->attribs, "destroy");
						if (tlist!=NULL) {
							tshockunit = *tlist;
							while (tshockunit) {
								if (strcmp(tshockunit->type->name, "shock")==0) break;
								tshockunit = tshockunit->next;
							}
							if (tshockunit && !tshockunit->data.v) {
								log_warning(("shockunit-trigger für Magier %s und Vertrauten %s restauriert.\n",
									itoa36(u->no), itoa36(ufamiliar->no)));
								tshockunit->data.v = u;
							}
							else ae = NULL;
						}
					}
					if (ae==NULL) {
						/* Wenn der Vertraute stirbt, schockt er den Magier */
						add_trigger(&ufamiliar->attribs, "destroy", trigger_shock(u));
					}

				} else {
					log_error(("Magier %s hat ein at_familiar, aber keinen Vertrauten.\n",
						itoa36(u->no)));
				}
			}
			u = u->next;
		}
		r=r->next;
	}
}
#endif

static void
update_igjarjuk_quest(void)
{
	unit * u;
	faction *f = findfaction(atoi36("rr"));

	if (!f) return;
	for (u=f->units;u;u=u->nextF) {
		u->race = new_race[RC_TEMPLATE];
	}
}


#if RESOURCE_CONVERSION
extern struct attrib_type at_resources;
void
init_resourcefix(void)
{
	at_register(&at_resources);
}

static int
read_resfix(void)
{
#if 0
	FILE * F = fopen("resource.fix", "r");
	if (F==NULL) return;
	while (!feof(F)) {
		int x, y, stone, iron, laen, age;
		int lstone, liron, llaen; /* level */
		region * r;
		fscanf(F, "%d %d %d %d %d %d %d %d %d\n", &x, &y, &age, &stone, &iron, &laen, &lstone, &liron, &llaen);
		r = findregion(x, y);
		if (r) {
			rawmaterial * rm = r->resources;
			while (rm) {
				if (rm->type==&rm_iron && liron>0) {
					rm->amount = iron;
					rm->level = liron;
				}
				else if (rm->type==&rm_laen && llaen>0) {
					rm->amount = laen;
					rm->level = llaen;
				}
				else if (rm->type==&rm_stones && lstone>0) {
					rm->amount = stone;
					rm->level = lstone;
				}
				rm = rm->next;
			}
			r->age = (unsigned short)age;
		}
	}
	fclose(F);
#else
	return -1;
#endif
}

static int
convert_resources(void)
{
#if 0
	region *r;

	for(r=regions;r;r=r->next) {
		attrib *a = a_find(r->attribs, &at_resources);
		r->resources = 0;
		terraform_resources(r);

		if (a==NULL) continue;
		else {
			int INIT_STONE = 20; /* skip this many weeks */
			double ironmulti = 0.40;
			double laenmulti = 0.50;
			double stonemulti = 0.30; /* half the stones used */
			rawmaterial * rmiron = rm_get(r, rm_iron.rtype);
			rawmaterial * rmlaen = rm_get(r, rm_laen.rtype);
			rawmaterial * rmstone = rm_get(r, rm_stones.rtype);

			int oldiron;
			int oldlaen = MAXLAENPERTURN * min(r->age, 100) / 2;
			int oldstone = terrain[rterrain(r)].quarries * max(0, r->age - INIT_STONE);
			int iron = a->data.sa[0];
			int laen = a->data.sa[1];
			int stone, level;
			int i, base;

			/** STONE **/
			for (i=0;terrain[r->terrain].rawmaterials[i].type;++i) {
				if (terrain[r->terrain].rawmaterials[i].type == &rm_stones) break;
			}
			if (terrain[r->terrain].rawmaterials[i].type) {
				base = terrain[r->terrain].rawmaterials[i].base;
				stone = max (0, (int)(oldstone * stonemulti));
				level = 1;
				base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				while (stone >= base) {
					stone-=base;
					++level;
					base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				}
				rmstone->level = level;
				rmstone->amount = base - stone;
				assert (rmstone->amount > 0);
				log_printf("CONVERSION: %d stones @ level %d in %s\n", rmstone->amount, rmstone->level, regionname(r, NULL));
			} else {
				log_error(("found stones in %s of %s\n", terrain[r->terrain].name, regionname(r, NULL)));
			}

			/** IRON **/
			if (r_isglacier(r) || r->terrain==T_ICEBERG) {
				oldiron = GLIRONPERTURN * min(r->age, 100) / 2;
			} else {
				oldiron = IRONPERTURN * r->age;
			}
			for (i=0;terrain[r->terrain].rawmaterials[i].type;++i) {
				if (terrain[r->terrain].rawmaterials[i].type == &rm_iron) break;
			}
			if (terrain[r->terrain].rawmaterials[i].type) {
				base = terrain[r->terrain].rawmaterials[i].base;
				iron = max(0, (int)(oldiron * ironmulti - iron ));
				level = 1;
				base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				while (iron >= base) {
					iron-=base;
					++level;
					base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
				}
				rmiron->level = level;
				rmiron->amount = base - iron;
				assert (rmiron->amount > 0);
				log_printf("CONVERSION: %d iron @ level %d in %s\n", rmiron->amount, rmiron->level, regionname(r, NULL));
			} else {
				log_error(("found iron in %s of %s\n", terrain[r->terrain].name, regionname(r, NULL)));
			}

			/** LAEN **/
			if (laen>=0) {
				if (rmlaen==NULL) {
					rmlaen = calloc(sizeof(rawmaterial), 1);
					rmlaen->next = r->resources;
					r->resources = rmlaen;
				}
				for (i=0;terrain[r->terrain].rawmaterials[i].type;++i) {
					if (terrain[r->terrain].rawmaterials[i].type == &rm_laen) break;
				}
				if (terrain[r->terrain].rawmaterials[i].type) {
					laen = max(0, (int)(oldlaen * laenmulti - laen));
					level = 1;
					base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
					while (laen >= base) {
						laen-=base;
						++level;
						base = (int)(terrain[r->terrain].rawmaterials[i].base*(1+level*terrain[r->terrain].rawmaterials[i].divisor));
					}
					rmlaen->level = level;
					rmlaen->amount = base - laen;
					assert(rmlaen->amount>0);
					log_printf("CONVERSION: %d laen @ level %d in %s\n", rmlaen->amount, rmlaen->level, regionname(r, NULL));
					rmlaen = NULL;
				}
			}
			if (rmlaen) {
				struct rawmaterial *res;
				struct rawmaterial ** pres = &r->resources;
				if (laen!=-1) log_error(("found laen in %s of %s\n", terrain[r->terrain].name, regionname(r, NULL)));
				while (*pres!=rmlaen) pres = &(*pres)->next;
				res = *pres;
				*pres = (*pres)->next;
				free(res);
			}
#ifndef NDEBUG
			{
				rawmaterial * res = r->resources;
				while (res) {
					assert(res->amount>0);
					assert(res->level>0);
					res = res->next;
				}
			}
#endif
		}
	}
	return 0;
#else
	return -1;
#endif
}

#endif

#if 0
static int
convert_resources(void)
{
	region *r;
	attrib *a;

	for(r=regions;r;r=r->next) {

		/* Iron */
		if(rterrain(r) == T_MOUNTAIN) {
			a_add(&r->attribs, a_new(&at_iron))->data.i = 25;
		} else if(rterrain(r) == T_GLACIER || rterrain(r) == T_ICEBERG_SLEEP) {
			a_add(&r->attribs, a_new(&at_iron))->data.i = 2;
		}
		/* Laen */
		if(rterrain(r) == T_MOUNTAIN) {
			a = a_find(r->attribs, &at_laen);
			if(a) a->data.i = 6;
		}
		/* Stone */
		if(terrain[rterrain(r)].quarries > 0) {
			a_add(&r->attribs, a_new(&at_stone))->data.i = terrain[rterrain(r)].quarries;
		}
	}
	return 0;
}

static void
fix_negpotion(void)
{
	region *r;
	unit *u;
	int i;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			i = get_effect(u, oldpotiontype[P_FAST]);
			if (i < 0){
				change_effect(u, oldpotiontype[P_FAST], -i);
			}
		}
	}
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
dump_sql(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		if (f->unique_id==0) {
			f->unique_id = ++max_unique_id;
		}
		if (f->age!=1 && f->no!=MONSTER_FACTION) {
			fprintf(sqlstream, "INSERT INTO users (id, email) VALUES (%d, '%s');\n",
				f->unique_id, f->email);
			fprintf(sqlstream, "INSERT INTO factions (id, user, name, password, race, locale, lastorders, banner, email) "
				"VALUES ('%s', %d, '%s', '%s', '%s', '%s', %u, '%s', '%s');\n",
				itoa36(f->no), f->unique_id, sqlquote(f->name), sqlquote(f->passw), LOC(default_locale, rc_name(f->race, 1)),
				locale_name(f->locale), f->lastorders, sqlquote(f->banner), f->email);
		}
	}
	return 0;
}

#if NEW_RESOURCEGROWTH
static int
randomized_resources(void)
{
	region *r;

	for(r = regions; r; r=r->next) if(r->land) {
		terrain_t ter = rterrain(r);

		/* Dieses if verhindert, das Laen nocheinmal verteilt wird. */
		if(ter == T_MOUNTAIN || ter == T_GLACIER || ter == T_ICEBERG || ter == T_ICEBERG_SLEEP) continue;

		terraform_resources(r);
	}
	return 0;
}
#endif

static int
fix_idleout(void)
{
	faction *f;

	f = findfaction(0);
	if(f){
		fset(f, FL_NOIDLEOUT);
	}
	f = findfaction(atoi36("muse"));
	if(f){
		fset(f, FL_NOIDLEOUT);
	}
	return 0;
}

static int
set_zip(void)
{
	faction *f;
	for(f=factions;f;f=f->next) {
		if(!(f->options & (1 << O_BZIP2)) && !(f->options & (1 << O_COMPRESS))) {
			f->options = f->options | (1 << O_COMPRESS);
		}
	}

	f = findfaction(0);
	f->options = f->options & ~(1 << O_COMPRESS);
	f->options = f->options | (1 << O_BZIP2);

	return 0;
}

static int
heal_all(void)
{
	region *r;
	unit *u;
	faction *f;
	message *msg = msg_message("healall","");

	for(f=factions; f; f=f->next) {
		freset(f, FL_DH);
	}

	for(r=regions; r; r=r->next) {
		for(u=r->units;u;u=u->next) {
			int max_hp = unit_max_hp(u) * u->number;
			if(u->hp < max_hp) { 
				u->hp = max_hp;
				fset(u->faction, FL_DH);
			} 
		}
	}
	
	for (f=factions; f; f=f->next) {
		if(fval(f,FL_DH)) {
			add_message(&f->msgs, msg);
			freset(f, FL_DH);
		}
	}

	msg_release(msg);
	return 0;
}

#if PEASANT_ADJUSTMENT == 1

#define WEIGHT ((double)0.5)
#define PLWEIGHT ((double)0.80)

static int
peasant_adjustment(void)
{
	terrain_t ter;
	int sum, avg, c;
	double freeall, pool; /* long long is illegal */
	double s;
	region *r;

	s = 0;
	for(r=regions; r; r=r->next) {
		s += rpeasants(r);
	}
	log_printf("BAUERN vorher: %lld\n",s);

	for(ter = 0; ter < MAXTERRAINS; ter++) {
		if(terrain[ter].production_max <= 0) continue;

		c = 0;
		sum = 0;
		pool = 0;
		freeall = 0;

		for(r=regions; r; r=r->next) if(rterrain(r) == ter) {
			unit *u;

			c++;
			sum += rpeasants(r);
			for(u = r->units; u; u=u->next) {
				if(lifestyle(u) > 0) sum += u->number;
				/* Sonderregel Bauernmob */
				if (u->race == new_race[RC_PEASANT]) sum += u->number;
			}
		}
		if (c==0) continue;
		avg = sum/c;
		
		for(r=regions; r; r=r->next) if(rterrain(r) == ter) {
			unit *u;
			int playerp = 0;
			int p_weg;
			int soll;

			for(u = r->units; u; u=u->next) {
				if(lifestyle(u) > 0) playerp += u->number;
				/* Sonderregel Bauernmob */
				if (u->race == new_race[RC_PEASANT]) playerp += u->number;
			}

			soll = (int)((avg + playerp + rpeasants(r)) * WEIGHT);

			if(playerp * PLWEIGHT + rpeasants(r) > soll) {
				p_weg = (int)(min(((playerp * PLWEIGHT) + rpeasants(r)) - soll, rpeasants(r)));
				assert(p_weg >= 0);
				if (p_weg > 0){
					log_printf("BAUERN %s: war %d, minus %d, playerp %d\n",
							regionname(r, NULL), rpeasants(r), p_weg, playerp);
				}
				pool += p_weg;
				rsetpeasants(r, rpeasants(r) - p_weg);
				assert(rpeasants(r) >= 0);
			}

			freeall += max(0,production(r) * MAXPEASANTS_PER_AREA
				- ((rtrees(r,2)+rtrees(r,1)/2) * TREESIZE) - playerp);
		}
		
		for(r=regions; r; r=r->next) if(rterrain(r) == ter) {
			unit *u;
			double free;
			int newp;
			int playerp = 0;
			
			for(u = r->units; u; u=u->next) {
				if(lifestyle(u) > 0) playerp += u->number;
				/* Sonderregel Bauernmob */
				if (u->race == new_race[RC_PEASANT]) playerp += u->number;
			}

			free = max(0,production(r) * MAXPEASANTS_PER_AREA
        - ((rtrees(r,2)+rtrees(r,1)/2) * TREESIZE) - playerp);

			newp = (int)((pool * free)/freeall);

			rsetpeasants(r, rpeasants(r)+newp);

			assert(rpeasants(r) >= 0);
		}
	}
	
	s = 0;
	for(r=regions; r; r=r->next) {
		s += rpeasants(r);
	}
	log_printf("BAUERN nachher: %lld\n",s);

	return 0;
}

static int
orc_conversion2(void)
{
	faction *f;
	unit *u;
	region *r;

	for(f=factions; f; f=f->next) {
		if(f->race == new_race[RC_ORC]) {
			f->race = new_race[RC_URUK];
		}
	}

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			if (u->race==new_race[RC_ORC] || u->race==new_race[RC_SNOTLING]) {
				/* convert to either uruk or snotling */
				if (has_skill(u, SK_MAGIC) || has_skill(u, SK_ALCHEMY)) {
					u->race = new_race[RC_URUK];
					u->irace = new_race[RC_URUK];
				}
			}
		}
	}

	return 0;
}

static int
orc_conversion(void)
{
	faction *f;
	unit *u;
	region *r;

	for(f=factions; f; f=f->next) {
		if(f->race == new_race[RC_ORC]) {
			f->race = new_race[RC_URUK];
		}
	}

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			if(u->race == new_race[RC_ORC]) {
				/* convert to either uruk or snotling */
				if(effskill(u, SK_MAGIC) >= 1
						|| effskill(u, SK_ALCHEMY) >= 1
					  || get_item(u, I_CHASTITY_BELT) >= u->number) {
					u->race = new_race[RC_URUK];
					u->irace = new_race[RC_URUK];
				} else {
					u->race = new_race[RC_SNOTLING];
					u->irace = new_race[RC_SNOTLING];
				}
			}
		}
	}

	return 0;
}
#endif

static int
fix_astralplane(void)
{
	plane * astralplane = getplanebyname("Astralraum");
	if (astralplane) {
		freset(astralplane, PFL_NOCOORDS);
		set_ursprung(findfaction(MONSTER_FACTION), astralplane->id, 0, 0);
	}
	return 0;
}

static int
fix_watchers(void)
{
	plane * p = getplanebyid (59034966);
	if (p) {
		faction * f = findfaction(atoi36("gm04"));
		watcher * w = calloc(sizeof(watcher), 1);
		w->faction = f;
		w->mode = see_unit;
		w->next = p->watchers;
		p->watchers	= w;
	}
	p = getplanebyid(1137);
	if (p) {
		faction * f = findfaction(atoi36("rr"));
		watcher * w = calloc(sizeof(watcher), 1);
		w->faction = f;
		w->mode = see_unit;
		w->next = p->watchers;
		p->watchers = w;
	}
	return 0;
}

static int
fix_questcoors(void)
{
	plane * p = getplanebyid (59034966);
	if (p) {
		faction * f;
		region * eternath = findregion(-8750, 3631);
		if (eternath) for (f=factions;f;f=f->next) {
			ursprung * u = f->ursprung;
			while (u) {
				if (u->id == p->id) {
					u->x = 0;
					u->y = 0;
					break;
				}
				u=u->next;
			}
		}
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

static int
fix_seeds(void)
{
	region *r;

	for(r=regions; r; r=r->next) {
		if(rtrees(r,0) > 25 && rtrees(r,0) > rtrees(r,2)/2) {
			rsettrees(r,0,rtrees(r,2)/2);
		}
	}

	return 0;
}

void
korrektur(void)
{
#if TEST_LOCALES
	setup_locales();
#endif
	fix_astralplane();
	fix_firewalls();
	fix_gates();
#ifdef TEST_GM_COMMANDS
	setup_gm_faction();
#endif
	update_gms();
	verify_owners(false);
	/* fix_herbtypes(); */
#ifdef CONVERT_TRIGGER
	convert_triggers();
#endif
	fix_migrants();
	no_teurefremde(1);
	update_igjarjuk_quest();
	fix_allies();
	update_gmquests(); /* test gm quests */
	/* fix_unitrefs(); */
	stats();
	do_once("sql2", dump_sql());
	do_once("fw01", fix_watchers());
#if NEW_RESOURCEGROWTH
	/* do not remove do_once calls - old datafiles need them! */
	do_once("rgrw", convert_resources());
	do_once("rsfx", read_resfix());
#endif
	/* do_once("xepl", create_xe()); */
#if GROWING_TREES
	do_once("grtr", growing_trees());
#endif

	do_once("fgms", fix_gms());
#if NEW_RESOURCEGROWTH
	do_once("rndr", randomized_resources());
#endif
	do_once("idlo", fix_idleout());
	do_once("szip", set_zip());
	do_once("heal", heal_all());
	do_once("fquc", fix_questcoors());
	do_once("fsee", fix_seeds());
	do_once("orc2", orc_conversion2());
	do_once("witm", warn_items());
	warn_password();

	/* seems something fishy is going on, do this just 
	 * to be on the safe side: 
	 */
	fix_demand();
  /* trade_orders(); */
	if (global.data_version < NEWROAD_VERSION) {
		newroads();
	}

	/* immer ausführen, wenn neue Sprüche dazugekommen sind, oder sich
	 * Beschreibungen geändert haben */
	show_newspells();
	fix_age();

	/* Immer ausführen! Erschafft neue Teleport-Regionen, wenn nötig */
	create_teleport_plane();

	if (global.data_version<TYPES_VERSION) fix_icastles();
#ifdef XMAS2000
	santa_comes_to_town();
#endif
#ifdef FUZZY_BASE36
	enable_fuzzy = true;
#endif
}


void
korrektur_end(void)
{
	/* fix_balsamfiasko(); */
#ifdef XMAS2001
	do_once("2001", xmas2001());
#endif
#if PEASANT_ADJUSTMENT == 1
	do_once("peas", peasant_adjustment());
	do_once("orcc", orc_conversion());
#endif

}

void
init_conversion(void)
{
#if defined(CONVERT_TRIGGER)
	at_register(&at_relation);
	at_register(&at_relbackref);
	at_register(&at_trigger);
	at_register(&at_action);
#endif
}
