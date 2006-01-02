extern "C" {
#include <lua50/lua.h>
#include <lua50/lauxlib.h>
#include <lua50/lualib.h>
}

#ifndef LUABIND_BETA
# include <boost/version.hpp>
# if BOOST_VERSION < 103300
#  define LUABIND_BETA 7
#  define LUABIND_DEVEL 1
# else
#  define LUABIND_BETA 7
#  define LUABIND_DEVEL 2
# endif
#endif
