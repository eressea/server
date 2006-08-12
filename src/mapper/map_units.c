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

/* wenn config.h nicht vor curses included wird, kompiliert es unter windows nicht */

#include <config.h>
#include <curses.h>
#include <eressea.h>
#include "mapper.h"

/* kernel includes */
#include <goodies.h>
#include <building.h>
#include <faction.h>
#include <item.h>
#include <movement.h>
#include <race.h>
#include <region.h>
#include <reports.h>
#include <ship.h>
#include <skill.h>
#include <unit.h>
#include <base36.h>

#include <attributes/movement.h>

/* libc includes */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define findunit(f,r) findunitg(f,r)

static region *shipregion;

void
SpecialFunctionUnit(unit *u)
{
  WINDOW *win;

  win = openwin(60, 5, "< Specials Units >");
  wmove(win, 1, 2);
  wAddstr("B - give balloon, M - set noMovement");
  wmove(win, 2, 2);
  wrefresh(win);
  switch(getch()) {
  case 'b':
  case 'B':
    {
      ship *sh = new_ship(st_find("balloon"), u->faction->locale, u->region);
      set_string(&sh->name, "Ballon");
      set_string(&sh->display, "Eine große leuchtendrote Werbeaufschrift "
          "für den Xontormia Express prangt auf einer Seite des Luftgefährts.");
      sh->size = 5;
      leave(u->region, u);
      u->ship = sh;
      fset(u, UFL_OWNER);
    }
    break;
  case 'm':
  case 'M':
    set_movement(&u->attribs, MV_CANNOTMOVE);
    break;
  }
  delwin(win);
}

unit *
make_new_unit(region * r)
{
  unit *u;
  faction *f;
  WINDOW *win;
  char *fac_nr36;
  int i, p, anz, q, y;
  win = openwin(SX - 10, 17, "< Neue Einheit erschaffen >");

  if (r->units)
    p = r->units->faction->no;
  else
    p = 0;

  do {
    fac_nr36 = my_input(win, 2, 1, "Parteinummer: ", itoa36(p));
    if(fac_nr36 == NULL || *fac_nr36 == 0) {
      delwin(win);
      return 0;
    }
    f = findfaction(atoi36(fac_nr36));
  } while (!f);
  wmove(win, 1, 2);
  wclrtoeol(win);
  wmove(win, 1, win->_maxx);
  waddch(win, '|');
  wmove(win, 1, 2);
  wAddstr(factionname(f));
  wrefresh(win);

  anz = map_input(win, 2, 2, "Anzahl Personen", 0, 9999, 1);
  if (anz > 0) {
    q = 0;
    y = 4;
    wmove(win, y, 4);
    for (i = 0; i < MAXRACES; i++) {
      if(new_race[i] == NULL) continue;
      sprintf(buf, "%d=%s; ", i, new_race[i]->_name[0]);
      q += strlen(buf);
      if (q > SX - 20) {
        q = strlen(buf);
        y++;
        wmove(win, y, 4);
      }
      wAddstr(buf);
    }
    q = map_input(win, 2, 3, "Rasse", 0, MAXRACES-1, old_race(f->race));

    if(new_race[q] != NULL) {
      u = createunit(r, f, anz, new_race[q]);
      if (p==0)
        u->age = (short)map_input(win, 2, 4, "Alter", 1, 99, 1);
      else
        u->age = 0;
      for (; y > 3; y--) {
        wmove(win, y, 4);
        wclrtoeol(win);
        wrefresh(win);
        wmove(win, y, win->_maxx);
        waddch(win, '|');
        wrefresh(win);
      }

      buf[0] = 0;
      strcpy(buf, my_input(win, 2, 4, "Name: ", NULL));
      if (buf[0])
        set_string(&u->name, buf);
      if (!strlen(u->name))
        set_string(&u->name, buf);
      set_money(u, map_input(win, 2, 5, "Silber", 0, 999999, anz * 10));
    } else
      u = NULL;
  } else
    u = NULL;

  delwin(win);
  return u;
}

void
copy_unit(unit * from, unit * to)
{
  memcpy(to, from, sizeof(unit));
}


static int
input_string(const char * text, char * result, size_t len) {
  WINDOW * wn;
  int res;
  wn = newwin(1, SX, SY-1, 0);
  werase(wn);
  wmove(wn, 0, 0);
  wattron(wn, A_BOLD);
  waddstr(wn, (char*)text);
  waddstr(wn, ": ");
  echo();
  curs_set(1);
  res = wgetnstr(wn, result, len);
  curs_set(0);
  noecho();
  delwin(wn);
  return res;
}

static boolean i_modif = false;

static void
chg_item(selection * s, void * data) {
  unit * u = (unit *)data;
  int i;

  i = input_string("Anzahl", buf, 80);
  if (i==ERR) return;
  i = atoi(buf);
  if (i) {
    const item_type * itype = (const item_type *)s->data;
    item * itm = i_change(&u->items, itype, i);
    int k = itm?itm->number:0;
    sprintf(buf, "%s: %d", locale_string(NULL, resourcename(itype->rtype, GR_PLURAL)), k);
    i_modif = true;
    free(s->str);
    s->str = strdup(buf);
  }
}

