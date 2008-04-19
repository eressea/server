/* vi: set ts=2:
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
  extern int gmmain(int argc, char *argv[]);
  extern int curses_readline(struct lua_State * L, const char * prompt);

  extern void highlight_region(struct region *r);
  extern void select_coordinate(struct selection * selected, int x, int y);
  extern void run_mapper(void);

  extern int force_color;

#ifdef __cplusplus
}
#endif

#endif
