#include "races.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/pathfinder.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <kernel/attrib.h>
#include <util/functions.h>
#include <util/rng.h>

#include <string.h>
#include <stdio.h>

void age_firedragon(struct unit *u);
void age_dragon(struct unit *u);
void age_skeleton(struct unit *u);
void age_zombie(struct unit *u);
void age_ghoul(struct unit *u);

void equip_newunits(struct unit *u)
{
    struct region *r = u->region;
    const struct resource_type *rtype;
    switch (old_race(u_race(u))) {
    case RC_ELF:
        rtype = rt_find("fairyboot");
        set_show_item(u->faction, rtype->itype);
        break;
    case RC_GOBLIN:
        rtype = rt_find("roi");
        set_show_item(u->faction, rtype->itype);
        scale_number(u, 10);
	
        break;
    case RC_HUMAN:
        if (u->building == NULL) {
            const building_type *btype = bt_find("castle");
            if (btype != NULL) {
                building *b = new_building(btype, r, u->faction->locale, 10);
                u_set_building(u, b);
                building_set_owner(u);
            }
        }
        break;
    case RC_CAT:
        rtype = rt_find("roi");
        set_show_item(u->faction, rtype->itype);
        break;
    case RC_AQUARIAN:
    {
        ship *sh = new_ship(st_find("boat"), r, u->faction->locale);
        sh->size = sh->type->construction->maxsize;
        u_set_ship(u, sh);
    }
    break;
    default:
        break;
    }
}

/* Die Funktionen werden ueber den hier registrierten Namen in races.xml
 * in die jeweilige Rassendefiniton eingebunden */
void register_races(void)
{
    /* function age for race->age() */
    register_function((pf_generic)age_skeleton, "age_skeleton");
    register_function((pf_generic)age_zombie, "age_zombie");
    register_function((pf_generic)age_ghoul, "age_ghoul");
    register_function((pf_generic)age_dragon, "age_dragon");
    register_function((pf_generic)age_firedragon, "age_youngdragon");
}
