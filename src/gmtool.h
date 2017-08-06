/* 
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2006   |  
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *  
 */

#ifndef H_GMTOOL
#define H_GMTOOL

#ifdef __cplusplus
extern "C" {
#endif

    struct lua_State;
    struct selection;
    struct state;
    struct region;
    struct terrain_type;
    struct newfaction;

  int curses_readline(struct lua_State *L, char *buffer, size_t size,
    const char *prompt);

  void highlight_region(struct region *r, int on);
  void select_coordinate(struct selection *selected, int x, int y, int on);
  void run_mapper(void);

  extern int force_color;
  extern int gm_codepage;

  struct state *state_open(void);
  void state_close(struct state *);

  void make_block(int x, int y, int radius, const struct terrain_type *terrain);
  void seed_players(struct newfaction **players, bool new_island);

#ifdef __cplusplus
}
#endif
#endif
