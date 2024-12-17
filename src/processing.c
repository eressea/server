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
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/save.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/language.h>
#include <util/log.h>
#include <util/path.h>
#include <util/rng.h>

#include <stdio.h>
#include <string.h>

typedef struct equip_t {
    int count;
    const char *name;
} equip_t;

static void equip_items(unit *u, const struct equip_t equipment[])
{
    int i;
    for (i = 0; equipment[i].count; ++i) {
        i_change(&u->items, it_find(equipment[i].name), equipment[i].count);
    }
}

static void equip_first_unit(unit *u, int mask)
{
    /** mirrors the E2 defaults as of 12/2024 */
    const equip_t equipment[] = {
        { 10, "log" },
        { 2500, "money" },
        { 4, "stone" },
        { 0, NULL }
    };
    if (mask & EQUIP_ITEMS) {
        equip_items(u, equipment);
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

static bool equip_undead(unit *u, int mask)
{
    i_change(&u->items, it_find("rustysword"), u->number);
    if (0 == (rng_int() % 2)) {
        i_change(&u->items, it_find("rustychainmail"), u->number);
    }
    if (0 == (rng_int() % 3)) {
        i_change(&u->items, it_find("rustyshield"), u->number);
    }
    return false;
}

static bool equip_monster_spoils(unit *u, const char *rcname, int mask)
{
    const struct monster_t
    {
        const char *rcname;
        equip_t items[3];
    } spoils[] = {
        {
            "seaserpent", {
                { 6, "dragonblood" },
                { 1, "dragonhead" },
                { 0, NULL }
            }
        },
        {
            "youngdragon", {
                { 1, "dragonhead" },
                { 0, NULL }
            },
        },
        {
            "dragon", {
                { 4, "dragonblood" },
                { 1, "dragonhead" },
                { 0, NULL }
            },
        },
        {
            "wyrm", {
                { 10, "dragonblood" },
                { 1, "dragonhead" },
                { 0, NULL }
            },
        },
        {
            NULL,
        }
    };
    const race *rc = rc_find(rcname);
    if (rcname) {
        int i;
        for (i = 0; spoils[i].rcname; ++i) {
            if (0 == strcmp(rcname, spoils[i].rcname)) {
                equip_items(u, spoils[i].items);
            }
        }
    }
    return false;
}

static bool equip_default(unit *u, const char *name, int mask)
{
    if (strncmp("new_", name, 4) == 0) {
        // TODO: new unit, i.e. new_orc, see init.lua
        return true;
    }
    else if (strncmp("spo_", name, 4) == 0) {
        // TODO: monster spoils, see init.lua
        return equip_monster_spoils(u, name + 4, mask);
    }
    else if (strncmp("seed_", name, 5) == 0) {
        equip_newunits(u);
        return true;
    }
    else if (strcmp("first_unit", name) == 0) {
        equip_first_unit(u, mask);
        return true;
    }
    else if (strcmp("rising_undead", name) == 0) {
        return equip_undead(u, mask);
    }
    return false;
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
