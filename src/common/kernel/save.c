/* vi: set ts=2:
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
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
#include <kernel/eressea.h>
#include "save.h"

#include "alchemy.h"
#include "alliance.h"
#include "border.h"
#include "building.h"
#include "faction.h"
#include "group.h"
#include "item.h"
#include "karma.h"
#include "magic.h"
#include "message.h"
#include "move.h"
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
#include "version.h"

#include "textstore.h"
#include "binarystore.h"

/* attributes includes */
#include <attributes/key.h>

/* util includes */
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/event.h>
#include <util/filereader.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/parser.h>
#include <util/rand.h>
#include <util/resolve.h>
#include <util/rng.h>
#include <util/sql.h>
#include <util/storage.h>
#include <util/umlaut.h>
#include <util/unicode.h>

#include <libxml/encoding.h>

/* libc includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#define xisdigit(c)     (((c) >= '0' && (c) <= '9') || (c) == '-')

#define ESCAPE_FIX
#define MAXORDERS 256
#define MAXPERSISTENT 128

/* exported symbols symbols */
const char * xmlfile = "eressea.xml";
const char * g_datadir;
int firstx = 0, firsty = 0;
int enc_gamedata = 0;

/* local symbols */
static region * current_region;

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
  setvbuf(F, 0, _IOFBF, 32 * 1024);  /* 32 kb buffer size */
  return F;
}

int
freadstr(FILE * F, int encoding, char * start, size_t size)
{
  char * str = start;
  boolean quote = false;
  for (;;) {
    int c = fgetc(F);

    if (isspace(c)) {
      if (str==start) {
        continue;
      }
      if (!quote) {
        *str = 0;
        return (int)(str-start);
      }
    }
    switch (c) {
      case EOF:
        return EOF;
      case '"':
        if (!quote && str!=start) {
          log_error(("datafile contains a \" that isn't at the start of a string.\n"));
          assert(!"datafile contains a \" that isn't at the start of a string.\n");
        }
        if (quote) {
          *str = 0;
          return (int)(str-start);
        }
        quote = true;
        break;
      case '\\':
        c = fgetc(F);
        switch (c) {
      case EOF:
        return EOF;
      case 'n':
        if ((size_t)(str-start+1)<size) {
          *str++ = '\n';
        }
        break;
      default:
        if ((size_t)(str-start+1)<size) {
          if (encoding == XML_CHAR_ENCODING_8859_1 && c&0x80) {
            char inbuf = (char)c;
            size_t inbytes = 1;
            size_t outbytes = size-(str-start);
            int ret = unicode_latin1_to_utf8(str, &outbytes, &inbuf, &inbytes);
            if (ret>0) str+=ret;
            else {
              log_error(("input data was not iso-8859-1! assuming utf-8\n"));
              encoding = XML_CHAR_ENCODING_ERROR;
              *str++ = (char)c;
            }
          } else {
            *str++ = (char)c;
          }
        }
        }
        break;
      default:
        if ((size_t)(str-start+1)<size) {
          if (encoding == XML_CHAR_ENCODING_8859_1 && c&0x80) {
            char inbuf = (char)c;
            size_t inbytes = 1;
            size_t outbytes = size-(str-start);
            int ret = unicode_latin1_to_utf8(str, &outbytes, &inbuf, &inbytes);
            if (ret>0) str+=ret;
            else {
              log_error(("input data was not iso-8859-1! assuming utf-8\n"));
              encoding = XML_CHAR_ENCODING_ERROR;
              *str++ = (char)c;
            }
          } else {
            *str++ = (char)c;
          }
        }
    }
  }
}


/** writes a quoted string to the file
* no trailing space, since this is used to make the creport.
*/
int
fwritestr(FILE * F, const char * str)
{
  int nwrite = 0;
  fputc('\"', F);
  if (str) while (*str) {
    int c = (int)(unsigned char)*str++;
    switch (c) {
      case '"':
      case '\\':
        fputc('\\', F);
        fputc(c, F);
        nwrite+=2;
        break;
      case '\n':
        fputc('\\', F);
        fputc('n', F);
        nwrite+=2;
        break;
      default:
        fputc(c, F);
        ++nwrite;
    }
  }
  fputc('\"', F);
  return nwrite + 2;
}

#if 0
#define store->r_id(store) ri36(F)

static void
rds(struct storage * store, char **ds)
{
  static char buffer[DISPLAYSIZE + 1]; /*Platz für null-char nicht vergessen!*/
  char *s = &buffer[0];
  int c = getc(F);
  while (c != '"') {
    if (c == EOF) {
      *s = 0;
      fprintf(stderr, "Die Datei bricht vorzeitig ab.\n");
      abort();
    }
    c = getc(F);
  }

  c = getc(F);

  while (c != '"') {
    if (c == EOF) {
      *s = 0;
      fprintf(stderr, "Die Datei bricht vorzeitig ab.\n");
      abort();
    }
    if (s - buffer < DISPLAYSIZE) {
      *s++ = (char)c;
    }
    c = getc(F);
  }

  c = getc(F);
  *s = 0;
  if (ds) {
    *ds = realloc(*ds, sizeof(char) * (strlen(buffer) + 1));
    strcpy(*ds, buffer);
  }
}


static void
xrds(struct storage * store, char **ds, int encoding)
{
  static char buffer[DISPLAYSIZE + 1]; /*Platz für null-char nicht vergessen!*/
  int len = store->r_str_buf(store, buffer, sizeof(buffer));

  if (len>=0) {
    if (ds) {
      *ds = realloc(*ds, sizeof(char) * (len + 1));
      strcpy(*ds, buffer);
    }
  }
}

