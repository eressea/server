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
#include "eressea.h"
#include "save.h"

#include "faction.h"
#include "magic.h"
#include "item.h"
#include "unit.h"
#include "building.h"
#include "spell.h"
#include "race.h"
#include "attrib.h"
#include "plane.h"
#include "movement.h"
#include "alchemy.h"
#include "region.h"
#include "resources.h"
#include "unit.h"
#include "skill.h"
#include "message.h"
#include "objtypes.h"
#include "border.h"
#include "karma.h"
#include "ship.h"
#include "pathfinder.h"
#include "group.h"

#ifdef USE_UGROUPS
#include "ugroup.h"
#include <attributes/ugroup.h>
#endif

/* attributes includes */
#include <attributes/key.h>

/* util includes */
#include <attrib.h>
#include <base36.h>
#include <event.h>
#include <resolve.h>
#include <sql.h>
#include <umlaut.h>

/* libc includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define xisdigit(c)     (((c) >= '0' && (c) <= '9') || (c) == '-')
#define COMMENT_CHAR    ';'

#define ESCAPE_FIX

int minfaction = 0;
const char * g_datadir;
/* imported symbols */
extern int cmsg[MAX_MSG][ML_MAX];
extern int quiet;
extern int dice_rand(const char *s);
static region * current_region;

int firstx = 0, firsty = 0;

#if RESOURCE_CONVERSION
int laen_read(attrib * a, FILE * F)
{
	int laen;
	fscanf(F, "%d", &laen);
	read_laen(current_region, laen);
	return AT_READ_FAIL;
}
#endif

char *
rns(FILE * f, char *c, size_t size)
{
	char * s = c;
	do {
		*s = (char) getc(f);
	} while (*s!='"');

	for (;;) {
		*s = (char) getc(f);
		if (*s=='"') break;
		if (s<c+size) ++s;
	}
	*s = 0;
	return c;
}

extern unsigned int __at_hashkey(const char* s);

FILE *
cfopen(const char *filename, const char *mode)
{
	FILE * F = fopen(filename, mode);

	if (F == 0) {
		perror(filename);
		return NULL;
	}
	setvbuf(F, 0, _IOFBF, 32 * 1024);	/* 32 kb buffer size */
	return F;
}

/* ------------------------------------------------------------- */

/* Dummy-Funktion für die Kompatibilität */

#define rid(F) ((global.data_version<BASE36_VERSION)?ri(F):ri36(F))

int nextc;

#ifdef OLD_RC
void
rc(FILE * F)
{
	nextc = getc(F);
}

#else
#define rc(F) (nextc = getc(F))
#endif

void
rds(FILE * F, char **ds)
{
	static char buffer[DISPLAYSIZE + 1]; /*Platz für null-char nicht vergessen!*/
	char *s = &buffer[0];

	while (nextc != '"') {
		if (nextc == EOF) {
			*s = 0;
			fprintf(stderr, "Die Datei bricht vorzeitig ab.\n");
			abort();
		}
		assert(s <= buffer + DISPLAYSIZE + 1);
		rc(F);
	}

	rc(F);

	while (nextc != '"') {
		if (nextc == EOF) {
			assert(s <= buffer + DISPLAYSIZE + 1);
			*s = 0;
			fprintf(stderr, "Die Datei bricht vorzeitig ab.\n");
			abort();
		}
		*s++ = (char)nextc;
		if (s - buffer > DISPLAYSIZE) {
			assert(s <= buffer + DISPLAYSIZE + 1);
			*s = 0;
			printf("\nDer String %s wurde nicht terminiert.\n", s);
			exit(1);
		}
		rc(F);
	}

	rc(F);
	assert(s <= buffer + DISPLAYSIZE + 1);
	*s = 0;
	(*ds) = realloc(*ds, sizeof(char) * (strlen(buffer) + 1));

	strcpy(*ds, buffer);
}

#define rcf(F) (getc(F));
void
rsf(FILE * F, char *s, size_t len)
{
	char * begin = s;
	int nextc;
	do {
		nextc = rcf(F);
		if (nextc == EOF) {
			puts("Die Datei bricht vorzeitig ab.");
			abort();
		}
	} while (nextc != '"');

	for (;;) {
		nextc = rcf(F);
		if (nextc == '"') break;
		else if (nextc == EOF) {
			puts("Die Datei bricht vorzeitig ab.");
			abort();
		}
		if (s-begin<(int)len-1)
			*s++ = (char)nextc;
	}
	*s = 0;
}

static void
rs(FILE * F, char *s)
{
	boolean apos = false;
	while (isspace(nextc)) rc(F);
	if (nextc=='"') {
		apos=true;
		rc(F);
	}
	for (;;) {
		if (nextc=='"') {
			rc(F);
			break;
		} else if (!apos && isspace(nextc)) break;
		*s++ = (char)nextc;
		rc(F);
	}
	*s = 0;
}

static int
ri(FILE * F)
{
	int i = 0, vz = 1;

	while (!xisdigit(nextc)) {
		if (nextc == EOF) {
			puts("Die Datei bricht vorzeitig ab.");
			abort();
		}
		rc(F);
	}

	while (xisdigit(nextc)) {
		if (nextc == '-')
			vz = -1;
		else
			i = 10 * i + (nextc - '0');
		rc(F);
	}

	return i * vz;
}

static int
ri36(FILE * F)
{
	char buf[64];
	int i = 0;
	rc(F);
	while (!isalnum(nextc)) rc(F);
	while (isalnum(nextc)) {
		buf[i++]=(char)nextc;
		rc(F);
	}
	buf[i]=0;
	i = atoi36(buf);
	return i;
}

#define MAXLINE 4096*16
char *
getbuf(FILE * F)
{
	char lbuf[MAXLINE];
	boolean cont = false;
	boolean quote = false;
	boolean comment = false;
	char * cp = buf;

	lbuf[MAXLINE-1] = '@';

	do {
		boolean eatwhite = true;
		boolean start = true;
		unsigned char * end;
		unsigned char * bp = (unsigned char * )fgets(lbuf, MAXLINE, F);

		comment = (boolean)(comment && cont);

		if (!bp) return NULL;

		end = bp + strlen((const char *)bp);
		if (*(end-1)=='\n') *(--end) = 0;
		else {
			while (bp && !lbuf[MAXLINE-1] && lbuf[MAXLINE-2]!='\n') {
				/* wenn die zeile länger als erlaubt war,
				 * wird der rest weggeworfen: */
				bp = (unsigned char *)fgets(buf, 1024, F);
				comment = false;
			}
			bp = (unsigned char *)lbuf;
		}
		cont = false;
		while (cp!=buf+MAXLINE && (char*)bp!=lbuf+MAXLINE && *bp) {
			if (isspace(*bp)) {
				if (eatwhite)
				{
					do { ++bp; } while ((char*)bp!=lbuf+MAXLINE && isspace(*bp));
					if (!quote && !start && !comment) *(cp++)=' ';
				}
				else {
					do {
						if (!comment) *(cp++)=SPACE_REPLACEMENT;
						++bp;
					} while (cp!=buf+MAXLINE && (char*)bp!=lbuf+MAXLINE && isspace(*bp));
				}
			}
			else if (iscntrl(*bp)) {
				*(cp++) = '?';
				++bp;
			} else {
				cont = false;
				if (*bp==COMMENT_CHAR && !quote) {
					bp = max(end-1, bp+1);
					comment=true;
				}
				else {
					if (*bp=='"') {
						quote = (boolean)!quote;
						eatwhite=true;
					}
					else if (*bp=='\\') cont=true;
					else {
						if (!comment) *(cp++) = *bp;
						eatwhite = (boolean)!quote;
					}
					++bp;
				}
			}
			start = false;
		}
		if (cp==buf+MAXLINE) {
			--cp;
		}
		*cp=0;
	} while (cont || cp==buf);
	return buf;
}

