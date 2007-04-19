/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#define INDENT 0

#include <config.h>
#include <eressea.h>

/* modules includes */
#include <modules/score.h>

/* attributes includes */
#include <attributes/overrideroads.h>
#include <attributes/viewrange.h>
#include <attributes/otherfaction.h>
#include <attributes/alliance.h>
#ifdef AT_OPTION
# include <attributes/option.h>
#endif

/* gamecode includes */
#include "creport.h"
#include "economy.h"
#include "monster.h"
#include "laws.h"

/* kernel includes */
#include <kernel/alchemy.h>
#include <kernel/alliance.h>
#include <kernel/border.h>
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/message.h>
#include <kernel/movement.h>
#include <kernel/objtypes.h>
#include <kernel/order.h>
#include <kernel/plane.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/render.h>
#include <kernel/reports.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/skill.h>
#include <kernel/teleport.h>
#include <kernel/terrain.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>
#include <kernel/alliance.h>

/* util includes */
#include <util/bsdstring.h>
#include <util/goodies.h>
#include <util/base36.h>
#include <util/nrmessage.h>
#include <util/translation.h>
#include <util/message.h>
#include <util/rng.h>
#include <util/log.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>

#ifdef HAVE_STAT
#include <sys/types.h>
#include <sys/stat.h>
#endif

extern int quiet;
extern int *storms;
extern int  weeks_per_month;
extern int  months_per_year;

boolean nocr = false;
boolean nonr = false;
boolean nosh = false;
boolean nomer = false;
boolean noreports = false;

static size_t
strxcpy(char * dst, const char * src) {
  size_t s = 0;
  while ((*dst++ = *src++)!=0) ++s;
  return s;
}

static char *
gamedate_season(const struct locale * lang)
{
  static char buf[256];
  gamedate gd;

  get_gamedate(turn, &gd);

  sprintf(buf, LOC(lang, "nr_calendar_season"),
    LOC(lang, weeknames[gd.week]),
    LOC(lang, monthnames[gd.month]),
    gd.year,
    LOC(lang, agename),
    LOC(lang, seasonnames[gd.season]));

  return buf;
}

static char *
gamedate2(const struct locale * lang)
{
  static char buf[256];
  gamedate gd;

  get_gamedate(turn, &gd);
  sprintf(buf, "in %s des Monats %s im Jahre %d %s.",
    LOC(lang, weeknames2[gd.week]),
    LOC(lang, monthnames[gd.month]),
    gd.year,
    LOC(lang, agename));
  return buf;
}

void
rpc(FILE * F, char c, size_t num)
{
  while(num > 0) {
    putc(c, F);
    num--;
  }
}

void
rnl(FILE * F)
{
  fputc('\n', F);
}

static void
centre(FILE * F, const char *s, boolean breaking)
{
  /* Bei Namen die genau 80 Zeichen lang sind, kann es hier Probleme
   * geben. Seltsamerweise wird i dann auf MAXINT oder aehnlich
   * initialisiert. Deswegen keine Strings die laenger als REPORTWIDTH
   * sind! */

  if (breaking && REPORTWIDTH < strlen(s)) {
    strlist *T, *SP = 0;
    sparagraph(&SP, s, 0, 0);
    T = SP;
    while (SP) {
      centre(F, SP->s, false);
      SP = SP->next;
    }
    freestrlist(T);
  } else {
    rpc(F, ' ', (REPORTWIDTH - strlen(s)+1) / 2);
    fputs(s, F);
    putc('\n', F);
  }
}

static void
rparagraph(FILE *F, const char *str, ptrdiff_t indent, int hanging_indent, char mark)
{
  static const char * spaces = "                                ";
  size_t length = REPORTWIDTH;
  const char * end, * begin;

  /* find out if there's a mark + indent already encoded in the string. */
  if (!mark) {
    const char * x = str;
    while (*x == ' ') ++x;
    indent += x - str;
    if (x[0] && indent && x[1]==' ') {
      indent += 2;
      mark = x[0];
      str = x + 2;
      hanging_indent -= 2;
    }
  }
  begin = end = str;

  while (*begin) {
    const char * last_space = begin;

    if (mark && indent>=2) {
      fwrite(spaces, sizeof(char), indent-2, F);
      fputc(mark, F);
      fputc(' ', F);
      mark = 0;
    } else if (begin==str) {
      fwrite(spaces, sizeof(char), indent, F);
    } else {
      fwrite(spaces, sizeof(char), indent+ hanging_indent, F);
    }
    while (*end && end<=begin+length-indent) {
      if (*end==' ') {
        last_space = end;
      }
      ++end;
    }
    if (*end==0) last_space = end;
    if (last_space==begin) {
      /* there was no space in this line. clip it */
      last_space = end;
    }
    fwrite(begin, sizeof(char), last_space-begin, F);
    begin = last_space;
    while (*begin==' ') {
      ++begin;
    }
    if (begin>end) begin = end;
    fputc('\n', F);
  }
}

static void
report_spell(FILE * F,  spell *sp, const struct locale * lang)
{
  int k, itemanz, costtyp;
  int dh = 0;
  char * bufp;

  rnl(F);
  centre(F, spell_name(sp, lang), true);
  rnl(F);
  rparagraph(F, LOC(lang, "nr_spell_description"), 0, 0, 0);
  rparagraph(F, spell_info(sp, lang), 2, 0, 0);

  bufp = buf;
  bufp += strxcpy(bufp, LOC(lang, "nr_spell_type"));
  *bufp++ = ' ';
  if (sp->sptyp & PRECOMBATSPELL) {
    bufp += strxcpy(bufp, "Präkampfzauber");
  } else if (sp->sptyp & COMBATSPELL) {
    bufp += strxcpy(bufp, "Kampfzauber");
  } else if (sp->sptyp & POSTCOMBATSPELL) {
    bufp += strxcpy(bufp, "Postkampfzauber");
  } else {
    bufp += strxcpy(bufp, "Normaler Zauber");
  }
  rparagraph(F, buf, 0, 0, 0);

  sprintf(buf, "%s %d", LOC(lang, "nr_spell_level"), sp->level);
  rparagraph(F, buf, 0, 0, 0);

  sprintf(buf, "%s %d", LOC(lang, "nr_spell_rank"), sp->rank);
  rparagraph(F, buf, 0, 0, 0);

  rparagraph(F, LOC(lang, "nr_spell_components"), 0, 0, 0);
  for (k = 0; sp->components[k].type; ++k) {
    const resource_type * rtype = sp->components[k].type;
    itemanz = sp->components[k].amount;
    costtyp = sp->components[k].cost;
    if (itemanz > 0){
      if (sp->sptyp & SPELLLEVEL) {
        bufp = buf + sprintf(buf, "  %d %s", itemanz, LOC(lang, resourcename(rtype, itemanz!=1)));
        if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR ) {
          bufp += strxcpy(bufp, " * Stufe");
        }
      } else {
        if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR ) {
          itemanz *= sp->level;
        }
        sprintf(buf, "%d %s", itemanz, LOC(lang, resourcename(rtype, itemanz!=1)));
      }
      rparagraph(F, buf, 2, 2, '-');
    }
  }

  bufp = buf + strxcpy(buf, LOC(lang, "nr_spell_modifiers"));
  if (sp->sptyp & FARCASTING) {
    bufp += strxcpy(bufp, " Fernzauber");
    dh = 1;
  }
  if (sp->sptyp & OCEANCASTABLE) {
    if (dh == 1) {
      bufp += strxcpy(bufp, ",");
    }
    bufp += strxcpy(bufp, " Seezauber");
    dh = 1;
  }
  if (sp->sptyp & ONSHIPCAST) {
    if (dh == 1){
      bufp += strxcpy(bufp, ",");
    }
    bufp += strxcpy(bufp, " Schiffszauber");
    dh = 1;
  }
  if (sp->sptyp & NOTFAMILIARCAST) {
    if (dh == 1){
      bufp += strxcpy(bufp, ", k");
    } else {
      bufp += strxcpy(bufp, " K");
    }
    bufp += strxcpy(bufp, "ann nicht vom Vertrauten gezaubert werden");
    dh = 1;
  }
  if (dh == 0) bufp += strxcpy(bufp, " Keine");
  rparagraph(F, buf, 0, 0, 0);

  rparagraph(F, LOC(lang, "nr_spell_syntax"), 0, 0, 0);
  if (!sp->syntax) {
    if (sp->sptyp & ISCOMBATSPELL) {
      bufp = buf + strxcpy(buf, LOC(lang, keywords[K_COMBAT]));
    } else {
      bufp = buf + strxcpy(buf, LOC(lang, keywords[K_CAST]));
    }
    /* Reihenfolge beachten: Erst REGION, dann STUFE! */
    if (sp->sptyp & FARCASTING) {
      bufp += snprintf(bufp, sizeof(buf)-(bufp-buf), " [%s x y]", LOC(lang, parameters[P_REGION]));
    }
    if (sp->sptyp & SPELLLEVEL) {
      bufp += snprintf(bufp, sizeof(buf)-(bufp-buf), " [%s n]", LOC(lang, parameters[P_LEVEL]));
    }
    bufp += strxcpy(bufp, " \"");
    bufp += strxcpy(bufp, spell_name(sp, lang));
    bufp += strxcpy(bufp, "\" ");
    if (sp->sptyp & ONETARGET){
      if (sp->sptyp & UNITSPELL) {
        bufp += strxcpy(bufp, "<Einheit-Nr>");
      } else if (sp->sptyp & SHIPSPELL) {
        bufp += strxcpy(bufp, "<Schiff-Nr>");
      } else if (sp->sptyp & BUILDINGSPELL) {
        bufp += strxcpy(bufp, "<Gebäude-Nr>");
      }
    } else {
      if (sp->sptyp & UNITSPELL) {
        bufp += strxcpy(bufp, "<Einheit-Nr> [<Einheit-Nr> ...]");
      } else if (sp->sptyp & SHIPSPELL) {
        bufp += strxcpy(bufp, "<Schiff-Nr> [<Schiff-Nr> ...]");
      } else if (sp->sptyp & BUILDINGSPELL) {
        bufp += strxcpy(bufp, "<Gebäude-Nr> [<Gebäude-Nr> ...]");
      }
    }
    rparagraph(F, buf, 2, 0, 0);
  } else {
    rparagraph(F, sp->syntax, 2, 0, 0);
  }
  rnl(F);
}

void
sparagraph(strlist ** SP, const char *s, int indent, char mark)
{

  /* Die Liste SP wird mit dem String s aufgefuellt, mit indent und einer
   * mark, falls angegeben. SP wurde also auf 0 gesetzt vor dem Aufruf.
   * Vgl. spunit (). */

  int i, j, width;
  int firstline;
  static char buf[REPORTWIDTH + 1];

  width = REPORTWIDTH - indent;
  firstline = 1;

  for (;;) {
    i = 0;

    do {
      j = i;
      while (s[j] && s[j] != ' ')
        j++;
      if (j > width) {

        /* j zeigt auf das ende der aktuellen zeile, i zeigt auf den anfang der
         * nächsten zeile. existiert ein wort am anfang der zeile, welches
         * länger als eine zeile ist, muß dieses hier abgetrennt werden. */

        if (i == 0)
          i = width - 1;
        break;
      }
      i = j + 1;
    }
    while (s[j]);

    for (j = 0; j != indent; j++)
      buf[j] = ' ';

    if (firstline && mark)
      buf[indent - 2] = mark;

    for (j = 0; j != i - 1; j++)
      buf[indent + j] = s[j];
    buf[indent + j] = 0;

    addstrlist(SP, buf);

    if (s[i - 1] == 0)
      break;

    s += i;
    firstline = 0;
  }
}

