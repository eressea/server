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

#include <config.h>
#include "eressea.h"

#include "weapons.h"

/* kernel includes */
#include <building.h>
#include <unit.h>
#include <message.h>
#include <creation.h>
#include <teleport.h>
#include <faction.h>
#include <race.h>
#include <reports.h>
#include <save.h>
#include <time.h>
#include <border.h>
#include <region.h>
#include <laws.h>
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

int nodescriptions = 0;
int nowrite = 0;
extern char * reportdir;
extern char * datadir;
extern char * basedir;

extern boolean nonr;
extern boolean nocr;
extern boolean nomer;
extern boolean nomsg;
extern boolean nobattle;
extern boolean nobattledebug;

extern void korrektur(void);
extern void korrektur_end(void);
extern void reorder(region * r);
extern void freeland(land_region * lr);
extern void init_conversion(void);
extern void register_triggers(void);

int mapdetail = 0;

extern void render_init(void);

static void
init_game(void)
{
	register_triggers();
	init_locales();
	{
		char zText[MAX_PATH];
		sprintf(zText, "%s/%s", resourcepath(), "askalon.xml");
		init_data(zText);
	}

	init_resources();
	register_items();
	register_buildings();
	init_weapons();

	init_conversion();

	render_init();
}

void
create_game(void)
{
	assert(regions==NULL || !"game is initialized");
	printf("Keine Spieldaten gefunden, erzeuge neues Spiel...\n");
	makedir("data", 0700);
	/* erste Insel generieren */
	new_region(0, 0);
	/* Monsterpartei anlegen */
	createmonsters();
	/* Teleportebene anlegen */
	create_teleport_plane();
}

void
map(void)
{
	char zText[16];
	FILE * f;
	region * r;
	sprintf(zText, "map-%d.cr", turn);
	f = fopen(zText, "wt");
	fputs("VERSION 42\n", f);
	fputs("\"Standard\";konfiguration\n", f);
	fprintf(f, "%d;runde\n", turn);
	for (r=regions;r;r=r->next) {
		fprintf(f, "REGION %d %d\n", r->x, r->y);
		fprintf(f, "\"%s\";name\n", rname(r, NULL));
		fprintf(f, "\"%s\";terrain\n", terrain[rterrain(r)].name);
		if (!mapdetail) continue;
		fprintf(f, "%d;silber\n", rmoney(r));
		if (r->display && strlen(r->display))
			fprintf(f, "\"%s\";beschr\n", r->display);
		fprintf(f, "%d;bauern\n", rpeasants(r));
		fprintf(f, "%d;baeume\n", rtrees(r));
		fprintf(f, "%d;pferde\n", rhorses(r));
		if (rterrain(r) == T_MOUNTAIN || rterrain(r) == T_GLACIER) {
			fprintf(f, "%d;eisen\n", riron(r));
			if (rlaen(r)>=0) fprintf(f, "%d;laen\n", rlaen(r));
		}
	}
	fclose(f);
}

void
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

void
writepasswd(void)
{
	FILE * F;
	faction *f;
	char zText[128];
	sprintf(zText, "%s/%s", datapath(), "/passwd");
	F = cfopen(zText, "w");
	if (!F)
		return;
	puts("Schreibe Passwörter...");

	for (f = factions; f; f = f->next) {
		fprintf(F, "%s:%s:%s:%s\n", factionid(f), f->name, f->passw, f->override);
	}
	fclose(F);
}

#ifdef FUZZY_BASE36
extern int fuzzy_hits;
#endif /* FUZZY_BASE36 */

void
processturn(char *filename)
{
	struct summary * begin, * end;
	begin = make_summary(false);
	printf(" - Korrekturen Runde %d\n", turn);
	korrektur();
	turn++;
	if (!quickleave) {
		puts(" - entferne Texte der letzten Runde");
		getgarbage();
		puts(" - Nehme Korrekturen am Datenbestand vor");
		if (!readorders(filename)) return;
#if BENCHMARK
		exit(0);
#endif
		processorders();
		score();
	}
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
		sprintf(ztext, "data/%d", turn);
		writegame(ztext, 0);
	}
}

void
doreports(void)
{
	struct summary * begin;
	begin = make_summary(true);
	printf("Schreibe die Reports der %d. Runde...\n", turn);
	reports();
	report_summary(begin, begin, false);
	free(begin);
}

void
showmap(int mode)
{
	FILE * F = cfopen("karte", "w");
	if (!F)
		return;
	puts("Schreibe Karte...");

	writemap(F, mode);

	fclose(F);
}

#if ENNO_CLEANUP

extern void creport_cleanup(void);
extern void reports_cleanup(void);
extern void render_cleanup(void);

