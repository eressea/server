#pragma once
#ifndef H_GMTOOL
#define H_GMTOOL

#include <stddef.h>
#include <stdbool.h>

struct lua_State;
struct selection;
struct state;
struct region;
struct terrain_type;
struct newfaction;

extern int force_color;

int curses_readline(struct lua_State *L, char *buffer, size_t size,
    const char *prompt);

void highlight_region(struct region *r, int on);
void select_coordinate(struct selection *selected, int x, int y, bool on);
void run_mapper(void);

struct state *state_open(void);
void state_close(struct state *);

void make_block(int x, int y, int radius, const struct terrain_type *terrain);
void seed_players(struct newfaction **players, bool new_island);

#endif