static unit *
unitorders(FILE * F, struct faction * f)
{
	int i;
	int p;
	unit *u;
	strlist *S, **SP;

	if (!f) return NULL;

	i = getid();
	u = findunitg(i, NULL);

	if (u && old_race(u->race) == RC_SPELL) return NULL;
	if (u && u->faction == f) {
		/* SP zeigt nun auf den Pointer zur ersten StrListe. Auf die
		 * erste StrListe wird mit u->orders gezeigt. Deswegen also
		 * &u->orders, denn SP zeigt nicht auf die selbe stelle wie
		 * u->orders, sondern auf die stelle VON u->orders! */

		freestrlist(u->orders);
		u->orders = 0;

		SP = &u->orders;

		for (;;) {

			/* Erst wenn wir sicher sind, dass kein Befehl
			 * eingegeben wurde, checken wir, ob nun eine neue
			 * Einheit oder ein neuer Spieler drankommt */

			if (!getbuf(F))
				break;

			if (igetkeyword(buf, u->faction->locale) == NOKEYWORD) {
				p = igetparam(buf, u->faction->locale);
				if (p == P_UNIT || p == P_FACTION || p == P_NEXT || p == P_GAMENAME)
					break;
			}
			if (buf[0]) {
				/* Nun wird eine StrListe S mit dem Inhalt buf2
				 * angelegt. S->next zeigt nirgends hin. */

				S = makestrlist(buf);

				/* Nun werden zwei Operationen ausgefuehrt
				 * (addlist2 ist ein #definiertes macro!):
				 * *SP = S       Der Pointer, auf den SP zeigte, zeigt nun auf S! Also
				 * entweder u->orders oder S(alt)->next zeigen nun auf das neue S!
				 *
				 * SP = &S->next SP zeigt nun auf den Pointer S->next. Nicht auf
				 * dieselbe Stelle wie S->next, sondern auf die Stelle VON S->next! */

				addlist2(SP, S);

				/* Und das letzte S->next darf natürlich nirgends mehr hinzeigen, es
				 * wird auf null gesetzt. Warum das nicht bei addlist2 getan wird, ist
				 * mir schleierhaft. So wie es jetzt implementiert ist, kann man es
				 * (hier auf alle Fälle) hinter addlist2 schreiben, oder auch nur am
				 * Ende einmal aufrufen - das wäre aber bei allen returns, und am ende
				 * des for (;;) blockes. Grmpf. Dann lieber hier, immer, und dafür
				 * sauberer... */

				*SP = 0;
			}
		}

		u->botschaften = NULL;	/* Sicherheitshalber */

	} else {
		/* cmistake(?, gc_add(strdup(buf)), 160, MSG_EVENT); */
		return NULL;
	}
	return u;
}
/* ------------------------------------------------------------ */

static faction *
factionorders(void)
{
	char b[16];
	char * fid = strnzcpy(b, getstrtoken(), 15);
	char * pass = getstrtoken();
	faction *f;

	f = findfaction(atoi36(fid));

	if (f!=NULL) {
		/* Kontrolliere, ob das Passwort richtig eingegeben wurde. Es
		 * muß in "Gänsefüßchen" stehen!! */

		/* War vorher in main.c:getgarbage() */
		if (!quiet) {
			printf(" %4s;", factionid(f));
			fflush(stdout);
		}
		freestrlist(f->mistakes);
		f->mistakes = 0;

		if (checkpasswd(f, pass) == false) {
			addstrlist(&f->mistakes, "Das Passwort wurde falsch eingegeben");
			return 0;
		}
		/* Die Partei hat sich zumindest gemeldet, so daß sie noch
		 * nicht als untätig gilt */

		f->lastorders = turn;

	} else
		log_warning(("Befehle für die ungültige Partei %s\n", fid));

	return f;
}

double
version(void)
{
	return RELEASE_VERSION * 0.1;
}
/* ------------------------------------------------------------- */

int
readorders(const char *filename)
{
	FILE * F;
	char *b;
	int nfactions=0;
	struct faction *f = NULL;

	F = cfopen(filename, "rt");
	if (F==NULL) return 0;

	puts(" - lese Befehlsdatei...\n");

	b = getbuf(F);

	/* Auffinden der ersten Partei, und danach abarbeiten bis zur letzten
	 * Partei */

	while (b) {
		const struct locale * lang = f?f->locale:default_locale;
		int p;
		const char * s;

		switch (igetparam(b, lang)) {
		case P_LOCALE:
			s = getstrtoken();
#define LOCALES
#ifdef LOCALES
			if (f && find_locale(s)) {
				f->locale = find_locale(s);
			}
#else
			if(strcmp(s, "de") == 0) {
				f->locale = find_locale(s);
			}
#endif

			b = getbuf(F);
			break;
		case P_GAMENAME:
		case P_FACTION:
			f = factionorders();
			if (f) {
				++nfactions;
			}

			b = getbuf(F);

			break;

			/* in factionorders wird nur eine zeile gelesen:
			 * diejenige mit dem passwort. Die befehle der units
			 * werden geloescht, und die Partei wird als aktiv
			 * vermerkt. */

		case P_UNIT:
			if (!f || !unitorders(F, f)) do {
				b = getbuf(F);
				if (!b) break;
				p = igetparam(b, lang);
			} while ((p != P_UNIT || !f) && p != P_FACTION && p != P_NEXT && p != P_GAMENAME);
			break;

			/* Falls in unitorders() abgebrochen wird, steht dort entweder eine neue
			 * Partei, eine neue Einheit oder das File-Ende. Das switch() wird erneut
			 * durchlaufen, und die entsprechende Funktion aufgerufen. Man darf buf
			 * auf alle Fälle nicht überschreiben! Bei allen anderen Einträgen hier
			 * muß buf erneut gefüllt werden, da die betreffende Information in nur
			 * einer Zeile steht, und nun die nächste gelesen werden muß. */

		case P_NEXT:
			f = NULL;
		default:
			b = getbuf(F);
		}
	}

	fclose(F);
	puts("\n");
	printf("   %d Befehlsdateien gelesen\n", nfactions);
	return 0;
}
/* ------------------------------------------------------------- */

/* #define INNER_WORLD  */
/* fürs debuggen nur den inneren Teil der Welt laden */
/* -9;-27;-1;-19;Sumpfloch */
int
inner_world(region * r)
{
	static int xy[2] =
	{18, -45};
	static int size[2] =
	{27, 27};

	if (r->x >= xy[0] && r->x < xy[0] + size[0] && r->y >= xy[1] && r->y < xy[1] + size[1])
		return 2;
	if (r->x >= xy[0] - 9 && r->x < xy[0] + size[0] + 9 && r->y >= xy[1] - 9 && r->y < xy[1] + size[1] + 9)
		return 1;
	return 0;
}

int maxregions = -1;
boolean dirtyload = false;

enum {
	U_MAN,
	U_UNDEAD,
	U_ILLUSION,
	U_FIREDRAGON,
	U_DRAGON,
	U_WYRM,
	U_SPELL,
	U_TAVERNE,
	U_MONSTER,
	U_BIRTHDAYDRAGON,
	U_TREEMAN,
	MAXTYPES
};

race_t
typus2race(unsigned char typus)
{
	if (typus>0 && typus <=11) return (race_t)(typus-1);
	return NORACE;
}

boolean
is_persistent(const char *s, const struct locale *lang)
{
	if (s==NULL) return false;
#ifdef AT_PERSISTENT
	if(*s == '@') return true;
#endif      /* Nur kurze Befehle! */
	switch(igetkeyword(s, lang)) {
		case K_BUY:
		case K_SELL:
		case K_CAST:
		case K_KOMMENTAR:
		case K_LIEFERE:
		case K_RESERVE:
			return true;
			break;
	}
	return false;
}

