/* vi: set ts=2:
 *
 *	$Id: save.c,v 1.5 2001/01/28 08:01:52 enno Exp $
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
#include "unit.h"
#include "skill.h"
#include "message.h"
#include "monster.h"
#include "objtypes.h"
#include "border.h"
#include "karma.h"
#include "ship.h"
#include "pathfinder.h"
#ifdef GROUPS
#include "group.h"
#endif

/* attributes includes */
#include <attributes/key.h>

/* util includes */
#include <event.h>
#include <base36.h>
#include <attrib.h>
#include <umlaut.h>
#include <resolve.h>

/* libc includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#if defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER)
# include "trigger.h"
extern void resolve2(void);
#endif

#define xisdigit(c)     ((c) == '-' || ((c) >= '0' && (c) <= '9'))
#define COMMENT_CHAR    ';'

#define ESCAPE_FIX

int inside_only = 0;
int minfaction = 0;
const char * g_datadir;
/* imported symbols */
extern int cmsg[MAX_MSG][ML_MAX];
extern int quiet;

int firstx = 0, firsty = 0;

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

/* ------------------------------------------------------------- */

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
#define wid(F, i) fprintf(F, itoa36(i))

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
	while (nextc != '"') {
		if (nextc == EOF) {
			puts("Die Datei bricht vorzeitig ab.");
			abort();
		}
		rc(F);
	}

	rc(F);

	while (nextc != '"') {
		if (nextc == EOF) {
			puts("Die Datei bricht vorzeitig ab.");
			abort();
		}
		*s++ = (char)nextc;
		rc(F);
	}

	rc(F);
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
static char *
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
readunit(FILE * F, struct faction * f)
{
	int i;
	int p;
	unit *u;
	strlist *S, **SP;

	if (!f) return NULL;

	i = getid();
	u = findunitg(i, NULL);

	if (u && u->race == RC_SPELL) return NULL;
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

			if (igetkeyword(buf) == NOKEYWORD) {
				p = igetparam(buf);
				if (p == P_UNIT || p == P_FACTION || p == P_NEXT)
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
readfaction(void)
{
	char b[16];
	char * fid = strncpy(b, getstrtoken(), 16);
	char * pass = getstrtoken();
	int i36 = atoi36(fid);
	int i10 = atoi(fid);
	faction *f;

	f = findfaction(i36);

	if (f==NULL || strcmp(f->passw, pass)) {
		faction * f2 = findfaction(i10);
		if (f2!=NULL && !strcmp(f2->passw, pass)) {
			f = f2;
			addstrlist(&f->mistakes, "Die Befehle wurden nicht als base36 eingeschickt!");
		}
	}
	if (f!=NULL) {
		/* Kontrolliere, ob das Passwort richtig eingegeben wurde. Es
		 * muß in "Gänsefüßchen" stehen!! */

		/* War vorher in main.c:getgarbage() */
		if (!quiet) printf(" %s;", factionid(f));
		freestrlist(f->mistakes);
		f->mistakes = 0;

		if (strcmp(f->passw, pass)) {
			addstrlist(&f->mistakes, "Das Passwort wurde falsch eingegeben");
			return 0;
		}
		/* Die Partei hat sich zumindest gemeldet, so daß sie noch
		 * nicht als untätig gilt */

		f->lastorders = turn;

	} else
		printf(" WARNUNG: Befehle für die ungültige Partei Nr. %s;", fid);

	return f;
}

double
version(void)
{
	return RELEASE_VERSION * 0.1;
}
/* ------------------------------------------------------------- */

/* TODO: soll hier weg */
extern building_type bt_caldera;

int
readorders(const char *filename)
{
	FILE * F;
	faction *f;
	char *b;
	int p;

	F = cfopen(filename, "r");
	if (F==NULL) return 0;

	printf(" - lese Befehlsdatei...\n");

	b = getbuf(F);

	/* Auffinden der ersten Partei, und danach abarbeiten bis zur letzten
	 * Partei */

	f = 0;

	while (b) {
		switch (igetparam(b)) {
		case P_FACTION:
			f = readfaction();

			b = getbuf(F);

			break;

			/* in readfaction wird nur eine zeile gelesen:
			 * diejenige mit dem passwort. Die befehle der units
			 * werden geloescht, und die Partei wird als aktiv
			 * vermerkt. */

		case P_UNIT:
			if (!f || !readunit(F, f)) do {
				b = getbuf(F);
				if (!b) break;
				p = igetparam(b);
			} while ((p != P_UNIT || !f) && p != P_FACTION && p != P_NEXT);
			break;

			/* Falls in readunit() abgebrochen wird, steht dort entweder eine neue
			 * Partei, eine neue Einheit oder das File-Ende. Das switch() wird erneut
			 * durchlaufen, und die entsprechende Funktion aufgerufen. Man darf buf
			 * auf alle Fälle nicht überschreiben! Bei allen anderen Einträgen hier
			 * muß buf erneut gefüllt werden, da die betreffende Information in nur
			 * einer Zeile steht, und nun die nächste gelesen werden muß. */

		case P_NEXT:
			f = 0;
		default:
			b = getbuf(F);
		}
	}

	fclose(F);
	return 1;
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
is_persistent(const char *s) 
{
#ifdef AT_PERSISTENT                                                          
	if(*s == '@') return true;                                                  
#endif      /* Nur kurze Befehle! */
	switch(igetkeyword(s)) {
		case K_BUY:
		case K_SELL:
		case K_KOMMENTAR:
		case K_LIEFERE:
		case K_RESERVE:
			return true;
			break;
	}
	return false;
}

#if _MSC_VER
#define PATH_MAX _MAX_PATH
#elif !defined(PATH_MAX)
#define PATH_MAX 2048
#endif

#if USE_EVENTS
void
print_hunger(void * data)
{
	unit * u = (unit*)data;
	fprintf(stderr, "%s hungert in %s.\n", unitname(u), regionid(u->region));
}
#endif

void
create_backup(char *file)
{
#if defined(HAVE_ACCESS)
	char bfile[PATH_MAX];
	char command[PATH_MAX*2+10];
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
	static char zText[PATH_MAX];
	if (g_datadir) return g_datadir;
	return strcat(strcpy(zText, basepath()), "/data");
}
	
void
read_dynamictypes(void)
{
	FILE * F;
	sprintf(buf, "%s/%d.dynamictypes", datapath(), turn);
	F = fopen(buf, "rt+");
	if (F==NULL) {
		sprintf(buf, "%s/%d.buildingtypes", datapath(), turn);
		F = fopen(buf, "rt+");
	}
	if (F==NULL) return;
	while (!feof(F)) {
		fscanf(F, "%s", buf);
		if (!strcmp("BUILDINGTYPE", buf)) bt_read(F);
		else if (!strcmp("RESOURCETYPE", buf)) rt_read(F);
		else if (!strcmp("ITEMTYPE", buf)) it_read(F);
	}
	fclose(F);
}

void
write_dynamictypes(void)
{
	const building_typelist * btl = buildingtypes;
	const item_type * itype = itemtypes;
	const resource_type * rtype = resourcetypes;
	FILE * F;

	sprintf(buf, "%s/%d.dynamictypes", datapath(), turn);
	F = fopen(buf, "wt");
	while (btl) {
		if (btl->type->flags & BTF_DYNAMIC) bt_write(F, btl->type);
		btl=btl->next;
	}
	fputs("\n", F);
	while (rtype) {
		if (rtype->flags & RTF_DYNAMIC) rt_write(F, rtype);
		rtype=rtype->next;
	}
	fputs("\n", F);
	while (itype) {
		if (itype->flags & ITF_DYNAMIC) it_write(F, itype);
		itype=itype->next;
	}
	fclose(F);
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

int
readgame(boolean backup)
{
	skill_t sk;
	item_t item;
	herb_t herb;
	potion_t potion;
	int i, n, p;
	faction *f, **fp;
	ally *sf, **sfp;
	region *r;
	building *b, **bp;
	ship *sh, **shp;
	unit *u;
	strlist *S;
	FILE * F;

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
	read_dynamictypes();
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

	printf(" - Version: %d.%d, Runde %d.\n",
		global.data_version / 10, global.data_version % 10, turn);

	/* Planes */
	planes = NULL;
	n = ri(F);
	if (global.data_version < PLANES_VERSION) {
		assert(n==0); /* Keine Planes definiert, hoffentlich. */
	} else {
		while(--n >= 0) {
			plane *pl = calloc(1, sizeof(plane));
			pl->id = ri(F);
			rds(F, &pl->name);
			pl->minx = ri(F);
			pl->maxx = ri(F);
			pl->miny = ri(F);
			pl->maxy = ri(F);
			pl->flags = ri(F);
			if (global.data_version>=ATTRIB_VERSION) a_read(F, &pl->attribs);
			addlist(&planes, pl);
		}
	}

	/* Read factions */

	n = ri(F);
	printf(" - Einzulesende Parteien: %d\n", n);
	fp = &factions;

	/* fflush (stdout); */

	while (--n >= 0) {
		f = (faction *) calloc(1, sizeof(faction));

		f->first = 0;
		if (global.data_version<FULL_BASE36_VERSION) {
			f->no = ri(F);
		} else {
			f->no = rid(F);
		}
		if (global.data_version < NEWMAGIC) {
			f->unique_id = max_unique_id + 1;
			max_unique_id++;
		} else {
			f->unique_id = ri(F);
		}

		rds(F, &f->name);

#ifndef AMIGA
		if (!quiet) printf("   - Lese Partei %s (%s)\n", f->name, factionid(f));
#endif

		rds(F, &f->banner);
		rds(F, &f->email);
		rds(F, &f->passw);
		if (global.data_version < LOCALE_VERSION) {
			f->locale = find_locale("de");
			/* if (f->no==44) f->locale=strdup("en"); */
		} else {
			rs(F, buf);
			f->locale = find_locale(buf);
		}
		f->lastorders = ri(F);
		f->age = ri(F);
		f->race = (char) ri(F);
		if (global.data_version < RACES_VERSION) {
			if (f->race==0) f->race=RC_UNDEAD;
			else --f->race;
		}
		if (global.data_version >= MAGIEGEBIET_VERSION)
			f->magiegebiet = (magic_t)ri(F);
		else
			f->magiegebiet = (magic_t)((rand() % 5)+1);

		if (global.data_version >= KARMA_VERSION)
			f->karma = ri(F);
		else
			f->karma = 0;

		if (global.data_version >= FACTIONFLAGS_VERSION)
			f->flags = ri(F);
		else
			f->flags = 0;

		if (global.data_version>=FATTRIBS_VERSION)
			a_read(F, &f->attribs);
		if (global.data_version>=MSGLEVEL_VERSION)
			read_msglevels(&f->warnings, F);

		if (global.data_version >= PLANES_VERSION) {
			int c = ri(F);
			int id, ux, uy;
			while(--c >= 0) {
				id = ri(F);
				ux = ri(F);
				uy = ri(F);
				set_ursprung(f,id,ux,uy);
			}
		} else {
			int ux, uy;
			ux = ri(F);
			uy = ri(F);
			set_ursprung(f,0,ux,uy);
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
			if (aid==0) continue;
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

#ifdef GROUPS
		if (global.data_version>=GROUPS_VERSION) read_groups(F, f);
#endif

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
	if (maxregions<0) maxregions = n;
	printf(" - Einzulesende Regionen: %d/%d ", maxregions, n);

	while (--n >= 0) {
		unit **up;
		char * name = NULL;
		boolean skip = false;

		int x = ri(F);
		int y = ri(F);
		int terrain;

		if (firstx && firsty) {
			if (x!=firstx || y!=firsty) {
				skip = true;
			} else {
				firstx=0;
				firsty=0;
				if (maxregions>0) n = min(n, maxregions)-1;
			}
		}
		if (maxregions==0) skip = true;
		if ((n%1024)==0) {	/* das spart extrem Zeit */
			printf("* %d,%d    \r", x, y);
			printf(" - Einzulesende Regionen: %d/%d      ", maxregions, n);
		}
		if (skip) {
			char * r;
			do {
				r = fgets(buf, BUFSIZE, F); /* skip region */
			} while (r && buf[0]!='\n');
			continue;
		}
		--maxregions;

		r = new_region(x, y);
		if (global.data_version < MEMSAVE_VERSION) {
			rds(F, &name);
		}
		rds(F, &r->display);
		terrain = ri(F);
#ifdef NO_FOREST
		if (global.data_version < NOFOREST_VERSION) {
			if (terrain>T_PLAIN) --terrain;
		}
#endif
		rsetterrain(r, (terrain_t)terrain);
		if (global.data_version >= MEMSAVE_VERSION) r->flags = (char) ri(F);

		if (global.data_version >= REGIONAGE_VERSION)
			r->age = (unsigned short) ri(F);
		else
			r->age = 0;

		if (global.data_version >= PLANES_VERSION && global.data_version &&
			global.data_version < MEMSAVE_VERSION) {
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
			i = ri(F); rsettrees(r, i);
			i = ri(F); rsethorses(r, i);
			i = ri(F); rsetiron(r, i);
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
				rsetlaen(r, i);
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
		assert(rtrees(r) >= 0);
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
		if (global.data_version>=ATTRIB_VERSION) a_read(F, &r->attribs);

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
			    int i = ri(F);
			    b->type = oldbuildings[i];
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

			if (global.data_version>=FULL_BASE36_VERSION)
				sh->no = rid(F);
			else
				sh->no = ri(F);
#ifdef HASHING
			shash(sh);
#endif
			rds(F, &sh->name);
			rds(F, &sh->display);

			if (global.data_version < SHIPTYPE_VERSION) {
				const ship_type * oldship[] = { &st_boat, &st_longboat, &st_dragonship, &st_caravelle, &st_trireme };
				int i = ri(F);
				sh->type = oldship[i];
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
			int number, n;
			unsigned char oldtype=0, oldtypus, olditypus;
			static unit dummyu;
#if USE_EVENTS
			static boolean init = false;
			if (!init) {
				register_handler(print_hunger, 42);
				init=true;
			}
#endif
#ifdef AMIGA
				u = &dummyu;
#else
			if (inside_only && inner_world(r) < 2)
				u = &dummyu;
			else
				u = (unit *) calloc(1, sizeof(unit));
#endif

#if USE_EVENTS
			add_handler(&u->attribs, "hunger", print_hunger, 0);
#endif
			u->no = rid(F);
			if (global.data_version>=GUARD_VERSION) {
				if (global.data_version>=FULL_BASE36_VERSION) {
					n = rid(F);
				} else {
					n = ri(F);
				}
				set_faction(u, findfaction(n));
			}
			uhash(u);
			rds(F, &u->name);
			rds(F, &u->display);
			if (global.data_version < MEMSAVE_VERSION) {
				rs(F, buf);
				if (strlen(buf)) usetprivate(u, buf);
			}
			number = ri(F);
			u->region = r;
			if (global.data_version<RACES_VERSION)
				oldtype = (unsigned char) ri(F);
			if (global.data_version<ITEMTYPE_VERSION)
				set_money(u, ri(F));
			u->age = (short)ri(F);
			if (global.data_version<RACES_VERSION) {
				oldtypus = (unsigned char) ri(F);
				olditypus = (unsigned char) ri(F);

				if (global.data_version>=FULL_BASE36_VERSION) {
					n = rid(F);
				} else {
					n = ri(F);
				}
				set_faction(u, findfaction(n));

				u->race = typus2race(oldtypus);
				u->irace = typus2race(olditypus);
				if (u->race==NORACE && oldtype == U_MAN)
					u->race = RC_DRACOID;
				else switch (oldtype) {
					case U_UNDEAD:
						u->race = RC_UNDEAD;
						u->irace = RC_UNDEAD;
						break;
					case U_ILLUSION:
						u->race = RC_ILLUSION;
						u->irace = u->faction->race;
						break;
					case U_FIREDRAGON:
						u->race = RC_FIREDRAGON;
						break;
					case U_DRAGON:
						u->race = RC_DRAGON;
						break;
					case U_WYRM:
						u->race = RC_WYRM;
						break;
					case U_SPELL:
						u->race = RC_SPELL;
						break;
					case U_MONSTER:
						u->race = RC_SPECIAL;
						break;
					case U_BIRTHDAYDRAGON:
						u->race = RC_BIRTHDAYDRAGON;
						u->irace = RC_DRAGON;
						break;
					case U_TREEMAN:
						u->race = RC_TREEMAN;
						break;
					default:
						u->irace = u->race;
				}
				if (u->irace==NORACE) u->irace=u->race;
			}
			else {
				u->race = (race_t) ri(F);
				u->irace = (race_t) ri(F);
				if (global.data_version<GUARD_VERSION)
					set_faction(u, findfaction(n = ri(F)));
			}
			if (u->faction == NULL)
				fprintf(stderr,"\nEinheit %s hat faction==NULL\n",unitname(u));
			if (u->race != RC_SPELL && !nonplayer(u)){
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
			if (global.data_version <= 73) {
				if (ri(F)) {
					guard(u, GUARD_ALL);
				} else {
					guard(u, GUARD_NONE);
				}
			} else
				u->flags = ri(F);
			if (global.data_version < GUARD_VERSION) {
#if RELEASE_VERSION < GUARDFIX_VERSION
				if (fval(u, FL_GUARD)) guard(u, GUARD_ALL);
#endif
			}
			/* Kurze persistente Befehle einlesen */
			if (global.data_version >= MEMSAVE_VERSION) {
				rs(F, buf);
				while(*buf != 0) {
					S = makestrlist(buf);
					addlist(&u->orders,S);
					rs(F, buf);
				}
			}
			rds(F, &u->lastorder);
			set_string(&u->thisorder, "");
			if (global.data_version < EFFSTEALTH_VERSION)
				u_seteffstealth(u, ri(F));

			assert(u->number >= 0);
			assert(u->race != NORACE);

			while ((sk = (skill_t) ri(F)) != NOSKILL) {
				set_skill(u, sk, ri(F));
				if (sk == SK_ALCHEMY) {
				/*	init_potions(r, u); */
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
					if (get_skill(u, SK_MAGIC) > 0) {
						/* ist Magier und muss in neuen Magier konvertiert werden */
						create_mage(u, u->faction->magiegebiet);
						/* bekommt anfangs soviel Aura wie er Magie kann */
						set_spellpoints(u, effskill(u, SK_MAGIC));
					}
				} else {
					i = ri(F);
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

			if (inside_only && inner_world(r) < 2) {
				destroy_unit(u);
				memset(u, 0, sizeof(unit));
			} else {
				addlist2(up,u);
			}
		}
	}
	if (global.data_version >= BORDER_VERSION) read_borders(F);
#if defined(OLD_TRIGGER) || defined(CONVERT_TRIGGER)
	if (global.data_version >= TIMEOUT_VERSION) load_timeouts(F);
#endif

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
#ifdef AMIGA
	fputs("Ok.", stderr);
#endif

        /* Unaufgeloeste Zeiger initialisieren */
	printf("\n - Referenzen initialisieren...\n");
	resolve();
#if defined(OLD_TRIGGER) || defined (CONVERT_TRIGGER)
	resolve2();
#endif
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
		for (b=r->buildings;b;b=b->next) {
			if (b->type==&bt_lighthouse) update_lighthouse(b);
		}
	}
	printf(" - Regionen initialisieren & verbinden...\n");
	for (r = regions; r; r = r->next) {
		for (u = r->units; u; u = u->next) {
			u->faction->alive = 1;
		}

	}
	fclose(F);
	findfaction(0)->alive = 1;
	return 0;
}
/* ------------------------------------------------------------- */

#define wc(F, c) putc(c, F)
#define wnl(F) putc('\n', F)
#define wspace(F) putc(' ', F)

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
	wc(F, '"');
	wsn(F, s);
	wc(F, '"');
}

void
wi(FILE * F, int n)
{
	sprintf(buf, "%d", n);
	wsn(F, buf);
}

void wi36(FILE * F, int n)
{
	sprintf(buf, "%s", itoa36(n));
	wsn(F, buf);
}

void
write_items(FILE *F, item *ilist)
{
	item * itm;
	for (itm=ilist;itm;itm=itm->next) if (itm->number) {
		ws(F, resourcename(itm->type->rtype, 0));
		wspace(F);
		wi(F, itm->number);
		wspace(F);
	}
	ws(F, "end");
}

void
writegame(char *path, char quiet)
{
	int i,n;
	faction *f;
	ally *sf;
	region *r;
	building *b;
	ship *sh;
	unit *u;
	plane *pl;
	ursprung *ur;
	strlist *S;
	const herb_type *rht;
	FILE * F;

	write_dynamictypes();

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
	wspace(F);
	wi(F, max_unique_id);
	wspace(F);
	wi(F, nextborder);

	/* Write planes */
	wnl(F);
	wi(F, listlen(planes));
	wnl(F);

	for(pl = planes; pl; pl=pl->next) {
		wi(F, pl->id);
		wspace(F);
		ws(F, pl->name);
		wspace(F);
		wi(F, pl->minx);
		wspace(F);
		wi(F, pl->maxx);
		wspace(F);
		wi(F, pl->miny);
		wspace(F);
		wi(F, pl->maxy);
		wspace(F);
		wi(F, pl->flags);
		wspace(F);
		a_write(F, pl->attribs);
		wnl(F);
	}


	/* Write factions */

	n=listlen(factions);
	wi(F, n);
	wnl(F);

	printf(" - Schreibe %d Parteien...\n",n);
	for (f = factions; f; f = f->next) {
#if RELEASE_VERSION>= FULL_BASE36_VERSION
		wid(F, f->no);
#else
		wi(F, f->no);
#endif
		wspace(F);
		wi(F, f->unique_id);
		wspace(F);
		ws(F, f->name);
		wspace(F);
		ws(F, f->banner);
		wspace(F);
		ws(F, f->email);
		wspace(F);
		ws(F, f->passw);
		wspace(F);
#if RELEASE_VERSION>=LOCALE_VERSION
		ws(F, f->locale);
		wspace(F);
#endif
		wi(F, f->lastorders);
		wspace(F);
		wi(F, f->age);
		wspace(F);
		wi(F, f->race);
		wnl(F);
		wi(F, f->magiegebiet);
		wspace(F);
		wi(F, f->karma);
		wspace(F);
		wi(F, f->flags);
		wspace(F);
		a_write(F, f->attribs);
		wnl(F);
		write_msglevels(f->warnings, F);
		wnl(F);
		wi(F, listlen(f->ursprung));
		for(ur = f->ursprung;ur;ur=ur->next) {
			wspace(F);
			wi(F, ur->id);
			wspace(F);
			wi(F, ur->x);
			wspace(F);
			wi(F, ur->y);
		}
		wnl(F);
		wspace(F);
		f->options = f->options & ~Pow(O_DEBUG);
		wi(F, f->options);
		wnl(F);
#if RELEASE_VERSION < TYPES_VERSION
		/* bis zu dieser Stufe sind die Sprüche bekannt */
		wi(F, 0);
		wnl(F);
#endif

#if RELEASE_VERSION < 79
		for (i = 0; i != MAXPOTIONS; i++) {
			wspace(F);
			wi(F, f->showpotion[i]);
		}
		wnl(F);
#endif
		wi(F, listlen(f->allies));
		for (sf = f->allies; sf; sf = sf->next) {
			int no = (sf->faction!=NULL)?sf->faction->no:0;
			wspace(F);
#if RELEASE_VERSION>= FULL_BASE36_VERSION
			wid(F, no);
#else
			wi(F, no);
#endif
			wspace(F);
			wi(F, sf->status);
		}
		wnl(F);
#ifdef GROUPS
#if RELEASE_VERSION>=GROUPS_VERSION
	write_groups(F, f->groups);
#endif
#endif
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
		}
		wnl(F);

		wi(F, r->x);
		wspace(F);
		wi(F, r->y);
		wspace(F);
		ws(F, r->display);
		wspace(F);
		wi(F, rterrain(r));
		wspace(F);
		wi(F, r->flags & RF_SAVEMASK);
		wspace(F);
		wi(F, r->age);
		wnl(F);
		if (landregion(rterrain(r))) {
			struct demand * demand;
			ws(F, r->land->name);
			wspace(F);
			wi(F, rtrees(r));
			wspace(F);
			wi(F, rhorses(r));
			wspace(F);
			wi(F, riron(r));
			wspace(F);
			rht =  rherbtype(r);
			if (rht) {
				ws(F, resourcename(rht->itype->rtype, 0));
			} else {
				ws(F, "noherb");
			}
			wspace(F);
			wi(F, rherbs(r));
			wspace(F);
			wi(F, rpeasants(r));
			wspace(F);
			wi(F, rmoney(r));
			wspace(F);
			if (r->land) for (demand=r->land->demands; demand; demand=demand->next) {
				ws(F, resourcename(demand->type->itype->rtype, 0));
				wspace(F);
				wi(F, demand->value);
				wspace(F);
			}
			ws(F, "end");
			wnl(F);
		}
		a_write(F, r->attribs);
		wnl(F);
		wi(F, listlen(r->buildings));
		wnl(F);
		for (b = r->buildings; b; b = b->next) {
#if RELEASE_VERSION>= FULL_BASE36_VERSION
			wid(F, b->no);
#else
			wi(F, b->no);
#endif
			wspace(F);
			ws(F, b->name);
			wspace(F);
			ws(F, b->display);
			wspace(F);
			wi(F, b->size);
			wspace(F);
#ifdef TODO
			/* gibts mit dem neuen Magiesystem nicht mehr, sind nun attribute
			wi(F, b->zauber);
			wspace(F);
			*/
#endif
			ws(F, b->type->_name);
			wnl(F);
			a_write(F, b->attribs);
			wnl(F);
		}

		wi(F, listlen(r->ships));
		wnl(F);
		for (sh = r->ships; sh; sh = sh->next) {
#if RELEASE_VERSION>= FULL_BASE36_VERSION
			wid(F, sh->no);
#else
			wi(F, sh->no);
#endif
			wspace(F);
			ws(F, sh->name);
			wspace(F);
			ws(F, sh->display);
			wspace(F);
#if RELEASE_VERSION < SHIPTYPE_VERSION
			wi(F, sh->type);
#else
			ws(F, sh->type->name[0]);
#endif
			wspace(F);
			wi(F, sh->size);
#if RELEASE_VERSION > 76
			wspace(F);
			wi(F, sh->damage);
#endif
			wspace(F);
			wi(F, sh->coast);
			wnl(F);
			a_write(F, sh->attribs);
			wnl(F);
		}

		wi(F, listlen(r->units));
		wnl(F);
		for (u = r->units; u; u = u->next) {
			skill_t sk;
			wid(F, u->no);
			wspace(F);
#if RELEASE_VERSION>= FULL_BASE36_VERSION
			wid(F, u->faction->no);
#else
			wi(F, u->faction->no);
#endif
			wspace(F);
			ws(F, u->name);
			wspace(F);
			ws(F, u->display);
			wspace(F);
#ifndef NDEBUG
			if (u->race != RC_SPELL)
				assert(u->number == u->debug_number);
#endif
			wi(F, u->number);
			wspace(F);
			wi(F, u->age);
			wspace(F);
			wi(F, u->race);
			wspace(F);
			wi(F, u->irace);
			wspace(F);
			if (u->building)
#if RELEASE_VERSION>= FULL_BASE36_VERSION
				wid(F, u->building->no);
#else
				wi(F, u->building->no);
#endif
			else
				wi(F, 0);
			wspace(F);
			if (u->ship)
#if RELEASE_VERSION>= FULL_BASE36_VERSION
				wid(F, u->ship->no);
#else
				wi(F, u->ship->no);
#endif
			else
				wi(F, 0);
			wspace(F);
			wi(F, u->status);
			wspace(F);
			wi(F, u->flags & FL_SAVEMASK);
#if RELEASE_VERSION < GUARDFIX_VERSION
			wspace(F);
			wi(F, getguard(u));
#endif
			for(S=u->orders; S; S=S->next) {
				if (is_persistent(S->s)) {
					wspace(F);
					ws(F, S->s);
				}
			}
			wspace(F);
			ws(F, ""); /* Abschluß der persistenten Befehle */
			wspace(F);
			ws(F, u->lastorder);
#if RELEASE_VERSION < EFFSTEALTH_VERSION
			wspace(F);
			wi(F, u_geteffstealth(u));
#endif
			wnl(F);

			assert(u->number >= 0);
#ifdef MONEY_BUG
			if (get_money(u) < 0)
				printf("Einheit %s hat %d silber", unitname(u), get_money(u));
#else
			assert(u->race != NORACE);
#endif

			for (sk = 0; sk != MAXSKILLS; sk++) {
				if (get_skill(u, sk) > 0) {
					wi(F, sk);
					wspace(F);
					wi(F, get_skill(u, sk));
					wspace(F);
				}
			}
			wi(F, -1);
			wnl(F);
			write_items(F, u->items);
			wnl(F);
			wi(F, u->hp);
			wnl(F);
#if RELEASE_VERSION < MAGE_ATTRIB_VERSION
			if (is_mage(u)) {
				m = get_mage(u);
				wspace(F);
				wi(F, m->magietyp);
				wspace(F);
				wi(F, m->spellpoints);
				wspace(F);
				wi(F, m->spchange);
				for (i = 0; i != MAXCOMBATSPELLS; i++){
					wspace(F);
					wi(F, m->combatspell[i]);
					wspace(F);
					wi(F, m->combatspelllevel[i]);
				}
				wspace(F);
				wi(F, -1);
				wnl(F);
				/* BUG: Endlosschleife! */
				for (sp = m->spellptr;sp;sp=sp->next){
					wspace(F);
					wi(F, sp->spellid);
				}
				wspace(F);
			}
			wi(F, -1);
			wnl(F);
#endif
			a_write(F, u->attribs);
			wnl(F);
		}
	}
	wnl(F);
	write_borders(F);
	wnl(F);
#ifdef OLD_TRIGGER
	save_timeouts(F);
#endif
	fclose(F);
	printf("\nOk.\n");
}
/* ------------------------------------------------------------- */

void
curse_write(const attrib * a,FILE * f) {
	int flag;
	int mage_no;
	curse * c = (curse*)a->data.v;

	flag = (c->flag & ~(CURSE_ISNEW));

	if (c->magician){
		mage_no = c->magician->no;
	}else{
		mage_no = -1;
	}

	fprintf(f, "%d %d %d %d %d %d %d ", c->no, (int)c->cspellid, flag,
			c->duration, c->vigour, mage_no, c->effect);

	switch(cursedaten[c->cspellid].typ){
		case CURSETYP_UNIT:
		{
			curse_unit * cc = (curse_unit*)c->data;
			fprintf(f, "%d ", cc->cursedmen);
			break;
		}
		case CURSETYP_SKILL:
		{
			curse_skill * cc = (curse_skill*)c->data;
			fprintf(f, "%d %d ", (int)cc->skill, cc->cursedmen);
			break;
		}
		case CURSETYP_SECONDID:
		{
			curse_secondid * cc = (curse_secondid*)c->data;
			fprintf(f, "%d %d ", (int)cc->secondid, cc->i);
			break;
		}
		case CURSETYP_NORM:
		default:
			break;
	}
}

int
curse_read(attrib * a, FILE * f) {
	int cspellid;
	int mageid;
	curse * c = (curse*)a->data.v;

	if (global.data_version < CURSE_NO_VERSION){
		fscanf(f, "%d %d %d %d %d %d ",&cspellid, &c->flag, &c->duration,
				&c->vigour, &mageid, &c->effect);
		c->no = newunitid();
	} else {
		fscanf(f, "%d %d %d %d %d %d %d ", &c->no, &cspellid, &c->flag,
				&c->duration, &c->vigour, &mageid, &c->effect);
	}

	c->type = &cursedaten[cspellid];
	c->cspellid = (curse_t)cspellid;


	/* beim Einlesen sind noch nicht alle units da, muss also
	 * zwischengespeichert werden. */
	if (mageid == -1){
		c->magician = (unit *)NULL;
	} else {
		ur_add((void*)mageid, (void**)&c->magician, resolve_unit);
	}

	switch(cursedaten[cspellid].typ){
		case CURSETYP_UNIT:
		{
			curse_unit * cc = calloc(1, sizeof(curse_unit));

			c->data = cc;
			fscanf(f, "%d ", &cc->cursedmen);
			break;
		}
		case CURSETYP_SKILL:
		{
			curse_skill * cc = calloc(1, sizeof(curse_skill));
			int skill;

			c->data = cc;
			fscanf(f, "%d %d ", &skill, &cc->cursedmen);
			cc->skill = (skill_t)skill;
			break;
		}
		case CURSETYP_SECONDID:
		{
			curse_secondid * cc = calloc(1, sizeof(curse_secondid));
			fscanf(f, "%d %d ", &cc->secondid, &cc->i);
			break;

		}
		case CURSETYP_NORM:
		default:
			break;
	}
	chash(c);

	return 1;
}

/* ------------------------------------------------------------- */

struct fjord { int size; faction * f; } fjord[3];


void
writeadresses(const char * name)
{
	faction *f;
	FILE * F = cfopen(name, "w");

	if (!F) return;
	/* adressen liste */

	for (f = factions; f; f = f->next) {
		if (f->no != MONSTER_FACTION) {
			fprintf(F, "%s:%s:%s\n", factionname(f), f->email, f->banner);
		}
	}
	fclose(F);
}

/* ------------------------------------------------------------- */

extern attrib_type at_traveldir_new;
void
attrib_init(void)
{
	/* Gebäudetypen registrieren */
	init_buildings();
	bt_register(&bt_castle);
	bt_register(&bt_lighthouse);
	bt_register(&bt_mine);
	bt_register(&bt_quarry);
	bt_register(&bt_harbour);
	bt_register(&bt_academy);
	bt_register(&bt_magictower);
	bt_register(&bt_smithy);
	bt_register(&bt_sawmill);
	bt_register(&bt_stables);
	bt_register(&bt_monument);
	bt_register(&bt_dam);
	bt_register(&bt_caravan);
	bt_register(&bt_tunnel);
	bt_register(&bt_inn);
	bt_register(&bt_stonecircle);
	bt_register(&bt_blessedstonecircle);
	bt_register(&bt_illusion);
	bt_register(&bt_generic);
	bt_register(&bt_caldera);

	/* Schiffstypen registrieren: */
	st_register(&st_boat);
	st_register(&st_longboat);
	st_register(&st_dragonship);
	st_register(&st_caravelle);
	st_register(&st_trireme);

	/* disable: st_register(&st_transport); */

	/* Alle speicherbaren Attribute müssen hier registriert werden */
	at_register(&at_unitdissolve);
	at_register(&at_key);
	at_register(&at_traveldir_new);
	at_register(&at_familiar);
	at_register(&at_familiarmage);
	at_register(&at_eventhandler);
	at_register(&at_stealth);
	at_register(&at_mage);
	at_register(&at_bauernblut);
	at_register(&at_countdown);
	at_register(&at_showitem);
	at_register(&at_curse);
	at_register(&at_cursewall);

	at_register(&at_seenspell);
	at_register(&at_reportspell);
	at_register(&at_deathcloud);

	/* neue REGION-Attribute */
	at_register(&at_direction);
	at_register(&at_moveblock);
#if AT_SALARY
	at_register(&at_salary);
#endif
	at_register(&at_horseluck);
	at_register(&at_peasantluck);
	at_register(&at_deathcount);
	at_register(&at_chaoscount);
	at_register(&at_woodcount);
	at_register(&at_laen);
	at_register(&at_road);

	/* neue UNIT-Attribute */
	at_register(&at_alias);
	at_register(&at_siege);
	at_register(&at_target);
	at_register(&at_potion);
	at_register(&at_potionuser);
	at_register(&at_contact);
	at_register(&at_effect);
	at_register(&at_private);

#if defined(OLD_TRIGGER)
	at_register(&at_pointer_tag);
	at_register(&at_relation);
	at_register(&at_relbackref);
	at_register(&at_trigger);
	at_register(&at_action);
#endif
	at_register(&at_icastle);
	at_register(&at_guard);
	at_register(&at_lighthouse);
	at_register(&at_group);
	at_register(&at_faction_special);
	at_register(&at_prayer_timeout);
	at_register(&at_prayer_effect);
	at_register(&at_wyrm);
	at_register(&at_building_generic_type);

/* border-typen */
	register_bordertype(&bt_noway);
	register_bordertype(&bt_fogwall);
	register_bordertype(&bt_wall);
	register_bordertype(&bt_illusionwall);
	register_bordertype(&bt_firewall);
	register_bordertype(&bt_wisps);
	register_bordertype(&bt_road);

#if USE_EVENTS
	at_register(&at_events);
#endif
	at_register(&at_jihad);
}

extern void skill_init(void);
extern void skill_done(void);
void game_done(void)
{
#if 0
	int l, i;
	for (i=0;i!=MAX_MSG;++i) {
		fprintf(stderr, "%s\t\n", report_options[i]);
		for (l=0;l!=ML_MAX;++l)
			fprintf(stderr, "\t%.8d", cmsg[i][l]);
		fprintf(stderr, report_options[i]);
		fprintf(stderr, "\n");
	}
#endif
	skill_done();
	gc_done();
}

extern void inittokens(void);
extern void create_teleport_plane(void);

void read_strings(FILE * F);
void read_messages(FILE * F);

const char * messages[] = {
	"res/messages.de",
	"res/messages.en",
	NULL
};

const char * strings[] = {
	"res/strings.de",
	"res/strings.en",
	NULL
};

void
init_locales(void)
{
	FILE * F;
	int i;
	for (i=0;strings[i];++i) {
		char zText[PATH_MAX];
		strcat(strcat(strcpy(zText, basepath()), "/"), strings[i]);
		F = fopen(zText, "r+");
		if (F) {
			read_strings(F);
			fclose(F);
		} else {
			sprintf(buf, "fopen(%s): ", zText);
			perror(buf);
		}
	}
	for (i=0;messages[i];++i) {
		char zText[PATH_MAX];
		strcat(strcat(strcpy(zText, basepath()), "/"), messages[i]);
		F = fopen(zText, "r+");
		if (F) {
			read_messages(F);
			fclose(F);
		} else {
			sprintf(buf, "fopen(%s): ", zText);
			perror(buf);
		}
	}
}

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
	if (id==0) {
		*f = NULL;
		return 0;
	}
	*f = findfaction(id);
	if (*f==NULL) ur_add((void*)id, (void**)f, resolve_faction);
	return 1;
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
