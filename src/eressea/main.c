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

#include <config.h>
#include "eressea.h"

/* initialization - TODO: init in separate module */
#include <attributes/attributes.h>
#include <spells/spells.h>
#include <triggers/triggers.h>
#include <items/itemtypes.h>

/* modules includes */
#include <modules/score.h>
#include <modules/xmas.h>
#include <modules/gmcmd.h>
#include <modules/infocmd.h>
#include <modules/autoseed.h>
#ifdef MUSEUM_MODULE
#include <modules/museum.h>
#endif
#ifdef WORMHOLE_MODULE
#include <modules/wormhole.h>
#endif
#ifdef ARENA_MODULE
#include <modules/arena.h>
#endif
#ifdef DUNGEON_MODULE
#include <modules/dungeon.h>
#endif

/* gamecode includes */
#include <gamecode/economy.h>
#include <gamecode/items.h>
#include <gamecode/laws.h>
#include <gamecode/monster.h>
#include <gamecode/creport.h>
#include <gamecode/report.h>
#include <gamecode/xmlreport.h>

/* kernel includes */
#include <kernel/xmlreader.h>
#include <kernel/spell.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/message.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/skill.h>
#include <kernel/teleport.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/border.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/item.h>

/* util includes */
#include <util/rand.h>
#include <util/rng.h>
#include <util/xml.h>
#include <util/goodies.h>
#include <util/log.h>
#include <util/sql.h>
#include <util/base36.h>

/* libc includes */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <locale.h>

#define ISLANDSIZE 20

/**
 ** global variables we are importing from other modules
 **/
extern char * g_reportdir;
extern char * g_datadir;
extern char * g_basedir;
extern char * g_resourcedir;

extern boolean nonr;
extern boolean nosh;
extern boolean nocr;
extern boolean noreports;
extern boolean nomer;
extern boolean nobattle;
extern boolean nomonsters;
extern boolean battledebug;
extern boolean dirtyload;

extern int loadplane;

extern void debug_messagetypes(FILE * out);
extern void free_region(region * r);
extern void render_init(void);
extern void free_borders(void);


#ifdef FUZZY_BASE36
extern int fuzzy_hits;
#endif /* FUZZY_BASE36 */

/**
 ** global variables that we are exporting
 **/
static char * orders = NULL;
static int nowrite = 0;
static boolean g_writemap = false;
static boolean opt_reportonly = false;
extern boolean opt_cr_absolute_coords;

struct settings global = {
	"Eressea", /* gamename */
	1000, /* maxunits */
};

