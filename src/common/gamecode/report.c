/* vi: set ts=2:
 *
 *
 *  Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#define INDENT 0
#define ECHECK_VERSION "4.01"

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
#include <kernel/border.h>
#include <kernel/build.h>
#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/group.h>
#include <kernel/item.h>
#include <kernel/karma.h>
#include <kernel/message.h>
#include <kernel/move.h>
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
#include <util/attrib.h>
#include <util/base36.h>
#include <util/bsdstring.h>
#include <util/goodies.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/message.h>
#include <util/nrmessage.h>
#include <util/rng.h>

#include <libxml/encoding.h>

/* libc includes */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
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

static char *
gamedate_season(const struct locale * lang)
{
  static char buf[256];
  gamedate gd;

  get_gamedate(turn, &gd);

  sprintf(buf, (const char *)LOC(lang, "nr_calendar_season"),
    LOC(lang, weeknames[gd.week]),
    LOC(lang, monthnames[gd.month]),
    gd.year,
    LOC(lang, agename),
    LOC(lang, seasonnames[gd.season]));

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

  do {
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
  } while (*begin);
}

static void
report_spell(FILE * F,  spell *sp, const struct locale * lang)
{
  int bytes, k, itemanz, costtyp;
  int dh = 0;
  char buf[4096];
  char * bufp = buf;
  size_t size = sizeof(buf) - 1;
  const char * params = sp->parameter;

  rnl(F);
  centre(F, spell_name(sp, lang), true);
  rnl(F);
  rparagraph(F, LOC(lang, "nr_spell_description"), 0, 0, 0);
  rparagraph(F, spell_info(sp, lang), 2, 0, 0);

  bytes = (int)strlcpy(bufp, LOC(lang, "nr_spell_type"), size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  
  if (size) { *bufp++ = ' '; --size; }
  if (sp->sptyp & PRECOMBATSPELL) {
    bytes = (int)strlcpy(bufp, LOC(lang, "sptype_precombat"), size);
  } else if (sp->sptyp & COMBATSPELL) {
    bytes = (int)strlcpy(bufp, LOC(lang, "sptype_combat"), size);
  } else if (sp->sptyp & POSTCOMBATSPELL) {
    bytes = (int)strlcpy(bufp, LOC(lang, "sptype_postcombat"), size);
  } else {
    bytes = (int)strlcpy(bufp, LOC(lang, "sptype_normal"), size);
  }
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  *bufp = 0;
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
    if (itemanz > 0) {
      size = sizeof(buf) - 1;
      bufp = buf;
      if (sp->sptyp & SPELLLEVEL) {
        bytes = snprintf(bufp, size, "  %d %s", itemanz, LOC(lang, resourcename(rtype, itemanz!=1)));
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR ) {
          bytes = snprintf(bufp, size, " * %s", LOC(lang, "nr_level"));
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
      } else {
        if (costtyp == SPC_LEVEL || costtyp == SPC_LINEAR ) {
          itemanz *= sp->level;
        }
        bytes = snprintf(bufp, size, "%d %s", itemanz, LOC(lang, resourcename(rtype, itemanz!=1)));
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      *bufp = 0;
      rparagraph(F, buf, 2, 2, '-');
    }
  }

  size = sizeof(buf) - 1;
  bufp = buf;  
  bytes = (int)strlcpy(buf, LOC(lang, "nr_spell_modifiers"), size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  if (sp->sptyp & FARCASTING) {
    bytes = (int)strlcpy(bufp, " Fernzauber", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    dh = 1;
  }
  if (sp->sptyp & OCEANCASTABLE) {
    if (dh == 1) {
      bytes = (int)strlcpy(bufp, ",", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
    bytes = (int)strlcpy(bufp, " Seezauber", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    dh = 1;
  }
  if (sp->sptyp & ONSHIPCAST) {
    if (dh == 1){
      bytes = (int)strlcpy(bufp, ",", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
    bytes = (int)strlcpy(bufp, " Schiffszauber", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    dh = 1;
  }
  if (sp->sptyp & NOTFAMILIARCAST) {
    if (dh == 1) {
      bytes = (int)strlcpy(bufp, ", k", size);
    } else {
      bytes = (int)strlcpy(bufp, " K", size);
    }
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, "ann nicht vom Vertrauten gezaubert werden", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    dh = 1;
  }
  if (dh == 0) {
    bytes = (int)strlcpy(bufp, " Keine", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  *bufp = 0;
  rparagraph(F, buf, 0, 0, 0);

  rparagraph(F, LOC(lang, "nr_spell_syntax"), 0, 0, 0);

  bufp = buf;
  size = sizeof(buf) - 1;
  
  if (sp->sptyp & ISCOMBATSPELL) {
    bytes = (int)strlcpy(bufp, LOC(lang, keywords[K_COMBAT]), size);
  } else {
    bytes = (int)strlcpy(bufp, LOC(lang, keywords[K_CAST]), size);
  }
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  /* Reihenfolge beachten: Erst REGION, dann STUFE! */
  if (sp->sptyp & FARCASTING) {
    bytes = snprintf(bufp, size, " [%s x y]", LOC(lang, parameters[P_REGION]));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  if (sp->sptyp & SPELLLEVEL) {
    bytes = snprintf(bufp, size, " [%s n]", LOC(lang, parameters[P_LEVEL]));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }

  bytes = (int)snprintf(bufp, size, " \"%s\"", spell_name(sp, lang));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  
  while (params && *params) {
    typedef struct starget {
      param_t param;
      int flag;
      const char * vars;
    } starget;
    starget targets[] = { 
      { P_REGION, REGIONSPELL, NULL }, 
      { P_UNIT, UNITSPELL, "par_unit" }, 
      { P_SHIP, SHIPSPELL, "par_ship" },
      { P_BUILDING, BUILDINGSPELL, "par_building" },
      { 0, 0, NULL } 
    };
    starget * targetp;
    char cp = *params++;
    int i, maxparam = 0;
    const char * locp;
    const char * syntaxp = sp->syntax;

    if (cp=='u') {
      targetp = targets+1;
      locp = LOC(lang, targetp->vars);
      bytes = (int)snprintf(bufp, size, " <%s>", locp);
      if (*params=='+') {
        ++params;
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)snprintf(bufp, size, " [<%s> ...]", locp);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    } else if (cp=='s') {
      targetp = targets+2;
      locp = LOC(lang, targetp->vars);
      bytes = (int)snprintf(bufp, size, " <%s>", locp);
      if (*params=='+') {
        ++params;
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)snprintf(bufp, size, " [<%s> ...]", locp);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    } else if (cp=='r') {
      bytes = (int)strlcpy(bufp, " <x> <y>", size);
      if (*params=='+') {
        ++params;
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, " [<x> <y> ...]", size);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    } else if (cp=='b') {
      targetp = targets+3;
      locp = LOC(lang, targetp->vars);
      bytes = (int)snprintf(bufp, size, " <%s>", locp);
      if (*params=='+') {
        ++params;
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)snprintf(bufp, size, " [<%s> ...]", locp);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    } else if (cp=='k') {
      if (*params=='c') {
        /* skip over a potential id */
        ++params;
      }
      for (targetp=targets;targetp->flag;++targetp) {
        if (sp->sptyp&targetp->flag) ++maxparam;
      }
      if (maxparam>1) {
        bytes = (int)strlcpy(bufp, " (", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      i = 0;
      for (targetp=targets;targetp->flag;++targetp) {
        if (sp->sptyp&targetp->flag) {
          if (i++!=0) {
            bytes = (int)strlcpy(bufp, " |", size);
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          }
          if (targetp->param) {
            locp = LOC(lang, targetp->vars);
            bytes = (int)snprintf(bufp, size, " %s <%s>", parameters[targetp->param], locp);
            if (*params=='+') {
              ++params;
              if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
              bytes = (int)snprintf(bufp, size, " [<%s> ...]", locp);
            }
          } else {
            bytes = (int)snprintf(bufp, size, " %s", parameters[targetp->param]);
          }
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
      }
      if (maxparam>1) {
        bytes = (int)strlcpy(bufp, " )", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
    } else if (cp=='i' || cp=='c') {
      const char * cstr;
      assert(syntaxp);
      cstr = strchr(syntaxp, ':');
      if (!cstr) {
        locp = LOC(lang, mkname("spellpar", syntaxp));
      } else {
        char substr[32];
        strncpy(substr, syntaxp, cstr-syntaxp);
        substr[cstr-syntaxp] = 0;
        locp = LOC(lang, mkname("spellpar", substr));
        syntaxp = substr + 1;
      }
      bytes = (int)snprintf(bufp, size, " <%s>", locp);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }
  *bufp = 0;
  rparagraph(F, buf, 2, 0, 0);
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
         * länger als eine zeile ist, muss dieses hier abgetrennt werden. */

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

  for (;a;a=a->next) {
    char buf[4096];

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
  char buf[8192];

  if (u->race == new_race[RC_SPELL]) return;

  {
    rnl(F);
    dh = bufunit(f, u, indent, mode, buf, sizeof(buf));
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
      if (!categorized || strcmp(nr_section(m->msg), section->name)==0) {
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
    if (!categorized) break;
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
      char buf[256];
      RENDER(f, buf, sizeof(buf), ("battle::header", "region", bm->r));
      rnl(F);
      centre(F, buf, true);
      rnl(F);
      rp_messages(F, bm->msgs, f, 0, false);
      bm = bm->next;
    }
  }
}

static void
prices(FILE * F, const region * r, const faction * f)
{
  const luxury_type *sale=NULL;
  struct demand * dmd;
  message * m;
  int bytes, n = 0;
  char buf[4096], * bufp = buf;
  size_t size = sizeof(buf) - 1;

  if (r->land==NULL || r->land->demands==NULL) return;
  for (dmd=r->land->demands;dmd;dmd=dmd->next) {
    if (dmd->value==0) sale = dmd->type;
    else if (dmd->value > 0) n++;
  }
  assert(sale!=NULL);

  m = msg_message("nr_market_sale", "product price",
    sale->itype->rtype, sale->price);

  bytes = (int)nr_render(m, f->locale, bufp, size, f);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  msg_release(m);

  if (n > 0) {
    bytes = (int)strlcpy(bufp, " ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_intro"), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, " ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

    for (dmd=r->land->demands;dmd;dmd=dmd->next) if(dmd->value > 0) {
      m = msg_message("nr_market_price", "product price",
        dmd->type->itype->rtype, dmd->value * dmd->type->price);
      bytes = (int)nr_render(m, f->locale, bufp, size, f);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      msg_release(m);
      n--;
      if (n == 0) {
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_end"), size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      else if (n == 1) {
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_final"), size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      } else {
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_trade_next"), size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
    }
  }
  /* Schreibe Paragraphen */
  *bufp = 0;
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
  char buf[8192];
  char * bufp = buf;
  size_t size = sizeof(buf);
  int bytes;

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

  bytes = (int)f_regionid(r, f, bufp, size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  
  if (partial == 1) {
    bytes = (int)strlcpy(bufp, " (durchgereist)", size);
  }
  else if (partial == 3) {
    bytes = (int)strlcpy(bufp, " (benachbart)", size);
  }
  else if (partial == 2) {
    bytes = (int)strlcpy(bufp, " (vom Turm erblickt)", size);
  } else {
    bytes = 0;
  }
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  
  /* Terrain */
  bytes = (int)strlcpy(bufp, ", ", size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  
  tname = terrain_name(r);
  bytes = (int)strlcpy(bufp, LOC(f->locale, tname), size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  
  /* Trees */
  trees  = rtrees(r,2);
  saplings = rtrees(r,1);
  if (production(r)) {
    if (trees > 0 || saplings > 0) {
      bytes = snprintf(bufp, size, ", %d/%d ", trees, saplings);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

      if (fval(r, RF_MALLORN)) {
        if (trees == 1) {
          bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_mallorntree"), size);
        } else {
          bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_mallorntree_p"), size);
        }
      }
      else if (trees == 1) {
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_tree"), size);
      } else {
        bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_tree_p"), size);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
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
        bytes = snprintf(bufp, size, ", %d %s/%d", visible, 
                         LOC(f->locale, res->type->name),
                         res->level + itype->construction->minskill - 1);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
    }
  }

  /* peasants & silver */
  if (rpeasants(r)) {
    int n = rpeasants(r);
    bytes = snprintf(bufp, size, ", %d", n);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    
    if (fval(r, RF_ORCIFIED)) {
      bytes = (int)strlcpy(bufp, " ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      
      bytes = (int)strlcpy(bufp, LOC(f->locale, n==1?"rc_orc":"rc_orc_p"), size);
    } else {
      bytes = (int)strlcpy(bufp, " ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, LOC(f->locale, n==1?"peasant":"peasant_p"), size);
    }
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  if (rmoney(r) && partial == 0) {
    bytes = snprintf(bufp, size, ", %d ", rmoney(r));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, LOC(f->locale, resourcename(oldresourcetype[R_SILVER], rmoney(r)!=1)), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  /* Pferde */

  if (rhorses(r)) {
    bytes = snprintf(bufp, size, ", %d ", rhorses(r));
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, LOC(f->locale, resourcename(oldresourcetype[R_HORSE], (rhorses(r)>1)?GR_PLURAL:0)), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  bytes = (int)strlcpy(bufp, ".", size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  

  if (r->display && r->display[0]) {
    bytes = (int)strlcpy(bufp, " ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();    
    bytes = (int)strlcpy(bufp, r->display, size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

    n = r->display[strlen(r->display) - 1];
    if (n != '!' && n != '?' && n != '.') {
      bytes = (int)strlcpy(bufp, ".", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }

  {
    const faction * owner = region_owner(r);
    if (owner!=NULL) {
      bytes = snprintf(bufp, size, " Die Region ist im Besitz von %s.",
                       factionname(owner));
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }
  a = a_find(r->attribs, &at_overrideroads);

  if (a) {
    bytes = (int)strlcpy(bufp, " ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, (char *)a->data.v, size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  } else {
    int nrd = 0;

    /* Nachbarregionen, die gesehen werden, ermitteln */
    for (d = 0; d != MAXDIRECTIONS; d++) {
      if (see[d] && rconnect(r, d)) nrd++;
    }
    /* list directions */

    dh = false;
    for (d = 0; d != MAXDIRECTIONS; d++) if (see[d]) {
      region * r2 = rconnect(r, d);
      if(!r2) continue;
      nrd--;
      if (dh) {
        char regname[4096];
        if (nrd == 0) {
          bytes = (int)strlcpy(bufp, " ", size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();          
          bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_nb_final"), size);
        } else {
          bytes = (int)strlcpy(bufp, LOC(f->locale, "nr_nb_next"), size);
        }
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, LOC(f->locale, directions[d]), size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        f_regionid(r2, f, regname, sizeof(regname));
        bytes = snprintf(bufp, size, trailinto(r2, f->locale), regname);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      else {
        bytes = (int)strlcpy(bufp, " ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        MSG(("nr_vicinitystart", "dir region", d, r2), bufp, size, f->locale, f);
        bufp += strlen(bufp);
        dh = true;
      }
    }
    /* Spezielle Richtungen */
    for (a = a_find(r->attribs, &at_direction);a && a->type==&at_direction;a=a->next) {
      spec_direction * d = (spec_direction *)(a->data.v);
      bytes = (int)strlcpy(bufp, " ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, LOC(f->locale, d->desc), size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, " (\"", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, LOC(f->locale, d->keyword), size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, "\")", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, ".", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      dh = 1;
    }
  }
  rnl(F);
  *bufp = 0;
  rparagraph(F, buf, 0, 0, 0);

  if (partial==0 && rplane(r) == get_astralplane() &&
      !is_cursed(r->attribs, C_ASTRALBLOCK, 0)) {
    /* Sonderbehandlung Teleport-Ebene */
    region_list *rl = astralregions(r, inhabitable);
    region_list *rl2;

    if (rl) {
      bufp = buf;
      size = sizeof(buf) - 1;
      bytes = (int)strlcpy(bufp, "Schemen der Regionen ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      rl2 = rl;
      while (rl2) {
        bytes = (int)f_regionid(rl2->data, f, bufp, size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        rl2 = rl2->next;
        if (rl2) {
          bytes = (int)strlcpy(bufp, ", ", size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
      }
      bytes = (int)strlcpy(bufp, " sind erkennbar.", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      free_regionlist(rl);
      /* Schreibe Paragraphen */
      rnl(F);
      *bufp = 0;
      rparagraph(F, buf, 0, 0, 0);
    }
  }

  n = 0;

  /* Wirkungen permanenter Sprüche */
  print_curses(F, f, r, TYP_REGION,0);

  /* Produktionsreduktion */
  a = a_find(r->attribs, &at_reduceproduction);
  if (a) {
    const char * str = LOC(f->locale, "nr_reduced_production");
    rparagraph(F, str, 0, 0, 0);
  }

  if (edges) rnl(F);
  for (e=edges;e;e=e->next) {
    boolean first = true;
    bufp = buf;
    size = sizeof(buf) - 1;
    for (d=0;d!=MAXDIRECTIONS;++d) {
      if (!e->exist[d]) continue;
      if (first) bytes = (int)strlcpy(bufp, "Im ", size);
      else if (e->lastd==d) bytes = (int)strlcpy(bufp, " und im ", size);
      else bytes = (int)strlcpy(bufp, ", im ", size );
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, LOC(f->locale, directions[d]), size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      first = false;
    }
    if (!e->transparent) bytes = (int)strlcpy(bufp, " versperrt ", size);
    else bytes = (int)strlcpy(bufp, " befindet sich ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, e->name, size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    if (!e->transparent) bytes = (int)strlcpy(bufp, " die Sicht.", size);
    else bytes = (int)strlcpy(bufp, ".", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    *bufp = 0;
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
  int number = 0, p = rpeasants(r);
  message * m;
  item *itm, *items = NULL;
  char buf[4096];

  /* count */
  for (u = r->units; u; u = u->next) {
    if (u->faction == f && u->race != new_race[RC_SPELL]) {
      for (itm=u->items;itm;itm=itm->next) {
        i_change(&items, itm->type, itm->number);
      }
      number += u->number;
    }
  }
  /* print */
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
  /* info about units */

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
    char buf[8192];
    char * bufp = buf;
    int bytes;
    size_t size = sizeof(buf) - 1;

    /* How many are we listing? For grammar. */
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
    rnl(F);

    for (a = abegin; a && a->type==&at_travelunit; a = a->next) {
      unit * u = (unit*)a->data.v;

      if (r!=u->region && (u->ship==NULL || fval(u, UFL_OWNER))) {
        if (cansee_durchgezogen(f, r, u, 0)) {
          ++counter;
          if (u->ship != NULL) {
            if (counter == 1) {
              bytes = (int)strlcpy(bufp, "Die ", size);
            } else {
              bytes = (int)strlcpy(bufp, "die ", size);
            }
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, shipname(u->ship), size);
          } else {
            bytes = (int)strlcpy(bufp, unitname(u), size);
          }
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          
          if (counter + 1 < maxtravel) {
            bytes = (int)strlcpy(bufp, ", ", size);
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          } else if (counter + 1 == maxtravel) {
            bytes = (int)strlcpy(bufp, LOC(f->locale, "list_and"), size);
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          }
        }
      }
    }
    if (maxtravel == 1) {
      bytes = (int)strlcpy(bufp, " hat die Region durchquert.", size);
    } else {
      bytes = (int)strlcpy(bufp, " haben die Region durchquert.", size);
    }
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    *bufp = 0;
    rparagraph(F, buf, 0, 0, 0);
  }
}

static int
buildingmaintenance(const building * b, const resource_type * rtype)
{
  const building_type * bt = b->type;
  int c, cost=0;
  static boolean init = false;
  static const curse_type * nocost_ct;
  if (!init) { init = true; nocost_ct = ct_find("nocostbuilding"); }
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
report_template(const char * filename, report_context * ctx, const char * charset)
{
  faction * f = ctx->f;
  region *r;
  plane *pl;
  FILE * F = fopen(filename, "wt");
  seen_region * sr = NULL;
  char buf[8192], * bufp;
  size_t size;
  int bytes;
  
  int enc = xmlParseCharEncoding(charset);

  if (F==NULL) {
    perror(filename);
    return -1;
  }

  if (enc==XML_CHAR_ENCODING_UTF8) {
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
    fwrite(utf8_bom, 1, 3, F);
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

        bufp = buf;
        size = sizeof(buf) - 1;
        bytes = snprintf(bufp, size, "%s %s;    %s [%d,%d$",
                         LOC(u->faction->locale, parameters[P_UNIT]),
                         unitid(u), u->name, u->number, get_money(u));
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        if (u->building != NULL && fval(u, UFL_OWNER)) {
          building * b = u->building;
          int cost = buildingmaintenance(b, r_silver);

          if (cost > 0) {
            bytes = (int)strlcpy(bufp, ",U", size);
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
            bytes = (int)strlcpy(bufp, itoa10(cost), size);
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          }
        } else if (u->ship) {
          if (fval(u, UFL_OWNER)) {
            bytes = (int)strlcpy(bufp, ",S", size);
          } else {
            bytes = (int)strlcpy(bufp, ",s", size);
          }
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          bytes = (int)strlcpy(bufp, shipid(u->ship), size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
        if (lifestyle(u) == 0) {
          bytes = (int)strlcpy(bufp, ",I", size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
        bytes = (int)strlcpy(bufp, "]", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

        *bufp = 0;
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
show_allies(const faction * f, const ally * allies, char * buf, size_t size)
{
  int allierte = 0;
  int i=0, h, hh = 0;
  int bytes, dh = 0;
  const ally * sf;
  char * bufp = buf; /* buf already contains data */

  --size; /* leave room for a null-terminator */

  for (sf = allies; sf; sf = sf->next) {
    int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
    if (mode > 0) ++allierte;
  }

  for (sf = allies; sf; sf = sf->next) {
    int mode = alliedgroup(NULL, f, sf->faction, sf, HELP_ALL);
    if (mode <= 0) continue;
    i++;
    if (dh) {
      if (i == allierte) {
        bytes = (int)strlcpy(bufp, LOC(f->locale, "list_and"), size);
      } else {
        bytes = (int)strlcpy(bufp, ", ", size);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
    dh = 1;
    hh = 0;
    bytes = (int)strlcpy(bufp, factionname(sf->faction), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, " (", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    if ((mode & HELP_ALL) == HELP_ALL) {
      bytes = (int)strlcpy(bufp, "Alles", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    } else {
      for (h = 1; h < HELP_ALL; h *= 2) {
        int p = MAXPARAMS;
        if ((mode & h) == h) {
          switch (h) {
          case HELP_TRAVEL:
            p = P_TRAVEL;
            break;
          case HELP_MONEY:
            p = P_MONEY;
            break;
          case HELP_FIGHT:
            p = P_FIGHT;
            break;
          case HELP_GIVE:
            p = P_GIVE;
            break;
          case HELP_GUARD:
            p = P_GUARD;
            break;
          case HELP_FSTEALTH:
            p = P_FACTIONSTEALTH;
            break;
          }
        }
        if (p!=MAXPARAMS) {
          if (hh) {
            bytes = (int)strlcpy(bufp, ", ", size);
            if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          }
          bytes = (int)strlcpy(bufp, parameters[p], size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          hh = 1;
        }
      }
    }
    bytes = (int)strlcpy(bufp, ")", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }
  bytes = (int)strlcpy(bufp, ".", size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  *bufp = 0;
}

static void
allies(FILE * F, const faction * f)
{
  const group * g = f->groups;
  char buf[16384];

  if (f->allies) {
    int bytes;
    size_t size = sizeof(buf);
    if (!f->allies->next) {
      bytes = (int)strlcpy(buf, "Wir helfen der Partei ", size);
    } else {
      bytes = (int)strlcpy(buf, "Wir helfen den Parteien ", size);
    }
    size -= bytes;
    show_allies(f, f->allies, buf + bytes, size);
    rparagraph(F, buf, 0, 0, 0);
    rnl(F);
  }

  while (g) {
    if (g->allies) {
      int bytes;
      size_t size = sizeof(buf);
      if (!g->allies->next) {
        bytes = snprintf(buf, size, "%s hilft der Partei ", g->name);
      } else {
        bytes = snprintf(buf, size, "%s hilft den Parteien ", g->name);
      }
      size -= bytes;
      show_allies(f, g->allies, buf + bytes, size);
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
    char buf[8192];
    char * bufp = buf;
    size_t size = sizeof(buf) - 1;
    int bytes;
    
    bytes = (int)strlcpy(bufp, "Die Region wird von ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

    for (i = 0; i!=nextguard+(tarned?1:0); ++i) {
      if (i!=0) {
        if (i == nextguard-(tarned?0:1)) {
          bytes = (int)strlcpy(bufp, LOC(see->locale, "list_and"), size);
        } else {
          bytes = (int)strlcpy(bufp, ", ", size);
        }
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      if (i<nextguard) {
        bytes = (int)strlcpy(bufp, factionname(guardians[i]), size);
      } else {
        bytes = (int)strlcpy(bufp, "unbekannten Einheiten", size);
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
    bytes = (int)strlcpy(bufp, " bewacht.", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    rnl(F);
    *bufp = 0;
    rparagraph(F, buf, 0, 0, 0);
  }
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
      char buf[8192];
      char label = '-';

      sprintf(buf, "%s: %s; %s", factionname(f), f->email, f->banner);
      if (ALLIED(uf, f)) label = '*';
      else if (alliedfaction(NULL, uf, f, HELP_ALL)) label = '+';
      rparagraph(F, buf, 4, 0, label);
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
  int i, bytes;
  unit *u;
  const char * bname;
  const struct locale * lang = NULL;
  const building_type * type = b->type;
  static const struct building_type * bt_illusion;
  char buffer[8192], * bufp = buffer;
  size_t size = sizeof(buffer) - 1;

  if (!bt_illusion) bt_illusion = bt_find("illusion");
  if (f) lang = f->locale;

  bytes = snprintf(bufp, size, "%s, %s %d, ", buildingname(b), LOC(f->locale, "nr_size"), b->size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

  if (b->type==bt_illusion) {
    attrib * a = a_find(b->attribs, &at_icastle);
    if (a!=NULL) {
      type = ((icastle_data*)a->data.v)->type;
    }
  }
  bname = LOC(lang, buildingtype(type, b, b->size));
  bytes = (int)strlcpy(bufp, bname, size);
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  if (type!=b->type) {
    unit * owner = buildingowner(r, b);
    if (owner && owner->faction==f) {
      /* illusion. report real type */
      bname = LOC(lang, buildingtype(b->type, b, b->size));
      bytes = snprintf(bufp, size, " (%s)", bname);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }

  if (b->size < type->maxsize) {
    bytes = (int)strlcpy(bufp, " (im Bau)", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }

  if (b->besieged > 0 && mode>=see_lighthouse) {
    bytes = (int)strlcpy(bufp, ", belagert von ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, itoa10(b->besieged), size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, " Personen ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    if (b->besieged >= b->size * SIEGEFACTOR) {
      bytes = (int)strlcpy(bufp, "(abgeschnitten)", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    }
  }
  i = 0;
  if (b->display && b->display[0]) {
    bytes = (int)strlcpy(bufp, "; ", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
    bytes = (int)strlcpy(bufp, b->display, size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
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

  if (i != '!' && i != '?' && i != '.') {
    bytes = (int)strlcpy(bufp, ".", size);
    if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  }

#endif
  *bufp = 0;
  rparagraph(F, buffer, 2, 0, 0);

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
report_plaintext(const char * filename, report_context * ctx, const char * charset)
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
  int bytes, ix = want(O_STATISTICS);
  int wants_stats = (f->options & ix);
  FILE * F = fopen(filename, "wt");
  seen_region * sr = NULL;
  char buf[8192];
  char * bufp;
  int enc = xmlParseCharEncoding(charset);
  size_t size;

  if (F==NULL) {
    perror(filename);
    return -1;
  }
  if (enc==XML_CHAR_ENCODING_UTF8) {
    const unsigned char utf8_bom[4] = { 0xef, 0xbb, 0xbf };
    fwrite(utf8_bom, 1, 3, F);
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
    const char * s;
    if (f->age <= 1) {
      ADDMSG(&f->msgs, msg_message("changepasswd",
        "value", f->passw));
    }
    RENDER(f, buf, sizeof(buf), ("newbie_password", "password", f->passw));
    rnl(F);
    centre(F, buf, true);
    s = locale_getstring(f->locale, "newbie_info_1");
    if (s) {
      rnl(F);
      centre(F, s, true);
    }
    s = locale_getstring(f->locale, "newbie_info_2");
    if (s) {
      rnl(F);
      centre(F, s, true);
    }
    if ((f->options & want(O_COMPUTER)) == 0) {
      f->options |= want(O_COMPUTER);
      s = locale_getstring(f->locale, "newbie_info_3");
      if (s) {
        rnl(F);
        centre(F, s, true);
      }
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

  /* Insekten-Winter-Warnung */
  if (f->race == new_race[RC_INSECT]) {
    static int thisseason = -1;
    if (thisseason<0) thisseason = get_gamedate(turn+1, 0)->season;
    if (thisseason == 0) {
      centre(F, LOC(f->locale, "nr_insectwinter"), true);
      rnl(F);
    } else {
      static int nextseason = -1;
      if (nextseason<0) nextseason = get_gamedate(turn+2, 0)->season;
      if (nextseason == 0) {
        centre(F, LOC(f->locale, "nr_insectfall"), true);
        rnl(F);
      }
    }
  }

  bufp = buf;
  size = sizeof(buf) - 1;
  bytes = snprintf(buf, size,"%s:", LOC(f->locale, "nr_options"));
  if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
  for (op = 0; op != MAXOPTIONS; op++) {
    if (f->options & want(op)) {
      bytes = (int)strlcpy(bufp, " ", size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      bytes = (int)strlcpy(bufp, LOC(f->locale, options[op]), size);
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
#ifdef AT_OPTION
      if(op == O_NEWS) {
        attrib *a = a_find(f->attribs, &at_option_news);
        if(!a) {
          /* Zur Altlastenbeseitigung */
          f->options = f->options & ~op;
        } else {
          int sec = a->data.i;
          int i;
          bytes = (int)strlcpy(bufp, "(", size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          for (i=1; sec != 0; i *= 2) {
            if(sec & i) {
              bytes = (int)strlcpy(bufp, itoa10(i), size);
              if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
              sec = sec & ~i;
              if (sec) {
                bytes = (int)strlcpy(bufp, ",", size);
                if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
              }
            }
          }
          bytes = (int)strlcpy(bufp, ")", size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
      }
#endif
      flag++;
    }
  }
  if (flag > 0) {
    rnl(F);
    *bufp = 0;
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
      snprintf(buf, sizeof(buf), "%s %d", LOC(f->locale, "nr_level"), ptype->level);
      centre(F, buf, true);
      rnl(F);
      
      bufp = buf;
      size = sizeof(buf) - 1;
      bytes = snprintf(bufp, size, "%s: ", LOC(f->locale, "nr_herbsrequired"));
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      while (m->number) {
        bytes = (int)strlcpy(bufp, LOC(f->locale, resourcename(m->rtype, 0)), size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        ++m;
        if (m->number)
        bytes = (int)strlcpy(bufp, ", ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      *bufp = 0;
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
      
      bufp = buf;
      size = sizeof(buf) - 1;
      if (of == f) {
        int n = 0, p = 0;
        getshipweight(sh, &n, &p);
        n = (n+99) / 100; /* 1 Silber = 1 GE */

        bytes = snprintf(bufp, size, "%s, %s, (%d/%d)", shipname(sh),
          LOC(f->locale, sh->type->name[0]), n, shipcapacity(sh) / 100);
      } else {
        bytes = snprintf(bufp, size, "%s, %s", shipname(sh), LOC(f->locale, sh->type->name[0]));
      }
      if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();

      assert(sh->type->construction->improvement==NULL); /* sonst ist construction::size nicht ship_type::maxsize */
      if (sh->size!=sh->type->construction->maxsize) {
        bytes = snprintf(bufp, size, ", %s (%d/%d)",
          LOC(f->locale, "nr_undercons"), sh->size,
          sh->type->construction->maxsize);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      if (sh->damage) {
        int percent = (sh->damage*100+DAMAGE_SCALE-1)/(sh->size*DAMAGE_SCALE);
        bytes = snprintf(bufp, size, ", %d%% %s", percent, LOC(f->locale, "nr_damaged"));
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      if (!fval(r->terrain, SEA_REGION)) {
        if (sh->coast != NODIRECTION) {
          bytes = (int)strlcpy(bufp, ", ", size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
          bytes = (int)strlcpy(bufp, LOC(f->locale, coasts[sh->coast]), size);
          if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        }
      }
      ch = 0;
      if (sh->display && sh->display[0]) {
        bytes = (int)strlcpy(bufp, "; ", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        bytes = (int)strlcpy(bufp, sh->display, size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
        ch = sh->display[strlen(sh->display) - 1];
      }
      if (ch != '!' && ch != '?' && ch != '.') {
        bytes = (int)strlcpy(bufp, ".", size);
        if (wrptr(&bufp, &size, bytes)!=0) WARN_STATIC_BUFFER();
      }
      *bufp = 0;
      rparagraph(F, buf, 2, 0, 0);

      print_curses(F, f, sh, TYP_SHIP, 4);

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

/******* end summary ******/

void
report_init(void)
{
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

