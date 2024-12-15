#include "processing.h"

#include "bind_config.h"
#include "eressea.h"
#include "laws.h"
#include "orderfile.h"

#include <kernel/calendar.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/save.h>

#include <util/language.h>
#include <util/log.h>
#include <util/path.h>

#include <stdio.h>

void run_turn(void)
{
    char path[PATH_MAX];
    const char *install_path = config_get("config.install");
    const char *config_path = config_get("game.rules");
    int lastturn;
    /*
     * TODO:
     * lua_changeresource replacement (see resources.lua)
     * default equipment sets (see equipment.lua)
     * replace the lua callbacks in helpers.c (see callbacks.h)
     * spells that have lua-only implmentations (see spells.lua)
     */
    if (!config_path) {
        log_fatal("game.rules is null");
    }

    if (0 != config_read(
        path_join(path_join("conf", config_path, path, sizeof(path)), "config.json", path, sizeof(path)),
        install_path))
    {
        log_fatal("could not read JSON data");
    }
    free_gamedata();
    init_resources();
    init_locales(init_locale);

    lastturn = turn;
    if (lastturn < 0) {
        lastturn = current_turn();
    }
    turn = lastturn;
    snprintf(path, sizeof(path), "%d.dat", turn);
    if (0 != readgame(path)) {
        log_fatal("could not read game data %s", path);
    }
    turn_begin();
    // plan_monsters();

    snprintf(path, sizeof(path), "orders.%d", turn);
    if (0 != readorders(path)) {
        log_fatal("could not read game data %s", path);
    }
    turn_process();

    turn_end();
    remove_empty_factions();
    snprintf(path, sizeof(path), "%d.dat", turn);
    if (0 != writegame(path)) {
        log_fatal("could not write game data %s", path);
    }
}
