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

#define LOCALE_CHECK
#ifdef __LCC__
#undef LOCALE_CHECK
#endif

#include <config.h>
#include <eressea.h>

#include "korrektur.h"

/* initialization - TODO: init in separate module */
#include <races/races.h>
#include <attributes/attributes.h>
#include <triggers/triggers.h>
#include <items/items.h>

/* modules includes */
#include <modules/dungeon.h>
#include <modules/score.h>
#include <modules/gmcmd.h>
#ifdef INFOCMD_MODULE
# include <modules/infocmd.h>
#endif

/* gamecode includes */
#include <economy.h>
#include <goodies.h>
#include <laws.h>

/* kernel includes */
#include <alchemy.h>
#include <building.h>
#include <faction.h>
#include <message.h>
#include <plane.h>
#include <race.h>
#include <skill.h>
#include <technology.h>
#include <teleport.h>
#include <unit.h>
#include <region.h>
#include <reports.h>
#include <resources.h>
#include <save.h>
#include <ship.h>
#include <border.h>
#include <region.h>
#include <item.h>

/* util includes */
#include <rand.h>
#include <sql.h>
#include <base36.h>

#ifdef HAVE_LUA
/* lua includes */
#include "lua/bindings.h"
#include <lua.hpp>
#include <luabind/luabind.hpp>
#endif

/* libc includes */
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <ctime>
#include <clocale>


/**
 ** global variables we are importing from other modules
 **/

extern "C" {
#ifdef BETA_CODE
extern int xml_writeitems(const char * filename);
extern int xml_writeships(void);
extern int xml_writebuildings(void);
#endif

extern item_type * i_silver;

extern boolean nonr;
extern boolean nocr;
extern boolean noreports;
extern boolean nomer;
extern boolean nomsg;
extern boolean nobattle;
extern boolean nobattledebug;
extern boolean dirtyload;

extern int loadplane;

extern void debug_messagetypes(FILE * out);
extern void free_region(region * r);
extern void render_init(void);
extern void free_borders(void);
}


/**
 ** global variables wthat we are exporting
 **/
static char * orders = NULL;
static char * xmlfile = NULL;
static int nowrite = 0;
static boolean g_writemap = false;
static boolean g_killeiswald = false;

struct settings global = {
	"Eressea", /* gamename */
	"eressea", /* resourcepath */
	1000, /* maxunits */
};

static int
crwritemap(void)
{
  FILE * F = fopen("world.cr", "w+");
  region * r;
  for (r=regions;r;r=r->next) {
    plane * p = rplane(r);
    fprintf(F, "REGION %d %d %d\n", r->x, r->y, p?p->id:0);
    fprintf(F, "\"%s\";Name\n\"%s\";Terrain\n", rname(r, default_locale), LOC(default_locale, rterrain(r)->name));
  }
  fclose(F);
	return 0;
}

static void
game_init(void)
{
	init_triggers();

	report_init();
	creport_init();

	debug_language("locales.log");
	register_skills();

	register_raceclass();
	register_races();
	register_resources();
	register_terrains();
	register_buildings();
	register_ships();
	register_items();
#ifdef MAGIC
	register_spells();
#endif
  register_dungeon();
	register_technology();
	register_alchemy();
#ifdef MUSEUM_MODULE
	register_museum();
#endif
#ifdef ARENA_MODULE
	register_arena();
#endif

	init_data(xmlfile?xmlfile:"eressea.xml");
	init_locales();

	init_attributes();
	init_resources();
	init_items();
	init_economy();
	init_rawmaterials();

	init_gmcmd();
#ifdef INFOCMD_MODULE
	init_info();
#endif
	init_conversion();

#ifdef REMOVE_THIS
	render_init();
	{
		FILE * F = fopen("messagetypes.txt", "w");
		debug_messagetypes(F);
		fclose(F);
		abort();
	}
#endif
}

static void
getgarbage(void)
{
	faction *f;

	/* Get rid of stuff that was only relevant last turn */

	for (f = factions; f; f = f->next) {
/*		memset(f->showdata, 0, sizeof f->showdata); */

		freestrlist(f->mistakes);
		f->mistakes = 0;
		/* TODO: free msgs */
	}
#if 0
	for (r = regions; r; r = r->next) {
		freestrlist(r->comments);
		r->comments = 0;
		freestrlist(r->botschaften);
		r->botschaften = 0;
	}
#endif
}

