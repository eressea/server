/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2006   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *  
 */

/* wenn config.h nicht vor curses included wird, kompiliert es unter windows nicht */
#include <config.h>
#include <curses.h>
#include <eressea.h>

#include "gmtool.h"
#include "editing.h"
#include "curses/listbox.h"

#include <modules/autoseed.h>
#include <modules/xmas.h>
#include <modules/gmcmd.h>
#ifdef MUSEUM_MODULE
#include <modules/museum.h>
#endif
#ifdef ARENA_MODULE
#include <modules/arena.h>
#endif
#ifdef WORMHOLE_MODULE
#include <modules/wormhole.h>
#endif
#ifdef DUNGEON_MODULE
#include <modules/dungeon.h>
#endif

#include <kernel/building.h>
#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/plane.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/teleport.h>
#include <kernel/terrainid.h>
#include <kernel/unit.h>
#include <kernel/resources.h>
#include <kernel/save.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/xmlreader.h>

#include <spells/spells.h>
#include <attributes/attributes.h>
#include <triggers/triggers.h>
#include <items/itemtypes.h>

#include <util/log.h>
#include <util/base36.h>

#include <string.h>
#include <locale.h>

extern char * g_reportdir;
extern char * g_datadir;
extern char * g_basedir;
extern char * g_resourcedir;

static int g_quit;
static const char * g_logfile = "gmtool.log";
static int force_color = 0;

#define IFL_SHIPS     (1<<0)
#define IFL_UNITS     (1<<1)
#define IFL_FACTIONS  (1<<2)
#define IFL_BUILDINGS (1<<3)

static int
usage(const char * prog, const char * arg)
{
  if (arg) {
    fprintf(stderr, "unknown argument: %s\n\n", arg);
  }
  fprintf(stderr, "Usage: %s [options]\n"
    "-b basedir       : gibt das basisverzeichnis an\n"
    "-d datadir       : gibt das datenverzeichnis an\n"
    "-r resdir        : gibt das resourceverzeichnis an\n"
    "-t turn          : read this datafile, not the most current one\n"
    "-v               : be verbose\n"
    "-l logfile       : write messages to <logfile>\n"
    "--xml xmlfile    : read settings from <xmlfile>.\n"
    "--lomem          : save RAM\n"
    "--color          : force color mode\n"
    "--help           : help\n", prog);
  return -1;
}

static int
read_args(int argc, char **argv)
{
  int i;
 
  quiet = 1;
  turn = first_turn;

  for (i=1;i!=argc;++i) {
    if (argv[i][0]!='-') {
      return usage(argv[0], argv[i]);
    } else if (argv[i][1]=='-') { /* long format */
      if (strncmp(argv[i]+2, "xml", 3)==0) {
        if (argv[i][5]=='=') xmlfile = argv[i]+6;
        else xmlfile = argv[++i];
      } else if (strncmp(argv[i]+2, "log", 3)==0) {
        if (argv[i][5]=='=') g_logfile = argv[i]+6;
        else g_logfile = argv[++i];
      } else if (strncmp(argv[i]+2, "res", 3)==0) {
        if (argv[i][5]=='=') g_resourcedir = argv[i]+6;
        else g_resourcedir = argv[++i];
      } else if (strncmp(argv[i]+2, "base", 4)==0) {
        if (argv[i][6]=='=') g_basedir = argv[i]+7;
        else g_basedir = argv[++i];
      } else if (strncmp(argv[i]+2, "turn", 4)==0) {
        if (argv[i][6]=='=') turn = atoi(argv[i]+7);
        else turn = atoi(argv[++i]);
      } else if (strncmp(argv[i]+2, "data", 4)==0) {
        if (argv[i][6]=='=') g_datadir = argv[i]+7;
        else g_datadir = argv[++i];
      } else if (strcmp(argv[i]+2, "lomem")==0) {
        lomem = true;
      } else if (strcmp(argv[i]+2, "color")==0) {
        force_color = 1;
      } else if (strcmp(argv[i]+2, "help")==0)
        return usage(argv[0], NULL);
      else
        return usage(argv[0], argv[i]);
    } else {
      int k;
      boolean next = false;

      for (k=0;!next && argv[i][k];++k) {
        int c = argv[i][k];
        switch (c) {
        case 'd':
          g_datadir = argv[++i];
          next = true;
          break;
        case 'r':
          g_resourcedir = argv[++i];
          next = true;
          break;
        case 'b':
          g_basedir = argv[++i];
          next = true;
          break;
        case 'i':
          xmlfile = argv[++i];
          next = true;
          break;
        case 't':
          turn = atoi(argv[++i]);
          next = true;
          break;
        case 'v':
          quiet = 0;
          break;
        default:
          usage(argv[0], argv[i]);
        }
      }
    }
  }
  return 0;
}