int
hat_in_region(item_t it, region * r, faction * f)
{
  unit *u;

  for (u = r->units; u; u = u->next) {
    if (u->faction == f && get_item(u, it) > 0) {
      return 1;
    }
  }
  return 0;
}

static void
print_curses(FILE *F, const faction *viewer, const void * obj, typ_t typ, int indent)
{
  attrib *a = NULL;
  int self = 0;
  region *r;

  /* Die Sichtbarkeit eines Zaubers und die Zaubermeldung sind bei
   * Gebäuden und Schiffen je nach, ob man Besitzer ist, verschieden.
   * Bei Einheiten sieht man Wirkungen auf eigene Einheiten immer.
   * Spezialfälle (besonderes Talent, verursachender Magier usw. werde
   * bei jedem curse gesondert behandelt. */
  if (typ == TYP_SHIP){
    ship * sh = (ship*)obj;
    unit * owner  = shipowner(sh);
    a = sh->attribs;
    r = sh->region;
    if (owner) {
      if (owner->faction == viewer){
        self = 2;
      } else { /* steht eine person der Partei auf dem Schiff? */
        unit *u = NULL;
        for (u = r->units; u; u = u->next) {
          if (u->ship == sh) {
            self = 1;
            break;
          }
        }
      }
    }
  } else if (typ == TYP_BUILDING) {
    building * b = (building*)obj;
    unit * owner;
    a = b->attribs;
    r = b->region;
    if ((owner = buildingowner(r,b)) != NULL){
      if (owner->faction == viewer){
        self = 2;
      } else { /* steht eine Person der Partei in der Burg? */
        unit *u = NULL;
        for (u = r->units; u; u = u->next) {
          if (u->building == b) {
            self = 1;
            break;
          }
        }
      }
    }
  } else if (typ == TYP_UNIT) {
    unit *u = (unit *)obj;
    a = u->attribs;
    r = u->region;
    if (u->faction == viewer){
      self = 2;
    }
  } else if (typ == TYP_REGION) {
    r = (region *)obj;
    a = r->attribs;
  } else {
    /* fehler */
  }

  for(;a;a=a->next) {

    if (fval(a->type, ATF_CURSE)) {
      curse *c = (curse *)a->data.v;
      message * msg;
      
      if (c->type->cansee) {
        self = c->type->cansee(viewer, obj, typ, c, self);
      }
      msg = msg_curse(c, obj, typ, self);

      if (msg) {
        rnl(F);
        nr_render(msg, viewer->locale, buf, sizeof(buf), viewer);
        rparagraph(F, buf, indent, 2, 0);
        msg_release(msg);
      }
    } else if (a->type==&at_effect && self) {
      effect_data * data = (effect_data *)a->data.v;
      if (data->value>0) {
        sprintf(buf, "Auf der Einheit lieg%s %d Wirkung%s %s.",
          (data->value==1 ? "t" : "en"),
          data->value,
          (data->value==1 ? "" : "en"),
          LOC(default_locale, resourcename(data->type->itype->rtype, 0)));
        rnl(F);
        rparagraph(F, buf, indent, 2, 0);
      }
    }
  }
}

static void
rps_nowrap(FILE * F, const char *s)
{
  const char *x = s;
  size_t indent = 0;

  while (*x++ == ' ');
  indent = x - s - 1;
  if (*(x - 1) && indent && *x == ' ')
    indent += 2;
  x = s;
  while (*s) {
    if (s == x) {
      x = strchr(x + 1, ' ');
      if (!x)
        x = s + strlen(s);
    }
    rpc(F, *s++, 1);
  }
}

static void
rpunit(FILE * F, const faction * f, const unit * u, int indent, int mode)
{
  attrib *a_otherfaction;
  char marker;
  int dh;
  boolean isbattle = (boolean)(mode == see_battle);

  if (u->race == new_race[RC_SPELL]) return;

  {
    rnl(F);
    dh = bufunit(f, u, indent, mode);
  }

  a_otherfaction = a_find(u->attribs, &at_otherfaction);

  if (u->faction == f) {
    marker = '*';
  } else {
    if (a_otherfaction && f != u->faction && get_otherfaction(a_otherfaction) == f
        && !fval(u, UFL_PARTEITARNUNG)) {
      marker = '!';
    } else {
      if (dh && !fval(u, UFL_PARTEITARNUNG)) {
        marker = '+';
      } else {
        marker = '-';
      }
    }
  }

  rparagraph(F, buf, indent, 0, marker);

  if (!isbattle) {
    print_curses(F, f, u, TYP_UNIT, indent);
  }
}

static void
rp_messages(FILE * F, message_list * msgs, faction * viewer, int indent, boolean categorized)
{
  nrsection * section;
  if (!msgs) return;
  for (section = sections; section; section=section->next) {
    int k = 0;
    struct mlist * m = msgs->begin;
    while (m) {
      /* messagetype * mt = m->type; */
      if (strcmp(nr_section(m->msg), section->name)==0) {
        char lbuf[8192];

        if (!k && categorized) {
          const char * section_title;
          char cat_identifier[24];

          rnl(F);
          sprintf(cat_identifier, "section_%s", section->name);
          section_title = LOC(viewer->locale, cat_identifier);
          centre(F, section_title, true);
          rnl(F);
          k = 1;
        }
        nr_render(m->msg, viewer->locale, lbuf, sizeof(lbuf), viewer);
        rparagraph(F, lbuf, indent, 2, 0);
      }
      m = m->next;
    }
  }
}

static void
rp_battles(FILE * F, faction * f)
{
  if (f->battles!=NULL) {
    struct bmsg * bm = f->battles;
    rnl(F);
    centre(F, LOC(f->locale, "section_battle"), false);
    rnl(F);

    while (bm) {
      RENDER(f, buf, 80, ("battle::header", "region", bm->r));
      rnl(F);
      centre(F, buf, true);
      rnl(F);
      rp_messages(F, bm->msgs, f, 0, false);
      bm = bm->next;
    }
  }
}

size_t
f_regionid(const region * r, const faction * f, char * buffer, size_t size)
{

  if (!r) {
    strncpy(buffer, "(Chaos)", size);
  } else {
    plane * pl = r->planep;
    strncpy(buffer, rname(r, f->locale), size);
    buffer[size-1]=0;
    if (pl==NULL || !fval(pl, PFL_NOCOORDS)) {
      sprintf(buffer+strlen(buffer), " (%d,%d%s%s)", region_x(r,f), region_y(r,f), pl?",":"", pl?pl->name:"");
    }
  }
  return strlen(buffer);
}

static char *
f_regionid_s(const region * r, const faction * f)
{
  static int i = 0;
  static char bufs[4][NAMESIZE + 20];
  char * buf = bufs[(++i)%4];

  f_regionid(r, f, buf, NAMESIZE + 20);
  return buf;
}

static void
prices(FILE * F, const region * r, const faction * f)
{
  const luxury_type *sale=NULL;
  struct demand * dmd;
  message * m;
  int n = 0;

  if (r->land==NULL || r->land->demands==NULL) return;
  for (dmd=r->land->demands;dmd;dmd=dmd->next) {
    if (dmd->value==0) sale = dmd->type;
    else if (dmd->value > 0) n++;
  }
  assert(sale!=NULL);

  m = msg_message("nr_market_sale", "product price",
    sale->itype->rtype, sale->price);
  nr_render(m, f->locale, buf, sizeof(buf), f);
  msg_release(m);

  if (n > 0) {
    char * bufp = buf + strlen(buf);
    bufp += strxcpy(bufp, " ");
    bufp += strxcpy(bufp, LOC(f->locale, "nr_trade_intro"));
    bufp += strxcpy(bufp, " ");

    for (dmd=r->land->demands;dmd;dmd=dmd->next) if(dmd->value > 0) {
      m = msg_message("nr_market_price", "product price",
        dmd->type->itype->rtype, dmd->value * dmd->type->price);
      nr_render(m, f->locale, bufp, sizeof(buf)-(bufp-buf), f);
      msg_release(m);
      n--;
      bufp += strlen(bufp);
      if (n == 0) {
        bufp += strxcpy(bufp, LOC(f->locale, "nr_trade_end"));
      }
      else if (n == 1) {
        strcpy(bufp++, " ");
        bufp += strxcpy(bufp, LOC(f->locale, "nr_trade_final"));
        strcpy(bufp++, " ");
      } else {
        bufp += strxcpy(bufp, LOC(f->locale, "nr_trade_next"));
        strcpy(bufp++, " ");
      }
    }
  }
  /* Schreibe Paragraphen */
  rparagraph(F, buf, 0, 0, 0);

}

boolean
see_border(const border * b, const faction * f, const region * r)
{
  boolean cs = b->type->fvisible(b, f, r);
  if (!cs) {
    cs = b->type->rvisible(b, r);
    if (!cs) {
      const unit * us = r->units;
      while (us && !cs) {
        if (us->faction==f) {
          cs = b->type->uvisible(b, us);
          if (cs) break;
        }
        us=us->next;
      }
    }
  }
  return cs;
}

const char *
trailinto(const region * r, const struct locale * lang)
{
  char ref[32];
  const char * s;
  if (r) {
    const char * tname = terrain_name(r);
    strcat(strcpy(ref, tname), "_trail");
    s = locale_string(lang, ref);
    if (s && *s) {
      if (strstr(s, "%s"))  return s;
    }
  }
  return "%s";
}

static void
eval_trail(struct opstack ** stack, const void * userdata) /* (int, int) -> int */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct locale * lang = (const struct locale*)opop(stack).v;
  const struct region * r = (const struct region*)opop(stack).v;
  const char * trail = trailinto(r, lang);
  const char * rn = f_regionid_s(r, f);
  variant var;
  char * x = var.v = balloc(strlen(trail)+strlen(rn));
  sprintf(x, trail, rn);
  opush(stack, var);
}

