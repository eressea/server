#include <platform.h>
#pragma warning(push)
#pragma warning(disable: 4100)
#include "config.pkg.c"
#include "eressea.pkg.c"
#include "game.pkg.c"
#include "locale.pkg.c"
#include "log.pkg.c"
#include "process.pkg.c"
#include "settings.pkg.c"
#pragma warning(pop)

void tolua_bind_open(lua_State * L) {
    tolua_eressea_open(L);
    tolua_process_open(L);
    tolua_settings_open(L);
    tolua_game_open(L);
    tolua_config_open(L);
    tolua_locale_open(L);
    tolua_log_open(L);
}
