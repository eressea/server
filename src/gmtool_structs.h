#ifndef H_GMTOOL_STRUCTS
#define H_GMTOOL_STRUCTS

#include <stddef.h>
#include <curses.h>

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
    /* Eine Koordinate im Editor, nicht normalisiert */
    int x, y;
    struct plane* pl;
} coordinate;

typedef struct map_region {
    struct region* r;
    coordinate coord;
} map_region;

typedef struct view {
    struct map_region* regions;
    struct plane* pl;
    coordinate topleft;         /* upper left corner in map. */
    extent size;                /* dimensions. */
} view;

typedef struct tag {
    coordinate coord;
    struct tag* nexthash;
} tag;

#define MAXTHASH 512

typedef struct selection {
    tag* tags[MAXTHASH];
} selection;

typedef struct state {
    coordinate cursor;
    selection* selected;
    struct state* prev;
    view display;
    int modified;
    unsigned int info_flags;
    struct window* wnd_info;
    struct window* wnd_map;
    struct window* wnd_status;
} state;

typedef struct window {
    bool(*handlekey) (struct window* win, struct state* st, int key);
    void (*paint) (struct window* win, const struct state* st);

    WINDOW* handle;
    void* data;
    struct window* next;
    struct window* prev;
    bool initialized;
    int update;
} window;

extern state* current_state;

map_region* cursor_region(const view* v, const coordinate* c);
void cnormalize(const coordinate* c, int* x, int* y);
void set_info_function(void (*callback) (struct window*,
    const struct state*));

#define TWIDTH  2               /* width of tile */
#define THEIGHT 1               /* height of tile */

#endif