static void
game_init(void)
{
  init_triggers();
  init_xmas();

  reports_init();
  report_init();
  creport_init();
  xmlreport_init();

  debug_language("locales.log");
  register_races();
  register_resources();
  register_buildings();
  register_ships();
  register_itemfunctions();
  register_spells();
#ifdef DUNGEON_MODULE
  register_dungeon();
#endif

  register_itemtypes();
  register_xmlreader();
  enable_xml_gamecode();
  init_data(xmlfile);

  init_locales();
  init_attributes();
  init_races();
  init_itemtypes();
  init_races();
  init_rawmaterials();

  init_gmcmd();
#ifdef INFOCMD_MODULE
  init_info();
#endif

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
        if (set_email(&pwd->email, email)!=0) {
          log_error(("Invalid email address faction %s: %s\n", faction, email));
        }
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

static int
processturn(char *filename)
{
	struct summary * begin, * end;
  newfaction * players;
	int i;

  if (turn == 0) rng_init((int)time(0));
  else rng_init(turn);

#ifdef SHORTPWDS
  readshortpwds("passwords");
#endif
	begin = make_summary();
	turn++;
	if ((i=readorders(filename))!=0) return i;
  if (!nomonsters) {
    if (turn == 0) rng_init((int)time(0));
    else rng_init(turn);
    puts(" - Monster KI...");
    plan_monsters();
  }
	processorders();
  sprintf(buf, "%s/newfactions", basepath());
  players = read_newfactions(filename);
  while (players) {
    int n = listlen(players);
    int k = (n+ISLANDSIZE-1)/ISLANDSIZE;
    k = n / k;
    autoseed(&players, k, true);
  }
	score();
	update_guards();
        if (!noreports) {
          init_reports();
          reports();
        }
	free_units();
	puts(" - Beseitige leere Parteien");
	remove_empty_factions(true);
	end = make_summary();
	report_summary(end, begin, false);
	report_summary(end, begin, true);
	free(end);
	free(begin);
	writepasswd();
#ifdef FUZZY_BASE36
	fputs("==--------------------------==\n", stdout);
	fprintf(stdout, "## fuzzy base10 hits: %5d ##\n", fuzzy_hits);
	fputs("==--------------------------==\n", stdout);
#endif /* FUZZY_BASE36 */
	if (!nowrite) {
		char ztext[64];
		sprintf(ztext, "%d", turn);
		return writegame(ztext, 0);
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

	for (r = regions; r; r = r2) {
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
    free(pl->name);
		free(pl);
	}
	creport_cleanup();
	report_cleanup();
}
#endif

#include "magic.h"

#ifdef CRTDBG
void
init_crtdbg(void)
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
		"--lomem          : keine Messages (RAM sparen)\n"
		"--nobattle       : keine Kämpfe\n"
		"--nomonsters     : keine monster KI\n"
		"--nodebug        : keine Logfiles für Kämpfe\n"
    "--noreports      : absolut keine Reporte schreiben\n"
		"--debug          : schreibt Debug-Ausgaben in die Datei debug\n"
		"--nocr           : keine CRs\n"
		"--nonr           : keine Reports\n"
    "--nosh           : keine Mail-skripte\n"
		"--crabsolute     : absolute Koordinaten im CR\n"
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
      else if (strcmp(argv[i]+2, "nosh")==0) nosh = true;
			else if (strcmp(argv[i]+2, "lomem")==0) lomem = true;
			else if (strcmp(argv[i]+2, "nobattle")==0) nobattle = true;
			else if (strcmp(argv[i]+2, "nomonsters")==0) nomonsters = true;
			else if (strcmp(argv[i]+2, "nodebug")==0) battledebug = false;
			else if (strcmp(argv[i]+2, "crabsolute")==0) opt_cr_absolute_coords = true;
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
			case 'R':
				opt_reportonly = true;
				break;
			default:
				usage(argv[0], argv[i]);
		}
	}
	return 0;
}

void
confirm_newbies(void)
{
	faction * f = factions;
	while (f) {
		if (!fval(f, FFL_DBENTRY)) {
			if (f->subscription) {
				sql_print(("UPDATE subscriptions SET status='ACTIVE', faction='%s', race='%s' WHERE id=%u;\n", 
                   itoa36(f->no), dbrace(f->race), f->subscription));
				fset(f, FFL_DBENTRY);
			}
		}
		f = f->next;
	}
}

void
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
}

int
main(int argc, char *argv[])
{
	int i;
	char zText[MAX_PATH];

	sqlpatch = true;
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
#ifdef CRTDBG
	init_crtdbg();
#endif
#ifdef DMALLOC
  init_dmalloc();
#endif

	if ((i=read_args(argc, argv))!=0) return i;

	printf(
		"version %d.%d\n"
		"turn    %d.\n"
		"orders  %s.\n",
		global.data_version / 10, global.data_version % 10, turn, orders);

	kernel_init();
	game_init();

  sprintf(zText, "%d", turn);
  i = readgame(zText, false);
  if (i!=0) return i;

  confirm_newbies();
	update_subscriptions();
	{
		char zText[128];
		FILE * F;
		faction * f = factions;
		sprintf(zText, "subscriptions.%u", turn);
		F = fopen(zText, "w");
		while (f!=NULL) {
			fprintf(F, "%s:%u:%s:%s:%s:%u:\n",
					itoa36(f->no), f->subscription, f->email, f->override,
					dbrace(f->race), f->lastorders);
			f = f->next;
		}
		fclose(F);
	}

#ifdef BETA_CODE
	if (dungeonstyles) {
		struct dungeon * d = dungeonstyles;
		struct region * r = make_dungeon(d);
		make_dungeongate(findregion(0, 0), r, d);
	}
#endif
	writepasswd();

	if (opt_reportonly) {
		reports();
		return 0;
	}

	if (g_writemap) {
		return crwritemap("world.cr"); 
	}

  if (turn == 0) rng_init((int)time(0));
  else rng_init(turn);


	if ((i = processturn(orders))!=0) {
		return i;
	}

#ifdef CLEANUP_CODE
	game_done();
#endif
	kernel_done();
	log_close();
	return 0;
}
