/* vi: set ts=2:
 *
 *	$Id: korrektur.c,v 1.19 2001/02/10 14:18:00 enno Exp $
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

#include <modules/xmas2000.h>
#include <modules/museum.h>

/* gamecode includes */
#include <creation.h>
#include <economy.h>

/* kernel includes */
#include <border.h>
#include <building.h>
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
#include <skill.h>
#include <teleport.h>
#include <unit.h>
#include <spell.h>

/* util includes */
#include <attrib.h>
#include <base36.h>
#include <cvector.h>
#include <resolve.h>
#include <vset.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* attributes includes */
#include <attributes/targetregion.h>
#include <attributes/key.h>

static void
fix_skills(void)
{
	int magic;
	char zText[128];
	FILE * F = fopen("skills.fix", "r+");
	if (F==NULL) return;
	fprintf(stderr, "==-------------==\n");
	fprintf(stderr, "  Fixing skills  \n");
	fprintf(stderr, "==-------------==\n");
	fscanf(F, "%s", zText);
	magic = atoi36(zText);
	{ /* check for magic key */
		attrib * a = a_find(global.attribs, &at_key);
		/* make sure that this is done only once! */
		while (a && a->data.i!=magic) a=a->next;
		if (a) {
			fprintf(stderr, "WARNING: fix_skills() was called a second time\n");
			return;
		}
		a_add(&global.attribs, a_new(&at_key))->data.i = magic;
	}
	for (;;) {
		int from, self, teach, skill;
		int myskill, change, number;
		unit * u;
		if (fscanf(F, "%s %d %d %d %d %d\n", zText, &skill, &number, &from, &self, &teach)<=0) break;
		u = findunit(atoi36(zText));
		if (u==NULL) {
			fprintf(stderr, "unit %s not found!\n", zText);
			continue;
		}
		myskill = get_skill(u, (skill_t)skill);
		if (myskill < from+self) {
			/* todesfälle oder orkvermehrung führen zu veränderungen */
			change = (teach * myskill) / (from + teach);
		}
		else change = teach;
		change_skill(u, (skill_t)skill, change);
	}
}

/* make sure that this is done only once! */
#define do_once(magic, fun) \
{ \
	attrib * a = a_find(global.attribs, &at_key); \
	while (a && a->data.i!=(magic)) a=a->next; \
	if (a) { \
		fprintf(stderr, "WARNING: a unique fix was called a second time\n"); \
		return; \
	} \
	else (fun); \
}

static void
fix_vertrautenmagie(void)
{
	region *r;
	for(r=regions; r; r=r->next) {
		unit *u;
		for(u=r->units;u;u=u->next) {
			sc_mage *m = get_mage(u);
			if (m != NULL) {

				/* Vertraute und Monster haben alle Magiegebiet M_GRAU */
				if (m->magietyp != M_GRAU) continue;

				switch(u->race) {
					case RC_PSEUDODRAGON:
						addspell(u, SPL_FLEE);
						addspell(u, SPL_SLEEP);
						addspell(u, SPL_FRIGHTEN);
						break;
					case RC_NYMPH:
						addspell(u, SPL_SEDUCE);
						addspell(u, SPL_CALM_MONSTER);
						addspell(u, SPL_SONG_OF_CONFUSION);
						addspell(u, SPL_DENYATTACK);
						break;
					case RC_UNICORN:
						addspell(u, SPL_RESISTMAGICBONUS);
						addspell(u, SPL_SONG_OF_PEACE);
						addspell(u, SPL_CALM_MONSTER);
						addspell(u, SPL_HERO);
						addspell(u, SPL_HEALINGSONG);
						addspell(u, SPL_DENYATTACK);
						break;
					case RC_WRAITH:
						addspell(u, SPL_STEALAURA);
						addspell(u, SPL_FRIGHTEN);
						addspell(u, SPL_SUMMONUNDEAD);
						break;
					case RC_IMP:
						addspell(u, SPL_STEALAURA);
						break;
					case RC_DREAMCAT:
						addspell(u, SPL_ILL_SHAPESHIFT);
						addspell(u, SPL_TRANSFERAURA_TRAUM);
						break;
					case RC_FEY:
						addspell(u, SPL_DENYATTACK);
						addspell(u, SPL_CALM_MONSTER);
						addspell(u, SPL_SEDUCE);
						break;
					/* kein break, ein Wyrm hat alle Drachensprüche */
					case RC_WYRM:
						addspell(u, SPL_WYRMODEM);
					case RC_DRAGON:
						addspell(u, SPL_DRAGONODEM);
					case RC_FIREDRAGON:
					case RC_SEASERPENT:
						addspell(u, SPL_FIREDRAGONODEM);
						break;
				}
			}
		}
	}
}

