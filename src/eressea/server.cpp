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

/* config includes */
#include <config.h>
#include <eressea.h>

#include "korrektur.h"

/* initialization - TODO: init in separate module */
#include <attributes/attributes.h>
#include <spells/spells.h>
#include <triggers/triggers.h>
#include <items/items.h>

/* modules includes */
#ifdef DUNGEON_MODULE
#include <modules/dungeon.h>
#endif
#include <modules/score.h>
#include <modules/xmas.h>
#include <modules/gmcmd.h>
#include <modules/infocmd.h>
#ifdef MUSEUM_MODULE
#include <modules/museum.h>
#endif
#ifdef WORMHOLE_MODULE
#include <modules/wormhole.h>
#endif
#ifdef ARENA_MODULE
#include <modules/arena.h>
#endif

/* gamecode includes */
#include <economy.h>
#include <goodies.h>
#include <laws.h>

/* kernel includes */
#include <building.h>
#include <creport.h>
#include <faction.h>
#include <message.h>
#include <plane.h>
#include <race.h>
#include <skill.h>
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
#include <util/rand.h>
#include <util/log.h>
#include <util/sql.h>
#include <util/base36.h>

/* lua includes */
#include "lua/bindings.h"
#include <lua.hpp>
#include <luabind/luabind.hpp>

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
extern char * g_reportdir;
extern char * g_datadir;
extern char * g_basedir;
extern char * g_resourcedir;
extern item_type * i_silver;

extern boolean nonr;
extern boolean nocr;
extern boolean noreports;
extern boolean nomer;
extern boolean nomsg;
extern boolean nobattle;
extern boolean nomonsters;
extern boolean nobattledebug;
extern boolean dirtyload;

extern int demonfix;
extern int loadplane;

extern void debug_messagetypes(FILE * out);
extern void free_region(region * r);
extern void render_init(void);
extern void free_borders(void);
extern boolean opt_cr_absolute_coords;

#ifdef FUZZY_BASE36
extern int fuzzy_hits;
#endif /* FUZZY_BASE36 */
}

/**
 ** global variables that we are exporting
 **/
static char * orders = NULL;
static int nowrite = 0;
static boolean g_writemap = false;
static boolean opt_reportonly = false;
static const char * luafile = "default.lua";

struct settings global = {
	"Eressea", /* gamename */
	"eressea", /* resourcepath */
	1000, /* maxunits */
};

#if 0
static int
crwritemap(void)
{
  FILE * F = fopen("world.cr", "w+");
  region * r;
  for (r=regions;r;r=r->next) {
    plane * p = rplane(r);
    fprintf(F, "REGION %d %d %d\n", r->x, r->y, p?p->id:0);
    fprintf(F, "\"%s\";Name\n\"%s\";Terrain\n", rname(r, default_locale), LOC(default_locale, terrain[rterrain(r)].name));
  }
  fclose(F);
	return 0;
}
#endif