static void
game_init(void)
{
  init_triggers();
  init_xmas();

  register_races();
  register_resources();
  register_buildings();
  register_ships();
  register_spells();
#ifdef DUNGEON_MODULE
  register_dungeon();
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

  register_itemtypes();
  register_xmlreader();

  init_data(xmlfile);

  init_locales();
  /*  init_resources(); must be done inside the xml-read, because requirements use items */

  init_attributes();
  init_races();
  init_itemtypes();
  init_rawmaterials();

  init_gmcmd();
#ifdef INFOCMD_MODULE
  init_info();
#endif
}

static void
game_done(void)
{
}

static void
init_curses(void)
{
  initscr();

  if (has_colors() || force_color) {
    short bcol = COLOR_BLACK;
    start_color();
    if (can_change_color()) {
      init_color(COLOR_YELLOW, 1000, 1000, 0);
    }
    init_pair(COLOR_BLACK, COLOR_BLACK, bcol);
    init_pair(COLOR_GREEN, COLOR_GREEN, bcol);
    init_pair(COLOR_GREEN, COLOR_GREEN, bcol);
    init_pair(COLOR_RED, COLOR_RED, bcol);
    init_pair(COLOR_CYAN, COLOR_CYAN, bcol);
    init_pair(COLOR_WHITE, COLOR_WHITE, bcol);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, bcol);
    init_pair(COLOR_BLUE, COLOR_BLUE, bcol);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, bcol);
    init_pair(COLOR_YELLOW, COLOR_YELLOW, bcol);
    init_pair(COLOR_WHITE, COLOR_WHITE, bcol);

    attrset(COLOR_PAIR(COLOR_BLACK));
    bkgd(' ' | COLOR_PAIR(COLOR_BLACK));
    bkgdset(' ' | COLOR_PAIR(COLOR_BLACK));
  }

  keypad(stdscr, TRUE);  /* enable keyboard mapping */
  nonl();         /* tell curses not to do NL->CR/NL on output */
  cbreak();       /* take input chars one at a time, no wait for \n */
  noecho();       /* don't echo input */
  scrollok(stdscr, FALSE);
  wclear(stdscr);
}

static map_region *
mr_get(const view * vi, int xofs, int yofs)
{
  return vi->regions + xofs + yofs * vi->extent.width;
}

static coordinate *
point2coor(const point * p, coordinate * c)
{
  int x, y;
  assert(p && c);
  /* wegen division (-1/2==0):
  * verschiebung um (0x200000,0x200000) ins positive */
  x = p->x + TWIDTH*0x200000+TWIDTH*0x100000;
  y = p->y + THEIGHT*0x200000;
  c->x = (x - y * TWIDTH / 2) / TWIDTH - 0x200000;
  c->y = y / THEIGHT - 0x200000;
  return c;
}

static point *
coor2point(const coordinate * c, point * p)
{
  assert(c && p);
  p->x = c->x * TWIDTH + c->y * TWIDTH / 2;
  p->y = c->y * THEIGHT;
  return p;
}

static window * wnd_first, * wnd_last;

static window *
win_create(WINDOW * hwin)
{
  window * wnd = calloc(1, sizeof(window));
  wnd->handle = hwin;
  if (wnd_first!=NULL) {
    wnd->next = wnd_first;
    wnd_first->prev = wnd;
    wnd_first = wnd;
  } else {
    wnd_first = wnd;
    wnd_last = wnd;
  }
  return wnd;
}

static void
untag_region(selection * s, const coordinate * c)
{
  unsigned int key = ((c->x << 12) ^ c->y);
  tag ** tp = &s->tags[key & (MAXTHASH-1)];
  tag * t = NULL;
  while (*tp) {
    t = *tp;
    if (t->coord.p==c->p && t->coord.x==c->x && t->coord.y==c->y) break;
    tp=&t->nexthash;
  }
  if (!*tp) return;
  *tp = t->nexthash;
  free(t);
  return;
}