#if 0
static void
addunitsilver(unit * u, int count, int mode)
{
	if (mode)
		change_money(u, count * u->number);
	else
		u->money = count * u->number;
}

static void
addunititems(unit * u, int count, char item, int mode)
{
	if (mode)
		change_item(u, item, count * u->number);
	else
		set_item(u, item, count * u->number);
}

static void
addfactionsilver(faction * f, int count, int mode)
{
	unit * u;
	for (u=f->units;u;u=u->nextF) {
		addunitsilver(u, count, mode);
	}
}

static void
addfactionitems(faction * f, int count, char item, int mode)
{
	unit * u;
	for (u=f->units;u;u=u->nextF) {
		addunititems(u, count, item, mode);
	}
}

static void
showallspells(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		if (f->no==MONSTER_FACTION) continue;
		a_removeall(&f->attribs, &at_seenspell);
	}
}


static void
add_magrathea(void)
{
	unit * tans = ufindhash(atoi36("tans"));
	if (tans) u_setfaction(tans, findfaction(999));
}
#endif

#ifdef XMAS
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
		santa = createunit(r, findfaction(MONSTER_FACTION), 1, RC_ILLUSION);
		uunhash(santa);
		santa->no = atoi36("xmas");
		uhash(santa);
	}
	santa->age = 2;
	santa->irace = RC_DAEMON;
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
	plane * hell = create_new_plane(-1, "Hölle", 100000, 100000, 100020, 100020, PFL_NOCOORDS);

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
			if(u->race == RC_DRACOID) name_unit(u);
		}
	}
}
#endif

#if 0
static void
repair_illusion(void)
{
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			if (!race[u->race].nonplayer
				&& (get_item(u, I_PLATE_ARMOR) == u->number || get_item(u, I_CHAIN_MAIL) == u->number)
				&& get_item(u, I_HORSE) == u->number && get_skill(u, SK_SPEAR) == 0 && get_skill(u, SK_SWORD) == 0
				&& (get_item(u, I_SWORD) == u->number || get_item(u, I_SPEAR) == u->number))
			{
			  int i = 0;
			  skill_t sk;
			  for (sk=0;sk!=MAXSKILLS;++sk) if (sk!=SK_RIDING) i+=get_skill(u, sk);
			  if (get_skill(u, SK_OBSERVATION) == i) u->race=RC_SPELL;
			  else if (i) continue;
			  else {
				u->race = RC_ILLUSION;
				fprintf(stderr, "Illusion repariert: %s in Partei %s\n", unitname(u), factionid(u->faction));
			  }
			}
			if(!race[u->race].nonplayer && (
					strcmp(u->name, "Ausgemergelte Skelette") == 0 ||
					strcmp(u->name, "Zerfallende Zombies") == 0 ||
					strcmp(u->name, "Keuchende Ghoule") == 0
				)) {
				u->race = RC_UNDEAD;
				u->irace = RC_UNDEAD;
			}
		}
	}
}
#endif

#if 0
typedef struct border_info {
	border_type * type;
	region * from;
	direction_t dir;
	attrib * attribs;
} border_info;

static void *
resolve_border(void * data)
{
	border_info * info = (border_info *)data;
	region * to = rconnect(info->from, info->dir);
	if (!to) fprintf(stderr, "WARNING: border can only exist between two regions\n");
	else {
		border * b = new_border(info->type, info->from, to);
		b->attribs = info->attribs;
	}
	free(data);
	return NULL;
}
#endif

#if 0
static void
repair_undead(void)
{
	int no, age;
	FILE * f = fopen("undeads", "rt");
	if (!f) return;
	while (fscanf(f, "%d %d\n", &no, &age)!=EOF) {
		unit * u = findunitg(no, NULL);
		if (u && (u->race!=RC_UNDEAD || u->irace != RC_UNDEAD)) {
			if (u->age>=age) u->race = u->irace = RC_UNDEAD;
			fprintf(stderr, "Repariere %s\n", unitname(u));
		}
	}
}
#endif

#if 0
static void
reset_dragon_irace(void)
{
	region *r;
	unit *u;

	for(r=regions;r;r=r->next) {
		for(u=r->units;u;u=u->next) {
			if(u->race == RC_DRAGON || u->race == RC_WYRM) {
				u->irace = u->race;
			}
		}
	}
}
#endif