boolean
modify_items(unit * u)
{
  selection *ilist = NULL;
#if 0
  selection **ilast = &ilist;
  const item_type * itype = itemtypes;

  while (itype!=NULL) {
    selection * prev = NULL;
    item * itm = *i_find(&u->items, itype);
    int q = itm?itm->number:0;
    sprintf(buf, "%s: %d", locale_string(NULL, resourcename(itype->rtype, GR_PLURAL)), q);
    for (ilast = &ilist; *ilast; ilast=&(*ilast)->next) {
      if (strcmp((*ilast)->str, buf)>0) break;
      prev = *ilast;
    }
    insert_selection(ilast, prev, buf, (void*)itype);
    itype=itype->next;
    while (ilist->prev!=NULL) ilist=ilist->prev;
  }
#endif
  i_modif = false;
  do_selection(ilist, "Gegenstände", chg_item, (void*)u);
  while (ilist) {
    selection * s = ilist;
    ilist=ilist->next;
    free((void*)s->str);
    free(s);
  }
  return i_modif;
}

#if 0
char
modify_zauber(unit * u)
{
  int q, L;
  char i, x, modif = 0;
  WINDOW *wn;
  char *ZL[MAXSPELLS];

  if (SY > MAXSPELLS + 2)
    L = MAXSPELLS;
  else
    L = 10;

  wn = newwin(L + 2, 50, (SY - L - 2) / 2, (SX - 50) / 2);

  wclear(wn);
  /* wborder(wn, '|', '|', '-', '-', '.', '.', '`', '\''); */
  wborder(wn, 0, 0, 0, 0, 0, 0, 0, 0);
  wmove(wn, 0, 3);
  waddnstr(wn, "< Zauber >", -1);
  for (i = 0; i < MAXSPELLS; i++) {
    if (get_spell(u, i))
      sprintf(buf, "%s", spellnames[i]);
    else
      sprintf(buf, " (%s)", spellnames[i]);
    ZL[i] = strdup(buf);
    modif = 1;
    if (L > 10) {
      wmove(wn, i + 1, 4);
      waddnstr(wn, ZL[i], -1);
    }
  }

  x = 0;
  do {
    if (L == 10) {
      for (i = 0; i < L && i + x < MAXSPELLS; i++) {
        wmove(wn, i + 1, 2);
        wclrtoeol(wn);
        if (i == 0)
          wattron(wn, A_BOLD);
        waddnstr(wn, ZL[i + x], -1);
        if (i == 0)
          wattroff(wn, A_BOLD);
      }
      if (i < L)
        for (; i < L; i++) {
          wmove(wn, i + 1, 2);
          wclrtoeol(wn);
        }
      /* wborder(wn, '|', '|', '-', '-', '.', '.', '`', '\''); */
        wborder(wn, 0, 0, 0, 0, 0, 0, 0, 0);
      wmove(wn, 0, 3);
      waddnstr(wn, "< Zauber >", -1);
    } else {
      wmove(wn, x + 1, 1);
      waddnstr(wn, "-> ", -1);
      wattron(wn, A_BOLD);
      waddnstr(wn, ZL[x], -1);
      wattroff(wn, A_BOLD);
      wmove(wn, x + 1, 1);
    }
    wrefresh(wn);

    q = getch();

    if (L > 10) {
      waddnstr(wn, "   ", -1);
      waddnstr(wn, ZL[x], -1);
    }
    switch (q) {
    case KEY_DOWN:
      if (x < MAXSPELLS - 1)
        x++;
      else
        beep();
      break;
    case KEY_UP:
      if (x > 0)
        x--;
      else
        beep();
      break;
    case 10:
    case 13:
      if (get_spell(u, x)) {
        sprintf(buf, " (%s)", spellnames[x]);
        set_spell(u, x, false);
      } else {
        sprintf(buf, "%s", spellnames[x]);
        set_spell(u, x, true);
      }
      free(ZL[x]);
      ZL[x] = strdup(buf);
      modif = 1;
      break;
    }
  } while (q != 27 && q != 'q');
  delwin(wn);
  return modif;
}
#endif

