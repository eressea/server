#include <cutest/CuTest.h>
#include <stdio.h>

#include <platform.h>
#include "tests.h"

#include "tests_test.c"
#include <util/base36_test.c>
#include <util/functions_test.c>
#include <util/quicklist_test.c>
#include <util/umlaut_test.c>
#include <kernel/magic_test.c>
#include <kernel/move_test.c>
#include <kernel/item_test.c>
#include <kernel/ship_test.c>
#include <kernel/building_test.c>
#include <kernel/spell_test.c>
#include <kernel/curse_test.c>
#include <kernel/battle_test.c>
#include <kernel/pool_test.c>
#include <kernel/equipment_test.c>
#include <kernel/reports_test.c>
#include <kernel/spellbook_test.c>
#include <gamecode/laws_test.c>
#include <gamecode/market_test.c>

#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <util/functions.h>
#include <util/language.h>

int RunAllTests(void)
{
  CuString *output = CuStringNew();
  CuSuite *suite = CuSuiteNew();
  int flags = log_flags;

  log_flags = LOG_FLUSH | LOG_CPERROR;

  /* self-test */
  CuSuiteAddSuite(suite, get_tests_suite());
  /* util */
  CuSuiteAddSuite(suite, get_base36_suite());
  CuSuiteAddSuite(suite, get_quicklist_suite());
  CuSuiteAddSuite(suite, get_functions_suite());
  CuSuiteAddSuite(suite, get_umlaut_suite());
  /* kernel */
  CuSuiteAddSuite(suite, get_pool_suite());
  CuSuiteAddSuite(suite, get_curse_suite());
  CuSuiteAddSuite(suite, get_equipment_suite());
  CuSuiteAddSuite(suite, get_item_suite());
  CuSuiteAddSuite(suite, get_magic_suite());
  CuSuiteAddSuite(suite, get_move_suite());
  CuSuiteAddSuite(suite, get_reports_suite());
  CuSuiteAddSuite(suite, get_ship_suite());
  CuSuiteAddSuite(suite, get_spellbook_suite());
  CuSuiteAddSuite(suite, get_building_suite());
  CuSuiteAddSuite(suite, get_spell_suite());
  CuSuiteAddSuite(suite, get_battle_suite());
  /* gamecode */
  CuSuiteAddSuite(suite, get_market_suite());
  CuSuiteAddSuite(suite, get_laws_suite());

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);

  log_flags = flags;
  return suite->failCount;
}

struct race *test_create_race(const char *name)
{
  race *rc = rc_add(rc_new(name));
  rc->flags |= RCF_PLAYERRACE;
  rc->maintenance = 10;
  return rc;
}

struct region *test_create_region(int x, int y, const terrain_type *terrain)
{
  region *r = new_region(x, y, NULL, 0);
  terraform_region(r, terrain);
  rsettrees(r, 0, 0);
  rsettrees(r, 1, 0);
  rsettrees(r, 2, 0);
  rsethorses(r, 0);
  rsetpeasants(r, terrain->size);
  return r;
}

struct faction *test_create_faction(const struct race *rc)
{
  faction *f = addfaction("nobody@eressea.de", NULL, rc?rc:rc_find("human"), default_locale, 0);
  return f;
}

struct unit *test_create_unit(struct faction *f, struct region *r)
{
  unit *u = create_unit(r, f, 1, f?f->race:rc_find("human"), 0, 0, 0);
  return u;
}

void test_cleanup(void)
{
  test_clear_terrains();
  test_clear_resources();
  global.functions.maintenance = NULL;
  global.functions.wage = NULL;
  default_locale = 0;
  free_locales();
  free_spells();
  free_spellbooks();
  free_gamedata();
}

terrain_type *
test_create_terrain(const char * name, unsigned int flags)
{
  terrain_type * t;

  assert(!get_terrain(name));
  t = (terrain_type*)calloc(1, sizeof(terrain_type));
  t->_name = strdup(name);
  t->flags = flags;
  register_terrain(t);
  return t;
}

building * test_create_building(region * r, const building_type * btype)
{
  building * b = new_building(btype, r, default_locale);
  b->size = btype->maxsize>0?btype->maxsize:1;
  return b;
}

ship * test_create_ship(region * r, const ship_type * stype)
{
  ship * s = new_ship(stype, r, default_locale);
  s->size = stype->construction?stype->construction->maxsize:1;
  return s;
}

item_type * test_create_itemtype(const char ** names) {
  resource_type * rtype;
  item_type * itype;

  rtype = new_resourcetype(names, NULL, RTF_ITEM);
  itype = new_itemtype(rtype, ITF_ANIMAL|ITF_BIG, 5000, 7000);

  return itype;
}

/** creates a small world and some stuff in it.
 * two terrains: 'plain' and 'ocean'
 * one race: 'human'
 * one ship_type: 'boat'
 * one building_type: 'castle'
 * in 0.0 and 1.0 is an island of two plains, around it is ocean.
 */
void test_create_world(void)
{
  terrain_type *t_plain, *t_ocean;
  region *island[2];
  int i;
  building_type *btype;
  ship_type *stype;
  item_type * itype;
  const char * horses[2] = { "horse", "horse_p" };

  make_locale("de");
  init_resources();
  assert(!olditemtype[I_HORSE]);

  itype = test_create_itemtype(horses);
  olditemtype[I_HORSE] = itype;

  t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION);
  t_plain->size = 1000;
  t_ocean = test_create_terrain("ocean", SEA_REGION | SAIL_INTO | SWIM_INTO);
  t_ocean->size = 0;

  island[0] = test_create_region(0, 0, t_plain);
  island[1] = test_create_region(1, 0, t_plain);
  for (i = 0; i != 2; ++i) {
    int j;
    region *r = island[i];
    for (j = 0; j != MAXDIRECTIONS; ++j) {
      region *rn = r_connect(r, (direction_t)j);
      if (!rn) {
        rn = test_create_region(r->x + delta_x[j], r->y + delta_y[j], t_ocean);
      }
    }
  }

  test_create_race("human");

  btype = (building_type*)calloc(sizeof(building_type), 1);
  btype->flags = BTF_NAMECHANGE;
  btype->_name = strdup("castle");
  locale_setstring(default_locale, "castle", "castle");
  bt_register(btype);

  stype = (ship_type*)calloc(sizeof(ship_type), 1);
  stype->name[0] = strdup("boat");
  stype->name[1] = strdup("boat_p");
  locale_setstring(default_locale, "boat", "boat");
  st_register(stype);
}
