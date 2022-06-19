#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "eressea.h"

#include "donations.h"

#include "kernel/alliance.h"
#include "kernel/building.h"
#include "kernel/calendar.h"
#include "kernel/config.h"
#include "kernel/connection.h"
#include "kernel/curse.h"
#include "kernel/database.h"
#include "kernel/equipment.h"
#include "kernel/faction.h"
#include "kernel/item.h"
#include "kernel/plane.h"
#include "kernel/race.h"
#include "kernel/region.h"
#include "kernel/terrain.h"
#include "kernel/unit.h"

#include "util/aliases.h"
#include "util/functions.h"
#include "util/keyword.h"
#include "util/language.h"
#include "util/log.h"
#include "util/message.h"
#include "util/param.h"
#include "util/umlaut.h"

#include "modules/gmcmd.h"
#include "modules/xmas.h"
#include "modules/museum.h"
#include "triggers/triggers.h"
#include "items/xerewards.h"
#include "items/weapons.h"
#include "attributes/attributes.h"
#include "races/races.h"

#include "creport.h"
#include "items.h"
#include "magic.h"
#include "names.h"
#include "prefix.h"
#include "report.h"
#include "reports.h"
#include "spells.h"
#include "vortex.h"
#include "wormhole.h"

#include <util/strings.h>

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* manually free() everything at exit? */
#undef CLEANUP_CODE

void game_done(void)
{
    log_dead_factions();

#ifdef CLEANUP_CODE
    free_gamedata();
    creport_cleanup();
    report_cleanup();
    calendar_cleanup();
    free_functions();
    free_config();
    free_locales();
#endif
    free_aliases();
    free_prefixes();
    free_special_directions();
    kernel_done();
    swapdb_close();
}

void game_init(void)
{
    swapdb_open();
    errno = 0;
    kernel_init();
    register_triggers();
    register_xmas();

    register_nr();
    register_cr();

    register_races();
    register_spells();
    register_names();
    register_resources();
    register_itemfunctions();
    register_museum();
    wormholes_register();

    register_weapons();
    register_xerewards();
    register_attributes();
    register_gmcmd();

}

void init_terrains_translation(const struct locale *lang) {
    void **tokens;
    const terrain_type *terrain;

    tokens = get_translations(lang, UT_TERRAINS);
    for (terrain = terrains(); terrain != NULL; terrain = terrain->next) {
        variant var;
        const char *name;
        var.v = (void *)terrain;
        name = locale_string(lang, terrain->_name, false);
        if (name) {
            addtoken((struct tnode **)tokens, name, var);
        }
        else {
            log_debug("no translation for terrain %s in locale %s", terrain->_name, locale_name(lang));
        }
    }
}

void init_options_translation(const struct locale * lang) {
    void **tokens;
    int i;

    tokens = get_translations(lang, UT_OPTIONS);
    for (i = 0; i != MAXOPTIONS; ++i) {
        variant var;
        var.i = i;
        if (options[i]) {
            const char *name = locale_string(lang, options[i], false);
            if (name) {
                addtoken((struct tnode **)tokens, name, var);
            }
            else {
                log_debug("no translation for OPTION %s in locale %s", options[i], locale_name(lang));
            }
        }
    }
}

static void init_magic(struct locale *lang)
{
    void **tokens;
    tokens = get_translations(lang, UT_MAGIC);
    if (tokens) {
        const char *str = config_get("rules.magic.playerschools");
        char *sstr, *tok;
        if (str == NULL) {
            str = "gwyrrd illaun draig cerddor tybied";
        }

        sstr = str_strdup(str);
        tok = strtok(sstr, " ");
        while (tok) {
            variant var;
            const char *name;
            int i;
            for (i = 0; i != MAXMAGIETYP; ++i) {
                if (strcmp(tok, magic_school[i]) == 0) break;
            }
            assert(i != MAXMAGIETYP);
            var.i = i;
            name = LOC(lang, mkname("school", tok));
            if (name) {
                addtoken((struct tnode **)tokens, name, var);
            }
            else {
                log_warning("no translation for magic school %s in locale %s", tok, locale_name(lang));
            }
            tok = strtok(NULL, " ");
        }
        free(sstr);
    }
}

void init_locale(struct locale *lang)
{
    init_magic(lang);
    init_directions(lang);
    init_keywords(lang);
    init_skills(lang);
    init_races(lang);
    init_parameters(lang);

    init_options_translation(lang);
    init_terrains_translation(lang);
}

/** releases all memory associated with the game state.
 * call this function before calling read_game() to load a new game
 * if you have a previously loaded state in memory.
 */
void free_gamedata(void)
{
    free_ids();
    free_factions();
    free_donations();
    free_units();
    free_regions();
    free_borders();
    free_alliances();

    while (planes) {
        plane *pl = planes;
        planes = planes->next;
        free_plane(pl);
    }
}