char
modify_talente(unit * u, region * r)
{
  int q, L;
  item_t i;
  skill_t sk;
  unsigned char x;
  char modif=0;
  WINDOW *wn;
  char *TL[MAXSKILLS];
  if (SY > MAXSKILLS + 2)
    L = MAXSKILLS;
  else
    L = 10;

  wn = newwin(L + 2, 40, (SY - L - 2) / 2, (SX - 40) / 2);

  wclear(wn);
  /* wborder(wn, '|', '|', '-', '-', '.', '.', '`', '\''); */
  wborder(wn, 0, 0, 0, 0, 0, 0, 0, 0);
  wmove(wn, 0, 3);
  waddnstr(wn, "< Talente >", -1);
  for (sk = 0; sk != MAXSKILLS; ++sk) {
    sprintf(buf, "%s %d", skillname(sk, NULL), eff_skill(u, sk, r));
    TL[sk] = strdup(buf);
    if (L > 10) {
      wmove(wn, sk + 1, 4);
      waddnstr(wn, TL[sk], -1);
    }
  }

  x = 0;
  do {
    if (L == 10) {
      for (i = 0; i < L && i + x < MAXSKILLS; i++) {
        wmove(wn, i + 1, 2);
        wclrtoeol(wn);
        if (i == 0)
          wattron(wn, A_BOLD);
        waddnstr(wn, TL[i + x], -1);
        if (i == 0)
          wattroff(wn, A_BOLD);
      }
      if (i < L)
        for (; i < L; i++) {
          wmove(wn, i + 1, 2);
          wclrtoeol(wn);
        }
      /* wborder(wn, '|', '|', '-', '-', '.', '.', '`', '\''); */
      wborder(wn, 0, 0, 0, 0, 0, 0, 0, 0);
      wmove(wn, 0, 3);
      waddnstr(wn, "< Talente >", -1);
    } else {
      wmove(wn, x + 1, 1);
      waddnstr(wn, "-> ", -1);
      wattron(wn, A_BOLD);
      waddnstr(wn, TL[x], -1);
      wattroff(wn, A_BOLD);
      wmove(wn, x + 1, 1);
    }
    wrefresh(wn);

    q = getch();

    if (L > 10) {
      waddnstr(wn, "   ", -1);
      waddnstr(wn, TL[x], -1);
    }
    switch (q) {
      case KEY_DOWN:
        if (x < MAXSKILLS - 1)
          x++;
        else
          beep();
        break;
      case KEY_UP:
        if (x > 0)
          x--;
        else
          beep();
        break;
      case 10:
      case 13:
        if (L == 10) {
          wmove(wn, x + 2, 2);
          wclrtoeol(wn);
          wmove(wn, x + 2, 39);
          waddch(wn, '|');
          wrefresh(wn);
          q = map_input(wn, 2, 2, "Talentstufe", 0, 30, get_level(u, x));
        } else {
          q = map_input(0, 0, 0, "Talentstufe", 0, 30, get_level(u, x));
          touchwin(mywin);
          touchwin(wn);
          wrefresh(mywin);  /* altes Fenster überbügeln */
        }
        set_level(u, x, q);
        sprintf(buf, "%s %d", skillname(x, NULL), eff_skill(u, x, r));
        free(TL[x]);
        modif = 1;
        TL[x] = strdup(buf);
        q = 0;  /* sicherheitshalber, weil 27 oder 'q' -> Ende */
        break;
    }
  } while (q != 27 && q != 'q');
  delwin(wn);
  return modif;
}

boolean
modify_unit(region * r, unit * modunit)
{
  WINDOW *win;
  unit *u;
  int q, a, x, y, zt, zg;
  char modif = 0;
  int at;
  const item * itm;

  wclear(mywin);
  sprintf(buf, "< Einheit modifizieren: %s,%s >",
      Unitid(modunit), factionname(modunit->faction));
  movexy((SX - strlen(buf)) / 2, 1);
  addstr(buf);

  u = calloc(1, sizeof(unit));
  copy_unit(modunit, u);

  do {
    skill_t sk;
    item_t i;
    y = 0;
    wclear(mywin);
    Movexy(3, y);
    wprintw(mywin, (NCURSES_CONST char*)"Rasse: %s", u->race->_name[0]);
    Movexy(3, ++y);
    wprintw(mywin, (NCURSES_CONST char*)"Anzahl: %d", u->number);
    Movexy(3, ++y);
    wprintw(mywin, (NCURSES_CONST char*)"Silber: %d", get_money(u));
    Movexy(3, ++y);
    at = fval(modunit, UFL_PARTEITARNUNG);
    wprintw(mywin, (NCURSES_CONST char*)"Parteitarnung: %s", at ? "an" : "aus");
    Movexy(3, ++y);
    Addstr("Dämonentarnung: ");
#if 0 /* ja, wie denn? */
    if (u->race == new_race[RC_DAEMON] && u->itypus > 0 && u->typus != u->itypus)
      Addstr(typusdaten[u->itypus].name[0]);
    else {
#endif
      Addstr("keine");
      if (u->race != new_race[RC_DAEMON])
        Addstr(" möglich");
#if 0
    }
#endif

    Movexy(3, ++y);
    zt = y;
    q = 11;
    x = 0;
    Addstr("Talente: ");
    for (sk = 0; sk != MAXSKILLS; sk++) {
      if (has_skill(u, sk)) {
        if (x) {
          Addstr(", ");
          q += 2;
        }
        sprintf(buf, "%s %d", skillname(sk, NULL), eff_skill(u, sk, r));
        q += strlen(buf);
        if (q > SX - 8) {
          q = strlen(buf);
          y++;
          Movexy(5, y);
        }
        Addstr(buf);
        x = 1;
      }
    }
    if (!x)
      Addstr("keine");
    y++;

    Movexy(3, y);
    q = 15;
    x = false;
    zg = y;
    Addstr("Gegenstände: ");
    for (itm=u->items;itm;itm=itm->next) {
      if (x) {
        Addstr(", ");
        q += 2;
      }
      sprintf(buf, "%d %s", itm->number, locale_string(NULL, resourcename(itm->type->rtype, GR_PLURAL)));
      q += strlen(buf);
      if (q > SX - 8) {
        q = strlen(buf);
        y++;
        Movexy(5, y);
      }
      Addstr(buf);
      x = true;
    }
    if (!x)
      Addstr("keine");
    y++;
    y++;
    Movexy(3, y);
    Addstr("Anfangsbuchstabe, <Esc> für Ende");
    wrefresh(mywin);

    q = getch();
    switch (tolower(q)) {
      case 't':
        modif = modify_talente(u, r);
        break;
      case 'g':
        modif = (char)modify_items(u);
        break;
      case 'r':
        win = openwin(SX - 10, 5, "< Rasse wählen >");
        y = 2;
        q = 0;
        wmove(win, y, 4);
        for (i = 1; i < MAXRACES; i++) {
          sprintf(buf, "%d=%s; ", i, new_race[i]->_name[0]);
          q += strlen(buf);
          if (q > SX - 20) {
            q = strlen(buf);
            y++;
            wmove(win, y, 4);
          }
          wAddstr(buf);
        }
        u->race = new_race[map_input(win, 2, 1, "Rasse", 0, MAXRACES-1, old_race(modunit->faction->race))];
        modif = 1;
        delwin(win);
        break;
      case 'd':
        if (u->race == new_race[RC_DAEMON]) {
          race_t rc;
          win = openwin(SX - 10, 5, "< Tarnrasse wählen >");
          y = 2;
          q = 0;
          wmove(win, y, 4);
          for (rc = 1; rc != MAXRACES; rc++) {
            sprintf(buf, "%d=%s; ", rc, new_race[rc]->_name[0]);
            q += strlen(buf);
            if (q > SX - 20) {
              q = strlen(buf);
              y++;
              wmove(win, y, 4);
            }
            wAddstr(buf);
          }
          u->irace = new_race[map_input(win, 2, 1, "Tarnrasse", 0, MAXRACES, old_race(modunit->irace))];
          modif = 1;
          delwin(win);
        } else
          beep();
        break;
      case 'a':
        a = u->number;
        scale_number(u, map_input(0, 0, 0, "Anzahl Personen", 1, 9999, u->number));
        if (a != u->number)
          modif = 1;
        break;
      case 's':
        set_money(u, map_input(0, 0, 0, "Silber", 0, 999999, get_money(u)));
        modif = 1;
        break;
      case 'p':
        q = (int) yes_no(0, "Parteitarnung?",
               (char) ((fval(modunit, UFL_PARTEITARNUNG))
                 ? 'j' : 'n'));
        if (at && !q) {
          freset(modunit, UFL_PARTEITARNUNG);
          modif = 1;
        } else if (!at && q) {
          fset(modunit, UFL_PARTEITARNUNG);
          modif = 1;
        }
        break;
    }
  } while (q != 27 && q != 'q');

  if (modif) {
    if (yes_no(0, "Änderungen übernehmen?", 'j')) {
      copy_unit(u, modunit);
      modified = 1;
    }
  }
  free(u);
  movexy(0, 1);
  hline('-', SX + 1);
  refresh();
  return i2b(modif == 1);
}