void
create_backup(char *file)
{
#if defined(HAVE_ACCESS)
	char bfile[MAX_PATH];
	char command[MAX_PATH*2+10];
	int c = 1;
	do {
		sprintf(bfile, "%s.backup%d", file, c);
		c++;
	} while(access(bfile, R_OK) == 0);
	sprintf(command, "cp %s %s", file, bfile);
	system(command);
#endif
}

const char *
datapath(void)
{
	static char zText[MAX_PATH];
	if (g_datadir) return g_datadir;
	return strcat(strcpy(zText, basepath()), "/data");
}

void
read_items(FILE *F, item **ilist)
{
	for (;;) {
		rs(F, buf);
		if (!strcmp("end", buf)) break;
		i_change(ilist, it_find(buf), ri(F));
	}
}

#if RESOURCE_CONVERSION
struct attrib_type at_resources = { 
	"resources", NULL, NULL, NULL, NULL, NULL, ATF_UNIQUE 
};

void 
read_iron(struct region * r, int iron)
{
	attrib * a = a_find(r->attribs, &at_resources);
	assert(iron>=0);
	if (a==NULL) {
		a = a_add(&r->attribs, a_new(&at_resources));
		a->data.sa[1] = -1;
	}
	a->data.sa[0] = (short)iron;
}

void 
read_laen(struct region * r, int laen)
{
	attrib * a = a_find(r->attribs, &at_resources);
	assert(laen>=0);
	if (a==NULL) {
		a = a_add(&r->attribs, a_new(&at_resources));
		a->data.sa[0] = -1;
	}
	a->data.sa[1] = (short)laen;
}
#endif

#ifdef USE_UGROUPS
void
read_ugroups(FILE *file)
{
	int i;
	faction *f;
	ugroup *ug;
	int fno, ugid, ugmem;

	while(1) {
		fno = ri(file);
		if(fno == -1) break;
		f = findfaction(fno);
		while(1) {
			ugid  = ri(file);
			if(ugid == -1) break;
			ugmem = ri(file);
			ug = malloc(sizeof(ugroup));
			ug->id = ugid;
			ug->members = ugmem;
			ug->unit_array = malloc(ug->members * sizeof(unit *));
			for(i=0; i<ugmem; i++) {
				unit *u = findunitg(ri36(file), NULL);
				ug->unit_array[i] = u;
				a_add(&u->attribs, a_new(&at_ugroup))->data.i = ugid;
			}
			ug->next = f->ugroups;
			f->ugroups = ug;
		}
	}
}
#endif

int
readgame(boolean backup)
{
	int i, n, p;
	faction *f, **fp;
	region *r;
	building *b, **bp;
	ship *sh, **shp;
	unit *u;
	FILE * F;
	int rmax = maxregions;

	sprintf(buf, "%s/%d", datapath(), turn);
	if (backup) create_backup(buf);
	F = cfopen(buf, "r");
	if (F==NULL) {
		printf("Keine Spieldaten gefunden.\n");
#if 0
		printf("Neues Spiel (j/n)? ");
		if (tolower(getchar()) != 'j') {
			exit(0);
		}
		return creategame();
#else
		return -1;
#endif
	}

	rc(F);

	/* globale Variablen */

	global.data_version = ri(F);
	assert(global.data_version>=MIN_VERSION || !"unsupported data format");
	assert(global.data_version<=RELEASE_VERSION || !"unsupported data format");
#ifdef CONVERT_TRIGGER
	assert(global.data_version < NEWSOURCE_VERSION);
#else
	assert(global.data_version >= NEWSOURCE_VERSION);
#endif
	if (global.data_version >= GLOBAL_ATTRIB_VERSION) a_read(F, &global.attribs);
#ifndef COMPATIBILITY
	if (global.data_version < ITEMTYPE_VERSION) {
		fprintf(stderr, "kann keine alten datenfiles einlesen");
		exit(-1);
	}
#endif
	turn = ri(F);
	/* read_dynamictypes(); */
	if (global.data_version < NEWMAGIC) {
		max_unique_id = 0;
	} else {
		max_unique_id = ri(F);
	}

	if (global.data_version < BORDERID_VERSION) {
		nextborder = 0;
	} else {
		nextborder = ri(F);
	}

	/* Planes */
	planes = NULL;
	n = ri(F);
	while(--n >= 0) {
		plane *pl = calloc(1, sizeof(plane));
		pl->id = ri(F);
		rds(F, &pl->name);
		pl->minx = ri(F);
		pl->maxx = ri(F);
		pl->miny = ri(F);
		pl->maxy = ri(F);
		pl->flags = ri(F);
		if (global.data_version>WATCHERS_VERSION) {
			rs(F, buf);
			while (strcmp(buf, "end")!=0) {
				watcher * w = calloc(sizeof(watcher),1);
				int fno = atoi36(buf);
				w->mode = (unsigned char)ri(F);
				w->next = pl->watchers;
				pl->watchers = w;
				ur_add((void*)fno, (void**)&w->faction, resolve_faction);
				rs(F, buf);
			}
		}
		a_read(F, &pl->attribs);
		addlist(&planes, pl);
	}

	/* Read factions */

	n = ri(F);
	printf(" - Einzulesende Parteien: %d\n", n);
	fp = &factions;
	while (*fp) fp=&(*fp)->next;

	/* fflush (stdout); */

	while (--n >= 0) {
		faction * f = readfaction(F);
		addlist2(fp, f);
	}
	*fp = 0;

	/* Benutzte Faction-Ids */

	if (global.data_version < NEW_FACTIONID_VERSION) {
		init_used_faction_ids();
	} else {
		no_used_faction_ids = ri(F);
		/* used_faction_ids = gc_add(malloc(no_used_faction_ids*sizeof(int))); */
		used_faction_ids = malloc(no_used_faction_ids*sizeof(int));
		for(i=0; i < no_used_faction_ids; i++) {
			used_faction_ids[i] = ri(F);
		}
	}

	/* Regionen */

	n = ri(F);
	if (rmax<0) rmax = n;
	printf(" - Einzulesende Regionen: %d/%d\r", rmax, n);

	while (--n >= 0) {
		unit **up;
		boolean skip = false;
		int x = ri(F);
		int y = ri(F);

		if (firstx && firsty) {
			if (x!=firstx || y!=firsty) {
				skip = true;
			} else {
				firstx=0;
				firsty=0;
				if (rmax>0) rmax = min(n, rmax)-1;
			}
		}
		if (rmax==0) {
			if (dirtyload) break;
			skip = true;
		}
		if ((n%1024)==0) {	/* das spart extrem Zeit */
			printf(" - Einzulesende Regionen: %d/%d ", rmax, n);
			printf("* %d,%d    \r", x, y);
		}
		if (skip) {
			char * r;
			do {
				r = fgets(buf, BUFSIZE, F); /* skip region */
			} while (r && buf[0]!='\n');
			continue;
		}
		--rmax;

		r = readregion(F, x, y);

		/* Burgen */
		p = ri(F);
		bp = &r->buildings;

		while (--p >= 0) {

			b = (building *) calloc(1, sizeof(building));
			if (global.data_version>=FULL_BASE36_VERSION)
				b->no = rid(F);
			else
				b->no = ri(F);
			bhash(b);
			rds(F, &b->name);
			rds(F, &b->display);
			b->size = ri(F);
			if (global.data_version < TYPES_VERSION) {
				assert(!"data format is no longer supported");
			    /* b->type = oldbuildings[ri(F)]; */
			}
			else {
			    rs(F, buf);
			    b->type = bt_find(buf);
			}
			b->region = r;
			a_read(F, &b->attribs);
			addlist2(bp, b);
		}
		/* Schiffe */

		p = ri(F);
		shp = &r->ships;

		while (--p >= 0) {
			sh = (ship *) calloc(1, sizeof(ship));

			sh->region = r;
			if (global.data_version>=FULL_BASE36_VERSION)
				sh->no = rid(F);
			else
				sh->no = ri(F);
			shash(sh);
			rds(F, &sh->name);
			rds(F, &sh->display);

			if (global.data_version < SHIPTYPE_VERSION) {
				assert(!"cannot read old datafile with xml ship support");
			}
			else {
				rs(F, buf);
				sh->type = st_find(buf);
				assert(sh->type || !"ship_type not registered!");
			}
			if (global.data_version >= TYPES_VERSION) {
				sh->size = ri(F);
				sh->damage = ri(F);
			} else if (global.data_version > 76) {
				assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
				sh->size = sh->type->construction->maxsize - ri(F);
				sh->damage = DAMAGE_SCALE*sh->size*ri(F)/100;
			} else {
				int left = ri(F);
				if (rterrain(r) == T_OCEAN) {
					assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
					sh->size = sh->type->construction->maxsize-left;
				}
				sh->damage = 0;
			}

			/* Attribute rekursiv einlesen */

			sh->coast = (direction_t)ri(F);
			a_read(F, &sh->attribs);

			addlist2(shp, sh);
		}

		*shp = 0;

		/* Einheiten */

		p = ri(F);
		up = &r->units;

		while (--p >= 0) {
			unit * u = readunit(F);
			assert(u->region==NULL);
			u->region = r;
			addlist2(up,u);
		}
	}
	printf("\n");
	if (!dirtyload) {
		if (global.data_version >= BORDER_VERSION) read_borders(F);
#ifdef USE_UGROUPS
		if (global.data_version >= UGROUPS_VERSION) read_ugroups(F);
#endif
	}

#ifdef WEATHER

	/* Wetter lesen */

	weathers = NULL;

	if (global.data_version >= 81) {
		n = ri(F);
		while(--n >= 0) {
			weather *w;

			w = calloc(1, sizeof(weather));

			w->type      = ri(F);
			w->radius    = ri(F);
			w->center[0] = ri(F);
			w->center[1] = ri(F);
			w->move[0]   = ri(F);
			w->move[1]   = ri(F);

			addlist(&weathers, w);
		}
	}
#endif
	fclose(F);
#ifdef AMIGA
	fputs("Ok.", stderr);
#endif

        /* Unaufgeloeste Zeiger initialisieren */
	printf("\n - Referenzen initialisieren...\n");
	resolve();
	resolve_IDs();

	printf("\n - Leere Gruppen löschen...\n");
	for (f=factions; f; f=f->next) {
		group ** gp = &f->groups;
		while (*gp) {
			group * g = *gp;
			if (g->members==0) {
				*gp = g->next;
				free_group(g);
			} else
				gp = &g->next;
		}
	}
	resolve_IDs();

#if 0
	/* speziell für Runde 199->200 */
	printf("Actions korrigieren...\n");
	iuw_fix_rest();
#endif

	for (r=regions;r;r=r->next) {
		building * b;
		for (b=r->buildings;b;b=b->next) update_lighthouse(b);
	}
	printf(" - Regionen initialisieren & verbinden...\n");
	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			u->faction->alive = 1;
		}
	}
	if(findfaction(0)) {
		findfaction(0)->alive = 1;
	}
	if (maxregions>=0) {
		remove_empty_factions(false);
	}

	/* Regionen */
	for (r=regions;r;r=r->next) {
		building ** bp;
#if 0
		unit ** up;
		ship ** sp;

		a_age(&r->attribs);
		handle_event(&r->attribs, "create", r);
		/* Einheiten */
		for (up=&r->units;*up;) {
			unit * u = *up;
			a_age(&u->attribs);
			if (u==*up) handle_event(&u->attribs, "create", u);
			if (u==*up) up = &(*up)->next;
		}
		/* Schiffe */
		for (sp=&r->ships;*sp;) {
			ship * s = *sp;
			a_age(&s->attribs);
			if (s==*sp) handle_event(&s->attribs, "create", s);
			if (s==*sp) sp = &(*sp)->next;
		}
#endif
		/* Gebäude */
		for (bp=&r->buildings;*bp;) {
			building * b = *bp;
			a_age(&b->attribs);
			if (b==*bp) handle_event(&b->attribs, "create", b);
			if (b==*bp) bp = &(*bp)->next;
		}
	}

	return 0;
}
/* ------------------------------------------------------------- */