#define rcf(F) (getc(F));
void
rsf(struct storage * store, char *s, size_t len)
{
  char * begin = s;
  int c;
  do {
    c = getc(F);;
    if (c == EOF) {
      puts("Die Datei bricht vorzeitig ab.");
      abort();
    }
  } while (c != '"');

  for (;;) {
    c = getc(F);
    if (c == '"') break;
    else if (c == EOF) {
      puts("Die Datei bricht vorzeitig ab.");
      abort();
    }
    if (s-begin<(int)len-1)
      *s++ = (char)c;
  }
  *s = 0;
}

static void
rs(struct storage * store, char *s)
{
  boolean apos = false;
  int c = getc(F);
  while (isspace(c)) c = getc(F);
  if (c=='"') {
    apos = true;
    c = getc(F);
  }
  for (;;) {
    if (c=='"') {
      c = getc(F);
      break;
    } else if (!apos && isspace(c)) break;
    *s++ = (char)c;
    c = getc(F);
  }
  *s = 0;
}

#define store->r_str_buf(store, buf, size) store->r_tok_buf(store, buf) /* should check size but doesn't */

static int
ri(struct storage * store)
{
  int i = 0, vz = 1;

  int c = getc(F);
  while (!xisdigit(c)) {
    if (c == EOF) {
      puts("Die Datei bricht vorzeitig ab.");
      abort();
    }
    c = getc(F);
  }

  while (xisdigit(c)) {
    if (c == '-')
      vz = -1;
    else
      i = 10 * i + (c - '0');
    c = getc(F);
  }

  return i * vz;
}

static int
ri36(struct storage * store)
{
  char buf[16];
  int i = 0;
  int c = getc(F);
  while (!isalnum(c)) c = getc(F);
  while (isalnum(c)) {
    if (i+1<sizeof(buf)) {
      buf[i++]=(char)c;
    }
    c = getc(F);
  }
  buf[i]=0;
  i = atoi36(buf);
  return i;
}
#endif

#ifdef ENEMIES
static void
read_enemies(struct storage * store, faction * f)
{
  if (store->version<ENEMIES_VERSION) return;
  f->enemies = NULL;
  for (;;) {
    int fno = store->r_id(store);
    if (fno<0) break;
    else {
      faction_list * flist = malloc(sizeof(faction_list));
      flist->next = f->enemies;
      f->enemies = flist;
      ur_add((void*)fno, (void**)&flist->data, resolve_faction);
    }
  }
}

static void
write_enemies(struct storage * store, const faction_list * flist)
{
  while (flist) {
    write_faction_reference(flist->data, store);
  }
  store->w_id(store, -1);
}
#endif