static void
game_init(void)
{
	init_triggers();
	init_xmas();
	report_init();
	creport_init();

	debug_language("locales.log");
	register_races();
	register_resources();
	register_buildings();
	register_ships();
	register_items();
	register_spells();
#ifdef DUNGEON_MODULE
	register_dungeon();
#endif

	init_data(xmlfile);
	init_locales();

	init_attributes();
	init_resources();
	init_items();
	init_economy();
#if NEW_RESOURCEGROWTH
	init_rawmaterials();
#endif

	init_gmcmd();
	init_info();
	init_conversion();

#ifdef MUSEUM_MODULE
	register_museum();
#endif
#ifdef ARENA_MODULE
	register_arena();
#endif
#ifdef WORMHOLE_MODULE
  register_wormholes();
#endif

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

#ifdef SHORTPWDS
static void
readshortpwds()
{
  FILE * F;
  char zText[MAX_PATH];
  sprintf(zText, "%s/%s.%u", basepath(), "shortpwds", turn);

  F = fopen(zText, "r");
  if (F==NULL) {
    log_error(("could not open password file %s", zText));
  } else {
    while (!feof(F)) {
      faction * f;
      char passwd[16], faction[5], email[64];
      fscanf(F, "%s %s %s\n", faction, passwd, email);
      f = findfaction(atoi36(faction));
      if (f!=NULL) {
        shortpwd * pwd = (shortpwd*)malloc(sizeof(shortpwd));
        pwd->email = strdup(email);
        pwd->pwd = strdup(passwd);
        pwd->used = false;
        pwd->next = f->shortpwds;
        f->shortpwds = pwd;
      }
    }
    fclose(F);
  }
}
#endif

lua_State *
lua_init(void)
{
  lua_State * luaState = lua_open();
  luaopen_base(luaState);
  luaopen_math(luaState);
  luaopen_string(luaState);
  luaopen_io(luaState);
  luabind::open(luaState);
  bind_eressea(luaState);
#ifdef ALLIANCES
  bind_alliance(luaState);
#endif
  bind_region(luaState);
  bind_faction(luaState);
  bind_unit(luaState);
  bind_ship(luaState);
  bind_building(luaState);
  return luaState;
}

void
lua_done(lua_State * luaState)
{
  lua_close(luaState);
}

static void
update_subscriptions(void)
{
  FILE * F;
  char zText[MAX_PATH];
  faction * f;
  strcat(strcpy(zText, basepath()), "/subscriptions");
  F = fopen(zText, "r");
  if (F==NULL) {
    log_error(("could not open %s.\n", zText));
    return;
  }
  for (;;) {
    char zFaction[5];
    int subscription, fno;
    if (fscanf(F, "%d %s", &subscription, zFaction)<=0) break;
    fno = atoi36(zFaction);
    f = findfaction(fno);
    if (f!=NULL) {
      f->subscription=subscription;
    }
  }
  fclose(F);

  sprintf(zText, "subscriptions.%u", turn);
  F = fopen(zText, "w");
  for (f=factions;f!=NULL;f=f->next) {
    fprintf(F, "%s:%u:%s:%s:%s:%u:\n",
      itoa36(f->no), f->subscription, f->email, f->override,
      dbrace(f->race), f->lastorders);
  }
  fclose(F);
}

int
process_orders()
{
	struct summary * begin, * end;

#ifdef SHORTPWDS
  readshortpwds("passwords");
#endif
	begin = make_summary(false);
	printf(" - Korrekturen Runde %d\n", turn);
	korrektur();
	turn++;
	puts(" - entferne Texte der letzten Runde");
	getgarbage();
	puts(" - Nehme Korrekturen am Datenbestand vor");
#if BENCHMARK
	exit(0);
#endif
	processorders();
	score();
#ifdef WACH_WAFF
	remove_unequipped_guarded();
#endif
	korrektur_end();

  end = make_summary(true);
  report_summary(end, begin, false);
  report_summary(end, begin, true);
  free(end);
  free(begin);

  update_subscriptions();
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

#include "magic.h"

#ifdef MALLOCDBG
void
init_malloc_debug(void)
{
#if (defined(_MSC_VER))
# if MALLOCDBG == 2
#  define CHECKON() _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF)
# elif MALLOCDBG == 3
#  define CHECKON() _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & 0)
# elif MALLOCDBG == 1
#  define CHECKON() _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_DELAY_FREE_MEM_DF)
# endif
#endif
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
		"-R               : erstellt nur die Reports neu\n"
		"--nomsg          : keine Messages (RAM sparen)\n"
		"--nobattle       : keine Kämpfe\n"
		"--nomonsters     : keine monster KI\n"
		"--nodebug        : keine Logfiles für Kämpfe\n"
		"--debug          : schreibt Debug-Ausgaben in die Datei debug\n"
		"--nocr           : keine CRs\n"
		"--nonr           : keine Reports\n"
		"--crabsolute     : absolute Koordinaten im CR\n"
#ifdef USE_MERIAN
		"--nomer          : keine Meriankarten\n"
#endif
		"--help           : help\n", prog);
	return -1;
}

static void
setLuaString(lua_State * luaState, const char * name, const char * value)
{
  luabind::object globals = luabind::get_globals(luaState);
  globals[name] = value;
}

static void
setLuaNumber(lua_State * luaState, const char * name, double value)
{
  luabind::object globals = luabind::get_globals(luaState);
  globals[name] = value;
}

static int
read_args(int argc, char **argv, lua_State * luaState)
{
	int i;
  char * c;
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
			else if (strcmp(argv[i]+2, "nobattle")==0) nobattle = true;
			else if (strcmp(argv[i]+2, "nomonsters")==0) nomonsters = true;
			else if (strcmp(argv[i]+2, "nodebug")==0) nobattledebug = true;
			else if (strcmp(argv[i]+2, "crabsolute")==0) opt_cr_absolute_coords = true;
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
      case 'e':
        luafile = argv[++i];
        break;
			case 'D': /* DEBUG */
				demonfix = atoi(argv[++i]);
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
			case 'i':
				xmlfile = argv[++i];
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
        if (i<argc) {
          orders = argv[++i];
        } else {
          return usage(argv[0], argv[i]);
        }
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
      case 's':
        c = argv[++i];
        while (*c && (*c!='=')) ++c;
        if (*c) {
          *c++ = 0;
          setLuaString(luaState, argv[i], c);
        }
        break;
      case 'n':
        c = argv[++i];
        while (*c && (*c!='=')) ++c;
        if (*c) {
          *c++ = 0;
          setLuaNumber(luaState, argv[i], atof(c));
        }
        break;
			case 'l':
				log_open(argv[++i]);
				break;
			case 'w':
 				g_writemap = true;
				break;
			case 'R':
				opt_reportonly = true;
				break;
			default:
				usage(argv[0], argv[i]);
		}
	}

  /* add some more variables to the lua globals */
  setLuaString(luaState, "datapath", datapath());
  setLuaString(luaState, "basepath", basepath());
  setLuaString(luaState, "reportpath", reportpath());
  setLuaString(luaState, "resourcepath", resourcepath());
  setLuaString(luaState, "orderfile", orders);

  return 0;
}

#ifdef BETA_CODE
extern int xml_writeitems(const char * filename);
extern int xml_writeships(void);
extern int xml_writebuildings(void);
#endif

typedef struct lostdata {
	int x, y;
	int prevunit;
	int building;
	int ship;
} lostdata;

int
main(int argc, char *argv[])
{
	int i;
	char zText[MAX_PATH];

	sqlpatch = true;
	updatelog = fopen("update.log", "w");
	log_open("eressea.log");
	printf("\n%s PBEM host\n"
		"Copyright (C) 1996-2003 C. Schlittchen, K. Zedel, E. Rehling, H. Peters.\n\n"
	   "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n", global.gamename, version());

	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");
#ifdef LOCALE_CHECK
	if (!locale_check()) {
		log_error(("The current locale is not suitable for international Eressea.\n"));
		return -1;
	}
#endif
#ifdef MALLOCDBG
	init_malloc_debug();
#endif

  lua_State * luaState = lua_init();
  if ((i=read_args(argc, argv, luaState))!=0) return i;

	strcat(strcpy(zText, resourcepath()), "/timestrings");
	if ((i=read_datenames(zText))!=0) return i;

	kernel_init();
	game_init();

  // run the main script
  lua_dofile(luaState, luafile);

#ifdef CLEANUP_CODE
	game_done();
#endif
	kernel_done();
  lua_done(luaState);
	log_close();
	fclose(updatelog);
	return 0;
}