#define wc(F, c) putc(c, F);
#define wnl(F) putc('\n', F);
#define whs(F, s) fputs(s, F); putc(' ', F)

void
wsn(FILE * F, const char *s)
{
	if (!s)
		return;
	while (*s)
		wc(F, *s++);
}

void
ws(FILE * F, const char *s)
{
	fputc('"', F);
	wsn(F, s);
	fputs("\" ", F);
}

void
wi(FILE * F, int n)
{
	fprintf(F, "%d ", n);
}

void wi36(FILE * F, int n)
{
	fprintf(F, "%s ", itoa36(n));
}

void
write_items(FILE *F, item *ilist)
{
	item * itm;
	for (itm=ilist;itm;itm=itm->next) if (itm->number) {
		whs(F, resourcename(itm->type->rtype, 0));
		wi(F, itm->number);
	}
	fputs("end ", F);
}

#ifdef USE_UGROUPS
void
write_ugroups(FILE *file)
{
	faction *f;
	ugroup *ug;
	int i;

	for(f=factions; f; f=f->next) if(f->ugroups) {
		wi(file, f->no);
		for(ug = f->ugroups; ug; ug=ug->next) {
			wi(file, ug->id);
			wi(file, ug->members);
			for(i=0; i<ug->members; i++) {
				wi36(file, ug->unit_array[i]->no);
			}
		}
		fputs("-1\n", file);
	}
	fputs("-1\n", file);
}
#endif

#ifdef USE_PLAYERS
static void
export_players(const char * path)
{
	FILE * F;
	player * p = get_players();
	if (p==NULL) return;

	F = cfopen(path, "w");
	if (F==NULL) return;

	fputs("name;email;passwd;faction;info\n", F);
	while (p) {
		/* name */
		fputc('\"', F);
		if (p->name) fputs(p->name, F);

		/* email */
		fputs("\";\"", F);
		if (p->email) fputs(p->email, F);
		else if (p->faction) fputs(p->faction->email, F);
		fputs("\";\"", F);

		/* passwd */
		fputs("\";\"", F);
		if (p->faction) fputs(p->faction->passw, F);

		/* faction */
		fputs("\";\"", F);
		if (p->faction) fputs(itoa36(p->faction->no), F);

		/* info */
		fputs("\";\"", F);
		if (p->info) fputs(p->info, F);
		else if (p->faction) fputs(p->faction->banner, F);

		fputs("\"\n", F);

		p = next_player(p);
	}
	fclose(F);
}
#endif