static void
tag_region(selection * s, const coordinate * c)
{
  unsigned int key = ((c->x << 12) ^ c->y);
  tag ** tp = &s->tags[key & (MAXTHASH-1)];
  while (*tp) {
    tag * t = *tp;
    if (t->coord.p==c->p && t->coord.x==c->x && t->coord.y==c->y) return;
    tp=&t->nexthash;
  }
  *tp = calloc(1, sizeof(tag));
  (*tp)->coord = *c;
  return;
}

static int
tagged_region(selection * s, const coordinate * c)
{
  unsigned int key = ((c->x << 12) ^ c->y);
  tag ** tp = &s->tags[key & (MAXTHASH-1)];
  while (*tp) {
    tag * t = *tp;
    if (t->coord.x==c->x && t->coord.p==c->p && t->coord.y==c->y) return 1;
    tp=&t->nexthash;
  }
  return 0;
}

static int
mr_tile(const map_region * mr)
{
  if (mr!=NULL && mr->r!=NULL) {
    const region * r = mr->r;
    switch (r->terrain->_name[0]) {
    case 'o' : 
      return '.' | COLOR_PAIR(COLOR_CYAN);
    case 'd' : 
      return 'D' | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
    case 't' : 
      return '%' | COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
    case 'f' : 
      if (r->terrain->_name[1]=='o') { /* fog */
        return '.' | COLOR_PAIR(COLOR_YELLOW) | A_NORMAL;
      } else if (r->terrain->_name[1]=='i') { /* firewall */
        return '%' | COLOR_PAIR(COLOR_RED) | A_BOLD;
      }
    case 'h' :
      return 'H' | COLOR_PAIR(COLOR_YELLOW) | A_NORMAL;
    case 'm' :
      return '^' | COLOR_PAIR(COLOR_WHITE) | A_NORMAL;
    case 'p' : 
      if (r_isforest(r)) return '#' | COLOR_PAIR(COLOR_GREEN) | A_NORMAL;
      return '+' | COLOR_PAIR(COLOR_GREEN) | A_BOLD;
    case 'g' :
      return '*' | COLOR_PAIR(COLOR_WHITE) | A_BOLD;
    case 's' :
      return 'S' | COLOR_PAIR(COLOR_MAGENTA) | A_NORMAL;
    }
    return r->terrain->_name[0] | COLOR_PAIR(COLOR_RED);
  }
  return ' ' | COLOR_PAIR(COLOR_WHITE);
}

static void
paint_map(window * wnd, const state * st)
{
  WINDOW * win = wnd->handle;
  int lines = getmaxy(win);
  int cols = getmaxx(win);
  int x, y;

  lines = lines/THEIGHT;
  cols = cols/TWIDTH;
  for (y = 0; y!=lines; ++y) {
    int yp = (lines - y - 1) * THEIGHT;
    for (x = 0; x!=cols; ++x) {
      int attr = 0;
      int xp = x * TWIDTH + (y & 1) * TWIDTH/2;
      map_region * mr = mr_get(&st->display, x, y);

      if (mr && st && tagged_region(st->selected, &mr->coord)) {
        attr |= A_REVERSE;
      }
      if (mr) {
        mvwaddch(win, yp, xp, mr_tile(mr) | attr);
      }
    }
  }
}

static map_region *
cursor_region(const view * v, const coordinate * c)
{
  coordinate relpos;
  int cx, cy;

  relpos.x = c->x - v->topleft.x;
  relpos.y = c->y - v->topleft.y;
  cy = relpos.y;
  cx = relpos.x + cy/2;
  return mr_get(v, cx, cy);
}

static void
draw_cursor(WINDOW * win, selection * s, const view * v, const coordinate * c, int show)
{
  int lines = getmaxy(win)/THEIGHT;
  int xp, yp;
  int attr = 0;
  map_region * mr = cursor_region(v, c);
  coordinate relpos;
  int cx, cy;

  relpos.x = c->x - v->topleft.x;
  relpos.y = c->y - v->topleft.y;
  cy = relpos.y;
  cx = relpos.x + cy/2;

  yp = (lines - cy - 1) * THEIGHT;
  xp = cx * TWIDTH + (cy & 1) * TWIDTH/2;
  if (s && tagged_region(s, &mr->coord)) attr = A_REVERSE;
  if (mr->r) {
    mvwaddch(win, yp, xp, mr_tile(mr) | attr);
  }
  else mvwaddch(win, yp, xp, ' ' | attr | COLOR_PAIR(COLOR_YELLOW));
  if (show) {
    attr = A_BOLD;
    mvwaddch(win, yp, xp-1, '<' | attr | COLOR_PAIR(COLOR_YELLOW));
    mvwaddch(win, yp, xp+1, '>' | attr | COLOR_PAIR(COLOR_YELLOW));
  } else {
    attr = A_NORMAL;
    mvwaddch(win, yp, xp-1, ' ' | attr | COLOR_PAIR(COLOR_WHITE));
    mvwaddch(win, yp, xp+1, ' ' | attr | COLOR_PAIR(COLOR_WHITE));
  }
  wmove(win, yp, xp);
}