static unit *
unitorders(FILE * F, int enc, struct faction * f)
{
  int i;
  unit *u;

  if (!f) return NULL;

  i = getid();
  u = findunitg(i, NULL);

  if (u && u->race == new_race[RC_SPELL]) return NULL;
  if (u && u->faction == f) {
    order ** ordp;

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

      s = getbuf(F, enc);
      if (s==NULL) break;

      if (s[0]) {
        const char * stok = s;
        stok = parse_token(&stok);

        if (stok) {
          boolean quit = false;
          param_t param = findparam(stok, u->faction->locale);
          switch (param) {
            case P_UNIT:
            case P_REGION:
              quit = true;
              break;
            case P_FACTION:
            case P_NEXT:
            case P_GAMENAME:
              /* these terminate the orders, so we apply extra checking */
              if (strlen(stok)>=3) {
                quit = true;
                break;
              } else {
                quit = false;
              }
          }
          if (quit) break;
        }
        /* Nun wird der Befehl erzeut und eingehängt */
        *ordp = parse_order(s, u->faction->locale);
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
  faction * f = NULL;
  int fid = getid();

  f = findfaction(fid);
  
  if (f!=NULL && f->no!=MONSTER_FACTION) {
    const char * pass = getstrtoken();

    if (!checkpasswd(f, (const char *)pass, true)) {
      log_warning(("Invalid password for faction %s\n", itoa36(fid)));
      ADDMSG(&f->msgs, msg_message("wrongpasswd", "faction password",
                                   f->no, pass));
      return 0;
    }
    /* Die Partei hat sich zumindest gemeldet, so daß sie noch
     * nicht als untätig gilt */
    
    /* TODO: +1 ist ein Workaround, weil turn erst in process_orders
     * incrementiert wird. */
    f->lastorders = global.data_turn+1;
    
  } else {
    log_warning(("orders for invalid faction %s\n", itoa36(fid)));
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
  const char *b;
  int nfactions=0;
  struct faction *f = NULL;

  if (filename) F = cfopen(filename, "rb");
  if (F==NULL) return 0;

  puts(" - lese Befehlsdatei...\n");

  /* TODO: recognize UTF8 BOM */
  b = getbuf(F, enc_gamedata);

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

      b = getbuf(F, enc_gamedata);
      break;
    case P_GAMENAME:
    case P_FACTION:
      f = factionorders();
      if (f) {
        ++nfactions;
      }

      b = getbuf(F, enc_gamedata);
      break;

      /* in factionorders wird nur eine zeile gelesen:
       * diejenige mit dem passwort. Die befehle der units
       * werden geloescht, und die Partei wird als aktiv
       * vermerkt. */

    case P_UNIT:
      if (!f || !unitorders(F, enc_gamedata, f)) do {
        b = getbuf(F, enc_gamedata);
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
      b = getbuf(F, enc_gamedata);
      break;

    default:
      b = getbuf(F, enc_gamedata);
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
#ifdef HAVE_LINK
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
read_items(struct storage * store, item **ilist)
{
  for (;;) {
    char ibuf[32];
    const item_type * itype;
    store->r_str_buf(store, ibuf, sizeof(ibuf));
    if (!strcmp("end", ibuf)) break;
    itype = it_find(ibuf);
    assert(itype!=NULL);
    if (itype!=NULL) {
      int i = store->r_int(store);
      if (i<=0) {
        log_error(("data contains an entry with %d %s\n", i, itype->rtype->_name[1]));
      } else {
        i_change(ilist, itype, i);
      }
    }
  }
}

static void
read_alliances(struct storage * store)
{
  char pbuf[32];

  if (store->version<SAVEALLIANCE_VERSION) {
    if (!AllianceRestricted() && !AllianceAuto()) return;
  }

  store->r_str_buf(store, pbuf, sizeof(pbuf));
  while (strcmp(pbuf, "end")!=0) {
    char aname[128];
    store->r_str_buf(store, aname, sizeof(aname));
    makealliance(atoi36(pbuf), aname);
    store->r_str_buf(store, pbuf, sizeof(pbuf));
  }
}

#if 0
#define wc(F, c) putc(c, F);
#define wnl(F) putc('\n', F);
#define whs(F, s) fputs(s, F); putc(' ', F)

size_t
wsn(struct storage * store, const char *s)
{
  size_t n = 0;
  if (!s)
    return 0;
  while (*s) {
    wc(F, *s++);
    ++n;
  }
  return n;
}

size_t
ws(struct storage * store, const char *s)
{
  size_t n;
  fputc('"', F);
  n = wsn(F, s);
  fputs("\" ", F);
  return n+3;
}

static int
wi(struct storage * store, int n)
{
  return fprintf(F, "%d ", n);
}

static int
wi36(struct storage * store, int n)
{
  return fprintf(F, "%s ", itoa36(n));
}
#endif

void
write_alliances(struct storage * store)
{
  alliance * al = alliances;
  while (al) {
    store->w_id(store, al->id);
    store->w_tok(store, al->name);
    al = al->next;
    store->w_brk(store);
  }
  store->w_tok(store, "end");
  store->w_brk(store);
}

void
write_items(struct storage * store, item *ilist)
{
  item * itm;
  for (itm=ilist;itm;itm=itm->next) {
    assert(itm->number>=0);
    if (itm->number) {
      store->w_tok(store, resourcename(itm->type->rtype, 0));
      store->w_int(store, itm->number);
    }
  }
  store->w_tok(store, "end");
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

void
fwriteorder(FILE * F, const struct order * ord, const struct locale * lang)
{
  char obuf[1024];
  write_order(ord, lang, obuf, sizeof(obuf));
  if (obuf[0]) fputs(obuf, F);
}

static void
writeorder(struct storage * store, const struct order * ord, const struct locale * lang)
{
  char obuf[1024];
  write_order(ord, lang, obuf, sizeof(obuf));
  if (obuf[0]) store->w_str(store, obuf);
}

unit *
readunit(struct storage * store)
{
  skill_t sk;
  unit * u;
  int number, n, p;
  order ** orderp;
  char obuf[1024];

  n = store->r_id(store);
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
    int n = store->r_id(store);
    faction * f = findfaction(n);
    if (f!=u->faction) u_setfaction(u, f);
  }
  u->name = store->r_str(store);
  if (lomem) {
    store->r_str_buf(store, NULL, 0);
  } else {
    u->display = store->r_str(store);
  }
  number = store->r_int(store);
  u->age = (short)store->r_int(store);
  if (store->version<NEWRACE_VERSION) {
    u->race = new_race[(race_t)store->r_int(store)];
    u->irace = new_race[(race_t)store->r_int(store)];
  } else {
    char rname[32];
    
    if (store->version<STORAGE_VERSION) {
      char * space;
      store->r_str_buf(store, rname, sizeof(rname));
      space = strchr(rname, ' ');
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
    } else {
      store->r_tok_buf(store, rname, sizeof(rname));
    }
    u->race = rc_find(rname);
    assert(u->race);
    if (store->version<STORAGE_VERSION) {
      store->r_str_buf(store, rname, sizeof(rname));
    } else {
      store->r_tok_buf(store, rname, sizeof(rname));
    }
    if (rname[0]) u->irace = rc_find(rname);
    else u->irace = u->race;
  }
  if (u->race->describe) {
    const char * rcdisp = u->race->describe(u, u->faction->locale);
    if (u->display && rcdisp) {
      /* see if the data file contains old descriptions */
      if (strcmp(rcdisp, u->display)==0) {
        free(u->display);
        u->display = NULL;
      }
    }
  }
  if (u->faction == NULL) {
    log_error(("unit %s has faction == NULL\n", unitname(u)));
    u_setfaction(u, findfaction(MONSTER_FACTION));
    set_number(u, 0);
  }

  if (count_unit(u)) u->faction->no_units++;

  set_number(u, number);

  n = store->r_id(store);
  if (n>=0) u->building = findbuilding(n);

  n = store->r_id(store);
  if (n>=0) u->ship = findship(n);

  if (store->version <= 73) {
    if (store->r_int(store)) {
      fset(u, UFL_OWNER);
    } else {
      freset(u, UFL_OWNER);
    }
  }
  setstatus(u, store->r_int(store));
  if (store->version <= 73) {
    if (store->r_int(store)) {
      guard(u, GUARD_ALL);
    } else {
      guard(u, GUARD_NONE);
    }
  } else {
    u->flags = store->r_int(store) & ~UFL_DEBUG;
    u->flags &= UFL_SAVEMASK;
  }
  /* Persistente Befehle einlesen */
  free_orders(&u->orders);
  store->r_str_buf(store, obuf, sizeof(obuf));
  p = n = 0;
  orderp = &u->orders;
  while (obuf[0]) {
    if (!lomem) {
      order * ord = parse_order(obuf, u->faction->locale);
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
    }
    store->r_str_buf(store, obuf, sizeof(obuf));
  }
  if (store->version<NOLASTORDER_VERSION) {
    order * ord;
    store->r_str_buf(store, obuf, sizeof(obuf));
    ord = parse_order(obuf, u->faction->locale);
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
  if (store->version<NEWSKILL_VERSION) {
    /* convert old data */
    if (u->number) {
      while ((sk = (skill_t) store->r_int(store)) != NOSKILL) {
        int days = store->r_int(store) / u->number;
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
    while ((sk = (skill_t) store->r_int(store)) != NOSKILL) {
      int level = store->r_int(store);
      int weeks = store->r_int(store);
      if (level) {
        skill * sv = add_skill(u, sk);
        sv->level = sv->old = (unsigned char)level;
        sv->weeks = (unsigned char)weeks;
      }
    }
  }
  read_items(store, &u->items);
  u->hp = store->r_int(store);
  if (u->hp < u->number) {
    log_error(("Einheit %s hat %u Personen, und %u Trefferpunkte\n", itoa36(u->no),
           u->number, u->hp));
    u->hp=u->number;
  }

  a_read(store, &u->attribs);
  return u;
}

void
writeunit(struct storage * store, const unit * u)
{
  order * ord;
  int i, p = 0;
  write_unit_reference(u, store);
  write_faction_reference(u->faction, store);
  store->w_str(store, (const char *)u->name);
  store->w_str(store, u->display?(const char *)u->display:"");
  store->w_int(store, u->number);
  store->w_int(store, u->age);
  store->w_tok(store, u->race->_name[0]);
  store->w_tok(store, u->irace!=u->race?u->irace->_name[0]:"");
  write_building_reference(u->building, store);
  write_ship_reference(u->ship, store);
  store->w_int(store, u->status);
  store->w_int(store, u->flags & UFL_SAVEMASK);
  store->w_brk(store);
#ifndef LASTORDER
  for (ord = u->old_orders; ord; ord=ord->next) {
    if (++p<MAXPERSISTENT) {
      writeorder(store, ord, u->faction->locale);
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
        writeorder(store, ord, u->faction->locale);
      } else {
        log_error(("%s had %d or more persistent orders\n", unitname(u), MAXPERSISTENT));
        break;
      }
    }
  }
  /* write an empty string to terminate the list */
  store->w_str(store, "");
  store->w_brk(store);

  assert(u->race);

  for (i=0;i!=u->skill_size;++i) {
    skill * sv = u->skills+i;
    assert(sv->weeks<=sv->level*2+1);
    if (sv->level>0) {
      store->w_int(store, sv->id);
      store->w_int(store, sv->level);
      store->w_int(store, sv->weeks);
    }
  }
  store->w_int(store, -1);
  store->w_brk(store);
  write_items(store, u->items);
  store->w_brk(store);
  if (u->hp == 0) {
    log_error(("Einheit %s hat 0 Trefferpunkte\n", itoa36(u->no)));
    ((unit*)u)->hp = 1;
  }
  store->w_int(store, u->hp);
  store->w_brk(store);
  a_write(store, u->attribs);
  store->w_brk(store);
}

static region *
readregion(struct storage * store, short x, short y)
{
  region * r = findregion(x, y);
  const terrain_type * terrain;
  char token[32];
  unsigned int uid = 0;

  if (store->version>=UID_VERSION) {
    uid = store->r_int(store);
  } else {
    uid = generate_region_id();
  }

  if (r==NULL) {
    r = new_region(x, y, uid);
  } else {
    r->uid = uid;
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
  if (lomem) {
    store->r_str_buf(store, NULL, 0);
  } else {
    r->display = store->r_str(store);
  }
  
  if (store->version < TERRAIN_VERSION) {
    int ter = store->r_int(store);
    terrain = newterrain((terrain_t)ter);
  } else {
    char name[64];
    store->r_str_buf(store, name, sizeof(name));
    terrain = get_terrain(name);
    if (terrain==NULL) {
      log_error(("Unknown terrain '%s'\n", name));
      assert(!"unknown terrain");
    }
  }
  r->terrain = terrain;
  r->flags = (char) store->r_int(store);

  r->age = (unsigned short) store->r_int(store);

  if (fval(r->terrain, LAND_REGION)) {
    r->land = calloc(1, sizeof(land_region));
    r->land->name = store->r_str(store);
  }
  if (r->land) {
    int i;
    rawmaterial ** pres = &r->resources;
    if (store->version < GROWTREE_VERSION) {
      i = store->r_int(store);
      rsettrees(r, 2, i);
    } else {
      i = store->r_int(store);
      if (i<0) {
        log_error(("number of trees in %s is %d.\n", 
               regionname(r, NULL), i));
        i=0;
      }
      rsettrees(r, 0, i);
      i = store->r_int(store); 
      if (i<0) {
        log_error(("number of young trees in %s is %d.\n", 
               regionname(r, NULL), i));
        i=0;
      }
      rsettrees(r, 1, i);
      i = store->r_int(store); 
      if (i<0) {
        log_error(("number of seeds in %s is %d.\n", 
               regionname(r, NULL), i));
        i=0;
      }
      rsettrees(r, 2, i);
    }
    i = store->r_int(store); rsethorses(r, i);
    assert(*pres==NULL);
    for (;;) {
      rawmaterial * res;
      store->r_str_buf(store, token, sizeof(token));
      if (strcmp(token, "end")==0) break;
      res = malloc(sizeof(rawmaterial));
      res->type = rmt_find(token);
      if (res->type==NULL) {
        log_error(("invalid resourcetype %s in data.\n", token));
      }
      assert(res->type!=NULL);
      res->level = store->r_int(store);
      res->amount = store->r_int(store);
      res->flags = 0;

      if(store->version >= RANDOMIZED_RESOURCES_VERSION) {
        res->startlevel = store->r_int(store);
        res->base = store->r_int(store);
        res->divisor = store->r_int(store);
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
    *pres = NULL;

    store->r_str_buf(store, token, sizeof(token));
    if (strcmp(token, "noherb") != 0) {
      const resource_type * rtype = rt_find(token);
      assert(rtype && rtype->itype && fval(rtype->itype, ITF_HERB));
      rsetherbtype(r, rtype->itype);
    } else {
      rsetherbtype(r, NULL);
    }
    rsetherbs(r, (short)store->r_int(store));
    rsetpeasants(r, store->r_int(store));
    rsetmoney(r, store->r_int(store));
  }

  assert(r->terrain!=NULL);
  assert(rhorses(r) >= 0);
  assert(rpeasants(r) >= 0);
  assert(rmoney(r) >= 0);

  if (r->land) {
    for (;;) {
      const struct item_type * itype;
      store->r_str_buf(store, token, sizeof(token));
      if (!strcmp(token, "end")) break;
      itype = it_find(token);
      assert(itype->rtype->ltype);
      r_setdemand(r, itype->rtype->ltype, store->r_int(store));
    }
    if (store->version>=REGIONITEMS_VERSION) {
      read_items(store, &r->land->items);
    }
  }
  a_read(store, &r->attribs);

  return r;
}

void
writeregion(struct storage * store, const region * r)
{
  store->w_int(store, r->uid);
  store->w_str(store, r->display?(const char *)r->display:"");
  store->w_tok(store, r->terrain->_name);
  store->w_int(store, r->flags & RF_SAVEMASK);
  store->w_int(store, r->age);
  store->w_brk(store);
  if (fval(r->terrain, LAND_REGION)) {
    const item_type *rht;
    struct demand * demand;
    rawmaterial * res = r->resources;
    store->w_str(store, (const char *)r->land->name);
    assert(rtrees(r,0)>=0);
    assert(rtrees(r,1)>=0);
    assert(rtrees(r,2)>=0);
    store->w_int(store, rtrees(r,0));
    store->w_int(store, rtrees(r,1));
    store->w_int(store, rtrees(r,2));
    store->w_int(store, rhorses(r));

    while (res) {
      store->w_tok(store, res->type->name);
      store->w_int(store, res->level);
      store->w_int(store, res->amount);
      store->w_int(store, res->startlevel);
      store->w_int(store, res->base);
      store->w_int(store, res->divisor);
      res = res->next;
    }
    store->w_tok(store, "end");

    rht = rherbtype(r);
    if (rht) {
      store->w_tok(store, resourcename(rht->rtype, 0));
    } else {
      store->w_tok(store, "noherb");
    }
    store->w_int(store, rherbs(r));
    store->w_int(store, rpeasants(r));
    store->w_int(store, rmoney(r));
    if (r->land) for (demand=r->land->demands; demand; demand=demand->next) {
      store->w_tok(store, resourcename(demand->type->itype->rtype, 0));
      store->w_int(store, demand->value);
    }
    store->w_tok(store, "end");
#if RELEASE_VERSION>=REGIONITEMS_VERSION
    write_items(store, r->land->items);
    store->w_brk(store);
#endif
  }
  a_write(store, r->attribs);
  store->w_brk(store);
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
readfaction(struct storage * store)
{
  ally **sfp;
  int planes;
  int i = store->r_id(store);
  faction * f = findfaction(i);
  char email[128];
  char token[32];

  if (f==NULL) {
    f = (faction *) calloc(1, sizeof(faction));
    f->no = i;
  } else {
    f->allies = NULL; /* mem leak */
    while (f->attribs) a_remove(&f->attribs, f->attribs);
  }
  f->subscription = store->r_int(store);

  if (alliances!=NULL) {
    int allianceid = store->r_id(store);
    if (allianceid>=0) f->alliance = findalliance(allianceid);
    if (f->alliance) {
      faction_list * flist = malloc(sizeof(faction_list));
      flist->data = f;
      flist->next = f->alliance->members;
      f->alliance->members = flist;
    }
  }

  f->name = store->r_str(store);
  f->banner = store->r_str(store);

  log_info((3, "   - Lese Partei %s (%s)\n", f->name, factionid(f)));

  store->r_str_buf(store, email, sizeof(email));
  if (set_email(&f->email, email)!=0) {
    log_warning(("Invalid email address for faction %s: %s\n", itoa36(f->no), email));
    set_email(&f->email, "");
  }

  f->passw = store->r_str(store);
  if (store->version >= OVERRIDE_VERSION) {
    f->override = store->r_str(store);
  } else {
    f->override = strdup(itoa36(rng_int()));
  }

  store->r_str_buf(store, token, sizeof(token));
  f->locale = find_locale(token);
  f->lastorders = store->r_int(store);
  f->age = store->r_int(store);
  if (store->version < NEWRACE_VERSION) {
    race_t rc = (char) store->r_int(store);
    f->race = new_race[rc];
  } else {
    store->r_str_buf(store, token, sizeof(token));
    f->race = rc_find(token);
    assert(f->race);
  }
  f->magiegebiet = (magic_t)store->r_int(store);

#if KARMA_MODULE
  f->karma = store->r_int(store);
#else
  /* ignore karma */
  store->r_int(store);
#endif /* KARMA_MODULE */

  f->flags = store->r_int(store);

  a_read(store, &f->attribs);
  if (store->version>=CLAIM_VERSION) {
    read_items(store, &f->items);
  }
  for (;;) {
    int level;
    store->r_tok_buf(store, token, sizeof(token));
    if (strcmp("end", token)==0) break;
    level = store->r_int(store);
  } 
  planes = store->r_int(store);
  while(--planes >= 0) {
    int id = store->r_int(store);
    short ux = (short)store->r_int(store);
    short uy = (short)store->r_int(store);
    set_ursprung(f, id, ux, uy);
  }
  f->newbies = 0;
  
  i = f->options = store->r_int(store);

  if ((i & (want(O_REPORT)|want(O_COMPUTER)))==0 && f->no!=MONSTER_FACTION) {
    /* Kein Report eingestellt, Fehler */
    f->options = f->options | want(O_REPORT) | want(O_ZUGVORLAGE);
  }

  sfp = &f->allies;
  if (store->version<ALLIANCES_VERSION) {
    int p = store->r_int(store);
    while (--p >= 0) {
      int aid = store->r_id(store);
      int state = store->r_int(store);
      sfp = addally(f, sfp, aid, state);
    }
  } else {
    for (;;) {
      int aid = 0;
      if (store->version<STORAGE_VERSION) {
        store->r_tok_buf(store, token, sizeof(token));
        if (strcmp(token, "end")!=0) {
          aid = atoi36(token);
        }
      } else {
        aid = store->r_id(store);
      }
      if (aid>0) {
        int state = store->r_int(store);
        sfp = addally(f, sfp, aid, state);
      } else {
        break;
      }
    }
  }
  read_groups(store, f);
#ifdef ENEMIES
  read_enemies(store, f);
#endif
  return f;
}

void
writefaction(struct storage * store, const faction * f)
{
  ally *sf;
  ursprung *ur;

  write_faction_reference(f, store);
  store->w_int(store, f->subscription);
  if (alliances!=NULL) {
    if (f->alliance) store->w_id(store, f->alliance->id);
    else store->w_id(store, -1);
  }

  store->w_str(store, (const char *)f->name);
  store->w_str(store, (const char *)f->banner);
  store->w_str(store, f->email);
  store->w_tok(store, (const char *)f->passw);
  store->w_tok(store, (const char *)f->override);
  store->w_tok(store, locale_name(f->locale));
  store->w_int(store, f->lastorders);
  store->w_int(store, f->age);
  store->w_tok(store, f->race->_name[0]);
  store->w_brk(store);
  store->w_int(store, f->magiegebiet);
#if KARMA_MODULE
  store->w_int(store, f->karma);
#else
  store->w_int(store, 0);
#endif /* KARMA_MODULE */

  store->w_int(store, f->flags&FFL_SAVEMASK);
  a_write(store, f->attribs);
  store->w_brk(store);
#if RELEASE_VERSION>=CLAIM_VERSION
  write_items(store, f->items);
  store->w_brk(store);
#endif
  store->w_tok(store, "end");
  store->w_brk(store);
  store->w_int(store, listlen(f->ursprung));
  for(ur = f->ursprung;ur;ur=ur->next) {
    store->w_int(store, ur->id);
    store->w_int(store, ur->x);
    store->w_int(store, ur->y);
  }
  store->w_brk(store);
  store->w_int(store, f->options & ~want(O_DEBUG));
  store->w_brk(store);

  for (sf = f->allies; sf; sf = sf->next) {
    int no = (sf->faction!=NULL)?sf->faction->no:0;
    int status = alliedfaction(NULL, f, sf->faction, HELP_ALL);
    if (status!=0) {
      store->w_id(store, no);
      store->w_int(store, sf->status);
    }
  }
  store->w_id(store, -1);
  store->w_brk(store);
  write_groups(store, f->groups);
#ifdef ENEMIES
  write_enemies(store, f->enemies);
#endif
}

int
readgame(const char * filename, int mode, int backup)
{
  int i, n, p;
  faction *f, **fp;
  region *r;
  building *b, **bp;
  ship **shp;
  unit *u;
  int rmax = maxregions;
  char path[MAX_PATH];
  char token[32];
  const struct building_type * bt_lighthouse = bt_find("lighthouse");
  storage my_store = (mode==IO_BINARY)?binary_store:text_store;
  storage * store = &my_store;

  sprintf(path, "%s/%s", datapath(), filename);
  log_printf("- reading game data from %s\n", filename);
  if (backup) create_backup(path);

  store->encoding = enc_gamedata;
  if (store->open(store, path, IO_READ)!=0) {
    return -1;
  }
  enc_gamedata = store->encoding;

  assert(store->version>=MIN_VERSION || !"unsupported data format");
  assert(store->version<=RELEASE_VERSION || !"unsupported data format");
  assert(store->version >= GROWTREE_VERSION);
  if(store->version >= SAVEXMLNAME_VERSION) {
    char basefile[1024];
    const char *basearg = "(null)";

    store->r_str_buf(store, basefile, sizeof(basefile));
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
  a_read(store, &global.attribs);
  global.data_turn = turn = store->r_int(store);
  store->r_int(store); /* max_unique_id = */ 
  nextborder = store->r_int(store);

  /* Planes */
  planes = NULL;
  n = store->r_int(store);
  while(--n >= 0) {
    int id = store->r_int(store);
    plane *pl = getplanebyid(id);
    if (pl==NULL) {
      calloc(1, sizeof(plane));
    } else {
      log_warning(("the plane with id=%d already exists.\n", id));
    }
    pl->id = id;
    pl->name = store->r_str(store);
    pl->minx = (short)store->r_int(store);
    pl->maxx = (short)store->r_int(store);
    pl->miny = (short)store->r_int(store);
    pl->maxy = (short)store->r_int(store);
    pl->flags = store->r_int(store);
    if (store->version>WATCHERS_VERSION) {
      store->r_str_buf(store, token, sizeof(token));
      while (strcmp(token, "end")!=0) {
        watcher * w = calloc(sizeof(watcher),1);
        variant fno;
        fno.i = atoi36(token);
        w->mode = (unsigned char)store->r_int(store);
        w->next = pl->watchers;
        pl->watchers = w;
        ur_add(fno, (void**)&w->faction, resolve_faction);
        store->r_str_buf(store, token, sizeof(token));
      }
    }
    a_read(store, &pl->attribs);
    addlist(&planes, pl);
  }

  /* Read factions */
  if (store->version>=ALLIANCES_VERSION) {
    read_alliances(store);
  }
  n = store->r_int(store);
  log_info((1, " - Einzulesende Parteien: %d\n", n));
  fp = &factions;
  while (*fp) fp=&(*fp)->next;

  while (--n >= 0) {
    faction * f = readfaction(store);

    *fp = f;
    fp = &f->next;
    fhash(f);
  }
  *fp = 0;

  /* Benutzte Faction-Ids */
  if (store->version<STORAGE_VERSION) {
    i = store->r_int(store);
    while (i--) {
      store->r_int(store); /* used faction ids. ignore. */
    }
  }

  /* Regionen */

  n = store->r_int(store);
  assert(n<MAXREGIONS);
  if (rmax<0) rmax = n;
  log_info((1, " - Einzulesende Regionen: %d/%d\r", rmax, n));
  while (--n >= 0) {
    unit **up;
    short x = (short)store->r_int(store);
    short y = (short)store->r_int(store);

    if ((n & 0x3FF) == 0) {  /* das spart extrem Zeit */
      log_info((2, " - Einzulesende Regionen: %d/%d * %d,%d    \r", rmax, n, x, y));
    }
    --rmax;

    r = readregion(store, x, y);

    /* Burgen */
    p = store->r_int(store);
    bp = &r->buildings;

    while (--p >= 0) {

      b = (building *) calloc(1, sizeof(building));
      b->no = store->r_id(store);
      *bp = b;
      bp = &b->next;
      bhash(b);
      b->name = store->r_str(store);
      if (lomem) {
        store->r_str_buf(store, NULL, 0);
      }
      else {
        b->display = store->r_str(store);
      }
      b->size = store->r_int(store);
      store->r_str_buf(store, token, sizeof(token));
      b->type = bt_find(token);
      b->region = r;
      a_read(store, &b->attribs);
      if (b->type==bt_lighthouse) {
        r->flags |= RF_LIGHTHOUSE;
      }
    }
    /* Schiffe */

    p = store->r_int(store);
    shp = &r->ships;

    while (--p >= 0) {
      ship * sh = (ship *) calloc(1, sizeof(ship));
      sh->region = r;
      sh->no = store->r_id(store);
      *shp = sh;
      shp = &sh->next;
      shash(sh);
      sh->name = store->r_str(store);
      if (lomem) {
        store->r_str_buf(store, NULL, 0);
      }
      else {
        sh->display = store->r_str(store);
      }
      store->r_str_buf(store, token, sizeof(token));
      sh->type = st_find(token);
      if (sh->type==NULL) {
        /* old datafiles */
        sh->type = st_find((const char *)locale_string(default_locale, token));
      }
      assert(sh->type || !"ship_type not registered!");
      sh->size = store->r_int(store);
      sh->damage = store->r_int(store);

      /* Attribute rekursiv einlesen */

      sh->coast = (direction_t)store->r_int(store);
      a_read(store, &sh->attribs);
    }

    *shp = 0;

    /* Einheiten */

    p = store->r_int(store);
    up = &r->units;

    while (--p >= 0) {
      unit * u = readunit(store);
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
  log_info((1, "\n"));
  read_borders(store);

  store->close(store);

  /* Unaufgeloeste Zeiger initialisieren */
  log_info((1, "\n - Referenzen initialisieren...\n"));
  resolve();

  for (r=regions;r;r=r->next) {
    if (r->flags & RF_LIGHTHOUSE) {
      building * b;
      for (b=r->buildings;b;b=b->next) update_lighthouse(b);
    }
  }
  log_info((1, " - Regionen initialisieren & verbinden...\n"));
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
writegame(const char *filename, int mode)
{
  char *base;
  int n;
  faction *f;
  region *r;
  building *b;
  ship *sh;
  unit *u;
  plane *pl;
  char path[MAX_PATH];
  storage my_store = (mode==IO_BINARY)?binary_store:text_store;
  storage * store = &my_store;

  sprintf(path, "%s/%s", datapath(), filename);
#ifdef HAVE_UNISTD_H
  if (access(path, R_OK) == 0) {
    /* make sure we don't overwrite some hardlinkedfile */
    unlink(path);
  }
#endif

  store->encoding = enc_gamedata;
  if (store->open(store, path, IO_WRITE)!=0) {
    return -1;
  }

  /* globale Variablen */

  base = strrchr(xmlfile, '/');
  if (base) {
    store->w_str(store, base+1);
  } else {
    store->w_str(store, xmlfile);
  }
  store->w_brk(store);

  a_write(store, global.attribs);
  store->w_brk(store);

  store->w_int(store, turn);
  store->w_int(store, 0/*max_unique_id*/);
  store->w_int(store, nextborder);

  /* Write planes */
  store->w_brk(store);
  store->w_int(store, listlen(planes));
  store->w_brk(store);

  for(pl = planes; pl; pl=pl->next) {
    watcher * w;
    store->w_int(store, pl->id);
    store->w_str(store, pl->name);
    store->w_int(store, pl->minx);
    store->w_int(store, pl->maxx);
    store->w_int(store, pl->miny);
    store->w_int(store, pl->maxy);
    store->w_int(store, pl->flags);
    w = pl->watchers;
    while (w) {
      if (w->faction) {
        write_faction_reference(w->faction, store);
        store->w_int(store, w->mode);
      }
      w = w->next;
    }
    store->w_tok(store, "end");
    a_write(store, pl->attribs);
    store->w_brk(store);
  }


  /* Write factions */
#if RELEASE_VERSION>=ALLIANCES_VERSION
  write_alliances(store);
#endif
  n = listlen(factions);
  store->w_int(store, n);
  store->w_brk(store);

  log_info((1, " - Schreibe %d Parteien...\n", n));
  for (f = factions; f; f = f->next) {
    writefaction(store, f);
    store->w_brk(store);
  }

  /* Write regions */

  n=listlen(regions);
  store->w_int(store, n);
  store->w_brk(store);
  log_info((1, " - Schreibe Regionen: %d  \r", n));

  for (r = regions; r; r = r->next, --n) {
    /* plus leerzeile */
    if ((n%1024)==0) {  /* das spart extrem Zeit */
      log_info((2, " - Schreibe Regionen: %d  \r", n));
      fflush(stdout);
    }
    store->w_brk(store);
    store->w_int(store, r->x);
    store->w_int(store, r->y);
    writeregion(store, r);

    store->w_int(store, listlen(r->buildings));
    store->w_brk(store);
    for (b = r->buildings; b; b = b->next) {
      write_building_reference(b, store);
      store->w_str(store, b->name);
      store->w_str(store, b->display?b->display:"");
      store->w_int(store, b->size);
      store->w_tok(store, b->type->_name);
      store->w_brk(store);
      a_write(store, b->attribs);
      store->w_brk(store);
    }

    store->w_int(store, listlen(r->ships));
    store->w_brk(store);
    for (sh = r->ships; sh; sh = sh->next) {
      assert(sh->region == r);
      write_ship_reference(sh, store);
      store->w_str(store, (const char *)sh->name);
      store->w_str(store, sh->display?(const char *)sh->display:"");
      store->w_tok(store, sh->type->name[0]);
      store->w_int(store, sh->size);
      store->w_int(store, sh->damage);
      store->w_int(store, sh->coast);
      store->w_brk(store);
      a_write(store, sh->attribs);
      store->w_brk(store);
    }

    store->w_int(store, listlen(r->units));
    store->w_brk(store);
    for (u = r->units; u; u = u->next) {
      writeunit(store, u);
    }
  }
  store->w_brk(store);
  write_borders(store);
  store->w_brk(store);
  
  store->close(store);

  log_info((1, "\nOk.\n"));
  return 0;
}

int
a_readint(attrib * a, struct storage * store)
{
  /*  assert(sizeof(int)==sizeof(a->data)); */
  a->data.i = store->r_int(store);
  return AT_READ_OK;
}

void
a_writeint(const attrib * a, struct storage * store)
{
  store->w_int(store, a->data.i);
}

int
a_readshorts(attrib * a, struct storage * store)
{
  if (store->version<ATTRIBREAD_VERSION) {
    return a_readint(a, store);
  }
  a->data.sa[0] = (short)store->r_int(store);
  a->data.sa[1] = (short)store->r_int(store);
  return AT_READ_OK;
}

void
a_writeshorts(const attrib * a, struct storage * store)
{
  store->w_int(store, a->data.sa[0]);
  store->w_int(store, a->data.sa[1]);
}

int
a_readchars(attrib * a, struct storage * store)
{
  int i;
  if (store->version<ATTRIBREAD_VERSION) {
    return a_readint(a, store);
  }
  for (i=0;i!=4;++i) {
    a->data.ca[i] = (char)store->r_int(store);
  }
  return AT_READ_OK;
}

void
a_writechars(const attrib * a, struct storage * store)
{
  int i;
  
  for (i=0;i!=4;++i) {
    store->w_int(store, a->data.ca[i]);
  }
}

int
a_readvoid(attrib * a, struct storage * store)
{
  if (store->version<ATTRIBREAD_VERSION) {
    return a_readint(a, store);
  }
  return AT_READ_OK;
}

void
a_writevoid(const attrib * a, struct storage * store)
{
}

int
a_readstring(attrib * a, struct storage * store)
{
  a->data.v = store->r_str(store);
  return AT_READ_OK;
}

void
a_writestring(const attrib * a, struct storage * store)
{
  assert(a->data.v);
  store->w_str(store, (const char *)a->data.v);
}

void
a_finalizestring(attrib * a)
{
  free(a->data.v);
}