void
writegame(char *path, char quiet)
{
	int i,n;
	faction *f;
	region *r;
	building *b;
	ship *sh;
	unit *u;
	plane *pl;
	FILE * F;
#ifdef USE_PLAYERS
	char playerfile[MAX_PATH];

	sprintf(buf, "%s/%d.players", datapath(), turn);
	export_players(playerfile);
#endif
	/* write_dynamictypes(); */

	F = cfopen(path, "w");
	if (F==NULL)
		return;

	if (!quiet)
		printf("Schreibe die %d. Runde...\n", turn);

	/* globale Variablen */

	wi(F, RELEASE_VERSION);
	wnl(F);
#if RELEASE_VERSION >= GLOBAL_ATTRIB_VERSION
	a_write(F, global.attribs);
	wnl(F);
#endif
	wi(F, turn);
	wi(F, max_unique_id);
	wi(F, nextborder);

	/* Write planes */
	wnl(F);
	wi(F, listlen(planes));
	wnl(F);

	for(pl = planes; pl; pl=pl->next) {
#if RELEASE_VERSION >= WATCHERS_VERSION
		watcher * w;
#endif
		wi(F, pl->id);
		ws(F, pl->name);
		wi(F, pl->minx);
		wi(F, pl->maxx);
		wi(F, pl->miny);
		wi(F, pl->maxy);
		wi(F, pl->flags);
#if RELEASE_VERSION >= WATCHERS_VERSION
		w = pl->watchers;
		while (w) {
			if (w->faction) {
				wi36(F, w->faction->no);
				wi(F, w->mode);
			}
			w = w->next;
		}
		fputs("end ", F);
#endif
		a_write(F, pl->attribs);
		wnl(F);
	}


	/* Write factions */

	n=listlen(factions);
	wi(F, n);
	wnl(F);

	printf(" - Schreibe %d Parteien...\n",n);
	for (f = factions; f; f = f->next) {
		writefaction(F, f);
	}

	wi(F, no_used_faction_ids);
	wnl(F);

	for(i=0; i<no_used_faction_ids; i++) {
		wi(F, used_faction_ids[i]);
		wnl(F);
	}

	wnl(F);

	/* Write regions */

	n=listlen(regions);
	wi(F, n);
	wnl(F);
	printf(" - Schreibe Regionen: %d  \r", n);

	for (r = regions; r; r = r->next, --n) {
		/* plus leerzeile */
		if ((n%1024)==0) {	/* das spart extrem Zeit */
			printf(" - Schreibe Regionen: %d  \r", n);
			fflush(stdout);
		}
		wnl(F);
		wi(F, r->x);
		wi(F, r->y);
		writeregion(F, r);

		wi(F, listlen(r->buildings));
		wnl(F);
		for (b = r->buildings; b; b = b->next) {
			wi36(F, b->no);
			ws(F, b->name);
			ws(F, b->display);
			wi(F, b->size);
			ws(F, b->type->_name);
			wnl(F);
			a_write(F, b->attribs);
			wnl(F);
		}

		wi(F, listlen(r->ships));
		wnl(F);
		for (sh = r->ships; sh; sh = sh->next) {
			assert(sh->region == r);
			wi36(F, sh->no);
			ws(F, sh->name);
			ws(F, sh->display);
#if RELEASE_VERSION < SHIPTYPE_VERSION
			wi(F, sh->type);
#else
			ws(F, sh->type->name[0]);
#endif
			wi(F, sh->size);
			wi(F, sh->damage);
			wi(F, sh->coast);
			wnl(F);
			a_write(F, sh->attribs);
			wnl(F);
		}

		wi(F, listlen(r->units));
		wnl(F);
		for (u = r->units; u; u = u->next) {
			writeunit(F, u);
		}
	}
	wnl(F);
	write_borders(F);
	wnl(F);
#if RELEASE_VERSION >= UGROUPS_VERSION
	write_ugroups(F);
	wnl(F);
#endif
	fclose(F);
	printf("\nOk.\n");
}
/* ------------------------------------------------------------- */

void
curse_write(const attrib * a, FILE * f) {
	int flag;
	int mage_no;
	curse * c = (curse*)a->data.v;
	const curse_type * ct = c->type;

	flag = (c->flag & ~(CURSE_ISNEW));

	if (c->magician){
		mage_no = c->magician->no;
	}else{
		mage_no = -1;
	}

	fprintf(f, "%d %s %d %d %d %d %d ", c->no, ct->cname, flag,
			c->duration, c->vigour, mage_no, c->effect);

	if (c->type->write) c->type->write(f, c);
	else if (c->type->typ == CURSETYP_UNIT) {
		curse_unit * cc = (curse_unit*)c->data;
		fprintf(f, "%d ", cc->cursedmen);
	}
}

int
curse_read(attrib * a, FILE * f) {
	int mageid;
	curse * c = (curse*)a->data.v;
	const curse_type * ct;

	if (global.data_version >= CURSETYPE_VERSION) {
		char cursename[64];
		fscanf(f, "%d %s %d %d %d %d %d ", &c->no, cursename, &c->flag, 
			&c->duration, &c->vigour, &mageid, &c->effect);
		ct = ct_find(cursename);
	} else {
		int cspellid;
		if (global.data_version < CURSE_NO_VERSION) {
			fscanf(f, "%d %d %d %d %d %d ",&cspellid, &c->flag, &c->duration,
				&c->vigour, &mageid, &c->effect);
			c->no = newunitid();
		} else {
			fscanf(f, "%d %d %d %d %d %d %d ", &c->no, &cspellid, &c->flag,
				&c->duration, &c->vigour, &mageid, &c->effect);
		}
		ct = ct_find(oldcursename(cspellid));
	}
	assert(ct!=NULL);

	c->type = ct;

	/* beim Einlesen sind noch nicht alle units da, muss also
	 * zwischengespeichert werden. */
	if (mageid == -1){
		c->magician = (unit *)NULL;
	} else {
		ur_add((void*)mageid, (void**)&c->magician, resolve_unit);
	}

	if (c->type->read) c->type->read(f, c);
	else if (c->type->typ==CURSETYP_UNIT) {
		curse_unit * cc = calloc(1, sizeof(curse_unit));

		c->data = cc;
		fscanf(f, "%d ", &cc->cursedmen);
	}
	chash(c);

	return AT_READ_OK;
}

/* ------------------------------------------------------------- */

struct fjord { int size; faction * f; } fjord[3];

/* ------------------------------------------------------------- */

int
lastturn(void)
{
	int turn = 0;
#ifdef HAVE_READDIR
	DIR *data_dir = NULL;
	struct dirent *entry = NULL;
	const char * dir = datapath();
	data_dir = opendir(dir);
	if (data_dir != NULL) {
		entry = readdir(data_dir);
	}
	if (data_dir != NULL && entry != NULL) {
		turn = 0;
		do {
			int i = atoi(entry->d_name);
			if (i > turn)
				turn = i;
			entry = readdir(data_dir);
		} while (entry != NULL);
#ifdef HAVE_CLOSEDIR
		closedir(data_dir);
#endif
	}
#else
# error "requires dirent.h or an equivalent to compile!"
#endif
	return turn;
}

int
read_faction_reference(faction ** f, FILE * F)
{
	int id;
	if (global.data_version >= BASE36IDS_VERSION) {
		char zText[10];
		fscanf(F, "%s ", zText);
		id = atoi36(zText);
	} else {
		fscanf(F, "%d ", &id);
	}
	if (id<0) {
		*f = NULL;
		return AT_READ_FAIL;
	}
	*f = findfaction(id);
	if (*f==NULL) ur_add((void*)id, (void**)f, resolve_faction);
	return AT_READ_OK;
}

void
write_faction_reference(const faction * f, FILE * F)
{
#if RELEASE_VERSION >= BASE36IDS_VERSION
	fprintf(F, "%s ", itoa36(f->no));
#else
	fprintf(F, "%d ", f->no);
#endif
}