#if 0
static void
writefactiondata(void)
{
	FILE *F;
	char zText[128];

	sprintf(zText, "%s/faction-data.xml", basepath());
	F = cfopen(zText, "w");
	if(F) {
		faction *f;
		for(f = factions; f; f=f->next) {
			fprintf(F,"<faction>\n");
			fprintf(F,"\t<id>%s</id>\n",itoa36(f->no));
			fprintf(F,"\t<name>%s</name>\n",f->name);
			fprintf(F,"\t<email>%s</email>\n",f->email);
			fprintf(F,"\t<password>%s</password>\n",f->passw);
			fprintf(F,"\t<race>%s</race>\n",rc_name(f->race,1));
			fprintf(F,"\t<magic>%s</magic>\n",neue_gebiete[f->magiegebiet]);
			fprintf(F,"\t<nmrs>%d</nmrs>\n",turn-f->lastorders);
			fprintf(F,"\t<score>%d</score>\n",f->score);
			fprintf(F,"</faction>\n");
		}
	}
	fclose(F);
}
#endif

static void
writepasswd(void)
{
	FILE * F;
	char zText[128];

	sprintf(zText, "%s/passwd", basepath());
	F = cfopen(zText, "w");
	if (F) {
		faction *f;
		puts("Schreibe Passwörter...");

		for (f = factions; f; f = f->next) {
			fprintf(F, "%s:%s:%s:%s\n",
				factionid(f), f->email, f->passw, f->override);
		}
		fclose(F);
	}
}

#ifdef HAVE_LUA
lua_State * luaState;

void 
lua_init(void)
{
  luaState = lua_open();
  luaopen_base(luaState);
  luabind::open(luaState);
  bind_region(luaState);
  bind_unit(luaState);
  bind_ship(luaState);
  bind_building(luaState);
}

void 
lua_done(void)
{
  lua_close(luaState);
}
#endif

static int
processturn(char *filename)
{
  struct summary * begin, * end;
	int i;

	begin = make_summary(false);
	printf(" - Korrekturen Runde %d\n", turn);
	korrektur();
#ifdef HAVE_LUA
  lua_dofile(luaState, "test.lua");
#endif
	turn++;
	puts(" - entferne Texte der letzten Runde");
	getgarbage();
	puts(" - Nehme Korrekturen am Datenbestand vor");
	if ((i=readorders(filename))!=0) return i;
#if BENCHMARK
	exit(0);
#endif
	processorders();
	score();
#ifdef WACH_WAFF
	remove_unequipped_guarded();
#endif
	korrektur_end();
	if (!noreports) reports();
	free_units();
	puts(" - Beseitige leere Parteien");
	remove_empty_factions(true);
	end = make_summary(true);
	report_summary(end, begin, false);
	report_summary(end, begin, true);
	free(end);
	free(begin);
	writepasswd();
	if (!nowrite) {
		char ztext[64];
		sprintf(ztext, "%s/%d", datapath(), turn);
		writegame(ztext, 0);
	}
	return 0;
}

#ifdef CLEANUP_CODE
static void
game_done(void)
{
	/* Diese Routine enfernt allen allokierten Speicher wieder. Das ist nur
	 * zum Debugging interessant, wenn man Leak Detection hat, und nach
	 * nicht freigegebenem Speicher sucht, der nicht bis zum Ende benötigt
	 * wird (temporäre Hilsstrukturen) */
	unit *u, *u2;
	region *r, *r2;
	building *b, *b2;
	faction *f, *f2;
	ship *s, *s2;

	free(used_faction_ids);
	for (r = regions; r; r = r2) {
#if 0
		msg * m = r->msgs;
		while (m) {
			msg * x = m;
			m = m->next;
			if (x->type->finalize) x->type->finalize(x);
			free(x);
		}
			rm = rm->next;
		}
#endif
		for (u = r->units; u; u = u2) {
			u2 = u->next;
			stripunit(u);
			uunhash(u);
			free(u);
		}
		for (b = r->buildings; b; b = b2) {
			free(b->name);
			free(b->display);
			b2 = b->next;
			free(b);
		}
		for (s = r->ships; s; s = s2) {
			free(s->name);
			free(s->display);
			s2 = s->next;
			free(s);
		}
		r2 = r->next;
		free_region(r);
	}
	for (f = factions; f; f = f2) {
		stripfaction(f);
		f2 = f->next;
		free(f);
	}
	while (planes) {
		plane * pl = planes;
		planes = planes->next;
		free(pl);
	}