#if 0
static void
fix_irace(void)
{
	region *r;
	unit *u;

	for(r=regions; r; r=r->next) {
		for(u=r->units; u; u=u->next) {
			if(u->race != u->irace && u->race <= RC_AQUARIAN && u->race!=RC_DAEMON)
				u->irace = u->race;
		}
	}
}
#endif

#if 0
static void
fix_regions(void) {
	region * r;
	for (r=regions;r;r=r->next) {
		if (r->terrain!=T_GLACIER && r->terrain!=T_MOUNTAIN) {
			rsetiron(r, 0);
		}
		if (r->terrain==T_OCEAN) {
			unit * u;
			rsethorses(r, 0);
			rsetmoney(r, 0);
			rsetpeasants(r, 0);
			rsettrees(r, 0);
			for (u=r->units;u;u=u->next)
				if (humanoid(u) && u->ship==NULL) set_number(u, 0);
		}
	}
}
#endif

#if 0
static void
read_herbrepair(void) {
	FILE * f = fopen("repair.herbs", "rt");
	if (!f) return;
	while (!feof(f)) {
		int n, x;
		unit * u;
		fscanf(f, "%d %d", &n, &x);
		u = findunitg(x, NULL);
		if (!u) u = findunitg(n, NULL);
		fscanf(f, "%d", &x);
		while (x!=-1) {
			fscanf(f, "%d", &n);
			if (u) change_herb(u, (herb_t)x, n);
			fscanf(f, "%d", &x);
		}
	}
	fclose(f);
}

static void
write_herbrepair(void) {
	FILE * f = fopen("repair.herbs", "wt");
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			strlist * o;
			for (o=u->orders;o;o=o->next) {
				if (igetkeyword(o->s)==K_GIVE) {
					int n = read_unitid(u->faction, r);
					herb_t h;
					if (n<0) continue;
					if (getparam()==P_HERBS) {
						fprintf(f, "%d %d ", u->no, n);
						for (h=0;h!=MAXHERBS;++h) {
							int i = get_herb(u, h);
							if (i) fprintf(f, "%d %d ", h, i);
						}
						fputs("-1\n", f);
					}
				}
			}
		}
	}
	fclose(f);
}
#endif

#if 0
static void
write_migrepair(void)
{
	FILE * f = fopen("repair.migrants", "wt");
	region * r;
	for (r=regions;r;r=r->next) {
		if(rterrain(r) != T_OCEAN)
			fprintf(f, "%d %d %d %d\n", r->x, r->y, rhorses(r), rtrees(r));
	}
	fclose(f);
}

static void
read_migrepair(void)
{
	FILE * f = fopen("repair.migrants", "rt");
	int x, y, h, t;
	region *r;

	for(r=regions; r; r=r->next) freset(r, FL_DH);

	while (fscanf(f, "%d %d %d %d\n", &x, &y, &h, &t) != EOF) {
		r = findregion(x, y);
		if (r) {
			rsethorses(r, h);
			rsettrees(r, t);
			fset(r, FL_DH);
		}
	}

	for(r=regions; r; r=r->next) if(!fval(r, FL_DH) && rterrain(r) == T_PLAIN) {
		rsethorses(r, rand() % (terrain[rterrain(r)].production_max / 5));
	}

	fclose(f);
}
#endif

static void
fix_migrants(void) {
	region * r;
	for (r=regions;r;r=r->next) {
		unit * u;
		for (u=r->units;u;u=u->next) {
			if (u->race==RC_HUMAN) u->irace=RC_HUMAN;
			if (u->irace!=u->race && u->race!=RC_ILLUSION && u->race!=RC_DAEMON) {
				fprintf(stderr, "WARNUNG: %s ist ein %s, als %s getarnt\n", unitname(u), race[u->race].name[0], race[u->irace].name[0]);
			}
		}
	}
}

#if 0
static void
fix_folge_bug(void)
{
	region *td = findregion(64,-101);
	region *tl = findregion(65,-101);
	unit *u, *un;

	for(u=td->units;u;) {
		un = u->next;
		if(igetkeyword(u->lastorder) == K_FOLLOW) {
			move_unit(u, tl, NULL);
			freset(u, FL_MOVED);
			u = td->units;
		} else {
			u = un;
		}
	}
}
#endif

