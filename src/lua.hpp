#ifdef __cplusplus
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#ifdef __cplusplus
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

#endif
