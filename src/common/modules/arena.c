/* vi: set ts=2:
 *
 *	$Id: arena.c,v 1.3 2001/02/02 08:40:46 enno Exp $
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
#include "arena.h"

/* modules include */
#include "score.h"

/* attributes include */
#include <attributes/giveitem.h>

/* items include */
#include <items/demonseye.h>

/* kernel includes */
#include <unit.h>
#include <faction.h>
#include <item.h>
#include <movement.h>
#include <message.h>
#include <reports.h>
#include <plane.h>
#include <pool.h>
#include <region.h>
#include <building.h>
#include <magic.h>
#include <race.h>

/* util include */
#include <base36.h>
#include <resolve.h>
#include <functions.h>
#include <event.h>

/* libc include */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

extern int hashstring(const char* s);

/* exports: */
plane * arena = NULL;


/* local vars */
#define CENTRAL_VOLCANO 1

static int arena_id = 0;
static region * arena_center = NULL;
static region * tower_region[6];
static region * start_region[6];
static int newarena = 0;

static region *
arena_region(int magic) {
	return tower_region[magic];
}

static building *
arena_tower(int magic) {
	return arena_region(magic)->buildings;
}

static boolean
give_igjarjuk(const unit * src, const unit * d, const item_type * itype, int n, const char * cmd)
{
	sprintf(buf, "Eine höhere Macht hindert %s daran, das Objekt zu übergeben. "
			"'ES IST DEINS, MEIN KIND. DEINS GANZ ALLEIN'.", unitname(src));
	mistake(src, cmd, buf, MSG_COMMERCE);
	return false;
}

static int
leave_fail(unit * u) {
	sprintf(buf, "Der Versuch, die Greifenschwingen zu benutzen, schlug fehl. %s konnte die Ebene der Herausforderung nicht verlassen.", unitname(u));
	addmessage(NULL, u->faction, buf, MSG_MESSAGE, ML_IMPORTANT);
	return 1;
}

static int
leave_arena(struct unit * u, const struct item_type * itype, const char * cmd)
{
	if (!u->building && leave_fail(u)) return -1;
	if (u->building!=arena_tower(u->faction->magiegebiet) && leave_fail(u)) return -1;
	unused(cmd);
	unused(itype);
	assert(!"not implemented");
	return 0;
}

static resource_type rt_gryphonwing = {
	{ "griphonwing", "griphonwings" },
	{ "griphonwing", "griphonwings" },
	RTF_ITEM,
	&res_changeitem
};

static item_type it_gryphonwing = {
	&rt_gryphonwing,           /* resourcetype */
	ITF_NOTLOST|ITF_CURSED, 0, 0,       /* flags, weight, capacity */
	0, NOSKILL,              /* minskill, skill */
	NULL,                    /* construction */
	&leave_arena,
	&give_igjarjuk
};

static int
enter_fail(unit * u) {
	sprintf(buf, "In %s erklingt die Stimme des Torwächters: 'Nur wer ohne materielle Güter und noch lernbegierig ist, der darf die Ebene der Herausforderung betreten. Und vergiß nicht mein Trinkgeld.'. %s erhielt keinen Einlaß.", regionid(u->region), unitname(u));
	addmessage(NULL, u->faction, buf, MSG_MESSAGE, ML_IMPORTANT);
	return 1;
}

static int
enter_arena(unit * u, const item_type * itype, const char * cmd)
{
	skill_t sk;
	region * r = u->region;
	unit * u2;
	int fee = u->faction->score / 5;
	unused(cmd);
	unused(itype);
	if (fee>2000) fee = 2000;
	if (getplane(r)==arena) return -1;
	if (u->number!=1 && enter_fail(u)) return -1;
	if (get_pooled(u, r, R_SILVER) < fee && enter_fail(u)) return -1;
	for (sk=0;sk!=MAXSKILLS;++sk) {
		if (get_skill(u, sk)>30 && enter_fail(u)) return -1;
	}
	for (u2=r->units;u2;u2=u2->next) if (u2->faction==u->faction) break;
#ifdef NEW_ITEMS
	assert(!"not implemented");
#else
	for (res=0;res!=MAXRESOURCES;++res) if (res!=R_SILVER && res!=R_ARENA_GATE && (is_item(res) || is_herb(res) || is_potion(res))) {
		int x = get_resource(u, res);
		if (x) {
			if (u2) {
				change_resource(u2, res, x);
				change_resource(u, res, -x);
			}
			else if (enter_fail(u)) return -1;
		}
	}
#endif
	if (get_money(u) > fee) {
		if (u2) change_money(u2, get_money(u) - fee);
		else if (enter_fail(u)) return -1;
	}
	sprintf(buf, "In %s öffnet sich ein Portal. Eine Stimme ertönt, und spricht: 'Willkommen in der Ebene der Herausforderung'. %s durchschreitet das Tor zu einer anderen Welt.", regionid(u->region), unitname(u));
	addmessage(NULL, u->faction, buf, MSG_MESSAGE, ML_IMPORTANT);
	new_use_pooled(u, &rt_gryphonwing, GET_SLACK|GET_RESERVE, 1);
	use_pooled(u, r, R_SILVER, fee);
	set_money(u, 109);
	fset(u, FL_PARTEITARNUNG);
	move_unit(u, start_region[rand() % 6], NULL);
	return 0;
}

