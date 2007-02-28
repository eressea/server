/* vi: set ts=2:
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

#include <config.h>
#include "eressea.h"
#include "save.h"

#include "alchemy.h"
#include "alliance.h"
#include "attrib.h"
#include "border.h"
#include "building.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "movement.h"
#include "objtypes.h"
#include "order.h"
#include "pathfinder.h"
#include "plane.h"
#include "race.h"
#include "region.h"
#include "resources.h"
#include "ship.h"
#include "skill.h"
#include "spell.h"
#include "terrain.h"
#include "terrainid.h" /* only for conversion code */
#include "unit.h"

/* attributes includes */
#include <attributes/key.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/goodies.h>
#include <util/resolve.h>
#include <util/sql.h>
#include <util/rand.h>
#include <util/rng.h>
#include <util/umlaut.h>

/* libc includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define xisdigit(c)     (((c) >= '0' && (c) <= '9') || (c) == '-')
#define COMMENT_CHAR    ';'

#define ESCAPE_FIX
#define MAXORDERS 256
#define MAXPERSISTENT 128

/* exported symbols symbols */
const char * xmlfile = "eressea.xml";
const char * g_datadir;
int firstx = 0, firsty = 0;

/* local symbols */
static region * current_region;


#ifdef RESOURCE_CONVERSION
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

#define rid(F) ri36(F)

static int nextc;
#define rc(F) (nextc = getc(F))

#undef CONVERT_DBLINK
#ifdef CONVERT_DBLINK

typedef struct uniquenode {
	struct uniquenode * next;
	int id;
	faction * f;
} uniquenode;

#define HASHSIZE 2039
static uniquenode * uniquehash[HASHSIZE];

static faction *
uniquefaction(int id)
{
	uniquenode * node = uniquehash[id%HASHSIZE];
	while (node && node->id!=id) node=node->next;
	return node?node->f:NULL;
}

static void
addunique(int id, faction * f)
{
	uniquenode * fnode = calloc(1, sizeof(uniquenode));
	fnode->f = f;
	fnode->id = id;
	fnode->next = uniquehash[id%HASHSIZE];
	uniquehash[id%HASHSIZE] = fnode;
}

typedef struct mapnode {
	struct mapnode * next;
	int fno;
	int subscription;
} mapnode;

static mapnode * subscriptions[HASHSIZE];