dbllist *pointer;
boolean isunit;

void
SetPointer(int dir)
{
  dbllist *h = pointer;
  int pl = pline;
  switch (dir) {
  case KEY_UP:
  case KEY_LEFT:
  case KEY_PPAGE:
  case '/':
  case 'n':
  case '<':
    while (h) {
      if (h->s[0] < ' ') {
        if (h->s[0] == '\025')  /* eine Einheit */
          isunit = true;
        else
          isunit = false;
        pointer = h;
        pline = pl;
        return;
      }
      h = h->prev;
      pl--;
    }
    break;
  case KEY_DOWN:
  case KEY_RIGHT:
  case KEY_NPAGE:
  case '>':
    while (h) {
      if (h->s[0] < ' ') {
        if (h->s[0] == '\025')  /* eine Einheit */
          isunit = true;
        else
          isunit = false;
        pointer = h;
        pline = pl;
        return;
      }
      h = h->next;
      pl++;
    }
  }

  /* Unten bzw. oben angekommen und nix gefunden? Dann andersrum */
  h = pointer;
  pl = pline;
  switch (dir) {
  case KEY_DOWN:
  case KEY_RIGHT:
  case KEY_NPAGE:
  case '>':
    while (h) {
      if (h->s[0] < ' ') {
        if (h->s[0] == '\025')  /* eine Einheit */
          isunit = true;
        else
          isunit = false;
        pointer = h;
        pline = pl;
        return;
      }
      h = h->prev;
      pl--;
    }
    break;
  case KEY_UP:
  case KEY_PPAGE:
  case KEY_LEFT:
  case '/':
  case 'n':
  case '<':
    while (h) {
      if (h->s[0] < ' ') {
        if (h->s[0] == '\025')  /* eine Einheit */
          isunit = true;
        else
          isunit = false;
        pointer = h;
        pline = pl;
        return;
      }
      h = h->next;
      pl++;
    }
  }
  return;
}

void
Umark(int p)
{
  Movexy(0, p);
  if (isunit)
    Addstr("=>");
  else
    Addstr("->");
  movexy(0, 1);
  hline('-', SX + 1);
  if (clipship) {
    sprintf(buf, "< Clipschiff: %s >", shipname(clipship));
    movexy(SX - strlen(buf) - 2, 1);
    addstr(buf);
  }
  movexy(0, SY - 1);
  hline('-', SX + 1);
  if (clipunit) {
    sprintf(buf, "< Clipunit:%s >", Unitid(clipunit));
    movexy(SX - strlen(buf) - 2, SY - 1);
    addstr(buf);
  }
  Movexy(0, p);
  wrefresh(mywin);
}

void
sAddstr(char *s)
{
  if (s[0] < ' ')
    s++;
  Addstr(s);
}

unit *clipunit;     /* hierin kann eine Einheit gespeichert werden */
region *clipregion;   /* hierher kommt die clipunit */

