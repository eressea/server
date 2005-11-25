extern "C" {
#include <lua50/lua.h>
#include <lua50/lauxlib.h>
#include <lua50/lualib.h>
}
#if BOOST_VERSION < 103300
# define LUABIND_OLD
#endif
