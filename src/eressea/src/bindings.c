#include <platform.h>

#include "spells/shipcurse.h"

#include <kernel/ship.h>
#include <kernel/unit.h>

#include <tolua.h>

static int
tolua_levitate_ship(lua_State * L)
{
  ship * sh = (ship *)tolua_tousertype(L, 1, 0);
  unit * mage = (unit *)tolua_tousertype(L, 2, 0);
  double power = (double)tolua_tonumber(L, 3, 0);
  int duration = (int)tolua_tonumber(L, 4, 0);
  int cno = levitate_ship(sh, mage, power, duration);
  tolua_pushnumber(L, (lua_Number)cno);
  return 1;
}

void
bind_eressea(struct lua_State * L)
{
  tolua_module(L, NULL, 0);
  tolua_beginmodule(L, NULL);
  {
    tolua_function(L, TOLUA_CAST "levitate_ship", tolua_levitate_ship);
  }
  tolua_endmodule(L);
}