static void
describe(FILE * F, const region * r, int partial, faction * f)
{
  int n;
  boolean dh;
  direction_t d;
  int trees;
  int saplings;
  attrib *a;
  const char *tname;
  struct edge {
    struct edge * next;
    char * name;
    boolean transparent;
    boolean block;
    boolean exist[MAXDIRECTIONS];
    direction_t lastd;
  } * edges = NULL, * e;
  boolean see[MAXDIRECTIONS];
  char * bufp = buf;

  for (d = 0; d != MAXDIRECTIONS; d++) {
    /* Nachbarregionen, die gesehen werden, ermitteln */
    region *r2 = rconnect(r, d);
    border *b;
    see[d] = true;
    if (!r2) continue;
    for (b=get_borders(r, r2);b;) {
      struct edge * e = edges;
      boolean transparent = b->type->transparent(b, f);
      const char * name = b->type->name(b, r, f, GF_DETAILED|GF_ARTICLE);

      if (!transparent) see[d] = false;
      if (!see_border(b, f, r)) {
        b = b->next;
        continue;
      }
      while (e && (e->transparent != transparent || strcmp(name,e->name))) e = e->next;
      if (!e) {
        e = calloc(sizeof(struct edge), 1);
        e->name = strdup(name);
        e->transparent = transparent;
        e->next = edges;
        edges = e;
      }
      e->lastd=d;
      e->exist[d] = true;
      b = b->next;
    }
  }

  bufp += f_regionid(r, f, bufp, sizeof(buf)-(bufp-buf));

  if (partial == 1) {
    bufp += strxcpy(bufp, " (durchgereist)");
  }
  else if (partial == 3) {
    bufp += strxcpy(bufp, " (benachbart)");
  }
  else if (partial == 2) {
    bufp += strxcpy(bufp, " (vom Turm erblickt)");
  }
  /* Terrain */

  bufp += strxcpy(bufp, ", ");

  tname = terrain_name(r);
  bufp += strxcpy(bufp, LOC(f->locale, tname));

  /* Bäume */

  trees  = rtrees(r,2);
  saplings = rtrees(r,1);
  if (production(r)) {
    if (trees > 0 || saplings > 0) {
      bufp += sprintf(bufp, ", %d/%d ", trees, saplings);
      if (fval(r, RF_MALLORN)) {
        if (trees == 1) {
          bufp += strxcpy(bufp, LOC(f->locale, "nr_mallorntree"));
        } else {
          bufp += strxcpy(bufp, LOC(f->locale, "nr_mallorntree_p"));
        }
      }
      else if (trees == 1) {
        bufp += strxcpy(bufp, LOC(f->locale, "nr_tree"));
      } else {
        bufp += strxcpy(bufp, LOC(f->locale, "nr_tree_p"));
      }
    }
  }

  /* iron & stone */
  if (partial == 0 && f != (faction *) NULL) {
    struct rawmaterial * res;
    for (res=r->resources;res;res=res->next) {
      int level = -1;
      int visible = -1;
      int maxskill = 0;
      const item_type * itype = resource2item(res->type->rtype);
      if (res->type->visible==NULL) {
        visible = res->amount;
        level = res->level + itype->construction->minskill - 1;
      } else {
        const unit * u;
        for (u=r->units; visible!=res->amount && u!=NULL; u=u->next) {
          if (u->faction == f) {
            int s = eff_skill(u, itype->construction->skill, r);
            if (s>maxskill) {
              if (s>=itype->construction->minskill) {
                level = res->level + itype->construction->minskill - 1;
              }
              maxskill = s;
              visible = res->type->visible(res, maxskill);
            }
          }
        }
      }
      if (level>=0 && visible >= 0) {
        bufp += snprintf(bufp, sizeof(buf)-(bufp-buf), ", %d %s/%d",
                         visible, LOC(f->locale, res->type->name),
                         res->level + itype->construction->minskill - 1);
      }
    }
  }

  /* peasants & silver */
  if (rpeasants(r)) {
    int n = rpeasants(r);
    bufp += sprintf(bufp, ", %d", n);

    if(fval(r, RF_ORCIFIED)) {
      strcpy(bufp++, " ");
      bufp += strxcpy(bufp, LOC(f->locale, n==1?"rc_orc":"rc_orc_p"));
    } else {
      strcpy(bufp++, " ");
      bufp += strxcpy(bufp, LOC(f->locale, n==1?"peasant":"peasant_p"));
    }
  }
  if (rmoney(r) && partial == 0) {
    bufp += sprintf(bufp, ", %d ", rmoney(r));
    bufp += strxcpy(bufp, LOC(f->locale, resourcename(oldresourcetype[R_SILVER], rmoney(r)!=1)));
  }
  /* Pferde */

  if (rhorses(r)) {
    bufp += sprintf(bufp, ", %d ", rhorses(r));
    bufp += strxcpy(bufp, LOC(f->locale, resourcename(oldresourcetype[R_HORSE], (rhorses(r)>1)?GR_PLURAL:0)));
  }
  strcpy(bufp++, ".");

  if (r->display && r->display[0]) {
    strcpy(bufp++, " ");
    bufp += strxcpy(bufp, r->display);

    n = r->display[strlen(r->display) - 1];
    if (n != '!' && n != '?' && n != '.') {
      strcpy(bufp++, ".");
    }
  }

  {
    const faction * owner = region_owner(r);
    if (owner!=NULL) {
      bufp += strxcpy(bufp, " Die Region ist im Besitz von ");
      bufp += strxcpy(bufp, factionname(owner));
      strcpy(bufp++, ".");
    }
  }

  if (!is_cursed(r->attribs, C_REGCONF, 0)) {
    attrib *a_do = a_find(r->attribs, &at_overrideroads);
    if(a_do) {
      strcpy(bufp++, " ");
      bufp += strxcpy(bufp, (char *)a_do->data.v);
    } else {
      int nrd = 0;

      /* Nachbarregionen, die gesehen werden, ermitteln */
      for (d = 0; d != MAXDIRECTIONS; d++)
        if (see[d] && rconnect(r, d)) nrd++;

      /* Richtungen aufzählen */

      dh = false;
      for (d = 0; d != MAXDIRECTIONS; d++) if (see[d]) {
        region * r2 = rconnect(r, d);
        if(!r2) continue;
        nrd--;
        if (dh) {
          if (nrd == 0) {
            strcpy(bufp++, " ");
            bufp += strxcpy(bufp, LOC(f->locale, "nr_nb_final"));
          } else {
            bufp += strxcpy(bufp, LOC(f->locale, "nr_nb_next"));
          }
          bufp += strxcpy(bufp, LOC(f->locale, directions[d]));
          strcpy(bufp++, " ");
          bufp += sprintf(bufp, trailinto(r2, f->locale),
              f_regionid_s(r2, f));
        }
        else {
          strcpy(bufp++, " ");
          MSG(("nr_vicinitystart", "dir region", d, r2), bufp, sizeof(buf)-(bufp-buf), f->locale, f);
          bufp += strlen(bufp);
          dh = true;
        }
      }
      /* Spezielle Richtungen */
      for (a = a_find(r->attribs, &at_direction);a && a->type==&at_direction;a=a->next) {
        spec_direction * d = (spec_direction *)(a->data.v);
        strcpy(bufp++, " ");
        bufp += strxcpy(bufp, d->desc);
        bufp += strxcpy(bufp, " (\"");
        bufp += strxcpy(bufp, d->keyword);
        bufp += strxcpy(bufp, "\")");
        strcpy(bufp++, ".");
        dh = 1;
      }
    }
  } else {
    bufp += strxcpy(bufp, " Große Verwirrung befällt alle Reisenden in dieser Region.");
  }
  rnl(F);
  rparagraph(F, buf, 0, 0, 0);

  if (partial==0 && rplane(r) == get_astralplane() &&
      !is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
    /* Sonderbehandlung Teleport-Ebene */
    region_list *rl = astralregions(r, inhabitable);
    region_list *rl2;

    if (rl) {
      /* TODO: Avoid buffer overflows */
      char * c = buf;
      c += strxcpy(c, "Schemen der Regionen ");
      rl2 = rl;
      while (rl2) {
        c += f_regionid(rl2->data, f, c, sizeof(buf)-(c-buf));
        rl2 = rl2->next;
        if (rl2) {
          c += strxcpy(c, ", ");
        }
      }
      strcpy(c, " sind erkennbar.");
      free_regionlist(rl);
      /* Schreibe Paragraphen */
      rnl(F);
      rparagraph(F, buf, 0, 0, 0);
    }
  }

  n = 0;

  /* Wirkungen permanenter Sprüche */
  print_curses(F, f, r, TYP_REGION,0);

  /* Produktionsreduktion */
  a = a_find(r->attribs, &at_reduceproduction);
  if(a) {
    sprintf(buf, "Die Region ist verwüstet, der Boden karg.");
    rparagraph(F, buf, 0, 0, 0);
  }

  if (edges) rnl(F);
  for (e=edges;e;e=e->next) {
    char * bufp = buf;
    boolean first = true;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      if (!e->exist[d]) continue;
      if (first) bufp += strxcpy(bufp, "Im ");
      else {
        if (e->lastd==d) bufp += strxcpy(bufp, " und im ");
        else bufp += strxcpy(bufp, ", im ");
      }
      bufp += strxcpy(bufp, LOC(f->locale, directions[d]));
      first = false;
    }
    if (!e->transparent) bufp += strxcpy(bufp, " versperrt ");
    else bufp += strxcpy(bufp, " befindet sich ");
    bufp += strxcpy(bufp, e->name);
    if (!e->transparent) bufp += strxcpy(bufp, " die Sicht.");
    else strcpy(bufp++, ".");
    rparagraph(F, buf, 0, 0, 0);
  }
  if (edges) {
    while (edges) {
      e = edges->next;
      free(edges->name);
      free(edges);
      edges = e;
    }
  }
}

static void
statistics(FILE * F, const region * r, const faction * f)
{
  const unit *u;
  int number, p;
  message * m;
  item *itm, *items = NULL;
  p = rpeasants(r);
  number = 0;

  /* zählen */
  for (u = r->units; u; u = u->next) {
    if (u->faction == f && u->race != new_race[RC_SPELL]) {
      for (itm=u->items;itm;itm=itm->next) {
        i_change(&items, itm->type, itm->number);
      }
      number += u->number;
    }
  }
  /* Ausgabe */
  rnl(F);
  m = msg_message("nr_stat_header", "region", r);
  nr_render(m, f->locale, buf, sizeof(buf), f);
  msg_release(m);
  rparagraph(F, buf, 0, 0, 0);
  rnl(F);

  /* Region */
  if (fval(r->terrain, LAND_REGION) && rmoney(r)) {
    m = msg_message("nr_stat_maxentertainment", "max", entertainmoney(r));
    nr_render(m, f->locale, buf, sizeof(buf), f);
    rparagraph(F, buf, 2, 2, 0);
    msg_release(m);
  }
  if (production(r) && (!fval(r->terrain, SEA_REGION) || f->race == new_race[RC_AQUARIAN])) {
    m = msg_message("nr_stat_salary", "max", wage(r, f, f->race));
    nr_render(m, f->locale, buf, sizeof(buf), f);
    rparagraph(F, buf, 2, 2, 0);
    msg_release(m);
  }
  if (p) {
    m = msg_message("nr_stat_recruits", "max", p / RECRUITFRACTION);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    rparagraph(F, buf, 2, 2, 0);
    msg_release(m);

    if (!TradeDisabled()) {
      if (buildingtype_exists(r, bt_find("caravan"))) {
        m = msg_message("nr_stat_luxuries", "max",
                        (p * 2) / TRADE_FRACTION);
      } else {
        m = msg_message("nr_stat_luxuries", "max",
                        p / TRADE_FRACTION);
      }
      nr_render(m, f->locale, buf, sizeof(buf), f);
      rparagraph(F, buf, 2, 2, 0);
      msg_release(m);
    }
  }
  /* Info über Einheiten */

  m = msg_message("nr_stat_people", "max", number);
  nr_render(m, f->locale, buf, sizeof(buf), f);
  rparagraph(F, buf, 2, 2, 0);
  msg_release(m);

  for (itm = items; itm; itm=itm->next) {
    sprintf(buf, "%s: %d",
            LOC(f->locale, resourcename(itm->type->rtype, GR_PLURAL)),
            itm->number);
    rparagraph(F, buf, 2, 2, 0);
  }
  while (items) i_free(i_remove(&items, items));
}