void
dblparagraph(dbllist ** SP, char *s, int indent, char mark)
{
  int i, j, width, delta = 0;
  int firstline;
  static char lbuf[128];
  width = SX - 5 - indent;
  firstline = 1;

  for (;;) {
    i = 0;

    do {
      j = i;
      while (s[j] && s[j] != ' ')
        j++;
      if (j > width) {
        if (i == 0)
          i = width - 1;
        break;
      }
      i = j + 1;
    }
    while (s[j]);

    j = 0;
    if (firstline) {
      if (*s == '\025') { /* \023 ist ^U => Unit-Kennung */
        lbuf[0] = '\025'; /* Kennung nach vorne
               * holen */
        delta = 1;
      }
    } else
      delta = 0;

    for (j = 0; j != indent; j++)
      lbuf[j + delta] = ' ';

    if (firstline && mark)
      lbuf[indent - 2 + delta] = mark;

    for (j = 0; j != i - 1; j++)
      lbuf[indent + j + delta] = s[j + delta];
    lbuf[indent + j + delta] = 0;

    adddbllist(SP, lbuf);

    if (!s[i - 1 + delta])
      break;

    s += i;
    firstline = 0;
  }
}

void
mapper_spunit(dbllist ** SP, unit * u, int indent)
{
  int i, dh;
  item * itm;
  char * bufp;

  strcpy(buf, "\025");
  sncat(buf, unitname(u), BUFSIZE);
  sncat(buf, ", ", BUFSIZE);
  sncat(buf, factionname(u->faction), BUFSIZE);

  if (fval(u, UFL_PARTEITARNUNG))
    sncat(buf, " (getarnt)", BUFSIZE);

  sncat(buf, ", ", BUFSIZE);
  icat(u->number);
  sncat(buf, " ", BUFSIZE);

  i = old_race(u->race);
  if (u->irace != u->race) {
    sncat(buf, u->irace->_name[u->number != 1], BUFSIZE);
    sncat(buf, " (", BUFSIZE);
    if (u->race == new_race[RC_ILLUSION])
      sncat(buf, u->faction->race->_name[u->number != 1], BUFSIZE);
    else
      sncat(buf, u->race->_name[u->number != 1], BUFSIZE);
    sncat(buf, ")", BUFSIZE);
  } else {
    if (u->race == new_race[RC_ILLUSION])
      sncat(buf, u->faction->race->_name[u->number != 1], BUFSIZE);
    else
      sncat(buf, u->race->_name[u->number != 1], BUFSIZE);
  }

  switch (u->status) {
    case ST_FIGHT:
      sncat(buf, ", vorne", BUFSIZE);
      break;
    case ST_BEHIND:
      sncat(buf, ", hinten", BUFSIZE);
      break;
    case ST_AVOID:
      sncat(buf, ", kämpft nicht", BUFSIZE);
      break;
    case ST_FLEE:
      sncat(buf, ", flieht", BUFSIZE);
      break;
    default:
      sncat(buf, ", unbekannt", BUFSIZE);
      break;
  }

  if(get_movement(&u->attribs, MV_CANNOTMOVE)) {
    sncat(buf, ", cannot move", BUFSIZE);
  }

  sncat(buf, " (", BUFSIZE); icat(u->hp/u->number); sncat(buf, " HP)", BUFSIZE);

  if (getguard(u))
    sncat(buf, ", bewacht die Region", BUFSIZE);

  if (usiege(u)) {
    sncat(buf, ", belagert ", BUFSIZE);
    sncat(buf, buildingname(usiege(u)), BUFSIZE);
  }
  sncat(buf, ", ", BUFSIZE);
  icat(get_money(u));
  sncat(buf, " Silber", BUFSIZE);

  dh = 0;
  bufp = buf + strlen(buf);
  if (u->skill_size>0) {
    skill * sv;
    struct locale * lang = find_locale("de");
    for (sv = u->skills;sv!=u->skills+u->skill_size;++sv) {
      bufp += spskill(bufp, sizeof(buf)-(bufp-buf), lang, u, sv, &dh, 1);
    }
  }
  dh = 0;

  for (itm = u->items;itm;itm=itm->next) {
    sncat(buf, ", ", BUFSIZE);

    if (!dh) {
      sncat(buf, "hat: ", BUFSIZE);
      dh = 1;
    }
    if (itm->number == 1)
      sncat(buf, locale_string(NULL, resourcename(itm->type->rtype, 0)), BUFSIZE);
    else {
      icat(itm->number);
      sncat(buf, " ", BUFSIZE);
      sncat(buf, locale_string(NULL, resourcename(itm->type->rtype, GR_PLURAL)), BUFSIZE);
    }
  }
  dh = 0;

  if (uprivate(u)) {
    sncat(buf, " (Bem: ", BUFSIZE);
    sncat(buf, uprivate(u), BUFSIZE);
    sncat(buf, ")", BUFSIZE);
  }
  sncat(buf, ".", BUFSIZE);

  dblparagraph(SP, buf, indent, '-');
  adddbllist(SP, " ");
}


