/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2006   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *  
 */

#ifndef H_GMTOOL_STRUCTS
#define H_GMTOOL_STRUCTS

#ifdef __cplusplus
extern "C" {
#endif

/* types imported from eressea: */
struct region;

typedef struct extent {
  /* Ein Vektor */
  int width, height;
} extent;

typedef struct point {
  /* Eine Koordinate in einer Ascii-Karte */
  int x, y;
} point;

typedef struct coordinate {
  /* Eine Koordinate in der Welt */
  int x, y, p;
} coordinate;

typedef struct map_region {
  struct region * r;
  coordinate coord;
} map_region;

typedef struct view {
  struct map_region * regions;
  int plane;
  coordinate topleft; /* upper left corner in map. */
  extent extent; /* dimensions. */
} view;

typedef struct tag {
  coordinate coord;
  struct tag * nexthash;
} tag;

#define MAXTHASH 512

typedef struct selection {
  tag * tags[MAXTHASH];
} selection;

typedef struct state {
  coordinate cursor;
  selection * selected;
  struct state * undo;
  struct faction * topf;
  view display;
  int modified;
  unsigned int info_flags;
  struct window * wnd_info;
  struct window * wnd_map;
  struct window * wnd_status;
} state;

extern map_region * cursor_region(const view * v, const coordinate * c);
extern state * current_state;

#define TWIDTH  2 /* width of tile */
#define THEIGHT 1 /* height of tile */

#ifdef __cplusplus
}
#endif

#endif