static void
durchreisende(FILE * F, const region * r, const faction * f)
{
  if (fval(r, RF_TRAVELUNIT)) {
    attrib *abegin = a_find(r->attribs, &at_travelunit), *a;
    int counter = 0, maxtravel = 0;
    char * str = buf;

    /* Wieviele sind aufzulisten? Für die Grammatik. */
    for (a = abegin; a && a->type==&at_travelunit; a = a->next) {
      unit * u = (unit*)a->data.v;

      if (r!=u->region && (u->ship==NULL || fval(u, UFL_OWNER))) {
        if (cansee_durchgezogen(f, r, u, 0)) {
          ++maxtravel;
        }
      }
    }

    if (maxtravel==0) {
      return;
    }

    /* Auflisten. */
    *str = '\0';
    rnl(F);

    for (a = abegin; a && a->type==&at_travelunit; a = a->next) {
      unit * u = (unit*)a->data.v;

      if (r!=u->region && (u->ship==NULL || fval(u, UFL_OWNER))) {
        if (cansee_durchgezogen(f, r, u, 0)) {
          ++counter;
          if (u->ship != NULL) {
            if (counter == 1) {
              str += strxcpy(str, "Die ");
            } else {
              str += strxcpy(str, "die ");
            }
            str += strxcpy(str, shipname(u->ship));
          } else {
            str += strxcpy(str, unitname(u));
          }
          if (counter + 1 < maxtravel) {
            str += strxcpy(str, ", ");
          } else if (counter + 1 == maxtravel) {
            str += strxcpy(str, " und ");
          }
        }
      }
    }
    if (maxtravel == 1) {
      str += strxcpy(str, " hat die Region durchquert.");
      rparagraph(F, buf, 0, 0, 0);
    } else {
      str += strxcpy(str, " haben die Region durchquert.");
      rparagraph(F, buf, 0, 0, 0);
    }
  }
}

static int
buildingmaintenance(const building * b, const resource_type * rtype)
{
  const building_type * bt = b->type;
  int c, cost=0;
  static boolean init = false;
  static const curse_type * nocost_ct;
  if (!init) { init = true; nocost_ct = ct_find("nocost"); }
  if (curse_active(get_curse(b->attribs, nocost_ct))) {
    return 0;
  }
  for (c=0;bt->maintenance && bt->maintenance[c].number;++c) {
    const maintenance * m = bt->maintenance + c;
    if (m->rtype==rtype) {
      if (fval(m, MTF_VARIABLE))
        cost += (b->size * m->number);
      else
        cost += m->number;
    }
  }
  return cost;
}

static int
report_template(const char * filename, report_context * ctx)
{
  faction * f = ctx->f;
  region *r;
  plane *pl;
  FILE * F = fopen(filename, "wt");
  seen_region * sr = NULL;

  if (F==NULL) {
    perror(filename);
    return -1;
  }

  rps_nowrap(F, "");
  rnl(F);
  rps_nowrap(F, LOC(f->locale, "nr_template"));
  rnl(F);
  rps_nowrap(F, "");
  rnl(F);

  sprintf(buf, "%s %s \"%s\"", LOC(f->locale, "ERESSEA"), factionid(f), LOC(f->locale, "enterpasswd"));
  rps_nowrap(F, buf);
  rnl(F);

  rps_nowrap(F, "");
  rnl(F);
  sprintf(buf, "; ECHECK %s-w4 -r%d -v%s", (f->options & want(O_SILBERPOOL)) ? "-l " : "",
    f->race->recruitcost, ECHECK_VERSION);
  /* -v3.4: ECheck Version 3.4.x */
  rps_nowrap(F, buf);
  rnl(F);

  for (r=ctx->first; sr==NULL && r!=ctx->last; r=r->next) {
    sr = find_seen(ctx->seen, r);
  }

  for (;sr!=NULL;sr=sr->next) {
    region * r = sr->r;
    unit *u;
    int dh = 0;

    if (sr->mode<see_unit) continue;

    for (u = r->units; u; u = u->next) {
      if (u->faction == f && u->race != new_race[RC_SPELL]) {
        order * ord;
        if (!dh) {
          rps_nowrap(F, "");
          rnl(F);
          pl = getplane(r);
          if (pl && fval(pl, PFL_NOCOORDS)) {
            sprintf(buf, "%s; %s", LOC(f->locale, parameters[P_REGION]), rname(r, f->locale));
          } else if (pl && pl->id != 0) {
            sprintf(buf, "%s %d,%d,%d ; %s", LOC(f->locale, parameters[P_REGION]), region_x(r,f),
              region_y(r,f), pl->id, rname(r, f->locale));
          } else {
            sprintf(buf, "%s %d,%d ; %s", LOC(f->locale, parameters[P_REGION]), region_x(r,f),
              region_y(r,f), rname(r, f->locale));
          }
          rps_nowrap(F, buf);
          rnl(F);
          sprintf(buf,"; ECheck Lohn %d", wage(r, f, f->race));
          rps_nowrap(F, buf);
          rnl(F);
          rps_nowrap(F, "");
          rnl(F);
        }
        dh = 1;

        sprintf(buf, "%s %s;    %s [%d,%d$", LOC(u->faction->locale, parameters[P_UNIT]),
          unitid(u), u->name, u->number, get_money(u));
        if (u->building != NULL && fval(u, UFL_OWNER)) {
          building * b = u->building;
          int cost = buildingmaintenance(b, r_silver);

          if (cost > 0) {
            scat(",U");
            icat(cost);
          }
#if TODO
          if (buildingdaten[u->building->typ].spezial != 0) {
            scat("+");
          }
#endif
        } else if (u->ship) {
          if (fval(u, UFL_OWNER))
            scat(",S");
          else
            scat(",s");
          scat(shipid(u->ship));
        }
        if (lifestyle(u) == 0)
          scat(",I");
        scat("]");

        rps_nowrap(F, buf);
        rnl(F);

#ifndef LASTORDER
        for (ord = u->old_orders; ord; ord = ord->next) {
          /* this new order will replace the old defaults */
          strcpy(buf, "   ");
          write_order(ord, u->faction->locale, buf+2, sizeof(buf)-2);
          rps_nowrap(F, buf);
          rnl(F);
        }
#endif
        for (ord = u->orders; ord; ord = ord->next) {
          if (u->old_orders && is_repeated(ord)) continue; /* unit has defaults */
          if (is_persistent(ord)) {
            strcpy(buf, "   ");
            write_order(ord, u->faction->locale, buf+2, sizeof(buf)-2);
            rps_nowrap(F, buf);
            rnl(F);
          }
        }

        /* If the lastorder begins with an @ it should have
        * been printed in the loop before. */
#ifdef LASTORDER
        if (u->lastorder && !is_persistent(u->lastorder)) {
          strcpy(buf, "   ");
          write_order(u->lastorder, u->faction->locale, buf+2, sizeof(buf)-2);
          rps_nowrap(F, buf);
          rnl(F);
        }
#endif
      }
    }
  }
  rps_nowrap(F, "");
  rnl(F);
  sprintf(buf, LOC(f->locale, parameters[P_NEXT]));
  rps_nowrap(F, buf);
  rnl(F);
  fclose(F);
  return 0;
}

static void
show_allies(const faction * f, const ally * allies)
{
  int allierte = 0;
  int i=0, h, hh = 0;
  int dh = 0;
  const ally * sf;
  for (sf = allies; sf; sf = sf->next) {
    int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
    if (mode > 0) ++allierte;
  }

  for (sf = allies; sf; sf = sf->next) {
    int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
    if (mode <= 0) continue;
    i++;
    if (dh) {
      if (i == allierte)
        scat(" und ");
      else
        scat(", ");
    }
    dh = 1;
    hh = 0;
    scat(factionname(sf->faction));
    scat(" (");
    if ((mode & HELP_ALL) == HELP_ALL) {
      scat("Alles");
    } else
      for (h = 1; h < HELP_ALL; h *= 2) {
        if ((mode & h) == h)
          switch (h) {
          case HELP_TRAVEL:
            scat("Durchreise");
            hh = 1;
            break;
          case HELP_MONEY:
            scat("Silber");
            hh = 1;
            break;
          case HELP_FIGHT:
            if (hh)
              scat(", ");
            scat("Kämpfe");
            hh = 1;
            break;
          case HELP_OBSERVE:
            if (hh)
              scat(", ");
            scat("Wahrnehmung");
            hh = 1;
            break;
          case HELP_GIVE:
            if (hh)
              scat(", ");
            scat("Gib");
            hh = 1;
            break;
          case HELP_GUARD:
            if (hh)
              scat(", ");
            scat("Bewache");
            hh = 1;
            break;
          case HELP_FSTEALTH:
            if (hh)
              scat(", ");
            scat("Parteitarnung");
            hh = 1;
            break;
          }
      }
    scat(")");
  }
}

static void
allies(FILE * F, const faction * f)
{
  const group * g = f->groups;
  if (f->allies) {
    if (!f->allies->next) {
      strcpy(buf, "Wir helfen der Partei ");
    } else {
      strcpy(buf, "Wir helfen den Parteien ");
    }
    show_allies(f, f->allies);
    scat(".");
    rparagraph(F, buf, 0, 0, 0);
    rnl(F);
  }

  while (g) {
    if (g->allies) {
      if (!g->allies->next) {
        sprintf(buf, "%s hilft der Partei ", g->name);
      } else {
        sprintf(buf, "%s hilft den Parteien ", g->name);
      }
      show_allies(f, g->allies);
      scat(".");
      rparagraph(F, buf, 0, 0, 0);
      rnl(F);
    }
    g = g->next;
  }
}

#ifdef ENEMIES
static void
enemies(FILE * F, const faction * f)
{
  faction_list * flist = f->enemies;
  if (flist!=NULL) {
    strcpy(buf, "Wir liegen im Krieg mit ");
    for (;flist!=NULL;flist = flist->next) {
      const faction * enemy = flist->data;
      scat(factionname(enemy));
      if (flist->next) {
        scat(", ");
      } else {
        scat(".");
      }
    }
    rparagraph(F, buf, 0, 0, 0);
    rnl(F);
  }
}
#endif

static void
guards(FILE * F, const region * r, const faction * see)
{
  /* die Partei  see  sieht dies; wegen
   * "unbekannte Partei", wenn man es selbst ist... */

  faction* guardians[512];

  int nextguard = 0;

  unit *u;
  int i;

  boolean tarned = false;
  /* Bewachung */

  for (u = r->units; u; u = u->next) {
    if (getguard(u)) {
      faction *f  = u->faction;
      faction *fv = visible_faction(see, u);

      if(fv != f && see != fv) {
        f = fv;
      }

      if (f != see && fval(u, UFL_PARTEITARNUNG)) {
        tarned=true;
      } else {
        for (i=0;i!=nextguard;++i) if (guardians[i]==f) break;
        if (i==nextguard) {
          guardians[nextguard++] = f;
        }
      }
    }
  }

  if (nextguard || tarned) {
    strcpy(buf, "Die Region wird von ");
  } else {
    return;
  }

  for (i = 0; i!=nextguard+(tarned?1:0); ++i) {
    if (i!=0) {
      if (i == nextguard-(tarned?0:1))
        scat(" und ");
      else
        scat(", ");
    }
    if (i<nextguard) scat(factionname(guardians[i]));

    else scat("unbekannten Einheiten");
  }
  scat(" bewacht.");
  rnl(F);
  rparagraph(F, buf, 0, 0, 0);
}