void
cleanup(void)
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

	creport_cleanup();
	reports_cleanup();
	render_cleanup();

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
}
#endif

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
	if (errorlevel) return -1;
}

#if MALLOCDBG
void
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

int
main(int argc, char *argv[])
{
	int i, errorlevel = 0;
	FILE * F;

	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");
#ifdef LOCALE_CHECK
	assert(locale_check() || !"ERROR: The current locale is not suitable for international Eressea.\n");
#endif
#if MALLOCDBG
	init_malloc_debug();
#endif

	F = fopen("res/spells", "wt");
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
		sprintf(buf, "fopen(%s): ", "res/spells");
		perror(buf);
	}
/*
	F = fopen("res/bonus", "wt");
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
		sprintf(buf, "fopen(%s): ", "res/spells");
		perror(buf);
	}
*/
	debug = 0;
	quickleave = 0;

	printf(
		"\n"
		"%s PBEM host\n"
		"Copyright (C) 1996-2001 C.Schlittchen, K.Zedel, E.Rehling, et al.\n\n"

		"Eressea is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"

		"Compilation: " __DATE__ " at " __TIME__ "\nVersion: %f\n", global.gamename, version());

	for (i = 1; i != argc; i++)
		if ((argv[i][0] == '-' || argv[i][0] == '/'))
			switch (argv[i][1]) {
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
			case 'x':
				maxregions = atoi(argv[++i]);
				maxregions = (maxregions*81+80) / 81;
				break;
			}

	{
		char zText[MAX_PATH];
		strcat(strcpy(zText, resourcepath()), "/timestrings");
		read_datenames(zText);
	}

	kernel_init();
	init_game();
	readgame(false);
#if MALLOCDBG
	assert(_CrtCheckMemory());