static resource_type rt_arenagate = {
	{ "eyeofdragon", "eyeofdragons" },
	{ "eyeofdragon", "eyeofdragons" },
	RTF_ITEM,
	&res_changeitem
};

static item_type it_arenagate = {
	&rt_arenagate,           /* resourcetype */
	ITF_NONE, 0, 0,          /* flags, weight, capacity */
	0, NOSKILL,              /* minskill, skill */
	NULL,                    /* construction */
	&enter_arena
};

/***
 ** Szepter der Tränen, Demo-Item
 ***/

static int
use_wand_of_tears(unit * user, const struct item_type * itype, const char * cmd)
{
	unit * u;
	unused(cmd);
	for (u=user->region->units;u;u=u->next) {
		if (u->faction != user->faction) {
			int i;

			for (i=0;i!=u->skill_size;++i)
				change_skill(u, u->skills[i].id, -10);
			add_message(&u->faction->msgs, new_message(u->faction, "wand_of_tears_effect%u:unit", u));
		}
	}
	add_message(&user->region->msgs, new_message(NULL, "wand_of_tears_usage%u:unit", user));
	return 0;
}

static void
init_wand_of_tears(void)
{
	const char * names[2] = {"wand_of_tears", "wands_of_tears"};
	const char * appearances[2] = {"wand", "wands"};
	item_type * itype = it_find(names[0]);
	int i;

	if (itype==NULL) {
		/* Dieser Teil kann, nachdem sie ausgeteilt wurden, gänzlich verschwinden. */
		resource_type * rtype = new_resourcetype(names, appearances, RTF_DYNAMIC|RTF_ITEM);
		itype = new_itemtype(rtype, ITF_DYNAMIC|ITF_NOTLOST, 1, 0, 0, NOSKILL);
		itype->use = use_wand_of_tears;
		for (i=0;i!=6;++i) {
			unit * u = tower_region[i]->units;
			if (u==NULL) continue;
			i_change(&u->items, itype, 1);
		}
	}
}

/**
 * Tempel der Schreie, Demo-Gebäude **/

static int
age_hurting(attrib * a) {
	building * b = (building *)a->data.v;
	unit * u;
	int active = 0;
	if (b==NULL) return 0;
	for (u=b->region->units;u;u=u->next) {
		if (u->building==b) {
			if (u->faction->magiegebiet==M_CHAOS) {
				active ++;
				add_message(&b->region->msgs, new_message(NULL, "praytoigjarjuk%u:unit", u));
			}
		}
	}
	if (active) for (u=b->region->units;u;u=u->next) if (!nonplayer_race(u->faction->race)) {
		int i;
		if (u->faction->magiegebiet!=M_CHAOS) {
			for (i=0;i!=active;++i) u->hp = (u->hp+1) / 2; /* make them suffer, but not die */
			add_message(&b->region->msgs, new_message(NULL, "cryinpain%u:unit", u));
		}
	}
	return 1;
}

static void
write_hurting(const attrib * a, FILE * F) {
	building * b = a->data.v;
	fprintf(F, "%d", b->no);
}

static int
read_hurting(attrib * a, FILE * F) {
	int i;
	fscanf(F, "%d", &i);
	a->data.v = (void*)findbuilding(i);
	if (a->data.v==NULL) {
		fprintf(stderr, "WARNING: temple of pain is broken\n");
		return 0;
	}
	return 1;
}

static attrib_type at_hurting = {
	"hurting", NULL, NULL, age_hurting, write_hurting, read_hurting
};