static void
rpline(FILE * F)
{
  static char line[REPORTWIDTH+1];
  if (line[0]!='-') {
    memset(line, '-', sizeof(line));
    line[REPORTWIDTH] = '\n';
  }
  fwrite(line, sizeof(char), sizeof(line), F);
}

static void
list_address(FILE * F, const faction * uf, const faction_list * seenfactions)
{
  const faction_list *flist = seenfactions;

  centre(F, LOC(uf->locale, "nr_addresses"), false);
  rnl(F);

  while (flist!=NULL) {
    const faction * f = flist->data;
    if (f->no!=MONSTER_FACTION) {
      sprintf(buf, "%s: %s; %s", factionname(f), f->email, f->banner);
      rparagraph(F, buf, 4, 0, (char)(ALLIED(uf, f)?'+':'*'));
#ifdef SHORTPWDS
      if (f->shortpwds) {
        shortpwd * spwd = f->shortpwds;
        while (spwd) {
          if (spwd->used) {
            sprintf(buf, "Vertretung: %s", spwd->email);
            rparagraph(F, buf, 6, '-');
          }
          spwd=spwd->next;
        }
      }
#endif

    }
    flist = flist->next;
  }
  rnl(F);
  rpline(F);
}

void
report_building(FILE *F, const region * r, const building * b, const faction * f, int mode)
{
  int i;
  unit *u;
  const char * bname;
  const struct locale * lang = NULL;
  const building_type * type = b->type;
  static const struct building_type * bt_illusion;

  if (!bt_illusion) bt_illusion = bt_find("illusion");
  if (f) lang = f->locale;

  sprintf(buf, "%s, %s %d, ", buildingname(b), LOC(f->locale, "nr_size"),
          b->size);

  if (b->type==bt_illusion) {
    attrib * a = a_find(b->attribs, &at_icastle);
    if (a!=NULL) {
      type = ((icastle_data*)a->data.v)->type;
    }
  }
  bname = LOC(lang, buildingtype(type, b, b->size));
  strcat(buf, bname);
  if (type!=b->type) {
    unit * owner = buildingowner(r, b);
    if (owner && owner->faction==f) {
      /* illusion. report real type */
      char lbuf[32];
      bname = LOC(lang, buildingtype(b->type, b, b->size));
      sprintf(lbuf, " (%s)", bname);
      strcat(buf, lbuf);
    }
  }

  if (b->size < type->maxsize) {
    scat(" (im Bau)");
  }

  if (b->besieged > 0 && mode>=see_lighthouse) {
    scat(", belagert von ");
    icat(b->besieged);
    scat(" Personen ");
    if (b->besieged >= b->size * SIEGEFACTOR) {
      scat("(abgeschnitten)");
    } else {
      scat("(unvollständig belagert)");
    }
  }
  i = 0;
  if (b->display && b->display[0]) {
    scat("; ");
    scat(b->display);
    i = b->display[strlen(b->display) - 1];
  }

#ifdef WDW_PYRAMID

  if (i != '!' && i != '?' && i != '.') {
    scat(", ");
  }

  if (b->type == bt_find("pyramid")) {
    unit * owner = buildingowner(r, b);
    scat("Größenstufe ");
    icat(wdw_pyramid_level(b));
    scat(".");

    if (owner && owner->faction==f) {
      const construction *ctype = b->type->construction;
      int completed = b->size;
      int c;

      scat(" Baukosten pro Größenpunkt: ");

      while(ctype->improvement != NULL &&
         ctype->improvement != ctype &&
         ctype->maxsize > 0 &&
         ctype->maxsize <= completed)
      {
        completed -= ctype->maxsize;
        ctype = ctype->improvement;
      }

      assert(ctype->materials != NULL);

      for (c=0;ctype->materials[c].number;c++) {
        const resource_type * rtype = ctype->materials[c].rtype;
        int number = ctype->materials[c].number;

        if(c > 0) {
          scat(", ");
        }
        icat(number);
        scat(" ");
        scat(locale_string(lang, resourcename(rtype, number!=1?GR_PLURAL:0)));
      }

      scat(".");

      scat(" Erforderlicher Talentwert: ");
      icat(b->type->construction->minskill);
      scat(".");
    }
  }

#else

  if (i != '!' && i != '?' && i != '.')
      scat(".");

#endif

  rparagraph(F, buf, 2, 0, 0);

  if (mode<see_lighthouse) return;

  print_curses(F, f, b, TYP_BUILDING, 4);

  for (u = r->units; u; u = u->next) {
    if (u->building == b) {
      assert(fval(u, UFL_OWNER) || !"you must call reorder_owners() first!");
      rpunit(F, f, u, 6, mode);
      u = u->next;
      break;
    }
  }
  for (;u!=NULL && u->building==b;u=u->next) {
    rpunit(F, f, u, 6, mode);
  }
}