int
showunits(region * r)
{
  unit *u, *x;
  building *b;
  ship *sh;
  dbllist *eh = NULL, *unten, *oben, *hlp = NULL, *such = NULL, *tmp;
  int line, ch, bottom, bot, f;
  size_t lt;
  char *s = NULL, *txt, *suchtext = 0, str[45], lbuf[256];

  clear();
  strncpy(str, rname(r, NULL), 44);
  str[44] = 0;
  movexy(0, 0);
  printw((NCURSES_CONST char*)"Einheiten in %s (%d,%d):", str, r->x, r->y);
  movexy(0, SY);
  addstr("/,n: Suche; N: NewUnit; G: GetUnit; P: PutUnit; M: Modify; S: Schiff; B: Burg");
  refresh();

  for (b = r->buildings; b; b = b->next) {
    if (b->type == bt_find("castle")) {
      sprintf(lbuf, "\002%s, Größe %d, %s", buildingname(b), b->size,
        buildingtype(b->type, b, b->size /*, NULL */));
    } else {
      sprintf(lbuf, "\002%s, Größe %d, %s", buildingname(b),
          b->size, buildingtype(b->type, b, b->size /*, NULL */));
      if (b->type->maxsize > 0 &&
          b->size < b->type->maxsize) {
        sncat(lbuf, " (im Bau)", BUFSIZE);
      }
    }
    adddbllist(&eh, lbuf);
    adddbllist(&eh, " ");
    for (u = r->units; u; u = u->next) {
      if (u->building == b && fval(u, UFL_OWNER)) {
        mapper_spunit(&eh, u, 4);
        break;
      }
    }
    for (u = r->units; u; u = u->next) {
      if (u->building == b && !fval(u, UFL_OWNER)) {
        mapper_spunit(&eh, u, 4);
      }
    }
  }
  for (sh = r->ships; sh; sh = sh->next) {
    int n = 0, p = 0;
    getshipweight(sh, &n, &p);
    n = (n+99) / 100; /* 1 Silber = 1 GE */
    sprintf(lbuf, "\023%s, %s, (%d/%d)", shipname(sh), sh->type->name[0],
      n, shipcapacity(sh)/100);
    if (sh->type->construction && sh->size!=sh->type->construction->maxsize) {
      f = 100 * (sh->size) / sh->type->construction->maxsize;
      sncat(lbuf, ", im Bau (", BUFSIZE);
      icat(f);
      sncat(lbuf, "%) ", BUFSIZE);
    }
    if (sh->damage) {
      sncat(lbuf, ", ", BUFSIZE);
      icat(sh->damage);
      sncat(lbuf, "% beschädigt", BUFSIZE);
    }
    adddbllist(&eh, lbuf);
    adddbllist(&eh, " ");
    for (u = r->units; u; u = u->next) {
      if (u->ship == sh && fval(u, UFL_OWNER)) {
        mapper_spunit(&eh, u, 4);
        break;
      }
    }
    for (u = r->units; u; u = u->next) {
      if (u->ship == sh && !fval(u, UFL_OWNER))
        mapper_spunit(&eh, u, 4);
    }
  }

  if (!r->units) {
    adddbllist(&eh, "keine Einheiten");
    adddbllist(&eh, " ");
  } else {
    for (u = r->units; u; u = u->next) {
      if (!u->ship && !u->building)
        mapper_spunit(&eh, u, 2);
    }
  }

  mywin = newwin(SY - 3, SX, 2, 0);
  wclear(mywin);
  bot = mywin->_maxy + 1;
  if (!pointer)
    pointer = eh;
  else {
    pointer = eh;
    for (f = 0; f < pline && pointer->next; f++)
      pointer = pointer->next;
  }
  oben = pointer;
  f = 4;
  while (oben->prev && --f!=0)
    oben = oben->prev;  /* Wenn dies ein "Fullredraw" ist, pointer dorthin,
                           wo wir vorher ungefähr waren. */
  for (line=0, unten=oben; line<bot && unten->next; line++, unten=unten->next) {
    Movexy(3, line);
    sAddstr(unten->s);
  }
  bottom = line - 1;
  tmp = oben;
  pline = 0;

  while (tmp != pointer) {
    pline++;
    tmp = tmp->next;
  }

  scrollok(mywin, TRUE);
  ch = KEY_DOWN;
  wrefresh(mywin);

  for (;;) {
    SetPointer(ch);
    if (pline < 0 || pline >= bot || !pointer)
      beep();
    else
      Umark(pline);
    ch = getch();
    if (pointer)
      Addstr("  ");
    switch (ch) {
    case KEY_DOWN:
      if (pointer->next) {
        pointer = pointer->next;
        pline++;
      } else
        beep();
      if (pline > bot - 5 && unten->next) {
        line = bot - 5;
        while (line && unten->next) {
          unten = unten->next;
          oben = oben->next;
          pline--;
          line--;
        }
        f = -1;
      }
      break;
    case KEY_UP:
      if (pointer->prev) {
        pointer = pointer->prev;
        pline--;
      } else
        beep();
      if (pline < 5 && oben != eh) {
        line = bot - 5;
        while (line && oben->prev) {
          unten = unten->prev;
          oben = oben->prev;
          pline++;
          line--;
        }
        f = -1;
      }
      break;
    case KEY_NPAGE:
    case KEY_RIGHT:
      for (line = 0; line < 20 && unten->next; line++) {
        oben = oben->next;
        unten = unten->next;
        if (pointer->next)
          pointer = pointer->next;
      }
      f = -1;
      break;
    case KEY_PPAGE:
    case KEY_LEFT:
      for (line = 0; line < 20 && oben->prev; line++) {
        oben = oben->prev;
        unten = unten->prev;
        if (pointer->prev)
          pointer = pointer->prev;
      }
      f = -1;
      break;
    case '/':
      suchtext = my_input(0, 0, 0, "Suchtext: ", NULL);
      such = eh;
    case 'n':
      if (suchtext) {
        line = 0;
        lt = strlen(suchtext);
        while (such->next && !line) {
          s = such->s;
          while (strlen(s) >= lt && !line) {
            if (strncasecmp(s, suchtext, lt) == 0)
              line = 1;
            s++;
          }
          if (!line)
            such = such->next;
        }
        if (line) {
          wclear(mywin);
          oben = such->prev ? such->prev : such;
          pline = 0;
          /* unten = hlp->prev; */
          while (oben->prev && oben->s[0] != '\025') {  /* eine Einheit */
            oben = oben->prev;
            unten = unten->prev;
          }
          pointer = oben;
          Movexy(3, 0);
          for (line = 1, hlp = oben; line < bot && hlp->next; line++, hlp = hlp->next) {
            Movexy(3, line);
            if (hlp == such) {
              txt = such->s;
              s--;
              ch = *s;
              *s = 0;
              sAddstr(txt);
              wattron(mywin, A_REVERSE);
              *s = (char)ch;
              txt = s;
              ch = txt[lt];
              txt[lt] = 0;
              Addstr(txt);
              wattroff(mywin, A_REVERSE);
              txt[lt] = (char)ch;
              txt += lt;
              Addstr(txt);
            } else
              sAddstr(hlp->s);
          }
          bottom = line - 1;
        } else {
          movexy(0, SY);
          beep();
          clrtoeol();
          printw((NCURSES_CONST char*)"'%s' nicht gefunden.", suchtext);
          refresh();
        }
      } else
        beep();
      break;
    case KEY_HELP:
    case '?':
    case 'h':
      movexy(0, SY);
      clrtoeol();
      addstr("/,n:Suche; N:New; G:Get; P:Put; M:Modify; B:Burg; S:Schiff;");
      refresh();
      break;
    case '>':
      while (unten->next) {
        unten = unten->next;
        oben = oben->next;
      }
      while (pointer->next)
        pointer = pointer->next;
      pline = bot;
      f = -1;
      break;
    case '<':
      pointer = such = oben = eh;
      pline = 0;
      f = -1;
      break;
    case 'X':
      if(clipunit) {
        SpecialFunctionUnit(clipunit);
        modified = 1;
        clipunit = 0;
        clipregion = 0;
        for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
          pline++;  /* Stelle merken */
        freelist(eh);
        delwin(mywin);
        return 1;
      }
      break;
    case 'N':
      if ((u = make_new_unit(r))!=NULL) {
        modified = 1;
        if ((s = strchr(pointer->s, '('))!=NULL) {
          s++;
          f = atoi36(s);
          switch (pointer->s[0]) {
          case '\002':
            b = findbuilding(f);
            sprintf(lbuf, "Einheit in %s als Eigner?", BuildingName(b));
            if (yes_no(0, lbuf, 'j')) {
              for (x = r->units; x; x = x->next)
                if (x->building == b && fval(x, UFL_OWNER)) {
                  freset(x, UFL_OWNER);
                  break;
                }
              u->building = b;
              fset(u, UFL_OWNER);
            }
            break;
          case '\023':
            sh = findship(f);
            if (sh) {
              sprintf(lbuf, "Einheit auf%s als Eigner?", shipname(sh));
              if (yes_no(0, lbuf, 'j')) {
                for (x = r->units; x; x = x->next)
                  if (x->ship == sh && fval(x, UFL_OWNER)) {
                    freset(x, UFL_OWNER);
                    break;
                  }
                u->ship = sh;
                fset(u, UFL_OWNER);
              }
            }
            break;
          case '\025':
            x = findunit(f, r);
            if(x) {
              if (x->building) {
                sprintf(lbuf, "Einheit in %s rein?", buildingname(x->building));
                if (yes_no(0, lbuf, 'j'))
                  u->building = x->building;
              } else if (x->ship) {
                sprintf(lbuf, "Einheit auf%s rauf?", shipname(x->ship));
                if (yes_no(0, lbuf, 'j'))
                  u->ship = x->ship;
              }
            }
            break;
          }
        }
        for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
          pline++;  /* Stelle merken */
        delwin(mywin);
        freelist(eh);
        return 1;
      }
    case 12:  /* ^L */
      f = -1;
      break;
    case 'S':
      if (!clipship)
        beep();
      else {
        sprintf(lbuf, "Schiff%s löschen?", shipname(clipship));
        if (yes_no(0, lbuf, 'n')) {
          modified = 1;
          for (x = shipregion->units; x; x = x->next)
            leave(shipregion, x);
          destroy_ship(clipship);
          clipship = 0;
          shipregion = 0;
          for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
            pline++;  /* Stelle merken */
          delwin(mywin);
          freelist(eh);
          return 1;
        } else
          wrefresh(mywin);
      }
      break;
    case 's':
      if (clipship && shipregion != r) {
        unit *un;
        sprintf(lbuf, "Schiff %s einfügen?", shipname(clipship));
        if (yes_no(0, lbuf, 'j')) {
          boolean owner_set = false;

          for (x = shipregion->units; x;) {
            un = x->next;
            if (x->ship == clipship) {
              f = (int) fval(x, UFL_OWNER);
              leave(shipregion, x);
              translist(&shipregion->units, &r->units, x);
              x->ship = clipship;
              if (owner_set == false && f) {
                owner_set = true;
                fset(x, UFL_OWNER);
              } else {
                freset(x, UFL_OWNER);
              }
            }
            x = un;
          }
          move_ship(clipship, shipregion, r, NULL);
          clipship = NULL;
          shipregion = NULL;
        }
      } else
        NeuesSchiff(r);
      for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
        pline++;  /* Stelle merken */
      freelist(eh);
      delwin(mywin);
      return 1;
    case 'b':
      NeueBurg(r);
      for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
        pline++;  /* Stelle merken */
      freelist(eh);
      delwin(mywin);
      return 1;
    case 'B':
      if (pointer) {
        if ((s = strchr(pointer->s, '('))!=NULL) {
          s++;
          f = atoi36(s);
          if (f) {
            b = findbuilding(f);
            if (b) {
              sprintf(lbuf, "Gebäude %s löschen?", BuildingName(b));
              if (yes_no(0, lbuf, 'n')) {
                modified = 1;
                for (x = r->units; x; x = x->next)
                  if (x->building == b)
                    leave(r, x);
                destroy_building(b);
                for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
                  pline++;  /* Stelle merken */
                delwin(mywin);
                freelist(eh);
                return 1;
              } else
                wrefresh(mywin);
            }
          }
        }
      }
      break;
    case 'U':
      f = map_input(0, 0, 0, "Einheit Nummer", 0, 99999, 0);
      if (f)
        clipunit = findunit(f, r);
      break;
    case 'M':
      if (!isunit || !pointer) {
        movexy(0, SY);
        clrtoeol();
        movexy(0, SY);
        addstr("keine Einheit hier");
        refresh();
      } else {
        s = strchr(pointer->s, '(');
        if (s) {
          char idbuf[12];
          int  i = 0;

          s++;
          while(*s != ')') {
            idbuf[i] = *s;
            i++; s++;
            assert(i<=11);
          }
          idbuf[i] = '\0';

          f  = atoi36(idbuf);

          x = findunit(f, r);
          if (x && modify_unit(r, findunit(f, r))) {
            for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
              pline++;  /* Stelle merken */
            freelist(eh);
            delwin(mywin);
            clipunit = x;
            clipregion = r;
            return 1;
          } else
            f = -1;
        }
      }
      break;
    case 'P':
    case 'p':
      if (!clipunit)
        beep();
      else {
        leave(clipregion, clipunit);
        if (r != clipregion)
          translist(&clipregion->units, &r->units, clipunit);
        modified = 1;
        if (pointer && (s = strchr(pointer->s, '('))!=NULL) {
          s++;
          f = atoi36(s);
          if (f)
            switch (pointer->s[0]) {
            case '\025':
              f = atoi36(s);
              x = findunit(f, r);
              clipunit->building = x->building;
              clipunit->ship = x->ship;
              clipregion = r;
              break;
            case '\002':
              f = atoi36(s);
              b = findbuilding(f);
              for (x = r->units; x; x = x->next)
                if (x->building == b && fval(x, UFL_OWNER))
                  freset(x, UFL_OWNER);
              clipunit->building = b;
              fset(clipunit, UFL_OWNER);
              break;
            case '\023':
              f = atoi(s);
              sh = findship(f);
              for (x = r->units; x; x = x->next)
                if (x->ship == sh && fval(x, UFL_OWNER))
                  freset(x, UFL_OWNER);
              clipunit->ship = sh;
              fset(clipunit, UFL_OWNER);
              break;
            }
        }
        for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
          pline++;  /* Stelle merken */
        delwin(mywin);
        freelist(eh);
        return 1; /* -> totaler Neuaufbau der
             * Liste */
      }
      break;
    case 'E':
      clipunit = 0;
      clipregion = 0;
      break;
    case 'D':
      if (!clipunit)
        beep();
      else {
        sprintf(lbuf, "Einheit %s löschen?", Unitid(clipunit));
        if (yes_no(0, lbuf, 'n')) {
          modified = 1;
          destroy_unit(clipunit);
          if (clipunit->number==0) remove_unit(clipunit);
          clipunit = 0;
          clipregion = 0;
          for (pline = 0, tmp = eh; tmp != pointer; tmp = tmp->next)
            pline++;  /* Stelle merken */
          freelist(eh);
          delwin(mywin);
          return 1;
        } else
          wrefresh(mywin);

      }
      break;
    case 'G':
    case 'g':
      if (!pointer) {
        movexy(0, SY);
        addstr("kann hier nichts aufnehmen");
        refresh();
      } else {
        if ((s = strchr(pointer->s, '('))!=NULL) {
          char idbuf[12];
          int  i = 0;

          s++;
          while(*s != ')') {
            idbuf[i] = *s;
            i++; s++;
            assert(i<=11);
          }
          idbuf[i] = '\0';

          f  = atoi36(idbuf);

          if (f)
            switch (pointer->s[0]) {
            case '\025':
              clipunit = findunit(f, r);
              clipregion = r;
              break;
            case '\023':
              clipship = findship(f);
              shipregion = r;
              break;
            }
        }
      }
      break;
    case 27:  /* Esc */
    case 'q':
      delwin(mywin);
      freelist(eh);
      pointer = NULL;
      return 0;
    }

    if (f < 0) {
      wclear(mywin);
      for (line = 0, unten = oben; line < bot && unten->next; line++, unten = unten->next) {
        Movexy(3, line);
        sAddstr(unten->s);
      }
      bottom = line - 1;
      f = 0;
    }
  }
}