static void
paint_status(window * wnd, const state * st)
{
  WINDOW * win = wnd->handle;
  const char * name = "";
  const char * terrain = "----";
  map_region * mr = cursor_region(&st->display, &st->cursor);
  if (mr && mr->r) {
    if (mr->r->land) {
      name = mr->r->land->name;
    } else {
      name = mr->r->terrain->_name;
    }
    terrain = mr->r->terrain->_name;
  }
  mvwprintw(win, 0, 0, "%4d %4d | %.4s | %.20s", st->cursor.x, st->cursor.y, terrain, name);
  wclrtoeol(win);
}

static boolean
handle_info_region(window * wnd, state * st, int c)
{
  return false;
}

static void
paint_info_region(window * wnd, const state * st)
{
  WINDOW * win = wnd->handle;
  int size = getmaxx(win)-2;
  int line = 0, maxline = getmaxy(win)-2;
  map_region * mr = cursor_region(&st->display, &st->cursor);

  unused(st);
  werase(win);
  wborder(win, 0, 0, 0, 0, 0, 0, 0, 0);
  if (mr && mr->r) {
    const region * r = mr->r;
    if (r->land) {
      mvwaddnstr(win, line++, 1, r->land->name, size);
    } else {
      mvwaddnstr(win, line++, 1, r->terrain->_name, size);
    }
    line++;
    mvwprintw(win, line++, 1, "%s, age %d", r->terrain->_name, r->age);
    if (r->land) {
      mvwprintw(win, line++, 1, "$:%6d  P:%5d", r->land->money, r->land->peasants);
      mvwprintw(win, line++, 1, "H:%6d  %s:%5d", r->land->horses, (r->flags&RF_MALLORN)?"M":"T", r->land->trees[1]+r->land->trees[2]);
    }
    line++;
    if (r->ships && (st->info_flags & IFL_SHIPS)) {
      ship * sh;
      wattron(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
      mvwaddnstr(win, line++, 1, "* ships:", size-5);
      wattroff(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
      for (sh=r->ships;sh && line<maxline;sh=sh->next) {
        mvwprintw(win, line, 1, "%.4s ", itoa36(sh->no));
        mvwaddnstr(win, line++, 6, (char*)sh->type->name[0], size-5);
      }
    }
    if (r->units && (st->info_flags & IFL_FACTIONS)) {
      unit * u;
      wattron(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
      mvwaddnstr(win, line++, 1, "* factions:", size-5);
      wattroff(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
      for (u=r->units;u && line<maxline;u=u->next) {
        if (!fval(u->faction, FL_MARK)) {
          mvwprintw(win, line, 1, "%.4s ", itoa36(u->faction->no));
          mvwaddnstr(win, line++, 6, u->faction->name, size-5);
          fset(u->faction, FL_MARK);
        }
      }
      for (u=r->units;u && line<maxline;u=u->next) {
        freset(u->faction, FL_MARK);
      }
    }
    if (r->units && (st->info_flags & IFL_UNITS)) {
      unit * u;
      wattron(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
      mvwaddnstr(win, line++, 1, "* units:", size-5);
      wattroff(win, A_BOLD | COLOR_PAIR(COLOR_YELLOW));
      for (u=r->units;u && line<maxline;u=u->next) {
        mvwprintw(win, line, 1, "%.4s ", itoa36(u->no));
        mvwaddnstr(win, line++, 6, u->name, size-5);
      }
    }
  }
}

static char *
askstring(WINDOW * win, const char * q, char * buffer, size_t size)
{
  werase(win);
  mvwaddstr(win, 0, 0, (char*)q);
  wmove(win, 0, (int)(strlen(q)+1));
  echo();
  wgetnstr(win, buffer, (int)size);
  noecho();
  return buffer;
}

static void
statusline(WINDOW * win, const char * str)
{
  mvwaddstr(win, 0, 0, (char*)str);
  wclrtoeol(win);
  wrefresh(win);
}

static void
terraform_at(coordinate * c, const terrain_type *terrain)
{
  if (terrain!=NULL) {
    short x = (short)c->x, y = (short)c->y;
    region * r = findregion(x, y);
    if (r==NULL) r = new_region(x, y);
    terraform_region(r, terrain);
  }
}

static void
terraform_selection(selection * selected, const terrain_type *terrain)
{
  int i;

  if (terrain==NULL) return;
  for (i=0;i!=MAXTHASH;++i) {
    tag ** tp = &selected->tags[i];
    while (*tp) {
      tag * t = *tp;
      short x = (short)t->coord.x, y = (short)t->coord.y;
      region * r = findregion(x, y);
      if (r==NULL) r = new_region(x, y);
      terraform_region(r, terrain);
      tp = &t->nexthash;
    }
  }
}

const terrain_type *
select_terrain(const terrain_type * default_terrain)
{
  list_selection *prev, *ilist = NULL, **iinsert;
  list_selection *selected = NULL;
  const terrain_type * terrain = terrains();

  if (!terrain) return NULL;
  iinsert = &ilist;
  prev = ilist;

  while (terrain) {
    insert_selection(iinsert, NULL, terrain->_name, (void*)terrain);
    terrain = terrain->next;
  }
  selected = do_selection(ilist, "Terrain", NULL, NULL);
  if (selected==NULL) return NULL;
  return (const terrain_type*)selected->data;
}

static coordinate *
region2coord(const region * r, coordinate * c)
{
  c->x = r->x;
  c->y = r->y;
  c->p = r->planep?r->planep->id:0;
  return c;
}

#ifdef __PDCURSES__
#define FAST_UP CTL_UP
#define FAST_DOWN CTL_DOWN
#define FAST_LEFT CTL_LEFT
#define FAST_RIGHT CTL_RIGHT
#else
#define FAST_UP KEY_PPAGE
#define FAST_DOWN KEY_NPAGE
#define FAST_LEFT KEY_SLEFT
#define FAST_RIGHT KEY_SRIGHT
#endif

static void
handlekeys(state * st)
{
  window * wnd;
  int c = getch();
  coordinate * cursor = &st->cursor;
  static char locate[80];
  static int findmode = 0;
  region *r;
  boolean invert = false;
  char sbuffer[80];

  switch(c) {
  case FAST_RIGHT:
    cursor->x+=10;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case FAST_LEFT:
    cursor->x-=10;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case FAST_UP:
    cursor->y+=10;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case FAST_DOWN:
    cursor->y-=10;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case KEY_UP:
    cursor->y++;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case KEY_DOWN:
    cursor->y--;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case KEY_RIGHT:
    cursor->x++;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case KEY_LEFT:
    cursor->x--;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    break;
  case KEY_SAVE:
  case KEY_F(2):
    if (st->modified) {
      char datafile[MAX_PATH];

      askstring(st->wnd_status->handle, "save as:", datafile, sizeof(datafile));
      if (strlen(datafile)>0) {
        create_backup(datafile);
        remove_empty_units();
        writegame(datafile, quiet);
        st->modified = 0;
      }
    }
    break;
  case 'B':
    make_block((short)st->cursor.x, (short)st->cursor.y, 6, select_terrain(NULL));
    st->modified = 1;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    st->wnd_map->update |= 1;
    break;
  case 0x02: /* CTRL+b */
    make_block((short)st->cursor.x, (short)st->cursor.y, 6, newterrain(T_OCEAN));
    st->modified = 1;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    st->wnd_map->update |= 1;
    break;
  case 0x09: /* tab = next selected*/
    if (regions!=NULL) {
      map_region * mr = cursor_region(&st->display, cursor);
      region * first = mr->r;
      region * r = (first&&first->next)?first->next:regions;
      while (r!=first) {
        coordinate coord;
        region2coord(r, &coord);
        if (tagged_region(st->selected, &coord)) {
          st->cursor = coord;
          st->wnd_info->update |= 1;
          st->wnd_status->update |= 1;
          break;
        }
        r = r->next;
        if (!r && first) r = regions;
      }
    }
    break;

  case 'a':
    if (regions!=NULL) {
      map_region * mr = cursor_region(&st->display, cursor);
      if (mr->r) {
        region * r = mr->r;
        if (r->planep==NULL) {
          r = r_standard_to_astral(r);
        } else if (r->planep==get_astralplane()) {
          r = r_astral_to_standard(r);
        } else {
          r = NULL;
        }
        if (r!=NULL) {
          region2coord(r, &st->cursor);
        } else {
          beep();
        }
      }
    }
    break;
  case 'g':
    askstring(st->wnd_status->handle, "goto-x:", sbuffer, 12);
    if (sbuffer[0]) {
      askstring(st->wnd_status->handle, "goto-y:", sbuffer+16, 12);
      if (sbuffer[16]) {
        st->cursor.x = atoi(sbuffer);
        st->cursor.y = atoi(sbuffer+16);
        st->wnd_info->update |= 1;
        st->wnd_status->update |= 1;
      }
    }
    break;
  case 't':
    terraform_at(&st->cursor, select_terrain(NULL));
    st->modified = 1;
    st->wnd_info->update |= 1;
    st->wnd_status->update |= 1;
    st->wnd_map->update |= 1;
    break;
  case 'I':
    statusline(st->wnd_status->handle, "info-");
    do {
      c = getch();
      switch (c) {
      case 's':
        st->info_flags ^= IFL_SHIPS;
        if (st->info_flags & IFL_SHIPS) statusline(st->wnd_status->handle, "info-ships true");
        else statusline(st->wnd_status->handle, "info-ships false");
        break;
      case 'b':
        st->info_flags ^= IFL_BUILDINGS;
        if (st->info_flags & IFL_BUILDINGS) statusline(st->wnd_status->handle, "info-buildings true");
        else statusline(st->wnd_status->handle, "info-buildings false");
      case 'f':
        st->info_flags ^= IFL_FACTIONS;
        if (st->info_flags & IFL_FACTIONS) statusline(st->wnd_status->handle, "info-factions true");
        else statusline(st->wnd_status->handle, "info-factions false");
        break;
      case 'u':
        st->info_flags ^= IFL_UNITS;
        if (st->info_flags & IFL_UNITS) statusline(st->wnd_status->handle, "info-units true");
        else statusline(st->wnd_status->handle, "info-units false");
        break;
      case 27: /* esc */
        break;
      default:
        beep();
        c = 0;
      }
    } while (c==0);
    break;
  case 'U':
    invert = true;
    /* !! intentional fall-through !! */
  case 'T':
    statusline(st->wnd_status->handle, "untag-"+(invert?0:2));
    findmode = getch();
    if (findmode=='n') { /* none */
      int i;
      statusline(st->wnd_status->handle, "tag-none");
      for (i=0;i!=MAXTHASH;++i) {
        tag ** tp = &st->selected->tags[i];
        while (*tp) {
          tag * t = *tp;
          *tp = t->nexthash;
          free(t);
        }
      }
    }
    else if (findmode=='u') {
      sprintf(sbuffer, "%stag-units", invert?"un":"");
      statusline(st->wnd_status->handle, sbuffer);
      for (r=regions;r;r=r->next) {
        if (r->units) {
          coordinate coord;
          if (invert) untag_region(st->selected, region2coord(r, &coord));
          else tag_region(st->selected, region2coord(r, &coord));
        }
      }
    } 
    else if (findmode=='s') {
      sprintf(sbuffer, "%stag-ships", invert?"un":"");
      statusline(st->wnd_status->handle, sbuffer);
      for (r=regions;r;r=r->next) {
        if (r->ships) {
          coordinate coord;
          if (invert) untag_region(st->selected, region2coord(r, &coord));
          else tag_region(st->selected, region2coord(r, &coord));
        }
      }
    } 
    else if (findmode=='f') {
      askstring(st->wnd_status->handle, "untag-faction:"+(invert?0:2), sbuffer, 12);
      if (sbuffer[0]) {
        faction * f = findfaction(atoi36(sbuffer));

        if (f!=NULL) {
          unit * u;
          coordinate coord;

          sprintf(sbuffer, "%stag-terrain: %s", invert?"un":"", itoa36(f->no));
          statusline(st->wnd_status->handle, sbuffer);
          for (u=f->units;u;u=u->nextF) {
            if (invert) untag_region(st->selected, region2coord(u->region, &coord));
            else tag_region(st->selected, region2coord(u->region, &coord));
          }
        } else {
          statusline(st->wnd_status->handle, "faction not found.");
          beep();
          break;
        }
      }
    }
    else if (findmode=='t') {
      const struct terrain_type * terrain;
      statusline(st->wnd_status->handle, "untag-terrain: "+(invert?0:2));
      terrain = select_terrain(NULL);
      if (terrain!=NULL) {
        sprintf(sbuffer, "%stag-terrain: %s", invert?"un":"", terrain->_name);
        statusline(st->wnd_status->handle, sbuffer);
        for (r=regions;r;r=r->next) {
          if (r->terrain==terrain) {
            coordinate coord;
            if (invert) untag_region(st->selected, region2coord(r, &coord));
            else tag_region(st->selected, region2coord(r, &coord));
          }
        }
      }
    } else {
      statusline(st->wnd_status->handle, "unknown command.");
      beep();
      break;
    }
    st->wnd_info->update |= 3;
    st->wnd_status->update |= 3;
    st->wnd_map->update |= 3;
    break;
  case ';':
    statusline(st->wnd_status->handle, "tag-");
    switch (getch()) {
    case 't':
      terraform_selection(st->selected, select_terrain(NULL));
      st->modified = 1;
      st->wnd_info->update |= 1;
      st->wnd_status->update |= 1;
      st->wnd_map->update |= 1;
      break;
    case 'm':
      break;
    default:
      statusline(st->wnd_status->handle, "unknown command.");
      beep();
    }
    break;
  case ' ':
    if (tagged_region(st->selected, cursor)) untag_region(st->selected, cursor);
    else tag_region(st->selected, cursor);
    break;
  case '/':
    statusline(st->wnd_status->handle, "find-");
    findmode = getch();
    if (findmode=='r') {
      askstring(st->wnd_status->handle, "find-region:", locate, sizeof(locate));
    } else if (findmode=='u') {
      askstring(st->wnd_status->handle, "find-unit:", locate, sizeof(locate));
    } else if (findmode=='f') {
      askstring(st->wnd_status->handle, "find-faction:", locate, sizeof(locate));
    } else {
      statusline(st->wnd_status->handle, "unknown command.");
      beep();
      break;
    }
    /* achtung: fall-through ist absicht: */
    if (!strlen(locate)) break;
  case 'n':
    if (findmode=='u') {
      unit * u = findunit(atoi36(locate));
      r = u?u->region:NULL;
    } else if (findmode && regions!=NULL) {
      struct faction * f = NULL;
      map_region * mr = cursor_region(&st->display, cursor);
      region * first = (mr->r && mr->r->next)?mr->r->next:regions;

      if (findmode=='f') {
        f = findfaction(atoi36(locate));
        if (f==NULL) {
          statusline(st->wnd_status->handle, "faction not found.");
          beep();
          break;
        }
      }
      for (r=first;;) {
        if (findmode=='r' && r->land && r->land->name && strstr(r->land->name, locate)) {
          break;
        } else if (findmode=='f') {
          unit * u;
          for (u=r->units;u;u=u->next) {
            if (u->faction==f) {
              break;
            }
          }
          if (u) break;
        }
        r = r->next;
        if (r==NULL) r = regions;
        if (r==first) {
          r = NULL;
          statusline(st->wnd_status->handle, "not found.");
          beep();
          break;
        }
      }
    } else {
      r = NULL;
    }
    if (r!=NULL) {
      region2coord(r, &st->cursor);
      st->wnd_info->update |= 1;
      st->wnd_status->update |= 1;
    }
    break;
  case 'Q':
    g_quit = 1;
    break;
  default:
    for (wnd=wnd_first;wnd!=NULL;wnd=wnd->next) {
      if (wnd->handlekey) {
        if (wnd->handlekey(wnd, st, c)) break;
      }
    }
    if (wnd==NULL) {
      sprintf(sbuffer, "getch: 0x%x", c);
      statusline(st->wnd_status->handle, sbuffer);
    }
    break;
  }
}

static void
init_view(view * display, WINDOW * win)
{
  display->topleft.x = 1;
  display->topleft.y = 1;
  display->topleft.p = 0;
  display->plane = 0;
  display->extent.width = getmaxx(win)/TWIDTH;
  display->extent.height = getmaxy(win)/THEIGHT;
  display->regions = calloc(display->extent.height * display->extent.width, sizeof(map_region));
}

static void
update_view(view * vi)
{
  int i, j;
  for (i=0;i!=vi->extent.width;++i) {
    for (j=0;j!=vi->extent.height;++j) {
      map_region * mr = mr_get(vi, i, j);
      mr->coord.x = vi->topleft.x + i - j/2;
      mr->coord.y = vi->topleft.y + j;
      mr->coord.p = vi->plane;
      mr->r = findregion((short)mr->coord.x, (short)mr->coord.y);
    }
  }
}

static void 
run_mapper(void)
{
  WINDOW * hwinstatus;
  WINDOW * hwininfo;
  WINDOW * hwinmap;
  int width, height, x, y;
  int split = 20;
  state st;
  point tl;
  init_curses();
  curs_set(1);

  getbegyx(stdscr, x, y);
  width = getmaxx(stdscr);
  height = getmaxy(stdscr);

  hwinmap = subwin(stdscr, getmaxy(stdscr)-1, getmaxx(stdscr)-split, y, x);
  hwininfo = subwin(stdscr, getmaxy(stdscr)-1, split, y, x+getmaxx(stdscr)-split);
  hwinstatus = subwin(stdscr, 1, width, height-1, x);

  st.wnd_map = win_create(hwinmap);
  st.wnd_map->paint = &paint_map;
  st.wnd_map->update = 1;
  st.wnd_info = win_create(hwininfo);
  st.wnd_info->paint = &paint_info_region;
  st.wnd_info->handlekey = &handle_info_region;
  st.wnd_info->update = 1;
  st.wnd_status = win_create(hwinstatus);
  st.wnd_status->paint = &paint_status;
  st.wnd_status->update = 1;
  st.display.plane = 0;
  st.cursor.p = 0;
  st.cursor.x = 0;
  st.cursor.y = 0;
  st.selected = calloc(1, sizeof(struct selection));
  st.modified = 0;
  st.info_flags = 0xFFFFFFFF;

  init_view(&st.display, hwinmap);
  coor2point(&st.display.topleft, &tl);

  while (!g_quit) {
    point p;
    window * wnd;
    view * vi = &st.display;

    getbegyx(hwinmap, x, y);
    width = getmaxx(hwinmap)-x;
    height = getmaxy(hwinmap)-y;
    coor2point(&st.cursor, &p);

    if (st.cursor.p != vi->plane) {
      vi->plane = st.cursor.p;
      st.wnd_map->update |= 1;
    }
    if (p.y < tl.y) {
      vi->topleft.y = st.cursor.y-vi->extent.height/2;
      st.wnd_map->update |= 1;
    }
    else if (p.y >= tl.y + vi->extent.height * THEIGHT) {
      vi->topleft.y = st.cursor.y-vi->extent.height/2;
      st.wnd_map->update |= 1;
    }
    if (p.x <= tl.x) {
      vi->topleft.x = st.cursor.x+(st.cursor.y-vi->topleft.y)/2-vi->extent.width / 2;
      st.wnd_map->update |= 1;
    }
    else if (p.x >= tl.x + vi->extent.width * TWIDTH-1) {
      vi->topleft.x = st.cursor.x+(st.cursor.y-vi->topleft.y)/2-vi->extent.width / 2;
      st.wnd_map->update |= 1;
    }

    if (st.wnd_map->update) {
      update_view(vi);
      coor2point(&vi->topleft, &tl);
    }
    for (wnd=wnd_last;wnd!=NULL;wnd=wnd->prev) {
      if (wnd->update && wnd->paint) {
        // if (wnd->update & 2) wclear(wnd->handle);
        if (wnd->update & 1) {
          wnd->paint(wnd, &st);
        }
        wnd->update = 0;
      }
    }
    draw_cursor(st.wnd_map->handle, st.selected, vi, &st.cursor, 1);
    for (wnd=wnd_first;wnd!=NULL;wnd=wnd->next) {
      wnoutrefresh(wnd->handle);
    }
    doupdate();
    draw_cursor(st.wnd_map->handle, st.selected, vi, &st.cursor, 0);

    handlekeys(&st);
  }
  curs_set(1);
  endwin();
}

int
main(int argc, char *argv[])
{
  int i;
  char * lc_ctype;
  char * lc_numeric;

  lc_ctype = setlocale(LC_CTYPE, "");
  lc_numeric = setlocale(LC_NUMERIC, "C");
  if (lc_ctype) lc_ctype = strdup(lc_ctype);
  if (lc_numeric) lc_numeric = strdup(lc_numeric);

  log_flags = LOG_FLUSH;
  log_open(g_logfile);

  global.data_version = RELEASE_VERSION;

  kernel_init();
  i = read_args(argc, argv);
  if (i!=0) return i;
  game_init();


  if (turn>first_turn) {
    char datafile[12];
    sprintf(datafile, "%u", turn);
    readgame(datafile, 0);
  }

  run_mapper();

  game_done();
  kernel_done();

  log_close();

  setlocale(LC_CTYPE, lc_ctype);
  setlocale(LC_NUMERIC, lc_numeric);

  free(lc_ctype);
  free(lc_numeric);

  return 0;
}