int
report_plaintext(const char * filename, report_context * ctx)
{
  int flag = 0;
  char ch;
  int dh;
  int anyunits;
  const struct region *r;
  faction * f = ctx->f;
  building *b;
  ship *sh;
  unit *u;
  char pzTime[64];
  attrib *a;
  message * m;
  unsigned char op;
  int ix = want(O_STATISTICS);
  int wants_stats = (f->options & ix);
  FILE * F = fopen(filename, "wt");
  seen_region * sr = NULL;

  if (F==NULL) {
    perror(filename);
    return -1;
  }

  strftime(pzTime, 64, "%A, %d. %B %Y, %H:%M", localtime(&ctx->report_time));
  m = msg_message("nr_header_date", "game date", global.gamename, pzTime);
  nr_render(m, f->locale, buf, sizeof(buf), f);
  msg_release(m);
  centre(F, buf, true);

  centre(F, gamedate_season(f->locale), true);
  rnl(F);
  sprintf(buf, "%s, %s/%s (%s)", factionname(f),
      LOC(f->locale, rc_name(f->race, 1)),
      LOC(f->locale, mkname("school", magietypen[f->magiegebiet])),
      f->email);
  centre(F, buf, true);
  if (f->alliance!=NULL) {
    centre(F, alliancename(f->alliance), true);
  }

#ifdef KARMA_MODULE
  buf[0] = 0;
  dh = 0;
  for (a=a_find(f->attribs, &at_faction_special); a && a->type==&at_faction_special; a=a->next) {
    char buf2[80];
    dh++;
    if (fspecials[a->data.sa[0]].maxlevel != 100) {
      sprintf(buf2, "%s (%d)", fspecials[a->data.sa[0]].name, a->data.sa[1]);
    } else {
      sprintf(buf2, "%s", fspecials[a->data.sa[0]].name);
    }
    if(dh > 1) strcat(buf, ", ");
    strcat(buf, buf2);
  }
  if(dh > 0) centre(F, buf, true);

  if (f->karma > 0) {
    sprintf(buf, "Deine Partei hat %d Karma.", f->karma);
    centre(F, buf, true);
  }
#endif

  dh = 0;
  if (f->age <= 2) {
    if (f->age <= 1) {
      ADDMSG(&f->msgs, msg_message("changepasswd",
        "value", f->passw));
    }
    RENDER(f, buf, sizeof(buf), ("newbie_password", "password", f->passw));
    rnl(F);
    centre(F, buf, true);
    rnl(F);
    centre(F, LOC(f->locale, "newbie_info_1"), true);
    rnl(F);
    centre(F, LOC(f->locale, "newbie_info_2"), true);
    if ((f->options & want(O_COMPUTER)) == 0) {
      f->options |= want(O_COMPUTER);
      rnl(F);
      centre(F, LOC(f->locale, "newbie_info_3"), true);
    }
  }
  rnl(F);
#ifdef SCORE_MODULE
  if (f->options & want(O_SCORE) && f->age > DISPLAYSCORE) {
    RENDER(f, buf, sizeof(buf), ("nr_score", "score average", f->score, average_score_of_age(f->age, f->age / 24 + 1)));
    centre(F, buf, true);
  }
#endif
  m = msg_message("nr_population", "population units", count_all(f), f->no_units);
  nr_render(m, f->locale, buf, sizeof(buf), f);
  msg_release(m);
  centre(F, buf, true);
  if (f->race == new_race[RC_HUMAN]) {
    int maxmig = count_maxmigrants(f);
    m = msg_message("nr_migrants", "units maxunits", count_migrants(f), maxmig);
    nr_render(m, f->locale, buf, sizeof(buf), f);
    msg_release(m);
    centre(F, buf, true);
  }
#ifdef HEROES
  {
    int maxh = maxheroes(f);
    if (maxh) {
      message * msg = msg_message("nr_heroes", "units maxunits", countheroes(f), maxh);
      nr_render(msg, f->locale, buf, sizeof(buf), f);
      msg_release(msg);
      centre(F, buf, true);
    }
  }
#endif

  if (f->items!=NULL) {
    message * msg = msg_message("nr_claims", "items", f->items);
    nr_render(msg, f->locale, buf, sizeof(buf), f);
    msg_release(msg);
    rnl(F);
    centre(F, buf, true);
  }

  if (f->age > 1 && f->lastorders != turn) {
    rnl(F);
    if (turn - f->lastorders == 1) {
      centre(F, LOC(f->locale, "nr_nmr"), true);
    } else {
      sprintf(buf,
        "Deine Partei hat seit %d Runden keinen Zug abgegeben! Wenn du"
        " drei Runden nacheinander keinen Zug abgibst, wird sie"
        " automatisch gelöscht.",
        turn - f->lastorders);
      centre(F, buf, true);
    }
    rnl(F);
  }

  /* Insekten-Winter-Warnung */
  if(f->race == new_race[RC_INSECT]) {
    static int thisseason = -1;
    if (thisseason<0) thisseason = get_gamedate(turn+1, 0)->season;
    if (thisseason == 0) {
      strcpy(buf, "Es ist Winter, und Insekten können nur in Wüsten oder mit "
        "Hilfe des Nestwärme-Tranks Personen rekrutieren.");
      centre(F, buf, true);
      rnl(F);
    } else {
      static int nextseason = -1;
      if (nextseason<0) nextseason = get_gamedate(turn+2, 0)->season;
      if (nextseason == 0) {
        strcpy(buf, "Es ist Spätherbst, und diese Woche ist die letzte vor dem "
          "Winter, in der Insekten rekrutieren können.");
        centre(F, buf, true);
        rnl(F);
      }
    }
  }

  sprintf(buf, "%s:", LOC(f->locale, "nr_options"));
  for (op = 0; op != MAXOPTIONS; op++) {
    if (f->options & want(op)) {
      scat(" ");
      scat(LOC(f->locale, options[op]));
#ifdef AT_OPTION
      if(op == O_NEWS) {
        attrib *a = a_find(f->attribs, &at_option_news);
        if(!a) {
          /* Zur Altlastenbeseitigung */
          f->options = f->options & ~op;
        } else {
          int sec = a->data.i;
          int i;
          scat("(");
          for(i=1; sec != 0; i *= 2) {
            if(sec & i) {
              icat(i);
              sec = sec & ~i;
              if(sec) scat(",");
            }
          }
          scat(")");
        }
      }
#endif
      flag++;
    }
  }
  if (flag > 0) {
    rnl(F);
    centre(F, buf, true);
  }

  rp_messages(F, f->msgs, f, 0, true);
  rp_battles(F, f);
  a = a_find(f->attribs, &at_reportspell);
  if (a) {
    rnl(F);
    centre(F, LOC(f->locale, "section_newspells"), true);
    while (a && a->type==&at_reportspell) {
      spell *sp = (spell *)a->data.v;
      report_spell(F, sp, f->locale);
      a = a->next;
    }
  }

  ch = 0;
  for (a=a_find(f->attribs, &at_showitem);a && a->type==&at_showitem;a=a->next) {
    const potion_type * ptype = resource2potion(((const item_type*)a->data.v)->rtype);
    const char * description = NULL;
    requirement * m;
    if (ptype!=NULL) {
      const char * pname = resourcename(ptype->itype->rtype, 0);
      m = ptype->itype->construction->materials;
      if (ch==0) {
        rnl(F);
        centre(F, LOC(f->locale, "section_newpotions"), true);
        ch = 1;
      }

      rnl(F);
      centre(F, LOC(f->locale, pname), true);
      sprintf(buf, "%s %d", LOC(f->locale, "nr_level"), ptype->level);
      centre(F, buf, true);
      rnl(F);
      sprintf(buf, "%s: ", LOC(f->locale, "nr_herbsrequired"));
      while (m->number) {
        scat(LOC(f->locale, resourcename(m->rtype, 0)));
        ++m;
        if (m->number) scat(", ");
      }
      centre(F, buf, true);
      rnl(F);
      if (description==NULL) {
        const char * potiontext = mkname("potion", pname);
        description = LOC(f->locale, potiontext);
      }
      centre(F, description, true);
    }
  }
  rnl(F);
  centre(F, LOC(f->locale, "nr_alliances"), false);
  rnl(F);

#ifdef ENEMIES
  enemies(F, f);
#endif
  allies(F, f);

  rpline(F);

  anyunits = 0;

  for (r=ctx->first;sr==NULL && r!=ctx->last;r=r->next) {
    sr = find_seen(ctx->seen, r);
  }
  for (;sr!=NULL;sr=sr->next) {
    region * r = sr->r;
    int stealthmod = stealth_modifier(sr->mode);

    if (sr->mode<see_lighthouse) continue;
    /* Beschreibung */

    if (sr->mode==see_unit) {
      anyunits = 1;
      describe(F, r, 0, f);
      if (!TradeDisabled() && !fval(r->terrain, SEA_REGION) && rpeasants(r)/TRADE_FRACTION > 0) {
        rnl(F);
        prices(F, r, f);
      }
      guards(F, r, f);
      durchreisende(F, r, f);
    }
    else {
      if (sr->mode==see_far) {
        describe(F, r, 3, f);
        guards(F, r, f);
        durchreisende(F, r, f);
      }
      else if (sr->mode==see_lighthouse) {
        describe(F, r, 2, f);
        durchreisende(F, r, f);
      } else {
        describe(F, r, 1, f);
        durchreisende(F, r, f);
      }
    }
    /* Statistik */

    if (wants_stats && sr->mode==see_unit)
        statistics(F, r, f);

    /* Nachrichten an REGION in der Region */

    if (sr->mode==see_unit || sr->mode==see_travel) {
      message_list * mlist = r_getmessages(r, f);
      rp_messages(F, r->msgs, f, 0, true);
      if (mlist) rp_messages(F, mlist, f, 0, true);
    }
    /* Burgen und ihre Einheiten */

    for (b = rbuildings(r); b; b = b->next) {
      rnl(F);
      report_building(F, r, b, f, sr->mode);
    }

    /* Restliche Einheiten */

    if (stealthmod>INT_MIN) {
      for (u = r->units; u; u = u->next) {
        if (!u->building && !u->ship) {
          if (u->faction == f || cansee(f, r, u, stealthmod)) {
            if (dh == 0 && !(rbuildings(r) || r->ships)) {
              dh = 1;
              /* rnl(F); */
            }
            rpunit(F, f, u, 4, sr->mode);
          }
        }
      }
    }

    /* Schiffe und ihre Einheiten */

    for (sh = r->ships; sh; sh = sh->next) {
      faction *of = NULL;

      rnl(F);

      /* Gewicht feststellen */

      for (u = r->units; u; u = u->next) {
        if (u->ship == sh && fval(u, UFL_OWNER)) {
          of = u->faction;
          break;
        }
      }
      if (of == f) {
        int n = 0, p = 0;
        getshipweight(sh, &n, &p);
        n = (n+99) / 100; /* 1 Silber = 1 GE */

        sprintf(buf, "%s, %s, (%d/%d)", shipname(sh),
          LOC(f->locale, sh->type->name[0]), n, shipcapacity(sh) / 100);
      } else {
        sprintf(buf, "%s, %s", shipname(sh), LOC(f->locale, sh->type->name[0]));
      }

      assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
      if (sh->size!=sh->type->construction->maxsize) {
        sprintf(buf+strlen(buf), ", %s (%d/%d)",
          LOC(f->locale, "nr_undercons"), sh->size,
          sh->type->construction->maxsize);
      }
      if (sh->damage) {
        int percent = (sh->damage*100+DAMAGE_SCALE-1)/(sh->size*DAMAGE_SCALE);
        sprintf(buf+strlen(buf), ", %d%% %s", percent, LOC(f->locale, "nr_damaged"));
      }
      if (!fval(r->terrain, SEA_REGION)) {
        if (sh->coast != NODIRECTION) {
          scat(LOC(f->locale, "list_and"));
          scat(LOC(f->locale, coasts[sh->coast]));
        }
      }
      ch = 0;
      if (sh->display && sh->display[0]) {
        scat("; ");
        scat(sh->display);

        ch = sh->display[strlen(sh->display) - 1];
      }
      if (ch != '!' && ch != '?' && ch != '.')
        scat(".");

      rparagraph(F, buf, 2, 0, 0);

      print_curses(F,f,sh,TYP_SHIP,4);

      for (u = r->units; u; u = u->next) {
        if (u->ship == sh && fval(u, UFL_OWNER)) {
          rpunit(F, f, u, 6, sr->mode);
          break;
        }
      }
      for (u = r->units; u; u = u->next) {
        if (u->ship == sh && !fval(u, UFL_OWNER)) {
          rpunit(F, f, u, 6, sr->mode);
        }
      }
    }

    rnl(F);
    rpline(F);
  }
  if (f->no != MONSTER_FACTION) {
    if (!anyunits) {
      rnl(F);
      rparagraph(F, LOC(f->locale, "nr_youaredead"), 0, 2, 0);
    } else {
      list_address(F, f, ctx->addresses);
    }
  }
  fclose(F);
  return 0;
}

void
base36conversion(void)
{
  region * r;
  for (r=regions;r;r=r->next) {
    unit * u;
    for (u=r->units;u;u=u->next) {
      if (forbiddenid(u->no)) {
        uunhash(u);
        u->no = newunitid();
        uhash(u);
      }
    }
  }
}

#define FMAXHASH 1021

struct fsee {
  struct fsee * nexthash;
  faction * f;
  struct see {
    struct see * next;
    faction * seen;
    unit * proof;
  } * see;
} * fsee[FMAXHASH];

#define REPORT_NR (1 << O_REPORT)
#define REPORT_CR (1 << O_COMPUTER)
#define REPORT_ZV (1 << O_ZUGVORLAGE)
#define REPORT_ZIP (1 << O_COMPRESS)
#define REPORT_BZIP2 (1 << O_BZIP2)

int
init_reports(void)
{
  update_intervals();

#ifdef HAVE_STAT
  {
    stat_type st;
    if (stat(reportpath(), &st)==0) return 0;
  }
#endif
  if (makedir(reportpath(), 0700)!=0) {
    if (errno!=EEXIST) {
      perror("could not create reportpath");
      return -1;
    }
  }
  return 0;
}


unit *
can_find(faction * f, faction * f2)
{
  int key = f->no % FMAXHASH;
  struct fsee * fs = fsee[key];
  struct see * ss;
  if (f==f2) return f->units;
  while (fs && fs->f!=f) fs=fs->nexthash;
  if (!fs) return NULL;
  ss=fs->see;
  while (ss && ss->seen!=f2) ss=ss->next;
  if (ss) {
    /* bei TARNE PARTEI yxz muss die Partei von unit proof nicht
     * wirklich Partei f2 sein! */
    /* assert(ss->proof->faction==f2); */
    return ss->proof;
  }
  return NULL;
}

static void
add_find(faction * f, unit * u, faction *f2)
{
  /* faction f sees f2 through u */
  int key = f->no % FMAXHASH;
  struct fsee ** fp = &fsee[key];
  struct fsee * fs;
  struct see ** sp;
  struct see * ss;
  while (*fp && (*fp)->f!=f) fp=&(*fp)->nexthash;
  if (!*fp) {
    fs = *fp = calloc(sizeof(struct fsee), 1);
    fs->f = f;
  } else fs = *fp;
  sp = &fs->see;
  while (*sp && (*sp)->seen!=f2) sp=&(*sp)->next;
  if (!*sp) {
    ss = *sp = calloc(sizeof(struct see), 1);
    ss->proof = u;
    ss->seen = f2;
  } else ss = *sp;
  ss->proof = u;
}

static void
update_find(void)
{
  region * r;
  static boolean initial = true;

  if (initial) for (r=regions;r;r=r->next) {
    unit * u;
    for (u=r->units;u;u=u->next) {
      faction * lastf = u->faction;
      unit * u2;
      for (u2=r->units;u2;u2=u2->next) {
        if (u2->faction==lastf || u2->faction==u->faction)
          continue;
        if (seefaction(u->faction, r, u2, 0)) {
          faction *fv = visible_faction(u->faction, u2);
          lastf = fv;
          add_find(u->faction, u2, fv);
        }
      }
    }
  }
  initial = false;
}

boolean
kann_finden(faction * f1, faction * f2)
{
  update_find();
  return (boolean)(can_find(f1, f2)!=NULL);
}

/******* summary *******/
int dropouts[2];
int * age = NULL;

typedef struct summary {
  int waffen;
  int factions;
  int ruestungen;
  int schiffe;
  int gebaeude;
  int maxskill;
  int heroes;
  int inhabitedregions;
  int peasants;
  int nunits;
  int playerpop;
  double playermoney;
  double peasantmoney;
  int armed_men;
  int poprace[MAXRACES];
  int factionrace[MAXRACES];
  int landregionen;
  int regionen_mit_spielern;
  int landregionen_mit_spielern;
  int orkifizierte_regionen;
  int inactive_volcanos;
  int active_volcanos;
  int spielerpferde;
  int pferde;
  struct language {
    struct language * next;
    int number;
    const struct locale * locale;
  } * languages;
} summary;