#endif
	/* verify_data(); */

	if (turn == 0)
		srand(time((time_t *) NULL));
	else
		srand(turn);

	/* Problem: In jedem Report ist die Regionsreihenfolge eine andere. */

	/* { int nregions=0; region *r; for(r=regions;r;r=r->next) nregions++;
	 * scramble(regions, nregions, sizeof(region *)); } */

	errorlevel = -1;

	for (i = 1; i != argc; i++)
		if (argv[i][0] == '-' ||
			argv[i][0] == '/')
			switch (argv[i][1]) {
			case 'c':
				korrektur();
				break;
			case 'r':
				errorlevel = 0;
				score();
				doreports();
				break;
			case 'Q':
				quickleave = 1;
				break;
			case 'n':
				if (strcmp(argv[i]+1, "nocr")==0) nocr = true;
				else if (strcmp(argv[i]+1, "nosave")==0) nowrite = true;
				else if (strcmp(argv[i]+1, "nonr")==0) nonr = true;
				else if (strcmp(argv[i]+1, "nomer")==0) nomer = true;
				else if (strcmp(argv[i]+1, "nomsg")==0) nomsg = true;
				else if (strcmp(argv[i]+1, "nobattle")==0) nobattle = true;
				else if (strcmp(argv[i]+1, "nodebug")==0) nobattledebug = true;
				break;
			case 'v':
				++i;
#define TEST_BORDERS 0
#if TEST_BORDERS
				{
					border * b;
					new_border(&bt_fogwall, findregion(5,-6), findregion(5, -5));
					new_border(&bt_noway, findregion(4,-5), findregion(5, -5));
					new_border(&bt_wall, findregion(5,-5), findregion(6, -5));
					b = new_border(&bt_illusionwall, findregion(4,-4), findregion(5, -5));
					b->data = (void*) 1; /* partei 1 hat's gezaubert */
				}
#endif
				if (i >= argc || argv[i][0]) {
					printf(" - verwende Befehlsdatei: %s\n", argv[i]);
					processturn(argv[i]);
					errorlevel = 0;
				} else {
					puts("Fehler: keine Befehlsdatei angegeben.\n\n");
					errorlevel = 1;
				}
				break;
#ifdef EXTRA_CR
			case 'p' :
				{
					FILE * out = fopen("planes.cr", "wt");
					faction * f;
					region * r;
					if (!out) break;
					fprintf(out, "VERSION 36\n");
					for (f = factions; f; f = f->next) cr_faction(out, f);
					for (r = regions;r;r=r->next) if (getplane(r)) cr_region(out, r);
				   fclose(out);
				}
				break;
			case 'w' :
				{
					FILE * out = fopen("world.cr", "wt");
					faction * f;
					region * r;
					if (!out) break;
					fprintf(out, "VERSION 36\n");
					for (f = factions; f; f = f->next) cr_faction(out, f);
					for (r = regions;r;r=r->next) cr_region(out, r);
				   fclose(out);
				}
				break;
#endif
			case 'a':
				writeadresses("adressen");
				exit(0);
				break;

			case '#':
				score();
				exit(0);
				break;
			case 'm':
				if (argv[i][2]=='d') mapdetail = 1;
				map();
				break;
			case 'o':
				reportdir = argv[++i];
				break;
			case 'd':
				if (!strcmp(argv[i] + 1, "debug")) {
					nodescriptions = 1;
					debug = fopen("debug", "w");
					break;
				}
				datadir = argv[++i];
				break;
			case 'b':
				basedir = argv[++i];
				break;
			case 'f':
				i++;
			case 't':
			case 'x':
				i++;
			case 'q':
				break;
			default:
				fprintf(stderr, "Usage: %s [options]\n"
					"-r               : schreibt die Reports neu\n"
					"-x n             : Lädt nur die ersten n regionen\n"
					"-g befehlsdatei  : verarbeitet Spielleiterbefehle\n"
					"-v befehlsdatei  : verarbeitet automatisch die angegebene Befehlsdatei\n"
					"-a               : gibt Adressenliste aus\n"
				    "-#               : gibt Scoreliste aus\n"
					"-d datadir       : gibt das datenverzeichnis an\n"
					"-o reportdir     : gibt das reportverzeichnis an\n"
					"-nomsg           : keine Messages (RAM sparen)\n"
					"-nocr            : keine CRs\n"
					"-nonr            : keine Reports\n"
					"-nobattle        : keine Kämpfe\n"
					"-debug           : schreibt Debug-Ausgaben in die Datei debug\n"
					"-?               : help\n", argv[0]);
				errorlevel = 1;
				break;
			}
	if (errorlevel >= 0) {
#if ENNO_CLEANUP
		cleanup();
#endif
		game_done();
		return errorlevel;
	}
	puts("? zeigt das Menue an.");
	printf("sizeof:\n region\t%d (%d)\n unit\t%d (%d)\n", (int)sizeof(region), listlen(regions), (int)sizeof(unit), -1);
	printf(" ship\t%d (%d)\n building\t%d(%d)\n", (int)sizeof(ship), -1, (int)sizeof(building), -1);

	for (;;) {
		if (quickleave) break;
		printf("> ");
		fgets(buf, 1024, stdin);

		switch (buf[0]) {
		case 'a':
			puts("Schreibe Liste der Adressen...");
			writeadresses("adressen");

			fclose(F);
			break;

		case 'c':
			korrektur();
			break;

		case 'k':
			showmap(M_TERRAIN);
			break;

		case 'p':
			showmap(M_FACTIONS);
			break;

		case 'u':
			showmap(M_UNARMED);
			break;

		case 'v':
			printf("Datei mit den Befehlen? ");
			fgets(buf, 1024, stdin);
			if (buf[0])
				processturn(buf);
			break;

		case 'r':
			doreports();
			break;

		case 's':
			{
				char ztext[64];
				sprintf(ztext, "data/%d", turn);
				writegame(ztext, 0);
			}
			break;

		case 'q':
#if ENNO_CLEANUP
			cleanup();
#endif
			game_done();
			return 0;
		case 'Q':
			quickleave = 1;
			break;
		case 'l':
			listnames();
			break;

		case '#':
			score();
			break;

#ifdef QTMAP
		case '*':
			qt_edit_map(argc, argv);
			break;
#endif

		default:
			puts("modify:\n"
				 " v - Befehle verarbeiten.\n"
				 " g - Spielleiterbefehle verarbeiten.\n"
				 " e - Erzeuge Regionen.\n"
				 " t - Terraform Region.\n"
				 " T - Terraform Block.\n"
				 " m - Erschaffe Einheiten und Monster.\n"
				 " b - Erbaue eine Burg.\n"
				 " n - Neue Spieler hinzufuegen.\n"
				 " M - Move unit.\n"
				 " c - Korrekturen durchführen.\n"
				 "information:\n"
				 " a - Adressen anzeigen.\n"
				 " i - Info ueber eine Region.\n"
				 " U - Info ueber units einer Region.\n"
				 " k - Karte anzeigen.\n"
				 " p - Politische Karte anzeigen.\n"
				 " u - Karte unbewaffneter Regionen anzeigen.\n"
				 " l - Liste aller Laendernamen zeigen.\n"
#ifdef QTMAP
				 " * - Qt-Karte anzeigen.\n"
#endif
				 "save:\n"
				 " r - Reports schreiben.\n"
				 " # - Scoreliste speichern.\n"
				 " s - Spielstand speichern.\n"
				 "\n"
				 " q - Beenden.");
		}
	}
	return 0;
}

struct settings global = {
	"Askalon", /* gamename */
};