#if 0
static void
katja_erschaffe_elf(void)
{
	region *xon = findregion(6,-4);
	unit *u;
	faction *mon = findfaction(MONSTER_FACTION);
	strlist *S;

	u = createunit(xon, mon, 1, RC_ELF);
	set_string(&u->name,"Corwin");
	set_string(&u->display,"Ein kleiner, unscheinbarer Elf.");
	set_item(u, I_BIRTHDAYAMULET, 1);
	u->money = 10;
	sprintf(buf, "GIB H 1 Katzenamulett");
	S = makestrlist(buf); addlist(&u->orders, S);
	sprintf(buf, "BOTSCHAFT AN EINHEIT H %s",estring("Ein kleiner Elf verbeugt sich vor euch: 'Nehmt dieses Geschenk vom Gott aller Götter. Auf das euch noch ein langes und sehr glückliches Leben beschieden sei.'"));
	S = makestrlist(buf); addlist(&u->orders, S);
	set_string(&u->thisorder, "LERNE WAHRNEHMUNG");
	set_string(&u->thisorder, "LERNE WAHRNEHMUNG");
}
#endif

#if 0
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
#endif

#if 0
static void
no_teurefremde(boolean convert)
{
	region *r;
	unit *u;

	for(r=regions;r;r=r->next) {
		for(u=r->units;u;u=u->next) {
			if(u->faction->no != MONSTER_FACTION
					&& u->race != RC_ILLUSION
					&& u->race != RC_UNDEAD
					&& u->race != u->faction->race
					&& kor_teure_talente(u)) {
				if(convert) {
					u->race = u->faction->race;
					u->faction->num_migrants -= u->number;
				} else {
					printf("* Warnung, teurer Migrant: %s\n", unitname(u));
				}
			}
		}
	}
}
#endif

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
#endif

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

#if 0
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
fix_randencounter(void) {
	region * r;
	for (r=regions;r;r=r->next) {
		if (!r->land || rterrain(r)==T_GLACIER || r->units || r->ships || r->buildings) continue;
		if (fval(r, RF_ENCOUNTER)) {
			fputs("\n\nWARNING: FI_RANDENCOUNTER WAS STILL ENABLED\n\n", stderr);
			return;
		}
		fset(r, RF_ENCOUNTER);
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
#endif

static void
name_seaserpents(void)
{
	region *r;
	unit *u;

	for (r=regions;r;r=r->next) {
		for (u=r->units;u;u=u->next) {
			if(u->race == RC_SEASERPENT && strncmp(u->name, "Nummer ", 7)) {
				set_string(&u->name, "Seeschlange");
			}
		}
	}
}

static void
give_questling(void)
{
	unit *u1, *u2;

	if((u1=findunitg(15474, NULL))==NULL)  /* Questling */
		return;
	if((u2=findunitg(1429665, NULL))==NULL)  /* Harmloser Wanderer */
		return;
	if (u1->faction==u2->faction) return;

	add_message(&u1->faction->msgs,
		new_message(u1->faction, "give%u:unit%u:target%r:region%X:resource%i:amount",
			u1, u2, u1->region, r_unit, 1));
	u_setfaction(u1, u2->faction);
	add_message(&u2->faction->msgs,
		new_message(u1->faction, "give%u:unit%u:target%r:region%X:resource%i:amount",
			u1, u2, u1->region, r_unit, 1));
}

#if 0
static void
old_rsetroad(region * r, int val)
{
	attrib * a = a_find(r->attribs, &at_road);
	if (!a && val) a = a_add(&r->attribs, a_new(&at_road));
	else if (a && !val) a_remove(&r->attribs, a);
	if (val) a->data.i = val;
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

	spellid_t newspellids[] = { SPL_NOSPELL };

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
				updatespelllist(u);
			}
		}
	}

}

static void
fix_laen(void)
{
	region *r;

	for(r=regions; r; r=r->next) {
		if(rterrain(r) != T_MOUNTAIN) rsetlaen(r, -1);
	}
}

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
		printf("WARNING: fix_buildings: cannot open building.txt!\n");
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

extern plane * arena;