unit *
readunit(FILE * F)
{
	skill_t sk;
	item_t item;
	herb_t herb;
	potion_t potion;
	unit * u;
	int number, n;

	n = rid(F);
	u = findunit(n);
	if (u==NULL) {
		u = calloc(sizeof(unit), 1);
		u->no = n;
		uhash(u);
	} else {
		while (u->attribs) a_remove(&u->attribs, u->attribs);
		while (u->items) i_free(i_remove(&u->items, u->items));
		free(u->skills);
		u->skills = 0;
		u->skill_size = 0;
		u_setfaction(u, NULL);
	}
	if (global.data_version>=GUARD_VERSION) {
		faction * f;
		if (global.data_version>=FULL_BASE36_VERSION) {
			n = rid(F);
		} else {
			n = ri(F);
		}
		f = findfaction(n);
		if (f!=u->faction) u_setfaction(u, f);
	}
	rds(F, &u->name);
	rds(F, &u->display);
	if (global.data_version < MEMSAVE_VERSION) {
		rs(F, buf);
		if (strlen(buf)) usetprivate(u, buf);
	}
	number = ri(F);
	if (global.data_version<ITEMTYPE_VERSION)
		set_money(u, ri(F));
	u->age = (short)ri(F);
	if (global.data_version<NEWRACE_VERSION) {
		u->race = new_race[(race_t)ri(F)];
		u->irace = new_race[(race_t)ri(F)];
	} else {
		rs(F, buf);
		if (strcmp(buf, "giant turtle")==0) strcpy(buf, "giantturtle");
		u->race = rc_find(buf);
		assert(u->race);
		rs(F, buf);
		if (strlen(buf)) u->irace = rc_find(buf);
		else u->irace = u->race;
	}
	if (global.data_version<GUARD_VERSION)
		u_setfaction(u, findfaction(n = ri(F)));
	if (u->faction == NULL) {
		log_error(("unit %s has faction == NULL\n", unitname(u)));
		abort();
	}
	if (playerrace(u->race)) {
		u->faction->no_units++;
	}
	set_number(u, number);
	if (global.data_version>=FULL_BASE36_VERSION) {
		u->building = findbuilding(rid(F));
		u->ship = findship(rid(F));
	} else {
		u->building = findbuilding(ri(F));
		u->ship = findship(ri(F));
	}

	if (global.data_version <= 73) {
		if (ri(F)) {
			fset(u, FL_OWNER);
		} else {
			freset(u, FL_OWNER);
		}
	}
	u->status = (status_t) ri(F);
	if (global.data_version < NEWSTATUS_VERSION) {
		switch (u->status) {
		case 0: u->status = ST_FIGHT; break;
		case 1: u->status = ST_BEHIND; break;
		case 2: u->status = ST_AVOID; break;
		case 3: u->status = ST_FLEE; break;
		default: assert(0);
		}
	}
	if (global.data_version <= 73) {
		if (ri(F)) {
			guard(u, GUARD_ALL);
		} else {
			guard(u, GUARD_NONE);
		}
	} else
		u->flags = ri(F) & ~UFL_DEBUG;
	if (global.data_version < GUARD_VERSION) {
#if RELEASE_VERSION < GUARDFIX_VERSION
		if (fval(u, 1)) guard(u, GUARD_ALL);
#endif
	}
	/* Kurze persistente Befehle einlesen */
	if (u->orders) {
		freestrlist(u->orders);
		u->orders = NULL;
	}
	if (global.data_version >= MEMSAVE_VERSION) {
		rs(F, buf);
		while(*buf != 0) {
			strlist *S = makestrlist(buf);
			addlist(&u->orders,S);
			rs(F, buf);
		}
	}
	rds(F, &u->lastorder);
	set_string(&u->thisorder, "");
	if (global.data_version < EFFSTEALTH_VERSION)
		u_seteffstealth(u, ri(F));

	assert(u->number >= 0);
	assert(u->race);
	if (global.data_version<NEWSKILL_VERSION) {
		/* convert old data */
		while ((sk = (skill_t) ri(F)) != NOSKILL) {
			int days = ri(F) / u->number;
			int lvl = level(days);
			int weeks = lvl + 1 - (days - level_days(lvl))/30;
			assert(weeks>0 && weeks<=lvl+1);
			if (lvl) {
				skill * sv = add_skill(u, sk);
				sv->level = sv->old = (unsigned char)lvl;
				sv->weeks = (unsigned char)weeks;
			}
		}
	} else {
		while ((sk = (skill_t) ri(F)) != NOSKILL) {
			int level = ri(F);
			int weeks = ri(F);
			if (level) {
				skill * sv = add_skill(u, sk);
				sv->level = sv->old = (unsigned char)level;
				sv->weeks = (unsigned char)weeks;
			}
		}
	}
	if (global.data_version>=ITEMTYPE_VERSION) {
		read_items(F, &u->items);
	} else {
		while ((item = (item_t) ri(F)) >= 0) {
			i_change(&u->items, olditemtype[item], ri(F));
		}

		while ((potion = (potion_t) ri(F)) >= 0) {
			i_change(&u->items, oldpotiontype[potion]->itype, ri(F));
		}

		while ((herb = (herb_t) ri(F)) >= 0) {
			i_change(&u->items, oldherbtype[herb]->itype, ri(F));
		}
	}
	u->hp = ri(F);
	/* assert(u->hp >= u->number); */
	if (global.data_version < MAGE_ATTRIB_VERSION) {
		if (global.data_version < NEWMAGIC) {
			if (has_skill(u, SK_MAGIC)) {
				/* ist Magier und muss in neuen Magier konvertiert werden */
				create_mage(u, u->faction->magiegebiet);
				/* bekommt anfangs soviel Aura wie er Magie kann */
				set_spellpoints(u, effskill(u, SK_MAGIC));
			}
		} else {
			int i = ri(F);
			if (i != -1){
				attrib * a;
				int csp = 0;

				sc_mage * mage = calloc(1, sizeof(sc_mage));
				mage->magietyp = (magic_t) i;
				mage->spellpoints = ri(F);
				mage->spchange = ri(F);
				while ((i = ri(F)) != -1) {
					mage->combatspell[csp] = (spellid_t) i;
					mage->combatspelllevel[csp] = ri(F);
					csp++;
				}
				while ((i = ri(F)) != -1) {
					addspell(u, (spellid_t) i);
				}
				mage->spellcount = 0;
				a = a_add(&u->attribs, a_new(&at_mage));
				a->data.v = mage;
			}
		}
	}
	a_read(F, &u->attribs);
	return u;
}

void
writeunit(FILE * F, const unit * u)
{
	strlist *S;
	int i;
	wi36(F, u->no);
	wi36(F, u->faction->no);
	ws(F, u->name);
	ws(F, u->display);
	assert(old_race(u->race) == RC_SPELL || u->number == u->debug_number);
	wi(F, u->number);
	wi(F, u->age);
	ws(F, u->race->_name[0]);
	ws(F, u->irace!=u->race?u->irace->_name[0]:"");
	if (u->building)
		wi36(F, u->building->no);
	else
		wi(F, 0);
	if (u->ship)
#if RELEASE_VERSION>= FULL_BASE36_VERSION
		wi36(F, u->ship->no);
#else
		wi(F, u->ship->no);
#endif
	else
		wi(F, 0);
	wi(F, u->status);
	wi(F, u->flags & UFL_SAVEMASK);
#if RELEASE_VERSION < GUARDFIX_VERSION
	wi(F, getguard(u));
#endif
	for(S=u->orders; S; S=S->next) {
		if (is_persistent(S->s, u->faction->locale)) {
			ws(F, S->s);
		}
	}
	ws(F, ""); /* Abschluß der persistenten Befehle */
	ws(F, u->lastorder);
#if RELEASE_VERSION < EFFSTEALTH_VERSION
	wi(F, u_geteffstealth(u));
#endif
	wnl(F);

	assert(u->number >= 0);
#ifdef MONEY_BUG
	if (get_money(u) < 0)
		printf("Einheit %s hat %d silber", unitname(u), get_money(u));
#else
	assert(u->race);
#endif

	for (i=0;i!=u->skill_size;++i) {
		skill * sv = u->skills+i;
		assert(sv->weeks<=sv->level*2+1);
		if (sv->level>0) {
			wi(F, sv->id);
			wi(F, sv->level);
			wi(F, sv->weeks);
		}
	}
	wi(F, -1);
	wnl(F);
	write_items(F, u->items);
	wnl(F);
	if (u->hp == 0) {
		log_error(("Einheit %s hat 0 Trefferpunkte\n", itoa36(u->no)));
		((unit*)u)->hp = 1;
	}
	wi(F, u->hp);
	wnl(F);
#if RELEASE_VERSION < MAGE_ATTRIB_VERSION
	if (is_mage(u)) {
		m = get_mage(u);
		wi(F, m->magietyp);
		wi(F, m->spellpoints);
		wi(F, m->spchange);
		for (i = 0; i != MAXCOMBATSPELLS; i++){
			wi(F, m->combatspell[i]);
			wi(F, m->combatspelllevel[i]);
		}
		wi(F, -1);
		wnl(F);
		/* BUG: Endlosschleife! */
		for (sp = m->spellptr;sp;sp=sp->next){
			wi(F, sp->spellid);
		}
	}
	wi(F, -1);
	wnl(F);
#endif
	a_write(F, u->attribs);
	wnl(F);
}