#ifdef LEAK_DETECT
	leak_report(stderr);
#endif
	creport_cleanup();
	report_cleanup();
}
#endif

#if MALLOCDBG
static void
init_malloc_debug(void)
{
#if (defined(_MSC_VER))
# if MALLOCDBG == 2
#  define CHECKON() _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF)
# elif MALLOCDBG == 1
#  define CHECKON() _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF)
# endif
#endif
}
#endif

#if 0
static void
write_stats(void)
{
	FILE * F;
	char zText[MAX_PATH];
	strcat(strcpy(zText, resourcepath()), "/spells");
	F = fopen(zText, "wt");
	if (F) {
		int i, m = -1;
		for (i=0;spelldaten[i].id;++i) {
			if (spelldaten[i].magietyp!=m) {
				m=spelldaten[i].magietyp;
				fprintf(F, "\n%s\n", magic_schools[m]);
			}
			fprintf(F, "%d\t%s\n", spelldaten[i].level, spelldaten[i].name);
		}
		fclose(F);
	} else {
		sprintf(buf, "fopen(%s): ", zText);
		perror(buf);
	}
	strcat(strcpy(zText, resourcepath()), "/bonus");
	F = fopen(buf, "wt");
	if (F) {
		race_t r;
		for (r=0;r!=MAXRACES;++r) {
			skill_t sk;
			int i = 0;
			fprintf(F, "const bonus %s_bonus = {\n\t", race[r].name[0]);
			for (sk=0;sk!=MAXSKILLS;sk++) {
				if (race[r].bonus[sk]) {
					if (i==8) {
						i = 0;
						fputs("\n\t", F);
					}
					fprintf(F, "{ SK_%s, %d }, ", skillname(sk, NULL), race[r].bonus[sk]);
					++i;
				}
			}
			fputs("{ SK_NONE, 0 }\n};\n", F);
		}
		fclose(F);
	} else {
		sprintf(buf, "fopen(%s): ", zText);
		perror(zText);
	}
}
#endif

static int
usage(const char * prog, const char * arg)
{
	if (arg) {
		fprintf(stderr, "unknown argument: %s\n\n", arg);
	}
	fprintf(stderr, "Usage: %s [options]\n"
		"-x n             : Lädt nur die ersten n regionen\n"
		"-f x y           : Lädt nur die regionen ab (x,y)\n"
		"-v befehlsdatei  : verarbeitet automatisch die angegebene Befehlsdatei\n"
		"-d datadir       : gibt das datenverzeichnis an\n"
		"-b basedir       : gibt das basisverzeichnis an\n"
		"-r resdir        : gibt das resourceverzeichnis an\n"
		"-t turn          : read this datafile, not the most current one\n"
		"-o reportdir     : gibt das reportverzeichnis an\n"
		"-l logfile       : specify an alternative logfile\n"
		"--noeiswald      : beruhigt ungemein\n"
		"--nomsg          : keine Messages (RAM sparen)\n"
		"--nobattle       : keine Kämpfe\n"
		"--nodebug        : keine Logfiles für Kämpfe\n"
		"--debug          : schreibt Debug-Ausgaben in die Datei debug\n"
		"--nocr           : keine CRs\n"
		"--nonr           : keine Reports\n"
#ifdef USE_MERIAN
		"--nomer          : keine Meriankarten\n"
#endif
		"--help           : help\n", prog);
	return -1;
}