summary *
make_summary(void)
{
  faction *f;
  region *r;
  unit *u;
  summary * s = calloc(1, sizeof(summary));

  for (f = factions; f; f = f->next) {
    const struct locale * lang = f->locale;
    struct language * plang = s->languages;
    while (plang && plang->locale != lang) plang=plang->next;
    if (!plang) {
      plang = calloc(sizeof(struct language), 1);
      plang->next = s->languages;
      s->languages = plang;
      plang->locale = lang;
    }
    ++plang->number;
    f->nregions = 0;
    f->num_total = 0;
    f->money = 0;
    if (f->alive && f->units) {
      s->factions++;
      /* Problem mit Monsterpartei ... */
      if (f->no!=MONSTER_FACTION) {
        s->factionrace[old_race(f->race)]++;
      }
    }
  }

  /* Alles zählen */

  for (r = regions; r; r = r->next) {
    s->pferde += rhorses(r);
    s->schiffe += listlen(r->ships);
    s->gebaeude += listlen(r->buildings);
    if (!fval(r->terrain, SEA_REGION)) {
      s->landregionen++;
      if (r->units) {
        s->landregionen_mit_spielern++;
      }
      if (fval(r, RF_ORCIFIED)) {
        s->orkifizierte_regionen++;
      }
      if (rterrain(r) == T_VOLCANO) {
        s->inactive_volcanos++;
      } else if(rterrain(r) == T_VOLCANO_SMOKING) {
        s->active_volcanos++;
      }
    }
    if (r->units) {
      s->regionen_mit_spielern++;
    }
    if (rpeasants(r) || r->units) {
      s->inhabitedregions++;
      s->peasants += rpeasants(r);
      s->peasantmoney += rmoney(r);

      /* Einheiten Info. nregions darf nur einmal pro Partei
       * incrementiert werden. */

      for (u = r->units; u; u = u->next) freset(u->faction, FL_DH);
      for (u = r->units; u; u = u->next) {
        f = u->faction;
        if (u->faction->no != MONSTER_FACTION) {
          skill * sv;
          item * itm;

          s->nunits++;
          s->playerpop += u->number;
          if (u->flags & UFL_HERO) {
            s->heroes += u->number;
          }
          s->spielerpferde += get_item(u, I_HORSE);
          s->playermoney += get_money(u);
          s->armed_men += armedmen(u);
          for (itm=u->items;itm;itm=itm->next) {
            if (itm->type->rtype->wtype) {
              s->waffen += itm->number;
            }
            if (itm->type->rtype->atype) {
              s->ruestungen += itm->number;
            }
          }

          s->spielerpferde += get_item(u, I_HORSE);

          for (sv = u->skills; sv != u->skills + u->skill_size; ++sv) {
            skill_t sk = sv->id;
            int aktskill = eff_skill(u, sk, r);
            if (aktskill > s->maxskill) s->maxskill = aktskill;
          }
          if (!fval(f, FL_DH)) {
            f->nregions++;
            fset(f, FL_DH);
          }
        }

        f->num_total += u->number;
        f->money += get_money(u);
        s->poprace[old_race(u->race)] += u->number;
      }
    }
  }

  return s;
}

static char *
pcomp(double i, double j)
{
   sprintf(buf, "%.0f (%s%.0f)", i, (i>=j)?"+":"", i-j);
   return buf;
}

static char *
rcomp(int i, int j)
{
  sprintf(buf, "%d (%s%d,%s%d%%)",
      i, (i>=j)?"+":"", i-j, (i>=j)?"+":"",j?((i-j)*100)/j:0);
  return buf;
}

static void
writemonument(void)
{
  FILE * F;
  region *r;
  building *b;
  building *buildings[7] = {NULL, NULL, NULL, NULL, NULL,  NULL, NULL};
  int size[7] = {0,0,0,0,0,0,0};
  int i, j, ra;
  int count = 0;
  unit *owner;

  for (r = regions; r; r = r->next) {
    for (b = r->buildings; b; b = b->next) {
      if (b->type == bt_find("monument") && b->display && *b->display) {
        freset(b, FL_DH);
        count++;
        if(b->size > size[6]) {
          for(i=0; i <= 6; i++) if(b->size >= size[i]) {
            for(j=5;j >= i; j--) {
              size[j+1] = size[j];
              buildings[j+1] = buildings[j];
            }
            buildings[i] = b;
            size[i] = b->size;
            break;
          }
        }
      }
    }
  }

  for(i=0; i <= 6; i++) if(buildings[i]) {
    fset(buildings[i], FL_DH);
  }
  {
    char zText[MAX_PATH];
    sprintf(zText, "%s/news-monument", basepath());
    F = cfopen(zText, "w");
    if (!F) return;
  }
  fprintf(F, "\n--- maintitle ---\n\n");
  for(i = 0; i<=6; i++) {
    if (buildings[i] != NULL) {
      fprintf(F, "In %s", rname(buildings[i]->region, NULL));
      if ((owner=buildingowner(buildings[i]->region,buildings[i]))!=NULL && !fval(owner,UFL_PARTEITARNUNG)) {
        fprintf(F, ", Eigentümer: %s", factionname(owner->faction));
      }
      fprintf(F, "\n\n");
      report_building(F, buildings[i]->region, buildings[i], findfaction(0), see_neighbour);
      fprintf(F, "\n\n");
    }
  }

  fprintf(F, "\n--- newcomer ---\n\n");

  if(count > 7) {
    ra = rng_int()%(count-7);
    j = 0;
    for (r = regions; r; r = r->next) {
      for (b = r->buildings; b; b = b->next) {
        if (b->type == bt_find("monument") && b->display && *b->display && !fval(b, FL_DH)) {
          j++;
          if(j == ra) {
            fprintf(F, "In %s", rname(b->region, NULL));
            if ((owner=buildingowner(b->region,b))!=NULL && !fval(owner,UFL_PARTEITARNUNG)) {
              fprintf(F, ", Eigentümer: %s", factionname(owner->faction));
            }
            fprintf(F, "\n\n");
            report_building(F, b->region, b, findfaction(0), see_neighbour);
            fprintf(F, "\n\n");
          }
        }
      }
    }
  }

  fclose(F);

}

static void
writeadresses(void)
{
  faction *f;
  FILE *F;
  char zText[MAX_PATH];
  sprintf(zText, "%s/adressen", basepath());
  F = cfopen(zText, "w");
  if (!F) return;

  for (f = factions; f; f = f->next) {
    if (f->no != MONSTER_FACTION && playerrace(f->race)) {
      fprintf(F, "%s:%s:%s\n", factionname(f), f->email, f->banner);
    }
  }
  fclose(F);
}

static void
writenewssubscriptions(void)
{
  char zText[MAX_PATH];
  FILE *F;

  sprintf(zText, "%s/news-subscriptions", basepath());
  F = cfopen(zText, "w");
  if (!F) return;
#ifdef AT_OPTION
  {
    faction *f;
    for(f=factions; f; f=f->next) {
      attrib *a = a_find(f->attribs, &at_option_news);
      if(!a) {
        fprintf(F, "%s:0\n", f->email);
      } else {
        fprintf(F, "%s:%d\n", f->email, a->data.i);
      }
    }
  }
#endif
  fclose(F);
}

static void
writeforward(void)
{
  FILE *forwardFile;
  region *r;
  faction *f;
  unit *u;

  {
    char zText[MAX_PATH];
    sprintf(zText, "%s/aliases", basepath());
    forwardFile = cfopen(zText, "w");
    if (!forwardFile) return;
  }

  for (f = factions; f; f = f->next) {
    if (f->no != MONSTER_FACTION) {
      fprintf(forwardFile,"partei-%s: %s\n", factionid(f), f->email);
    }
  }

  for (r = regions; r; r = r->next) {
    for (u = r->units; u; u = u->next) {
      if (u->faction->no != MONSTER_FACTION) {
        fprintf(forwardFile,"einheit-%s: %s\n", unitid(u), u->faction->email);
      }
    }
  }

  fclose(forwardFile);
}

static void
writeturn(void)
{
  char zText[MAX_PATH];
  FILE *f;

  sprintf(zText, "%s/datum", basepath());
  f = cfopen(zText, "w");
  if (!f) return;
  fputs(gamedate2(default_locale), f);
  fclose(f);
  sprintf(zText, "%s/turn", basepath());
  f = cfopen(zText, "w");
  if (!f) return;
  fprintf(f, "%d\n", turn);
  fclose(f);
}

static void
out_faction(FILE *file, faction *f)
{
  if (alliances!=NULL) {
    fprintf(file, "%s (%s/%d) (%.3s/%.3s), %d Einh., %d Pers., $%d, %d NMR\n",
      f->name, itoa36(f->no), f->alliance?f->alliance->id:0,
      LOC(default_locale, rc_name(f->race, 0)), magietypen[f->magiegebiet],
      f->no_units, f->num_total, f->money, turn - f->lastorders);
  } else {
    fprintf(file, "%s (%.3s/%.3s), %d Einh., %d Pers., $%d, %d NMR\n",
      factionname(f), LOC(default_locale, rc_name(f->race, 0)),
      magietypen[f->magiegebiet], f->no_units, f->num_total, f->money,
      turn - f->lastorders);
  }
}

