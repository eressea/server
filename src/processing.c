#include "processing.h"

#include "bind_config.h"
#include "eressea.h"
#include "laws.h"
#include "monsters.h"
#include "orderfile.h"

#include <races/races.h>

#include <kernel/calendar.h>
#include <kernel/callbacks.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/language.h>
#include <util/log.h>
#include <util/path.h>

#include <stdio.h>
#include <string.h>

static void equip_first_unit(unit *u)
{
    /** mirrors the E2 defaults as of 12/2024 */
    const struct equip_t {
        int count;
        const char *name;
    } equipment[] = {
        { 10, "log" },
        { 2500, "money" },
        { 4, "stone" },
        { 0, NULL },
    };
    int i;
    for (i = 0; equipment[i].count; ++i) {
        i_change(&u->items, it_find(equipment[i].name), equipment[i].count);
    }
    terraform_region(u->region, get_terrain("plain"));
    freset(u->region, RF_MALLORN);
    rsettrees(u->region, 0, 50);
    rsettrees(u->region, 1, 100);
    rsettrees(u->region, 2, 500);
    rsetpeasants(u->region, 4000);
    rsetmoney(u->region, 80000);
    rsethorses(u->region, 50);
}

static void equip_default(unit *u, const char *name)
{
    if (strcmp("first_unit", name) == 0) {
        equip_first_unit(u);
    }
    else if (strncmp("seed_", name, 5) == 0) {
        equip_newunits(u);
    }
}

void run_turn(void)
{
    char path[PATH_MAX];
    const char *install_path = config_get("config.install");
    const char *config_path = config_get("game.rules");
    int lastturn;
    /*
     * TODO:
     * default race skills for units (see equipment.lua)
     * spells that have lua-only implmentations (see spells.lua)
     */
    if (!config_path) {
        log_fatal("game.rules is null");
    }

    callbacks.equip_unit = equip_default;
    init_resources();
    if (0 != config_read(
        path_join(path_join("conf", config_path, path, sizeof(path)), "config.json", path, sizeof(path)),
        install_path))
    {
        log_fatal("could not read JSON data");
    }
    free_gamedata();
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
    plan_monsters(get_monsters());

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