static void
make_temple(region * r)
{
	const building_type * btype = bt_find("temple");
	building * b;
	if (btype==NULL)
		btype = bt_make("temple", BTF_UNIQUE | BTF_NOBUILD | BTF_INDESTRUCTIBLE, -1, 2, 50);
	else {
		b = r->buildings;
		while (b!=NULL && b->type!=btype) b = b->next;
		if (b!=NULL) return; /* gibt schon einen */
	}

	b = new_building(btype, r, NULL);
	b->size = btype->maxsize;
	b->name = strdup("Igjarjuk's Tempel der Schreie");
	b->display = strdup("Ein Schrein aus spitzen Knochen und lodernden Flammen, gewidmet dem Wyrm der Wyrme");
	a_add(&b->attribs, a_new(&at_hurting))->data.v=b;
}

/**
 * Initialisierung Türme */

static void
tower_init(void)
{
	int i, first = newarena;
	for (i=0;i!=6;++i) {
		region * r = tower_region[i] = findregion(arena_center->x+delta_x[i]*3, arena_center->y+delta_y[i]*3);
        if (r) {		                                                                                                                    
			start_region[i] = findregion(arena_center->x+delta_x[i]*2, arena_center->y+delta_y[i]*2);                               
			if (r->terrain!=T_DESERT) terraform(r, T_DESERT);                                                                       
			if (!r->buildings) {                                                                                                    
				building * b = new_building(&bt_castle, r, NULL);                                                                   
				b->size = 10;                                                                                                       
				if (i!=0) sprintf(buf, "Turm des %s", neue_gebiete[i]);                                                             
				else sprintf(buf, "Turm der Ahnungslosen");                                                                         
				set_string(&b->name, buf);                                                                                          
			}                                                                                                                       
		}          
	}
	if (first && !arena_center->buildings) {
		building * b = new_building(&bt_castle, arena_center, NULL);
		attrib * a;
		item * items;

		i_add(&items, i_new(&it_gryphonwing))->number=1;
		i_add(&items, i_new(&it_demonseye))->number=1;
		a = a_add(&b->attribs, make_giveitem(b, items));

		b->size = 10;
		set_string(&b->name, "Höhle des Greifen");
	}
}

static void
guardian_faction(plane * pl, int id)
{
	unsigned int i;
	region * r;
	faction * f = findfaction(id);

	if (!f) {
		f = calloc(1, sizeof(faction));
		f->banner = strdup("Sie dienen dem großen Wyrm");
		f->passw = strdup("    ");
		for (i = 0; i < 4; i++) f->passw[i] = (char) (97 + rand() % 26);
		f->email = strdup("igjarjuk@eressea-pbem.de");
		f->name = strdup("Igjarjuks Kundschafter");
		f->race = RC_ILLUSION;
		f->age = turn;
		f->locale = find_locale("de");
		f->options = want(O_COMPRESS) | want(O_REPORT) | want(O_COMPUTER) | want(O_ADRESSEN) | want(O_DEBUG);

		f->no = id;
		addlist(&factions, f);
	}
	if (f->race != RC_ILLUSION) {
		assert(!"guardian id vergeben");
		exit(0);
	}
	f->lastorders = turn;
	f->alive = true;
	for (r=regions;r;r=r->next) if (getplane(r)==pl && r->terrain!=T_FIREWALL)
	{
		unit * u;
		freset(r, RF_ENCOUNTER);
		for (u=r->units;u;u=u->next) {
			if (u->faction==f) break;
		}
		if (u) continue;
		u = createunit(r, f, 1, RC_GOBLIN);
		set_string(&u->name, "Igjarjuks Auge");
		set_item(u, I_RING_OF_INVISIBILITY, 1);
		u->thisorder = calloc(1, sizeof(char));
		u->lastorder = calloc(1, sizeof(char));
		fset(u, FL_PARTEITARNUNG);
		set_money(u, 1000);
	}
}

static void 
block_create(int x1, int y1, char terrain)
{
	int x, y;
	for (x=0;x!=BLOCKSIZE;++x) {
		for (y=0;y!=BLOCKSIZE;++y) {
			region * r = new_region(x1 + x, y1 + y);
			terraform(r, terrain);
		}
	}
}

#ifdef CENTRAL_VOLCANO
building_type bt_caldera = {
	"caldera", /* _name */
	BTF_NOBUILD|BTF_INDESTRUCTIBLE,  /* flags */
	1, -1, -1, /* capac, maxcap, maxsize */
	NULL, /* maintenance */
	NULL, /* construction */
	NULL       /* name() */
};


