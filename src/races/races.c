/* vi: set ts=2:
 * +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 * |                   |  Enno Rehling <enno@eressea.de>
 * | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 * | (c) 1998 - 2007   |
 * |                   |  This program may not be used, modified or distributed
 * +-------------------+  without prior permission by the authors of Eressea.
 *
 */

#include <platform.h>
#include <kernel/config.h>
#include "races.h"

#include <kernel/building.h>
#include <kernel/equipment.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/pathfinder.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/attrib.h>
#include <util/functions.h>
#include <util/rng.h>

void age_firedragon(struct unit *u);

void age_dragon(struct unit *u);

void age_illusion(struct unit *u);

void age_undead(struct unit *u);

void age_skeleton(struct unit *u);

void age_zombie(struct unit *u);

void age_ghoul(struct unit *u);

static void oldfamiliars(unit * u)
{
  char fname[64];

  /* these familiars have no special skills.
   */
  snprintf(fname, sizeof(fname), "%s_familiar", u->race->_name[0]);
  create_mage(u, M_GRAY);
  equip_unit(u, get_equipment(fname));
}

static void set_show_item(faction * f, item_t i)
{
  attrib *a = a_add(&f->attribs, a_new(&at_showitem));

  a->data.v = (void *)olditemtype[i];
}

static void equip_newunits(const struct equipment *eq, struct unit *u)
{
  struct region *r = u->region;

  switch (old_race(u->race)) {
    case RC_ELF:
      set_show_item(u->faction, I_FEENSTIEFEL);
      break;
    case RC_GOBLIN:
      set_show_item(u->faction, I_RING_OF_INVISIBILITY);
      set_number(u, 10);
      break;
    case RC_HUMAN:
      if (u->building == NULL) {
        const building_type *btype = bt_find("castle");

        if (btype != NULL) {
          building *b = new_building(btype, r, u->faction->locale);

          b->size = 10;
          u->building = b;
          fset(u, UFL_OWNER);
        }
      }
      break;
    case RC_CAT:
      set_show_item(u->faction, I_RING_OF_INVISIBILITY);
      break;
    case RC_AQUARIAN:
    {
      ship *sh = new_ship(st_find("boat"), u->faction->locale, r);

      sh->size = sh->type->construction->maxsize;
      u->ship = sh;
      fset(u, UFL_OWNER);
    }
      break;
    case RC_CENTAUR:
      rsethorses(r, 250 + rng_int() % 51 + rng_int() % 51);
      break;
  }
}

/* Die Funktionen werden über den hier registrierten Namen in races.xml
 * in die jeweilige Rassendefiniton eingebunden */
void register_races(void)
{
  /* function initfamiliar */
  register_function((pf_generic) oldfamiliars, "oldfamiliars");

  register_function((pf_generic) allowed_dragon, "movedragon");

  register_function((pf_generic) allowed_swim, "moveswimming");
  register_function((pf_generic) allowed_fly, "moveflying");
  register_function((pf_generic) allowed_walk, "movewalking");

  /* function age for race->age() */
  register_function((pf_generic) age_undead, "ageundead");
  register_function((pf_generic) age_illusion, "ageillusion");
  register_function((pf_generic) age_skeleton, "ageskeleton");
  register_function((pf_generic) age_zombie, "agezombie");
  register_function((pf_generic) age_ghoul, "ageghoul");
  register_function((pf_generic) age_dragon, "agedragon");
  register_function((pf_generic) age_firedragon, "agefiredragon");

  /* function itemdrop
   * to generate battle spoils
   * race->itemdrop() */
  register_function((pf_generic) equip_newunits, "equip_newunits");
}
