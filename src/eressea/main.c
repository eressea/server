/* vi: set ts=2:
 *
 *	$Id: main.c,v 1.11 2001/02/05 16:27:07 enno Exp $
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
#include <attributes/attributes.h>
#include <spells/spells.h>
#include <triggers/triggers.h>
#include <items/items.h>

/* modules includes */
#include <modules/arena.h>
#include <modules/museum.h>
#include <modules/score.h>
#include <modules/xmas2000.h>
#include <modules/gmcmd.h>

/* gamecode includes */
#include <creation.h>
#include <laws.h>

/* kernel includes */
#include <building.h>
#include <unit.h>
#include <message.h>
#include <teleport.h>
#include <faction.h>
#include <plane.h>
#include <race.h>
#include <reports.h>
#include <region.h>
#include <save.h>
#include <time.h>
#include <border.h>
#include <region.h>
#include <item.h>

/* util includes */
#include <rand.h>
#include <base36.h>

/* libc includes */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

/** 
 ** global variables we are importing from other modules
 **/
extern char * g_reportdir;
extern char * g_datadir;
extern char * g_basedir;
extern char * g_resourcedir;

extern boolean nonr;
extern boolean nocr;
extern boolean nomer;
extern boolean nomsg;
extern boolean nobattle;
extern boolean nobattledebug;

#ifdef FUZZY_BASE36
extern int fuzzy_hits;
#endif /* FUZZY_BASE36 */

/** 
 ** global variables wthat we are exporting 
 **/
struct settings global = {
	"Eressea", /* gamename */
};

extern void render_init(void);

#if 0
static void
print_potions(FILE * F)
{
	potion_type * p;
	for (p=potiontypes;p;p=p->next) {
		requirement * req = p->itype->construction->materials;
		int i;
		fprintf(F, "%s\n", locale_string(NULL, p->itype->rtype->_name[0]));
		for (i=0;req[i].number;++i) {
			fprintf(F, "  %s\n", locale_string(NULL, resname(req[i].type, 0)));
		}
	}
}
#endif

static char * orders = NULL;
static int nowrite = 0;

static void
game_init(void)
{
	init_triggers();

	init_races();
	init_spells();
	init_resources();
	init_items();
	init_attributes();

#ifdef USE_GM_COMMANDS
	init_gmcmd();
#endif
	init_conversion();

	init_museum();
	init_arena();
	init_xmas2000();
	render_init();
/*	print_potions(stdout);
	exit(0); */
}

void
create_game(void)
{
	assert(regions==NULL || !"game is initialized");
	printf("Keine Spieldaten gefunden, erzeuge neues Spiel in %s...\n", datapath());
	makedir(datapath(), 0700);
	/* erste Insel generieren */
	new_region(0, 0);
	/* Monsterpartei anlegen */
	createmonsters();
	/* Teleportebene anlegen */
	create_teleport_plane();
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

int quickleave = 0;

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
			fprintf(F, "%s:%s:%s\n", factionid(f), f->email, f->passw);
		}
		fclose(F);
	}
}

static int
processturn(char *filename)
{
	struct summary * begin, * end;
	int i;

	begin = make_summary(false);
	printf(" - Korrekturen Runde %d\n", turn);
	korrektur();
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
	reports();
	free_units();
	puts(" - Beseitige leere Parteien");
	remove_empty_factions();
	end = make_summary(true);
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
		sprintf(ztext, "%s/%d", datapath(), turn);
		writegame(ztext, 0);
	}
	return 0;
}

extern void creport_cleanup(void);
extern void reports_cleanup(void);
extern void freeland(land_region * lr);

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
		free(r->display);
		if (r->land) freeland(r->land);
		r2 = r->next;
		while (r->attribs) a_remove (&r->attribs, r->attribs);
		free(r);
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
	reports_cleanup();
}

#include "magic.h"

static boolean
locale_check(void) 
{
	int i, errorlevel = 0;
	unsigned char * umlaute = (unsigned char*)"äöüÄÖÜß";
	/* E: das prüft, ob umlaute funktionieren. Wenn äöü nicht mit isalpha() true sind, kriegen wir ärger. */
	for (i=0;i!=3;++i) {
		if (toupper(umlaute[i])!=(int)umlaute[i+3]) {
			++errorlevel;
			fprintf(stderr, "error in locale[%d, %d]: toupper(%c)==%c\n", i, umlaute[i], umlaute[i], toupper(umlaute[i]));
		}
	}
	for (i=0;umlaute[i]!=0;++i) {
		if (!isalpha(umlaute[i]) || isspace(umlaute[i]) || iscntrl(umlaute[i])) {
			++errorlevel;
			if (!isalpha(umlaute[i]))
				fprintf(stderr, "error in locale[%d, %d]: !isalpha(%c)\n", i, umlaute[i], umlaute[i]);
			if (iscntrl(umlaute[i]))
				fprintf(stderr, "error in locale(%d, %d]: iscntrl(%c)\n", i, umlaute[i], umlaute[i]);
			if (isspace(umlaute[i]))
				fprintf(stderr, "error in locale[%d, %d]: isspace(%c)\n", i, umlaute[i], umlaute[i]);
		}
	}
	if (errorlevel) return false;
	return true;
}

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
				fprintf(F, "\n%s\n", magietypen[m]);
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
					fprintf(F, "{ SK_%s, %d }, ", skillnames[sk], race[r].bonus[sk]);
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
			if (strcmp(argv[i]+1, "nocr")==0) nocr = true;
			else if (strcmp(argv[i]+2, "nosave")==0) nowrite = true;
			else if (strcmp(argv[i]+2, "nonr")==0) nonr = true;
			else if (strcmp(argv[i]+2, "nomsg")==0) nomsg = true;
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
			case 'x':
				maxregions = atoi(argv[++i]);
				maxregions = (maxregions*81+80) / 81;
				break;
			default:
				usage(argv[0], argv[i]);
		}
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	int i;
	char zText[MAX_PATH];

	printf("\n%s PBEM host\n"
		"Copyright (C) 1996-2001 C.Schlittchen, K.Zedel, E.Rehling, H.Peters.\n\n"
	   "Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n\n", global.gamename, version());

	setlocale(LC_ALL, "");
#ifdef LOCALE_CHECK
	if (!locale_check()) {
		puts("ERROR: The current locale is not suitable for international Eressea.\n");
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
	game_init();

	if ((i=readgame(false))!=0) return i;
	if ((i=processturn(orders))!=0) return i;

	game_done();
	kernel_done();

	return 0;
}