static int
caldera_handle(trigger * t, void * data)
{
	/* call an event handler on caldera.
	 * data.v -> ( variant event, int timer )
	 */
	building *b = (building *)t->data.v;
	if (b!=NULL) {
		unit ** up = &b->region->units;
		while (*up) {
			unit * u = *up;
			if (u->building==b) {
				sprintf(buf, "%s springt in die ewigen Feuer des Kraters.", unitname(u));
				if (u->items) {
					item ** ip = &u->items;
					strcat(buf, " Mit der sterblichen Hülle des Helden verglühen");
					while (*ip) {
						item * i = *ip;
						char zText[10];
						sprintf(zText, " %d %s", i->number, locale_string(NULL, resourcename(i->type->rtype, i->number!=1)));
						strcat(buf, zText);
						i_remove(ip, i);
						if (*ip==i) ip=&i->next;
						if (i->next) strcat(buf, ",");
					}
					strcat(buf, ".");
				}
				addmessage(u->region, NULL, buf, MSG_MESSAGE, ML_IMPORTANT);
				set_number(u, 0);
			}
			if (*up==u) up = &u->next;
		}
	} else
		fprintf(stderr, "\aERROR: could not perform caldera::handle()\n");
	unused(data);
	return 0;
}

static void
caldera_write(const trigger * t, FILE * F)
{
	building *b = (building *)t->data.v;
	fprintf(F, "%s ", itoa36(b->no));
}

static int
caldera_read(trigger * t, FILE * F)
{
	char zText[128];
	int i;

	fscanf(F, "%s", zText);
	i = atoi36(zText);
	t->data.v = findbuilding(i);
	if (t->data.v==NULL) ur_add((void*)i, &t->data.v, resolve_building);

	return 1;
}

struct trigger_type tt_caldera = {
	"caldera",
	NULL,
	NULL,
	caldera_handle,
	caldera_write,
	caldera_read
};

static trigger *
trigger_caldera(building * b)
{
	trigger * t = t_new(&tt_caldera);
	t->data.v = b;
	return t;
}

static void
init_volcano(void)
{
	building * b;
	region * r = arena_center;
	assert(arena_center);
	if (r->terrain!=T_DESERT) return; /* been done before */
	terraform(arena_center, T_VOLCANO_SMOKING);
	b = new_building(&bt_caldera, r, NULL);
	b->size = 1;
	b->name = strdup("Igjarjuk's Schlund");
	b->display = strdup("Feurige Lava fließt aus dem Krater des großen Vulkans. Alles wird von ihr verschlungen.");
	add_trigger(&b->attribs, "timer", trigger_caldera(b));
	tt_register(&tt_caldera);
}
#endif

void
create_arena(void)
{
	int x;
	arena_id = hashstring("arena");
	arena = getplanebyid(arena_id);
	if (arena!=NULL) return;
	score(); /* ist wichtig, damit alle Parteien einen score haben, wenn sie durchs Tor wollen. */
	guardian_faction(arena, 999);
	if (arena) arena_center = findregion(plane_center_x(arena), plane_center_y(arena));
	if (!arena_center) {
		newarena = 1;
		arena = create_new_plane(arena_id, "Arena", -10000, -10000, 0, BLOCKSIZE-1, PFL_LOWSTEALING | PFL_NORECRUITS | PFL_NOALLIANCES);
		block_create(arena->minx, arena->miny, T_OCEAN);
		arena_center = findregion(plane_center_x(arena), plane_center_y(arena));
		for (x=0;x!=BLOCKSIZE;++x) {
			int y;
			for (y=0;y!=BLOCKSIZE;++y) {
				region * r = findregion(arena->minx+x, arena->miny+y);
				freset(r, RF_ENCOUNTER);
				r->planep = arena;
				switch (distance(r, arena_center)) {
				case 4:
					terraform(r, T_FIREWALL);
					break;
				case 0:
					terraform(r, T_GLACIER);
					break;
				case 1:
					terraform(r, T_SWAMP);
					break;
				case 2:
					terraform(r, T_MOUNTAIN);
					break;
				}
			}
		}
	}
	make_temple(arena_center);
#ifdef CENTRAL_VOLCANO
	init_volcano();
#else
	if (arena_center->terrain!=T_DESERT) terraform(arena_center, T_DESERT);
#endif
	rsetmoney(arena_center, 0);
	rsetpeasants(arena_center, 0);
	tower_init();
	init_wand_of_tears();
}

void
init_arena(void)
{
	at_register(&at_hurting);
	init_demonseye();
	it_register(&it_arenagate);
	it_register(&it_gryphonwing);
	register_function((pf_generic)use_wand_of_tears, "use_wand_of_tears");
	tt_register(&tt_caldera);
}