static void
fix_age(void)
{
	faction * f;
	for (f=factions;f;f=f->next) {
		if (f->no!=MONSTER_FACTION && !nonplayer_race(f->race)) continue;
		if (f->age!=turn) {
			fprintf(stderr, "  - Alter von Partei %s anpassen", factionid(f));
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

static int
count_demand(const region *r)
{
	struct demand *dmd;
	int c = 0;
	for (dmd=r->land->demands;dmd;dmd=dmd->next) c++;
	return c;
}

static void
fix_demand_region(const region *r)
{
	direction_t d;

	for (d=0;d!=MAXDIRECTIONS;++d) {
		region *nr = rconnect(r, d);
		if (nr && nr->land && count_demand(nr) != 7) {
			const luxury_type *sale=NULL;
			const luxury_type *ltype;
			struct demand *dmd, *dmd2;

			for(dmd = nr->land->demands; dmd; dmd=dmd->next)
				if(dmd->value == 0) sale = dmd->type;

			dmd2 = NULL;
			for (ltype = luxurytypes;ltype;ltype=ltype->next) {
				dmd = malloc(sizeof(struct demand));
				dmd->type = ltype;
				if(ltype == sale) {
					dmd->value = 0;
				} else {
					dmd->value = 1+rand()%5;
				}
			}
		}
	}
}

static void
fix_demand(void)
{
	region *r;

	for (r=regions; r; r=r->next) if (r->land) {
		if (count_demand(r) != 7) fix_demand_region(r);
	}
}

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

static void
fix_prices(void)
{
	FILE *fp;
	char buf[256];
	char *token;
	int x, y, p, i;
	region *r;

	puts(" - Korrigiere Handelsgüterpreise");

	fp = fopen("prices.fix", "r");
	if (fp==NULL) return;

	while(fgets(buf, 256, fp)!=NULL) {
		token = strtok(buf, ":");		/* Name */
		token = strtok(NULL, ":"); x = atoi(token);
		token = strtok(NULL, ":"); y = atoi(token);
		r = findregion(x, y);
		assert(r != NULL);
		if(r->land) {
			for(i = 0; oldluxurytype[i]!=NULL; i++) {
				token = strtok(NULL, ":"); p = atoi(token);
				r_setdemand(r, oldluxurytype[i], p);
			}
		}
	}

	fclose(fp);
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

#if 0
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
#ifdef NO_FOREST
				if (r_isforest(r)) {
					int i = rand()%3+3;
					const char * name = terrain[T_PLAIN].herbs[i];
					const item_type * itype = finditemtype(name, NULL);
					r->land->herbtype = resource2herb(itype->rtype);
				}
#endif
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

static void
init_mwarden(void)
{
	unit *u = findunitg(atoi36("mwar"), NULL);
	attrib *a;

	if(!u) return;

	a = a_add(&u->attribs, a_new(&at_warden));
	a->data.i = 0;
}

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
						if (nonplayer(u) || (!nonplayer(u2) && u->race==RC_GOBLIN))
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

#include <items/lmsreward.h>
static void
lms_special(unit * u)
{
	if (u) i_change(&u->items, &it_lmsreward, 1);
}

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

void
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

void
fix_baumringel(void)
{
	region *r;
  const item_type *itype = finditemtype("Blauer Baumringel", NULL);
  const herb_type *htype = resource2herb(itype->rtype);

	for(r=regions; r; r=r->next) {
		if(rterrain(r) == T_PLAIN && rand()%6 == 5) {
     	rsetherbtype(r, htype);
		}
	}
}

#include <modules/gmcmd.h>
void setup_gm_faction(void);

void
korrektur(void)
{
#ifdef TEST_GM_COMMANDS
	setup_gm_faction();
#endif
	
	/* Wieder entfernen! */
	do_once(atoi36("trgr"), fix_targetregion_resolve())

	/* fix_herbtypes(); */
#ifdef CONVERT_TRIGGER
	do_once(atoi36("cvtr"), convert_triggers());
#endif
	fix_migrants();
	fix_allies();
#ifndef SKILLFIX_SAVE
	fix_skills();
#endif
	stats();
	/* Turn ist noch nicht inkrementiert! Also immer <eingelesenes Datenfile>.
	 * das gilt aber nicht, wenn man es aus der console ('c') aufruft.
	 */
	switch (turn) {
	case 188:
		name_seaserpents();
		break;
	case 189:
		break;
	case 190:
		give_questling();
		break;
	case 191:
		give_questling(); /* try again */
		break;
	case 194:
		remove_impossible_dragontargets();
		break;
	case 195:
		remove_impossible_dragontargets();
		break;
	case 197:
		fix_laen();
		break;
	case 208:
	  fix_feuerwand_orks();
	  break;
	case 209:
	  fix_buildings();
#if 0
	  fix_traveldir();
#endif
	  break;
	case 212:
		fix_prices();
		break;
	case 215:
		fix_options();
		init_region_age();
		break;
	case 217:
		init_mwarden();
		break;
	}
	do_once(atoi36("fxfa"), fix_vertrautenmagie());
	do_once(atoi36("uddp"), undo_deadpeasants());
  do_once(atoi36("lmsr"), lms_special(findunit(atoi36("tt3g"))))
	do_once(atoi36("brng"), fix_baumringel());
	do_once(atoi36("demd"), fix_demand());

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
#ifdef XMAS
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