static int
read_args(int argc, char **argv)
{
	int i;
	for (i=1;i!=argc;++i) {
		if (argv[i][0]!='-') {
			return usage(argv[0], argv[i]);
		} else if (argv[i][1]=='-') { /* long format */
			if (strcmp(argv[i]+2, "nocr")==0) nocr = true;
			else if (strcmp(argv[i]+2, "nosave")==0) nowrite = true;
			else if (strcmp(argv[i]+2, "noreports")==0) {
				noreports = true;
				nocr = true;
				nocr = true;
			}
			else if (strcmp(argv[i]+2, "xml")==0) xmlfile = argv[++i];
			else if (strcmp(argv[i]+2, "dirtyload")==0) dirtyload = true;
			else if (strcmp(argv[i]+2, "nonr")==0) nonr = true;
			else if (strcmp(argv[i]+2, "nomsg")==0) nomsg = true;
			else if (strcmp(argv[i]+2, "noeiswald")==0) g_killeiswald = true;
			else if (strcmp(argv[i]+2, "nobattle")==0) nobattle = true;
			else if (strcmp(argv[i]+2, "nodebug")==0) nobattledebug = true;
#ifdef USE_MERIAN
			else if (strcmp(argv[i]+2, "nomer")==0) nomer = true;
#endif
			else if (strcmp(argv[i]+2, "help")==0)
				return usage(argv[0], NULL);
			else
				return usage(argv[0], argv[i]);
		} else switch(argv[i][1]) {
			case 'o':
				g_reportdir = argv[++i];
				break;
			case 'd':
				g_datadir = argv[++i];
				break;
			case 'r':
				g_resourcedir = argv[++i];
				break;
			case 'b':
				g_basedir = argv[++i];
				break;
			case 't':
				turn = atoi(argv[++i]);
				break;
			case 'f':
				firstx = atoi(argv[++i]);
				firsty = atoi(argv[++i]);
				break;
			case 'q':
				quiet = 1;
				break;
			case 'v':
				if (i<argc) orders = argv[++i];
				else return usage(argv[0], argv[i]);
				break;
			case 'p':
				loadplane = atoi(argv[++i]);
				break;
			case 'x':
				maxregions = atoi(argv[++i]);
				maxregions = (maxregions*81+80) / 81;
				break;
			case 'X':
				dirtyload = true;
				break;
			case 'l':
				log_open(argv[++i]);
				break;
			case 'w':
 				g_writemap = true;
				break;
			default:
				usage(argv[0], argv[i]);
		}
	}
	return 0;
}

typedef struct lostdata {
	int x, y;
	int prevunit;
	int building;
	int ship;
} lostdata;

static void
confirm_newbies(void)
{
	const faction * f = factions;
	if (sqlstream) while (f) {
		if (f->age==0) {
			fprintf(sqlstream, "UPDATE subscriptions SET status='ACTIVE', faction='%s' WHERE game=%d AND password='%s';\n", itoa36(f->no), GAME_ID, f->passw);
		}
		f = f->next;
	}
}

int
main(int argc, char *argv[])
{
  int i;
	char zText[MAX_PATH];

	updatelog = fopen("update.log", "w");
	log_open("eressea.log");
	printf("\n%s PBEM host\n"
		"Copyright (C) 1996-2001 C.Schlittchen, K.Zedel, E.Rehling, H.Peters.\n\n"
	   "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n", global.gamename, version());

	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");
#ifdef LOCALE_CHECK
	if (!locale_check()) {
		log_error(("The current locale is not suitable for international Eressea.\n"));
		return -1;
	}
#endif
#if MALLOCDBG
	init_malloc_debug();
#endif

	if ((i=read_args(argc, argv))!=0) return i;

	printf(
		"version %d.%d\n"
		"turn    %d.\n"
		"orders  %s.\n",
		global.data_version / 10, global.data_version % 10, turn, orders);

	strcat(strcpy(zText, resourcepath()), "/timestrings");
	if ((i=read_datenames(zText))!=0) return i;

	kernel_init();
#ifdef HAVE_LUA
  lua_init();
#endif
	game_init();
#ifdef BETA_CODE
	/* xml_writeships(); */
	/* xml_writebuildings(); */
	/* xml_writeitems("items.xml"); */
	return 0;
#endif

	if ((i=readgame(false))!=0) return i;
	confirm_newbies();
#ifdef BETA_CODE
	if (dungeonstyles) {
		struct dungeon * d = dungeonstyles;
		struct region * r = make_dungeon(d);
		make_dungeongate(findregion(0, 0), r, d);
	}
#endif
	writepasswd();
	if (g_killeiswald) {
		region * r = findregion(0, 25);
		if (r) {
			/* mach sie alle zur schnecke... */
			unit * u;
			terraform(r, T_OCEAN);
			for (u=r->units;u;u=u->next) scale_number(u, 1);
		}
	}
	if (g_writemap) return crwritemap();

	if ((i=processturn(orders))!=0) return i;

#ifdef CLEANUP_CODE
	game_done();
#endif
#ifdef HAVE_LUA
  lua_done();
#endif
	kernel_done();
	log_close();
	fclose(updatelog);
	return 0;
}