void
convertunique(faction * f)
{
	int unique = f->subscription;
	static FILE * F = NULL;
	mapnode * mnode;
	addunique(unique, f);
	if (F==NULL) {
		static char zText[MAX_PATH];
		strcat(strcpy(zText, basepath()), "/subscriptions");
		F = fopen(zText, "r");
		if (F==NULL) {
			log_warning(("could not open %s.\n", zText));
			abort();
		}
		for (;;) {
			char zFaction[5];
			int subscription, fno;
			if (fscanf(F, "%d %s", &subscription, zFaction)<=0) break;
			mnode = calloc(1, sizeof(mapnode));
			fno = atoi36(zFaction);
			mnode->next = subscriptions[fno%HASHSIZE];
			mnode->fno = fno;
			mnode->subscription = subscription;
			subscriptions[fno%HASHSIZE] = mnode;
		}
	}
	mnode = subscriptions[f->no%HASHSIZE];
	while (mnode!=NULL && mnode->fno!=f->no) mnode = mnode->next;
	if (mnode) f->subscription = mnode->subscription;
	else {
		log_printf("%s %s %s %s\n",
				   itoa36(f->no), f->email, f->override, dbrace(f->race));
	}
}
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
		rc(F);
	}

	rc(F);

	while (nextc != '"') {
		if (nextc == EOF) {
			*s = 0;
			fprintf(stderr, "Die Datei bricht vorzeitig ab.\n");
			abort();
		}
    if (s - buffer < DISPLAYSIZE) {
      *s++ = (char)nextc;
    }
		rc(F);
	}

	rc(F);
	*s = 0;
  if (ds) {
    *ds = realloc(*ds, sizeof(char) * (strlen(buffer) + 1));
    strcpy(*ds, buffer);
  }
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
static char lbuf[MAXLINE];
static char *
getbuf(FILE * F)
{
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
      /* wenn die zeile länger als erlaubt war, ist sie ungültig,
       * und wird mit dem rest weggeworfen: */
      for (;;) {
        bp = (unsigned char *)fgets(lbuf, MAXLINE, F);
        if (bp==NULL) break;
        end = bp + strlen((const char *)bp);
        if (*(end-1)=='\n') break;
        lbuf[MAXLINE-1] = 0;
      }
      comment = false;
      bp = NULL;
    }
    cont = false;
    while (bp!=NULL && *bp && cp!=buf+MAXLINE && (char*)bp!=lbuf+MAXLINE) {
      if (isspace(*bp)) {
        if (cont) ++bp; /* removes spaces and \r afer a backslash */
        else if (eatwhite) {
          do { ++bp; } while ((char*)bp!=lbuf+MAXLINE && isspace(*bp));
          if (!quote && !start && !comment) *(cp++)=' ';
        }
        else {
          if (!comment) *(cp++)=*(bp++);
          while (cp!=buf+MAXLINE && (char*)bp!=lbuf+MAXLINE && isspace(*bp)) {
            if (!comment) *(cp++)=*bp;
            ++bp;
          }
          if (*bp==0) --cp;
        }
      }
      else if (iscntrl(*bp)) {
        *(cp++) = '?';
        ++bp;
      } else {
        cont = false;
        if (*bp==COMMENT_CHAR && !quote) {
          comment=true;
        }
        else {
          if (*bp=='"') {
            quote = (boolean)!quote;
            *cp++ = *bp;
            eatwhite=true;
          }
          else if (*bp=='\\') cont=true;
          else {
            if (!comment) *(cp++) = *bp;
            eatwhite = (boolean)!quote;
          }
        }
        ++bp;
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

#ifdef ENEMIES
static void
read_enemies(FILE * F, faction * f)
{
  if (global.data_version<ENEMIES_VERSION) return;
  f->enemies = NULL;
  for (;;) {
    char zText[32];
    fscanf(F, "%s", zText);
    if (strcmp(zText, "end")==0) break;
    else {
      int fno = atoi36(zText);
      faction_list * flist = malloc(sizeof(faction_list));
      flist->next = f->enemies;
      f->enemies = flist;
      ur_add((void*)fno, (void**)&flist->data, resolve_faction);
    }
  }
}

static void
write_enemies(FILE * F, const faction_list * flist)
{
  while (flist) {
    fprintf(F, "%s ", itoa36(flist->data->no));
  }
  fputs("end \n", F);
}
#endif

static unit *
unitorders(FILE * F, struct faction * f)
{
  int i;
  unit *u;

  if (!f) return NULL;

  i = getid();
  u = findunitg(i, NULL);

  if (u && u->race == new_race[RC_SPELL]) return NULL;
  if (u && u->faction == f) {
    order ** ordp;

    if (quiet==0) {
      printf(",_%4s_", itoa36(u->no));
      fflush(stdout);
    }

    if (!fval(u, UFL_ORDERS)) {
      /* alle wiederholbaren, langen befehle werden gesichert: */
      fset(u, UFL_ORDERS);
      u->old_orders = u->orders;
      ordp = &u->old_orders;
      while (*ordp) {
        order * ord = *ordp;
        if (!is_repeated(ord)) {
          *ordp = ord->next;
          ord->next = NULL;
          free_order(ord);
        } else {
          ordp = &ord->next;
        }
      }
    } else {
      free_orders(&u->orders);
    }
    u->orders = 0;

    ordp = &u->orders;

    for (;;) {
      const char * s;
      /* Erst wenn wir sicher sind, dass kein Befehl
      * eingegeben wurde, checken wir, ob nun eine neue
      * Einheit oder ein neuer Spieler drankommt */

      if (!getbuf(F)) break;

      init_tokens_str(buf);
      s = getstrtoken();

      if (findkeyword(s, u->faction->locale) == NOKEYWORD) {
        boolean quit = false;
        switch (findparam(s, u->faction->locale)) {
          case P_UNIT:
          case P_REGION:
          case P_FACTION:
          case P_NEXT:
          case P_GAMENAME:
            quit = true;
        }
        if (quit) break;
      }
      if (buf[0]) {
        /* Nun wird der Befehl erzeut und eingehängt */
        *ordp = parse_order(buf, u->faction->locale);
        if (*ordp) ordp = &(*ordp)->next;
      }
    }

  } else {
    /* cmistake(?, buf, 160, MSG_EVENT); */
    return NULL;
  }
  return u;
}

static faction *
factionorders(void)
{
  char fid[16];
  faction * f = NULL;

  strlcpy(fid, getstrtoken(), sizeof(fid));
#ifdef FUZZY_BASE36
  int id = atoi36(fid);
  if (id!=0) f = findfaction(id);
  if (f==NULL) {
    id = atoi(fid);
    if (id!=0) f = findfaction(id);
  }
#else  
  f = findfaction(atoi36(fid));
#endif
  
  if (f!=NULL) {
    const char * pass = getstrtoken();
    /* Kontrolliere, ob das Passwort richtig eingegeben wurde. Es
     * muß in "Gänsefüßchen" stehen!! */
    
    /* War vorher in main.c:getgarbage() */
    if (quiet==0) {
      printf(" %4s;", factionid(f));
      fflush(stdout);
    }
    
    if (checkpasswd(f, pass, true) == false) {
      log_warning(("Invalid password for faction %s\n", fid));
      ADDMSG(&f->msgs, msg_message("msg_errors", "string",
                                   "Das Passwort wurde falsch eingegeben"));
      return 0;
    }
    /* Die Partei hat sich zumindest gemeldet, so daß sie noch
     * nicht als untätig gilt */
    
    /* TODO: +1 ist ein Workaround, weil turn erst in process_orders
     * incrementiert wird. */
    f->lastorders = global.data_turn+1;
    
  } else {
    log_warning(("Befehle für die ungültige Partei %s\n", fid));
  }
  return f;
}

double
version(void)
{
	return RELEASE_VERSION * 0.1;
}
/* ------------------------------------------------------------- */

static param_t
igetparam (const char *s, const struct locale *lang)
{
  return findparam (igetstrtoken (s), lang);
}

int
readorders(const char *filename)
{
	FILE * F = NULL;
	char *b;
	int nfactions=0;
	struct faction *f = NULL;

	if (filename) F = cfopen(filename, "rt");
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
#undef LOCALE_CHANGE
#ifdef LOCALE_CHANGE
			if (f && find_locale(s)) {
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
      b = getbuf(F);
      break;

    default:
			b = getbuf(F);
      break;
		}
	}

	fclose(F);
	puts("\n");
	log_printf("   %d Befehlsdateien gelesen\n", nfactions);
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
int loadplane = 0;

boolean dirtyload = false;
boolean incomplete_data = false;

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

void
create_backup(char *file)
{
#ifdef HAVE_UNISTD_H
  char bfile[MAX_PATH];
  int c = 1;

  if (access(file, R_OK) == 0) return;
  do {
    sprintf(bfile, "%s.backup%d", file, c);
    c++;
  } while(access(bfile, R_OK) == 0);
  link(file, bfile);
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
    const item_type * itype;
		rs(F, buf);
		if (!strcmp("end", buf)) break;
    itype = it_find(buf);
    assert(itype!=NULL);
    if (itype!=NULL) {
      i_change(ilist, itype, ri(F));
    }
	}
}

#ifdef RESOURCE_CONVERSION
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

static void
read_alliances(FILE * F)
{
  char pbuf[32];

  if (global.data_version<SAVEALLIANCE_VERSION) {
    if (!AllianceRestricted() && !AllianceAuto()) return;
  }

  rs(F, pbuf);
	while (strcmp(pbuf, "end")!=0) {
		rs(F, buf);
		makealliance(atoi36(pbuf), buf);
		rs(F, pbuf);
	}
}

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

static void
wi(FILE * F, int n)
{
	fprintf(F, "%d ", n);
}

static void 
wi36(FILE * F, int n)
{
	fprintf(F, "%s ", itoa36(n));
}

void
write_alliances(FILE * F)
{
	alliance * al = alliances;
	while (al) {
		wi36(F, al->id);
		ws(F, al->name);
		al = al->next;
		wnl(F);
	}
	fprintf(F, "end ");
	wnl(F);
}

void
write_items(FILE *F, item *ilist)
{
	item * itm;
	for (itm=ilist;itm;itm=itm->next) if (itm->number) {
    fprintf(F, "%s %i ", resourcename(itm->type->rtype, 0), itm->number);
	}
	fputs("end ", F);
}

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

void
fwriteorder(FILE * F, const order * ord, const struct locale * lang)
{
  write_order(ord, lang, buf, sizeof(buf));
  if (buf[0]) fwritestr(F, buf);
}

unit *
readunit(FILE * F)
{
  skill_t sk;
  unit * u;
  int number, n, p;
  order ** orderp;

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
  {
    int n = rid(F);
    faction * f = findfaction(n);
    if (f!=u->faction) u_setfaction(u, f);
  }
  rds(F, &u->name);
  if (lomem) rds(F, 0);
  else rds(F, &u->display);
#ifndef NDEBUG
  if (strlen(u->name)>=NAMESIZE) u->name[NAMESIZE] = 0;
  if (u->display && strlen(u->display)>=DISPLAYSIZE) u->display[DISPLAYSIZE] = 0;
#endif
  number = ri(F);
  u->age = (short)ri(F);
  if (global.data_version<NEWRACE_VERSION) {
    u->race = new_race[(race_t)ri(F)];
    u->irace = new_race[(race_t)ri(F)];
  } else {
    char * space;
    
    rs(F, buf);
    space = strchr(buf, ' ');
    if (space!=NULL) {
      char * inc = space+1;
      char * outc = space;
      do {
        while (*inc==' ') ++inc;
        while (*inc) {
          *outc++ = *inc++;
          if (*inc==' ') break;
        }
      } while (*inc);
      *outc = 0;
    }
    u->race = rc_find(buf);
    assert(u->race);
    rs(F, buf);
    if (strlen(buf)) u->irace = rc_find(buf);
    else u->irace = u->race;
  }
  if (u->faction == NULL) {
    log_error(("unit %s has faction == NULL\n", unitname(u)));
    u_setfaction(u, findfaction(MONSTER_FACTION));
    set_number(u, 0);
  }

  if (count_unit(u)) u->faction->no_units++;

  set_number(u, number);
  u->building = findbuilding(rid(F));
  u->ship = findship(rid(F));

  if (global.data_version <= 73) {
    if (ri(F)) {
      fset(u, UFL_OWNER);
    } else {
      freset(u, UFL_OWNER);
    }
  }
  setstatus(u, ri(F));
  if (global.data_version < NEWSTATUS_VERSION) {
    switch (u->status) {
      case 0:
        setstatus(u, ST_FIGHT); 
        break;
      case 1: 
        setstatus(u, ST_BEHIND);
        break;
      case 2: 
        setstatus(u, ST_AVOID);
        break;
      case 3: 
        setstatus(u, ST_FLEE);
      default:
        assert(!"unknown status in data");
    }
  }
  if (global.data_version <= 73) {
    if (ri(F)) {
      guard(u, GUARD_ALL);
    } else {
      guard(u, GUARD_NONE);
    }
  } else {
    u->flags = ri(F) & ~UFL_DEBUG;
    u->flags &= UFL_SAVEMASK;
  }
  /* Persistente Befehle einlesen */
  free_orders(&u->orders);
  freadstr(F, buf, sizeof(buf));
  p = n = 0;
  orderp = &u->orders;
  while (*buf != 0) {
    order * ord = parse_order(buf, u->faction->locale);
    if (ord!=NULL) {
      if (++n<MAXORDERS) {
        if (!is_persistent(ord) || ++p<MAXPERSISTENT) {
          *orderp = ord;
          orderp = &ord->next;
          ord = NULL;
        } else if (p==MAXPERSISTENT) {
          log_warning(("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT));
        }
      } else if (n==MAXORDERS) {
        log_warning(("%s had %d or more orders\n", unitname(u), MAXORDERS));
      }
      if (ord!=NULL) free_order(ord);
    }
    freadstr(F, buf, sizeof(buf));
  }
  if (global.data_version<NOLASTORDER_VERSION) {
    order * ord;
    freadstr(F, buf, sizeof(buf));
    ord = parse_order(buf, u->faction->locale);
    if (ord!=NULL) {
#ifdef LASTORDER
      set_order(&u->lastorder, ord);
#else
      addlist(&u->orders, ord);
#endif
    }
  }
  set_order(&u->thisorder, NULL);

  assert(u->race);
  if (global.data_version<NEWSKILL_VERSION) {
    /* convert old data */
    if (u->number) {
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
	read_items(F, &u->items);
	u->hp = ri(F);
	if (u->hp < u->number) {
		log_error(("Einheit %s hat %u Personen, und %u Trefferpunkte\n", itoa36(u->no),
				   u->number, u->hp));
		u->hp=u->number;
	}

	if (global.data_version < MAGE_ATTRIB_VERSION) {
		int i = ri(F);
		if (i != -1){
			attrib * a;
			int csp = 0;
			sc_mage * mage = calloc(1, sizeof(sc_mage));

      mage->magietyp = (magic_t) i;
			mage->spellpoints = ri(F);
			mage->spchange = ri(F);
			while ((i = ri(F)) != -1) {
				mage->combatspells[csp].sp = find_spellbyid(mage->magietyp, (spellid_t)i);
				mage->combatspells[csp].level = ri(F);
				csp++;
			}
			while ((i = ri(F)) != -1) {
				add_spell(mage, find_spellbyid(mage->magietyp, (spellid_t)i));
			}
			mage->spellcount = 0;
			a = a_add(&u->attribs, a_new(&at_mage));
			a->data.v = mage;
		}
	}
	a_read(F, &u->attribs);
	return u;
}

void
writeunit(FILE * F, const unit * u)
{
	order * ord;
	int i, p = 0;
	wi36(F, u->no);
	wi36(F, u->faction->no);
	ws(F, u->name);
  ws(F, u->display?u->display:"");
	wi(F, u->number);
	wi(F, u->age);
	ws(F, u->race->_name[0]);
	ws(F, u->irace!=u->race?u->irace->_name[0]:"");
	if (u->building)
		wi36(F, u->building->no);
	else
		wi(F, 0);
	if (u->ship) wi36(F, u->ship->no);
	else wi36(F, 0);
	wi(F, u->status);
	wi(F, u->flags & UFL_SAVEMASK);
	wnl(F);
#ifndef LASTORDER
  for (ord = u->old_orders; ord; ord=ord->next) {
    if (++p<MAXPERSISTENT) {
      fwriteorder(F, ord, u->faction->locale);
      fputc(' ', F);
    } else {
      log_error(("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT));
      break;
    }
  }
#endif
  for (ord = u->orders; ord; ord=ord->next) {
    if (u->old_orders && is_repeated(ord)) continue; /* has new defaults */
    if (is_persistent(ord)) {
      if (++p<MAXPERSISTENT) {
        fwriteorder(F, ord, u->faction->locale);
        fputc(' ', F);
      } else {
        log_error(("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT));
        break;
      }
    }
  }
  /* write an empty string to terminate the list */
  fputs("\"\"", F);
  wnl(F);
#if RELEASE_VERSION<NOLASTORDER_VERSION
  /* the current default order */
  fwriteorder(F, u->lastorder, u->faction->locale);
  wnl(F);
#endif

  assert(u->race);

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
	a_write(F, u->attribs);
	wnl(F);
}

region *
readregion(FILE * F, short x, short y)
{
	region * r = findregion(x, y);
	const terrain_type * terrain;

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
  if (lomem) rds(F, 0);
  else rds(F, &r->display);
  
  if (global.data_version < TERRAIN_VERSION) {
    int ter = ri(F);
    if (global.data_version < NOFOREST_VERSION) {
      if (ter>T_PLAIN) --ter;
    }
    terrain = newterrain((terrain_t)ter);
  } else {
    char name[64];
    rs(F, name);
    terrain = get_terrain(name);
    if (terrain==NULL) {
      log_error(("Unknown terrain '%s'\n", name));
      assert(!"unknown terrain");
    }
  }
  r->terrain = terrain;
  r->flags = (char) ri(F);

  if (global.data_version >= REGIONAGE_VERSION)
      r->age = (unsigned short) ri(F);
  else
      r->age = 0;

  if (fval(r->terrain, LAND_REGION)) {
    r->land = calloc(1, sizeof(land_region));
    rds(F, &r->land->name);
#ifndef NDEBUG
    if (strlen(r->land->name)>=NAMESIZE) r->land->name[NAMESIZE] = 0;
#endif
  }
	if (r->land) {
		int i;
		if(global.data_version < GROWTREE_VERSION) {
			i = ri(F); rsettrees(r, 2, i);
		} else {
			i = ri(F);
			if (i<0) {
				log_error(("number of trees in %s is %d.\n", 
						   regionname(r, NULL), i));
				i=0;
			}
			rsettrees(r, 0, i);
			i = ri(F); 
			if (i<0) {
				log_error(("number of young trees in %s is %d.\n", 
						   regionname(r, NULL), i));
				i=0;
			}
			rsettrees(r, 1, i);
			i = ri(F); 
			if (i<0) {
				log_error(("number of seeds in %s is %d.\n", 
						   regionname(r, NULL), i));
				i=0;
			}
			rsettrees(r, 2, i);
		}
		i = ri(F); rsethorses(r, i);
		if (global.data_version < NEWRESOURCE_VERSION) {
			i = ri(F);
#ifdef RESOURCE_CONVERSION
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
        if (res->type==NULL) {
          log_error(("invalid resourcetype %s in data.\n", buf));
        }
				assert(res->type!=NULL);
				res->level = ri(F);
				res->amount = ri(F);

				if(global.data_version >= RANDOMIZED_RESOURCES_VERSION) {
					res->startlevel = ri(F);
					res->base = ri(F);
					res->divisor = ri(F);
				} else {
					int i;
					for (i=0;r->terrain->production[i].type;++i) {
						if (res->type->rtype == r->terrain->production[i].type) break;
					}

          res->base = dice_rand(r->terrain->production[i].base);
					res->divisor = dice_rand(r->terrain->production[i].divisor);
          res->startlevel = 1;
				}

				*pres = res;
				pres=&res->next;
			}
		}
		rs(F, buf);
		if (strcmp(buf, "noherb") != 0) {
      const resource_type * rtype = rt_find(buf);
      assert(rtype && rtype->itype && fval(rtype->itype, ITF_HERB));
      rsetherbtype(r, rtype->itype);
		} else {
			rsetherbtype(r, NULL);
		}
		rsetherbs(r, (short)ri(F));
		rsetpeasants(r, ri(F));
		rsetmoney(r, ri(F));
	}

	assert(r->terrain!=NULL);
	assert(rhorses(r) >= 0);
	assert(rpeasants(r) >= 0);
	assert(rmoney(r) >= 0);

	if (r->land) {
		for (;;) {
      const struct item_type * itype;
			rs(F, buf);
			if (!strcmp(buf, "end")) break;
      itype = it_find(buf);
      assert(itype->rtype->ltype);
			r_setdemand(r, itype->rtype->ltype, ri(F));
		}
    if (global.data_version>=REGIONITEMS_VERSION) {
      read_items(F, &r->land->items);
    }
	}
	a_read(F, &r->attribs);

	return r;
}

void
writeregion(FILE * F, const region * r)
{
  ws(F, r->display?r->display:"");
	ws(F, r->terrain->_name);
	wi(F, r->flags & RF_SAVEMASK);
	wi(F, r->age);
	wnl(F);
	if (fval(r->terrain, LAND_REGION)) {
		const item_type *rht;
		struct demand * demand;
    rawmaterial * res = r->resources;
		ws(F, r->land->name);
		assert(rtrees(r,0)>=0);
		assert(rtrees(r,1)>=0);
		assert(rtrees(r,2)>=0);
		wi(F, rtrees(r,0));
		wi(F, rtrees(r,1));
		wi(F, rtrees(r,2));
		wi(F, rhorses(r));

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

    rht = rherbtype(r);
		if (rht) {
			ws(F, resourcename(rht->rtype, 0));
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
		fputs("end \n", F);
#if RELEASE_VERSION>=REGIONITEMS_VERSION
    write_items(F, r->land->items);
    wnl(F);
#endif
	}
	a_write(F, r->attribs);
	wnl(F);
}

static ally **
addally(const faction * f, ally ** sfp, int aid, int state)
{
  struct faction * af = findfaction(aid);
  ally * sf;

  state &= ~HELP_OBSERVE;
#ifndef REGIONOWNERS
  state &= ~HELP_TRAVEL;
#endif

  if (state==0) return sfp;

  sf = calloc(1, sizeof(ally));
  sf->faction = af;
  if (!sf->faction) {
    variant id;
    id.i = aid;
    ur_add(id, (void**)&sf->faction, resolve_faction);
  }
  sf->status = state & HELP_ALL;

  while (*sfp) sfp=&(*sfp)->next;
  *sfp = sf;
  return &sf->next;
}

/** Reads a faction from a file.
 * This function requires no context, can be called in any state. The
 * faction may not already exist, however.
 */
faction *
readfaction(FILE * F)
{
  ally **sfp;
  int planes;
  int i = rid(F);
  faction * f = findfaction(i);
  char * email = NULL;

  if (f==NULL) {
    f = (faction *) calloc(1, sizeof(faction));
    f->no = i;
  } else {
    f->allies = NULL; /* mem leak */
    while (f->attribs) a_remove(&f->attribs, f->attribs);
  }
  f->subscription = ri(F);

  if (alliances!=NULL) {
    int allianceid = rid(F);
    if (allianceid!=0) f->alliance = findalliance(allianceid);
    if (f->alliance) {
      faction_list * flist = malloc(sizeof(faction_list));
      flist->data = f;
      flist->next = f->alliance->members;
      f->alliance->members = flist;
    }
  }

  rds(F, &f->name);
  rds(F, &f->banner);
#ifndef NDEBUG
  if (strlen(f->name)>=NAMESIZE) f->name[NAMESIZE] = 0;
  if (strlen(f->banner)>=DISPLAYSIZE) f->banner[DISPLAYSIZE] = 0;
#endif

  if (quiet==0) printf("   - Lese Partei %s (%s)\n", f->name, factionid(f));

  rds(F, &email);
  if (set_email(&f->email, email)!=0) {
    log_warning(("Invalid email address for faction %s: %s\n", itoa36(f->no), email));
    set_email(&f->email, "");
  }
  free(email);
  rds(F, &f->passw);
  if (global.data_version >= OVERRIDE_VERSION) {
    rds(F, &f->override);
  } else {
    f->override = strdup(itoa36(rng_int()));
  }

  if (global.data_version < LOCALE_VERSION) {
    f->locale = find_locale("de");
  } else {
    rs(F, buf);
    f->locale = find_locale(buf);
  }
  f->lastorders = ri(F);
  f->age = ri(F);
  if (global.data_version < NEWRACE_VERSION) {
    race_t rc = (char) ri(F);
    f->race = new_race[rc];
  } else {
    rs(F, buf);
    f->race = rc_find(buf);
    assert(f->race);
  }
#ifdef CONVERT_DBLINK
  convertunique(f);
#endif
  f->magiegebiet = (magic_t)ri(F);

#ifdef KARMA_MODULE
  f->karma = ri(F);
#else
  /* ignore karma */
  ri(F);
#endif /* KARMA_MODULE */

  f->flags = ri(F);

  a_read(F, &f->attribs);
  if (global.data_version>=CLAIM_VERSION) {
    read_items(F, &f->items);
  }
  for (;;) {
    int level;
    fscanf(F, "%s", buf);
    if (strcmp("end", buf)==0) break;
    fscanf(F, "%d ", &level);
  } 
  planes = ri(F);
  while(--planes >= 0) {
    int id = ri(F);
    short ux = (short)ri(F);
    short uy = (short)ri(F);
    set_ursprung(f, id, ux, uy);
  }
  f->newbies = 0;
  
  i = f->options = ri(F);

  if ((i & (want(O_REPORT)|want(O_COMPUTER)))==0 && f->no!=MONSTER_FACTION) {
    /* Kein Report eingestellt, Fehler */
    f->options = f->options | want(O_REPORT) | want(O_ZUGVORLAGE);
  }

  if (global.data_version < TYPES_VERSION) {
    int sk = ri(F); /* f->seenspell überspringen */
    spell_list * slist;
    for (slist=spells;slist!=NULL;slist=slist->next) {
      spell * sp = slist->data;

      if (sp->magietyp==f->magiegebiet && sp->level<=sk) {
        a_add(&f->attribs, a_new(&at_seenspell))->data.v = sp;
      }
    }
  }

  sfp = &f->allies;
  if (global.data_version<ALLIANCES_VERSION) {
    int p = ri(F);
    while (--p >= 0) {
      int aid = rid(F);
      int state = ri(F);
      sfp = addally(f, sfp, aid, state);
    }
  } else {
    for (;;) {
      rs(F, buf);
      if (strcmp(buf, "end")==0) break;
      else {
        int aid = atoi36(buf);
        int state = ri(F);
        sfp = addally(f, sfp, aid, state);
      }
    }
  }
  read_groups(F, f);
#ifdef ENEMIES
  read_enemies(F, f);
#endif
  return f;
}

void
writefaction(FILE * F, const faction * f)
{
	ally *sf;
	ursprung *ur;

	wi36(F, f->no);
	wi(F, f->subscription);
  if (alliances!=NULL) {
    if (f->alliance) wi36(F, f->alliance->id);
    else wi36(F, 0);
  }

	ws(F, f->name);
	ws(F, f->banner);
	ws(F, f->email);
	ws(F, f->passw);
	ws(F, f->override);
	ws(F, locale_name(f->locale));
	wi(F, f->lastorders);
	wi(F, f->age);
	ws(F, f->race->_name[0]);
	wnl(F);
	wi(F, f->magiegebiet);
#ifdef KARMA_MODULE
	wi(F, f->karma);
#else
  wi(F, 0);
#endif /* KARMA_MODULE */

	wi(F, f->flags&FFL_SAVEMASK);
	a_write(F, f->attribs);
	wnl(F);
#if RELEASE_VERSION>=CLAIM_VERSION
  write_items(F, f->items);
  wnl(F);
#endif
  fputs("end ", F);
  wnl(F);
	wi(F, listlen(f->ursprung));
	for(ur = f->ursprung;ur;ur=ur->next) {
		wi(F, ur->id);
		wi(F, ur->x);
		wi(F, ur->y);
	}
	wnl(F);
	wi(F, f->options & ~want(O_DEBUG));
	wnl(F);

	for (sf = f->allies; sf; sf = sf->next) {
		int no = (sf->faction!=NULL)?sf->faction->no:0;
		int status = alliedfaction(NULL, f, sf->faction, HELP_ALL);
		if (status!=0) {
			wi36(F, no);
			wi(F, sf->status);
		}
	}
	fprintf(F, "end ");
	wnl(F);
	write_groups(F, f->groups);
#ifdef ENEMIES
  write_enemies(F, f->enemies);
#endif
}

int
readgame(const char * filename, int backup)
{
  int i, n, p;
  faction *f, **fp;
  region *r;
  building *b, **bp;
  ship **shp;
  unit *u;
  FILE * F;
  int rmax = maxregions;

  sprintf(buf, "%s/%s", datapath(), filename);
  log_printf("- reading game data from %s\n", filename);
  if (backup) create_backup(buf);
  F = cfopen(buf, "r");
  if (F==NULL) {
    log_error(("Keine Spieldaten gefunden.\n"));
    return -1;
  }

  rc(F);

  /* globale Variablen */

  global.data_version = ri(F);
  assert(global.data_version>=MIN_VERSION || !"unsupported data format");
  assert(global.data_version<=RELEASE_VERSION || !"unsupported data format");
  assert(global.data_version >= NEWSOURCE_VERSION);
  if(global.data_version >= SAVEXMLNAME_VERSION) {
    char basefile[1024];
    const char *basearg = "(null)";

    rs(F, basefile);
    assert(xmlfile != NULL);
    basearg = strrchr(xmlfile, '/');
    if (basearg==NULL) {
      basearg = xmlfile;
    } else {
      ++basearg;
    }
    if (strcmp(basearg, basefile)!=0) {
      log_warning(("xmlfile mismatch: datafile contains %s, argument/default is %s\n", basefile, basearg));
      printf("WARNING: any key to continue, Ctrl-C to stop\n");
      getchar();
    }
  }
  if (global.data_version >= GLOBAL_ATTRIB_VERSION) {
    a_read(F, &global.attribs);
  }
  global.data_turn = turn = ri(F);
  ri(F); /* max_unique_id = */ 
  nextborder = ri(F);

  /* Planes */
  planes = NULL;
  n = ri(F);
  while(--n >= 0) {
    plane *pl = calloc(1, sizeof(plane));
    pl->id = ri(F);
    rds(F, &pl->name);
    pl->minx = (short)ri(F);
    pl->maxx = (short)ri(F);
    pl->miny = (short)ri(F);
    pl->maxy = (short)ri(F);
    pl->flags = ri(F);
    if (global.data_version>WATCHERS_VERSION) {
      rs(F, buf);
      while (strcmp(buf, "end")!=0) {
        watcher * w = calloc(sizeof(watcher),1);
        variant fno;
        fno.i = atoi36(buf);
        w->mode = (unsigned char)ri(F);
        w->next = pl->watchers;
        pl->watchers = w;
        ur_add(fno, (void**)&w->faction, resolve_faction);
        rs(F, buf);
      }
    }
    a_read(F, &pl->attribs);
    addlist(&planes, pl);
  }

  /* Read factions */
  if (global.data_version>=ALLIANCES_VERSION) {
    read_alliances(F);
  }
  n = ri(F);
  if (quiet<2) printf(" - Einzulesende Parteien: %d\n", n);
  fp = &factions;
  while (*fp) fp=&(*fp)->next;

  /* fflush (stdout); */

  while (--n >= 0) {
    faction * f = readfaction(F);

    *fp = f;
    fp = &f->next;
    fhash(f);
  }
  *fp = 0;

  /* Benutzte Faction-Ids */

  i = ri(F);
  while (i--) {
    ri(F); /* used faction ids. ignore. */
  }

  /* Regionen */

  n = ri(F);
  if (rmax<0) rmax = n;
  if (quiet<2) printf(" - Einzulesende Regionen: %d/%d\r", rmax, n);
  if (loadplane || dirtyload || firstx || firsty || maxregions>=0) {
    incomplete_data = true;
  }
  while (--n >= 0) {
    unit **up;
    boolean skip = false;
    short x = (short)ri(F);
    short y = (short)ri(F);
    plane * pl = findplane(x, y);

    if (firstx || firsty) {
      if (x!=firstx || y!=firsty) {
        skip = true;
      } else {
        firstx=0;
        firsty=0;
        if (rmax>0) rmax = min(n, rmax)-1;
      }
    }
    if (loadplane && (!pl || pl->id!=loadplane)) skip = true;
    if (rmax==0) {
      if (dirtyload) break;
      skip = true;
    }
    if (quiet<2 && (n & 0x3FF) == 0) {	/* das spart extrem Zeit */
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
      b->no = rid(F);
      *bp = b;
      bp = &b->next;
      bhash(b);
      rds(F, &b->name);
      if (lomem) rds(F, 0);
      else rds(F, &b->display);
#ifndef NDEBUG
      if (strlen(b->name)>=NAMESIZE) b->name[NAMESIZE] = 0;
      if (b->display && strlen(b->display)>=DISPLAYSIZE) b->display[DISPLAYSIZE] = 0;
#endif
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
    }
    /* Schiffe */

    p = ri(F);
    shp = &r->ships;

    while (--p >= 0) {
      ship * sh = (ship *) calloc(1, sizeof(ship));
      sh->region = r;
      sh->no = rid(F);
      *shp = sh;
      shp = &sh->next;
      shash(sh);
      rds(F, &sh->name);
      if (lomem) rds(F, 0);
      else rds(F, &sh->display);
#ifndef NDEBUG
      if (sh->name && strlen(sh->name)>=NAMESIZE) sh->name[NAMESIZE] = 0;
      if (sh->display && strlen(sh->display)>=DISPLAYSIZE) sh->display[DISPLAYSIZE] = 0;
#endif

      rs(F, buf);
      sh->type = st_find(buf);
      assert(sh->type || !"ship_type not registered!");
      sh->size = ri(F);
      sh->damage = ri(F);

      /* Attribute rekursiv einlesen */

      sh->coast = (direction_t)ri(F);
      a_read(F, &sh->attribs);
    }

    *shp = 0;

    /* Einheiten */

    p = ri(F);
    up = &r->units;

    while (--p >= 0) {
      unit * u = readunit(F);
      sc_mage * mage;

      assert(u->region==NULL);
      u->region = r;
      if (u->flags&UFL_GUARD) fset(r, RF_GUARDED);
      *up = u;
      up = &u->next;

      update_interval(u->faction, u->region);
      mage = get_mage(u);
      if (mage && mage->spellcount<0) {
        mage->spellcount = 0;
        updatespelllist(u);
      }
    }
  }
  if (quiet<2) printf("\n");
  if (!dirtyload) {
    read_borders(F);
  }

  fclose(F);

  /* Unaufgeloeste Zeiger initialisieren */
  if (quiet<2) printf("\n - Referenzen initialisieren...\n");
  resolve();

  if (quiet<2) printf("\n - Leere Gruppen löschen...\n");
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

  for (r=regions;r;r=r->next) {
    building * b;
    for (b=r->buildings;b;b=b->next) update_lighthouse(b);
  }
  if (quiet < 2) printf(" - Regionen initialisieren & verbinden...\n");
  for (f = factions; f; f = f->next) {
    for (u = f->units; u; u = u->nextF) {
      if (u->number>0) {
        f->alive = 1;
        break;
      }
    }
  }
  if (findfaction(0)) {
    findfaction(0)->alive = 1;
  }
  if (loadplane || maxregions>=0) {
    remove_empty_factions(false);
  }
  return 0;
}

int
writegame(const char *filename, int quiet)
{
  char *base;
  int n;
  faction *f;
  region *r;
  building *b;
  ship *sh;
  unit *u;
  plane *pl;
  FILE * F;
#ifdef USE_PLAYERS
  char playerfile[MAX_PATH];
#endif

#ifdef USE_PLAYERS
  sprintf(buf, "%s/%d.players", datapath(), turn);
  export_players(playerfile);
#endif

  sprintf(buf, "%s/%s", datapath(), filename);
#ifdef HAVE_UNISTD_H
  if (access(buf, R_OK) == 0) {
    /* make sure we don't overwrite some hardlinkedfile */
    unlink(buf);
  }
#endif
  F = cfopen(buf, "w");
  if (F==NULL)
    return -1;

  if (!quiet)
    printf("Schreibe die %d. Runde...\n", turn);

  /* globale Variablen */

  wi(F, RELEASE_VERSION);
  wnl(F);

  base = strrchr(xmlfile, '/');
  if(base) {
    ws(F, base+1);
  } else {
    ws(F, xmlfile);
  }
  wnl(F);

  a_write(F, global.attribs);
  wnl(F);

  wi(F, turn);
  wi(F, 0/*max_unique_id*/);
  wi(F, nextborder);

  /* Write planes */
  wnl(F);
  wi(F, listlen(planes));
  wnl(F);

  for(pl = planes; pl; pl=pl->next) {
    watcher * w;
    wi(F, pl->id);
    ws(F, pl->name);
    wi(F, pl->minx);
    wi(F, pl->maxx);
    wi(F, pl->miny);
    wi(F, pl->maxy);
    wi(F, pl->flags);
    w = pl->watchers;
    while (w) {
      if (w->faction) {
        wi36(F, w->faction->no);
        wi(F, w->mode);
      }
      w = w->next;
    }
    fputs("end ", F);
    a_write(F, pl->attribs);
    wnl(F);
  }


  /* Write factions */
#if RELEASE_VERSION>=ALLIANCES_VERSION
  write_alliances(F);
#endif
  n = listlen(factions);
  wi(F, n);
  wnl(F);

  if (quiet < 2) printf(" - Schreibe %d Parteien...\n",n);
  for (f = factions; f; f = f->next) {
    writefaction(F, f);
  }

  wi(F, 0); /* used faction ids. old stuff */
  wnl(F);

  /* Write regions */

  n=listlen(regions);
  wi(F, n);
  wnl(F);
  if (quiet<2) printf(" - Schreibe Regionen: %d  \r", n);

  for (r = regions; r; r = r->next, --n) {
    /* plus leerzeile */
    if (quiet<2 && (n%1024)==0) {	/* das spart extrem Zeit */
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
      ws(F, b->display?b->display:"");
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
      ws(F, sh->display?sh->display:"");
      ws(F, sh->type->name[0]);
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
  fclose(F);
  if (quiet<2) printf("\nOk.\n");
  return 0;
}

int
a_readint(attrib * a, FILE * f)
{
  /*	assert(sizeof(int)==sizeof(a->data)); */
  fscanf(f, "%d", &a->data.i);
  return AT_READ_OK;
}

void
a_writeint(const attrib * a, FILE * f)
{
  fprintf(f, "%d ", a->data.i);
}

int
a_readshorts(attrib * a, FILE * f)
{
  if (global.data_version<ATTRIBREAD_VERSION) {
    return a_readint(a, f);
  }
  fscanf(f, "%hd %hd", &a->data.sa[0], &a->data.sa[1]);
  return AT_READ_OK;
}

void
a_writeshorts(const attrib * a, FILE * f)
{
  fprintf(f, "%hd %hd ", a->data.sa[0], a->data.sa[1]);
}

int
a_readchars(attrib * a, FILE * f)
{
  int i;
  if (global.data_version<ATTRIBREAD_VERSION) {
    return a_readint(a, f);
  }
  for (i=0;i!=4;++i) {
    int c;
    fscanf(f, "%d", &c);
    a->data.ca[i] = (char)c;
  }
  return AT_READ_OK;
}

void
a_writechars(const attrib * a, FILE * f)
{
  int i;
  
  for (i=0;i!=4;++i) {
    int c = a->data.ca[i];
    fprintf(f, "%d ", c);
  }
}

int
a_readvoid(attrib * a, FILE * f)
{
  if (global.data_version<ATTRIBREAD_VERSION) {
    return a_readint(a, f);
  }
  return AT_READ_OK;
}

void
a_writevoid(const attrib * a, FILE * f)
{
}

static char *
read_quoted(FILE * f, char *c, size_t size)
{
  char * s = c;
  do {
    *s = (char) fgetc(f);
  } while (*s!='"');

  for (;;) {
    *s = (char) fgetc(f);
    if (*s=='"') break;
    if (s<c+size) ++s;
  }
  *s = 0;
  return c;
}

int
a_readstring(attrib * a, FILE * f)
{
  char zText[4096];
  read_quoted(f, zText, sizeof(zText));
  a->data.v = strdup(zText);
  return AT_READ_OK;
}

void
a_writestring(const attrib * a, FILE * f)
{
  assert(a->data.v);
  fprintf(f, "\"%s\" ", (char*)a->data.v);
}

void
a_finalizestring(attrib * a)
{
  free(a->data.v);
}