void
report_summary(summary * s, summary * o, boolean full)
{
  FILE * F = NULL;
  int i, newplayers = 0;
  faction * f;
  char zText[MAX_PATH];

  if (full) {
    sprintf(zText, "%s/parteien.full", basepath());
  } else {
    sprintf(zText, "%s/parteien", basepath());
  }
  F = cfopen(zText, "w");
  if (!F) return;
  printf("Schreibe Zusammenfassung (parteien)...\n");
  fprintf(F,   "%s\n%s\n\n", global.gamename, gamedate2(default_locale));
  fprintf(F,   "Auswertung Nr:         %d\n\n", turn);
  fprintf(F,   "Parteien:              %s\n", pcomp(s->factions, o->factions));
  fprintf(F,   "Einheiten:             %s\n", pcomp(s->nunits, o->nunits));
  fprintf(F,   "Spielerpopulation:     %s\n",
      pcomp(s->playerpop, o->playerpop));
  fprintf(F,   " davon bewaffnet:      %s\n",
      pcomp(s->armed_men, o->armed_men));
#ifdef HEROES
  fprintf(F,   " Helden:               %s\n", pcomp(s->heroes, o->heroes));
#endif

  if (full) {
    fprintf(F, "Regionen:              %d\n", listlen(regions));
    fprintf(F, "Bewohnte Regionen:     %d\n", s->inhabitedregions);
    fprintf(F, "Landregionen:          %d\n", s->landregionen);
    fprintf(F, "Spielerregionen:       %d\n", s->regionen_mit_spielern);
    fprintf(F, "Landspielerregionen:   %d\n", s->landregionen_mit_spielern);
    fprintf(F, "Orkifizierte Regionen: %d\n", s->orkifizierte_regionen);
    fprintf(F, "Inaktive Vulkane:      %d\n", s->inactive_volcanos);
    fprintf(F, "Aktive Vulkane:        %d\n\n", s->active_volcanos);
  }

  for (i = 0; i < MAXRACES; i++) {
    const race * rc = new_race[i];
    if (s->factionrace[i] && rc && playerrace(rc)
        && i != RC_TEMPLATE && i != RC_CLONE) {
      fprintf(F, "%14svölker: %s\n", LOC(default_locale, rc_name(rc, 3)),
              pcomp(s->factionrace[i], o->factionrace[i]));
    }
  }

  if(full) {
    fprintf(F, "\n");
    {
      struct language * plang = s->languages;
      while (plang!=NULL) {
        struct language * olang = o->languages;
        int nold = 0;
        while (olang && olang->locale!=plang->locale) olang=olang->next;
        if (olang) nold = olang->number;
        fprintf(F, "Sprache %12s: %s\n", locale_name(plang->locale),
          rcomp(plang->number, nold));
        plang=plang->next;
      }
    }
  }

  fprintf(F, "\n");
  if (full) {
    for (i = 0; i < MAXRACES; i++) {
      const race * rc = new_race[i];
      if (s->poprace[i]) {
        fprintf(F, "%20s: %s\n", LOC(default_locale, rc_name(rc, 1)),
                rcomp(s->poprace[i], o->poprace[i]));
      }
    }
  } else {
    for (i = 0; i < MAXRACES; i++) {
      const race * rc = new_race[i];
      if (s->poprace[i] && playerrace(rc)
          && i != RC_TEMPLATE && i != RC_CLONE) {
        fprintf(F, "%20s: %s\n", LOC(default_locale, rc_name(rc, 1)),
                rcomp(s->poprace[i], o->poprace[i]));
      }
    }
  }

  if (full) {
    fprintf(F, "\nWaffen:               %s\n", pcomp(s->waffen,o->waffen));
    fprintf(F, "Rüstungen:            %s\n",
        pcomp(s->ruestungen,o->ruestungen));
    fprintf(F, "ungezähmte Pferde:    %s\n", pcomp(s->pferde, o->pferde));
    fprintf(F, "gezähmte Pferde:      %s\n",
        pcomp(s->spielerpferde,o->spielerpferde));
    fprintf(F, "Schiffe:              %s\n", pcomp(s->schiffe, o->schiffe));
    fprintf(F, "Gebäude:              %s\n", pcomp(s->gebaeude, o->gebaeude));

    fprintf(F, "\nBauernpopulation:     %s\n", pcomp(s->peasants,o->peasants));

    fprintf(F, "Population gesamt:    %d\n\n", s->playerpop+s->peasants);

    fprintf(F, "Reichtum Spieler:     %s Silber\n",
        pcomp(s->playermoney,o->playermoney));
    fprintf(F, "Reichtum Bauern:      %s Silber\n",
        pcomp(s->peasantmoney, o->peasantmoney));
    fprintf(F, "Reichtum gesamt:      %s Silber\n\n",
        pcomp(s->playermoney+s->peasantmoney, o->playermoney+o->peasantmoney));
  }

  fprintf(F, "\n\n");

  newplayers = update_nmrs();
  
  for (i = 0; i <= NMRTimeout(); ++i) {
    if (i == NMRTimeout()) {
      fprintf(F, "+ NMR:\t\t %d\n", nmrs[i]);
    } else {
      fprintf(F, "%d NMR:\t\t %d\n", i, nmrs[i]);
    }
  }
  if (age) {
    if (age[2] != 0) {
      fprintf(F, "Erstabgaben:\t %d%%\n", 100 - (dropouts[0] * 100 / age[2]));
    }
    if (age[3] != 0) {
      fprintf(F, "Zweitabgaben:\t %d%%\n", 100 - (dropouts[1] * 100 / age[3]));
    }
  }
  fprintf(F, "Neue Spieler:\t %d\n", newplayers);

  if (full) {
    if (factions)
      fprintf(F, "\nParteien:\n\n");

    for (f = factions; f; f = f->next) {
      out_faction(F, f);
    }

    if (NMRTimeout() && full) {
      fprintf(F, "\n\nFactions with NMRs:\n");
      for (i = NMRTimeout(); i > 0; --i) {
        for(f=factions; f; f=f->next) {
          if(i == NMRTimeout()) {
            if(turn - f->lastorders >= i) {
              out_faction(F, f);
            }
          } else {
            if(turn - f->lastorders == i) {
              out_faction(F, f);
            }
          }
        }
      }
    }
  }

  fclose(F);

  if (full) {
    printf("Schreibe Liste der Adressen (adressen)...\n");
    writeadresses();
    writenewssubscriptions();
    writeforward();

    printf("writing date & turn\n");
    writeturn();

    writemonument();
  }
  free(nmrs);
  nmrs = NULL;
}
/******* end summary ******/

static void
eval_unit(struct opstack ** stack, const void * userdata) /* unit -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct unit * u = (const struct unit *)opop(stack).v;
  const char * c = u?unitname(u):LOC(f->locale, "an_unknown_unit");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_unit_dative(struct opstack ** stack, const void * userdata) /* unit -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct unit * u = (const struct unit *)opop(stack).v;
  const char * c = u?unitname(u):LOC(f->locale, "unknown_unit_dative");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_spell(struct opstack ** stack, const void * userdata) /* unit -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct spell * sp = (const struct spell *)opop(stack).v;
  const char * c = sp?spell_name(sp, f->locale):LOC(f->locale, "an_unknown_spell");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_unitname(struct opstack ** stack, const void * userdata) /* unit -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct unit * u = (const struct unit *)opop(stack).v;
  const char * c = u?u->name:LOC(f->locale, "an_unknown_unit");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}


static void
eval_unitid(struct opstack ** stack, const void * userdata) /* unit -> int */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct unit * u = (const struct unit *)opop(stack).v;
  const char * c = u?u->name:LOC(f->locale, "an_unknown_unit");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_unitsize(struct opstack ** stack, const void * userdata) /* unit -> int */
{
  const struct unit * u = (const struct unit *)opop(stack).v;
  variant var;

  var.i = u->number;
  opush(stack, var);
}

static void
eval_faction(struct opstack ** stack, const void * userdata) /* faction -> string */
{
  const struct faction * f = (const struct faction *)opop(stack).v;
  const char * c = factionname(f);
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_alliance(struct opstack ** stack, const void * userdata) /* faction -> string */
{
  const struct alliance * al = (const struct alliance *)opop(stack).v;
  const char * c = alliancename(al);
  variant var;
  if (c!=NULL) {
    size_t len = strlen(c);
    var.v = strcpy(balloc(len+1), c);
  }
  else var.v = NULL;
  opush(stack, var);
}

static void
eval_region(struct opstack ** stack, const void * userdata) /* region -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct region * r = (const struct region *)opop(stack).v;
  const char * c = regionname(r, f);
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_terrain(struct opstack ** stack, const void * userdata) /* region -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct region * r = (const struct region *)opop(stack).v;
  const char * c = LOC(f->locale, terrain_name(r));
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_ship(struct opstack ** stack, const void * userdata) /* ship -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct ship * u = (const struct ship *)opop(stack).v;
  const char * c = u?shipname(u):LOC(f->locale, "an_unknown_ship");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_building(struct opstack ** stack, const void * userdata) /* building -> string */
{
  const struct faction * f = (const struct faction *)userdata;
  const struct building * u = (const struct building *)opop(stack).v;
  const char * c = u?buildingname(u):LOC(f->locale, "an_unknown_building");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_weight(struct opstack ** stack, const void * userdata) /* region -> string */
{
  char buffer[32];
  const struct faction * f = (const struct faction *)userdata;
  const struct locale * lang = f->locale;
  int weight = opop_i(stack);
  variant var;

  if (weight % SCALEWEIGHT == 0) {
    if (weight==SCALEWEIGHT) {
      sprintf(buffer, "1 %s", LOC(lang, "weight_unit"));
    } else {
      sprintf(buffer, "%u %s", weight/SCALEWEIGHT, LOC(lang, "weight_unit_p"));
    }
  } else {
    if (weight==1) {
      sprintf(buffer, "1 %s %u", LOC(lang, "weight_per"), SCALEWEIGHT);
    } else {
      sprintf(buffer, "%u %s %u", weight, LOC(lang, "weight_per_p"), SCALEWEIGHT);
    }
  }

  var.v = strcpy(balloc(strlen(buffer)+1), buffer);
  opush(stack, var);
}

static void
eval_resource(struct opstack ** stack, const void * userdata)
{
  const faction * report = (const faction*)userdata;
  int j = opop(stack).i;
  const struct resource_type * res = (const struct resource_type *)opop(stack).v;
  const char * c = LOC(report->locale, resourcename(res, j!=1));
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_race(struct opstack ** stack, const void * userdata)
{
  const faction * report = (const faction*)userdata;
  int j = opop(stack).i;
  const race * r = (const race *)opop(stack).v;
  const char * c = LOC(report->locale, rc_name(r, j!=1));
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_order(struct opstack ** stack, const void * userdata) /* order -> string */
{
  const faction * report = (const faction*)userdata;
  const struct order * ord = (const struct order *)opop(stack).v;
  static char buf[256];
  size_t len;
  variant var;

  write_order(ord, report->locale, buf, sizeof(buf));
  len = strlen(buf);
  var.v = strcpy(balloc(len+1), buf);
  opush(stack, var);
}

static void
eval_resources(struct opstack ** stack, const void * userdata) /* order -> string */
{
  const faction * report = (const faction*)userdata;
  const struct resource * res = (const struct resource *)opop(stack).v;
  static char buf[256];
  size_t len = sizeof(buf);
  variant var;

  char * edit = buf;
  while (res!=NULL && len > 4) {
    const char * rname = resourcename(res->type, (res->number!=1)?NMF_PLURAL:0);
    int written = snprintf(edit, len, "%d %s", res->number, LOC(report->locale, rname));
    len -= written;
    edit += written;

    res = res->next;
    if (res!=NULL && len>2) {
      strcat(edit, ", ");
      edit += 2;
      len -= 2;
    }
  }
  *edit = 0;
  var.v = strcpy(balloc(edit-buf+1), buf);
  opush(stack, var);
}

static void
eval_direction(struct opstack ** stack, const void * userdata)
{
  const faction * report = (const faction*)userdata;
  int i = opop(stack).i;
  const char * c = LOC(report->locale, (i>=0)?directions[i]:"unknown_direction");
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_skill(struct opstack ** stack, const void * userdata)
{
  const faction * report = (const faction*)userdata;
  skill_t sk = (skill_t)opop(stack).i;
  const char * c = skillname(sk, report->locale);
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
}

static void
eval_int36(struct opstack ** stack, const void * userdata)
{
  int i = opop(stack).i;
  const char * c = itoa36(i);
  size_t len = strlen(c);
  variant var;

  var.v = strcpy(balloc(len+1), c);
  opush(stack, var);
  unused(userdata);
}

void
report_init(void)
{
  /* register functions that turn message contents to readable strings */
  add_function("alliance", &eval_alliance);
  add_function("region", &eval_region);
  add_function("terrain", &eval_terrain);
  add_function("weight", &eval_weight);
  add_function("resource", &eval_resource);
  add_function("race", &eval_race);
  add_function("faction", &eval_faction);
  add_function("ship", &eval_ship);
  add_function("unit", &eval_unit);
  add_function("unit.dative", &eval_unit_dative);
  add_function("unit.name", &eval_unitname);
  add_function("unit.id", &eval_unitid);
  add_function("unit.size", &eval_unitsize);
  add_function("building", &eval_building);
  add_function("skill", &eval_skill);
  add_function("order", &eval_order);
  add_function("direction", &eval_direction);
  add_function("int36", &eval_int36);
  add_function("trail", &eval_trail);
  add_function("spell", &eval_spell);
  add_function("resources", &eval_resources);

  register_reporttype("nr", &report_plaintext, 1<<O_REPORT);
  register_reporttype("txt", &report_template, 1<<O_ZUGVORLAGE);
}

void
report_cleanup(void)
{
  int i;
  for (i=0;i!=FMAXHASH;++i) {
    while (fsee[i]) {
      struct fsee * fs = fsee[i]->nexthash;
      free(fsee[i]);
      fsee[i] = fs;
    }
  }
}