region *
readregion(FILE * F, int x, int y)
{
	char * name = NULL;
	region * r = findregion(x, y);
	int ter;

	if (r==NULL) {
		r = new_region(x, y);
	} else {
		current_region = r;
		while (r->attribs) a_remove(&r->attribs, r->attribs);
		if (r->land) {
			free(r->land); /* mem leak */
			r->land->demands = 0; /* mem leak */
		}
		while (r->resources) {
			rawmaterial * rm = r->resources;
			r->resources = rm->next;
			free(rm);
		}
		r->land = 0;
	}
	if (global.data_version < MEMSAVE_VERSION) {
		rds(F, &name);
	}
	rds(F, &r->display);
	ter = ri(F);
	if (global.data_version < NOFOREST_VERSION) {
		if (ter>T_PLAIN) --ter;
	}
	rsetterrain(r, (terrain_t)ter);
	if (global.data_version >= MEMSAVE_VERSION) r->flags = (char) ri(F);

	if (global.data_version >= REGIONAGE_VERSION)
		r->age = (unsigned short) ri(F);
	else
		r->age = 0;

	if (global.data_version < MEMSAVE_VERSION) {
		ri(F);
	}
	if (global.data_version < MEMSAVE_VERSION) {
		if (ri(F)) fset(r, RF_CHAOTIC);
		else freset(r, RF_CHAOTIC);
	}

	if (global.data_version < MEMSAVE_VERSION) {
		if (landregion(rterrain(r))) {
			r->land = calloc(1, sizeof(land_region));
			rsetname(r, name);
		}
		free(name);
	} else if (landregion(rterrain(r))) {
		r->land = calloc(1, sizeof(land_region));
		rds(F, &r->land->name);
	}
	if (global.data_version < MEMSAVE_VERSION || r->land) {
		int i;
#if GROWING_TREES
		if(global.data_version < GROWTREE_VERSION) {
			i = ri(F); rsettrees(r, 2, i);
		} else {
			i = ri(F); rsettrees(r, 0, i);
			i = ri(F); rsettrees(r, 1, i);
			i = ri(F); rsettrees(r, 2, i);
		}
#else
		i = ri(F); rsettrees(r, i);
#endif
		i = ri(F); rsethorses(r, i);
#if NEW_RESOURCEGROWTH
		if (global.data_version < NEWRESOURCE_VERSION) {
			i = ri(F); 
#if RESOURCE_CONVERSION
			if (i!=0) read_iron(r, i);
#endif
		} else {
			rawmaterial ** pres = &r->resources;
			assert(*pres==NULL);
			for (;;) {
				rawmaterial * res;
				rs(F, buf);
				if (strcmp(buf, "end")==0) break;
				res = calloc(sizeof(rawmaterial), 1);
				res->type = rmt_find(buf);
				assert(res->type!=NULL);
				res->level = ri(F);
				res->amount = ri(F);

				if(global.data_version >= RANDOMIZED_RESOURCES_VERSION) {
					res->startlevel = ri(F);
					res->base = ri(F);
					res->divisor = ri(F);
				} else {
					int i;
					res->startlevel = 1;
					for (i=0; i<3; i++) {
						if(res->type == terrain[rterrain(r)].rawmaterials[i].type) break;
					}
					assert(i<=2);
					res->base = dice_rand(terrain[rterrain(r)].rawmaterials[i].base);
					res->divisor = dice_rand(terrain[rterrain(r)].rawmaterials[i].divisor);
				}

				*pres = res;
				pres=&res->next;
			}
		}
#else
		i = ri(F); rsetiron(r, i);
#endif
		if (global.data_version>=ITEMTYPE_VERSION) {
			rs(F, buf);
			if (strcmp(buf, "noherb") != 0) {
				rsetherbtype(r, ht_find(buf));
			} else {
				rsetherbtype(r, NULL);
			}
			rsetherbs(r, (short)ri(F));
		} else if (global.data_version>=MEMSAVE_VERSION) {
			int i = ri(F);
			terrain_t ter = rterrain(r);
			if (ter == T_ICEBERG || ter == T_ICEBERG_SLEEP) ter = T_GLACIER;
			if (ter > T_GLACIER || ter == T_OCEAN)
				rsetherbtype(r, NULL);
			else
				rsetherbtype(r, oldherbtype[(i-1)+3*(ter-1)]);
			rsetherbs(r, (short)ri(F));
		} else if (global.data_version<MEMSAVE_VERSION) {
			int i = ri(F);
#if NEW_RESOURCEGROWTH
#if RESOURCE_CONVERSION
			if (i!=0) read_laen(r, i);
#endif
#else
			rsetlaen(r, i);
#endif
			if (ri(F)) fset(r, RF_MALLORN);
			if (ri(F)) fset(r, RF_ENCOUNTER);
		}
		rsetpeasants(r, ri(F));
		rsetmoney(r, ri(F));
		if (global.data_version<ATTRIBFIX_VERSION) ri(F);
	}

	if (global.data_version<MEMSAVE_VERSION) {
		int chaoscount = ri(F);
		int deathcount = ri(F);
		attrib * a;

		if (deathcount>0) {
			a = a_find(r->attribs, &at_deathcount);
			if (!a) a = a_add(&r->attribs, a_new(&at_deathcount));
			a->data.i = deathcount;
		}
		if (chaoscount>0) {
			a = a_find(r->attribs, &at_chaoscount);
			if (!a) a = a_add(&r->attribs, a_new(&at_chaoscount));
			a->data.i = chaoscount;
		}
	}

	assert(rterrain(r) != NOTERRAIN);
	assert(rhorses(r) >= 0);
	assert(rpeasants(r) >= 0);
	assert(rmoney(r) >= 0);

	if (global.data_version < MEMSAVE_VERSION || r->land) {
		if (global.data_version<ITEMTYPE_VERSION) {
			int i, p = 0;
			for (i = 0; oldluxurytype[i]!=NULL; i++) {
				int k = ri(F);
				r_setdemand(r, oldluxurytype[i], k);
				if (k==0) {
					/* Prüfung ob nur ein Luxusgut verkauft wird. */
					if (p == 1) {
						/* Zuviele ... Wir setzen den Demand. */
						r_setdemand(r, oldluxurytype[i], (char) (1 + rand() % 5));
					} else ++p;
				}
			}
		} else {
			for (;;) {
				rs(F, buf);
				if (!strcmp(buf, "end")) break;
				r_setdemand(r, lt_find(buf), ri(F));
			}
		}
	}
	a_read(F, &r->attribs);

	return r;
}

void
writeregion(FILE * F, const region * r)
{
	ws(F, r->display);
	wi(F, rterrain(r));
	wi(F, r->flags & RF_SAVEMASK);
	wi(F, r->age);
	wnl(F);
	if (landregion(rterrain(r))) {
		const herb_type *rht;
		struct demand * demand;
		ws(F, r->land->name);
#if GROWING_TREES
		wi(F, rtrees(r,0));
		wi(F, rtrees(r,1));
		wi(F, rtrees(r,2));
#else
		wi(F, rtrees(r));
#endif
		wi(F, rhorses(r));
#if NEW_RESOURCEGROWTH == 0
		wi(F, riron(r));
#elif RELEASE_VERSION>=NEWRESOURCE_VERSION
		{
			rawmaterial * res = r->resources;
			while (res) {
				ws(F, res->type->name);
				wi(F, res->level);
				wi(F, res->amount);
				wi(F, res->startlevel);
				wi(F, res->base);
				wi(F, res->divisor);
				res = res->next;
			}
			fputs("end ", F);
		}
#else
		assert(!"invalid defines");
#endif
		rht =  rherbtype(r);
		if (rht) {
			ws(F, resourcename(rht->itype->rtype, 0));
		} else {
			ws(F, "noherb");
		}
		wi(F, rherbs(r));
		wi(F, rpeasants(r));
		wi(F, rmoney(r));
		if (r->land) for (demand=r->land->demands; demand; demand=demand->next) {
			ws(F, resourcename(demand->type->itype->rtype, 0));
			wi(F, demand->value);
		}
		fputs("end\n", F);
	}
	a_write(F, r->attribs);
	wnl(F);
}

/** Reads a faction from a file.
 * This function requires no context, can be called in any state. The
 * faction may not already exist, however.
 */
faction *
readfaction(FILE * F)
{
	ally *sf, **sfp;
	int planes, i, p;

	faction * f;

	if (global.data_version<FULL_BASE36_VERSION) {
		i = ri(F);
	} else {
		i = rid(F);
	}

	f = findfaction(i);
	if (f==NULL) {
		f = (faction *) calloc(1, sizeof(faction));
		f->no = i;
	} else {
		if (global.data_version < NEWMAGIC) {
			f->unique_id = max_unique_id + 1;
			max_unique_id++;
		}
		f->warnings = NULL; /* mem leak */
		f->allies = NULL; /* mem leak */
		while (f->attribs) a_remove(&f->attribs, f->attribs);
	}
	if (global.data_version >= NEWMAGIC) {
		f->unique_id = ri(F);
	}

	rds(F, &f->name);

#ifndef AMIGA
	if (!quiet) printf("   - Lese Partei %s (%s)\n", f->name, factionid(f));
#endif

	rds(F, &f->banner);
	rds(F, &f->email);
	rds(F, &f->passw);
	if (global.data_version >= OVERRIDE_VERSION) {
		rds(F, &f->override);
	} else {
		f->override = strdup(itoa36(rand()));
	}

	if (global.data_version < LOCALE_VERSION) {
		f->locale = find_locale("de");
	} else {
		rs(F, buf);
		f->locale = find_locale(buf);
	}
	f->lastorders = ri(F);
	f->age = ri(F);
/*
	if (sqlstream && f->age==0) {
		fprintf(sqlstream, 
			"UPDATE users SET status='ACTIVE' where email='%s';\n",
			f->email);
	}
*/
	if (global.data_version < NEWRACE_VERSION) {
		race_t rc = (char) ri(F);
		f->race = new_race[rc];
	} else {
		rs(F, buf);
		f->race = rc_find(buf);
		assert(f->race);
	}
	if (global.data_version >= MAGIEGEBIET_VERSION)
		f->magiegebiet = (magic_t)ri(F);
	else
		f->magiegebiet = (magic_t)((rand() % 5)+1);

	if (!playerrace(f->race)) {
		f->lastorders = turn+1;
	}
	if (global.data_version >= KARMA_VERSION)
		f->karma = ri(F);
	else
		f->karma = 0;

	if (global.data_version >= FACTIONFLAGS_VERSION)
		f->flags = ri(F);
	else
		f->flags = 0;
	freset(f, FFL_OVERRIDE);

	if (global.data_version>=FATTRIBS_VERSION)
		a_read(F, &f->attribs);
	if (global.data_version>=MSGLEVEL_VERSION)
		read_msglevels(&f->warnings, F);

	planes = ri(F);
	while(--planes >= 0) {
		int id = ri(F);
		int ux = ri(F);
		int uy = ri(F);
		set_ursprung(f, id, ux, uy);
	}
	f->newbies = 0;
	f->options = ri(F);

	if (((f->options & Pow(O_REPORT)) == 0)
		&& (f->options & Pow(O_COMPUTER)) == 0) {
		/* Kein Report eingestellt, Fehler */
		f->options = f->options | Pow(O_REPORT) | Pow(O_ZUGVORLAGE);
	}

	if (global.data_version<MSGLEVEL_VERSION) {
		if (global.data_version >= (HEX_VERSION-1)) {
			int maxopt = ri(F);
			for (i=0;i!=maxopt;++i) ri(F);
		} else if (global.data_version > 77) {
			for (i = 0; i != MAX_MSG; i++) ri(F);
		} else {
			for (i = 0; i != MAX_MSG - 1; i++) ri(F);
		}
	}

	if (global.data_version < 79) { /* showdata überspringen */
		assert(!"not implemented");
	} else {
		if (global.data_version >= NEWMAGIC && global.data_version < TYPES_VERSION) {
			int i, sk = ri(F); /* f->seenspell überspringen */
			for (i = 0; spelldaten[i].id != SPL_NOSPELL; i++) {
				if (spelldaten[i].magietyp == f->magiegebiet && spelldaten[i].level <= sk) {
					a_add(&f->attribs, a_new(&at_seenspell))->data.i = spelldaten[i].id;
				}
			}
		}
	}

	p = ri(F);
	sfp = &f->allies;
	while (--p >= 0) {
		int aid, state;
		if (global.data_version>=FULL_BASE36_VERSION) {
			aid = rid(F);
		} else {
			aid = ri(F);
		}
		state = ri(F);
		if (aid==0 || state==0) continue;
		sf = (ally *) calloc(1, sizeof(ally));

		sf->faction = findfaction(aid);
		if (!sf->faction) ur_add((void*)aid, (void**)&sf->faction, resolve_faction);
		sf->status = state;
#ifndef HELFE_WAHRNEHMUNG
		sf->status = sf->status & ~HELP_OBSERVE;
#endif

		addlist2(sfp, sf);
	}
	*sfp = 0;

	if (global.data_version>=GROUPS_VERSION) read_groups(F, f);
	return f;
}

void
writefaction(FILE * F, const faction * f)
{
	ally *sf;
	ursprung *ur;

	wi36(F, f->no);
	wi(F, f->unique_id);
	ws(F, f->name);
	ws(F, f->banner);
	ws(F, f->email);
	ws(F, f->passw);
#if RELEASE_VERSION>=OVERRIDE_VERSION
	ws(F, f->override);
#endif
#if RELEASE_VERSION>=LOCALE_VERSION
	ws(F, locale_name(f->locale));
#endif
	wi(F, f->lastorders);
	wi(F, f->age);
	ws(F, f->race->_name[0]);
	wnl(F);
	wi(F, f->magiegebiet);
	wi(F, f->karma);
	wi(F, f->flags);
	a_write(F, f->attribs);
	wnl(F);
	write_msglevels(f->warnings, F);
	wnl(F);
	wi(F, listlen(f->ursprung));
	for(ur = f->ursprung;ur;ur=ur->next) {
		wi(F, ur->id);
		wi(F, ur->x);
		wi(F, ur->y);
	}
	wnl(F);
	wi(F, f->options & ~Pow(O_DEBUG));
	wnl(F);
#if RELEASE_VERSION < TYPES_VERSION
	/* bis zu dieser Stufe sind die Sprüche bekannt */
	wi(F, 0);
	wnl(F);
#endif

	wi(F, listlen(f->allies));
	for (sf = f->allies; sf; sf = sf->next) {
		int no = (sf->faction!=NULL)?sf->faction->no:0;
		wi36(F, no);
		wi(F, sf->status);
	}
	wnl(F);
#if RELEASE_VERSION>=GROUPS_VERSION
	write_groups(F, f->groups);
#endif
}
