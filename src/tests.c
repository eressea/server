#include <cutest/CuTest.h>
#include <stdio.h>

#include <platform.h>
#include "tests.h"

CuSuite* get_base36_suite(void);
CuSuite* get_curse_suite(void);
CuSuite* get_market_suite(void);
CuSuite* get_laws_suite(void);

#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <util/language.h>

int RunAllTests(void) {
  CuString *output = CuStringNew();
  CuSuite* suite = CuSuiteNew();

  init_resources();

  CuSuiteAddSuite(suite, get_base36_suite());
  CuSuiteAddSuite(suite, get_curse_suite());
  CuSuiteAddSuite(suite, get_market_suite());
  CuSuiteAddSuite(suite, get_laws_suite());

  CuSuiteRun(suite);
  CuSuiteSummary(suite, output);
  CuSuiteDetails(suite, output);
  printf("%s\n", output->buffer);
  return suite->failCount;
}

struct race * test_create_race(const char * name)
{
  race * rc = rc_add(rc_new("human"));
  return rc;
}

struct region * test_create_region(int x, int y, const struct terrain_type * terrain)
{
  region * r = new_region(x, y, NULL, 0);
  terraform_region(r, terrain);
  rsettrees(r, 0, 0);
  rsettrees(r, 1, 0);
  rsettrees(r, 2, 0);
  rsethorses(r, 0);
  rsetpeasants(r, terrain->size);
  return r;
}

struct faction * test_create_faction(const struct race * rc)
{
  faction * f = addfaction("nobody@eressea.de", NULL, rc, default_locale, 0);
  return f;
}

struct unit * test_create_unit(struct faction * f, struct region * r)
{
  unit * u = create_unit(r, f, 1, f->race, 0, 0, 0);
  return u;
}

void test_cleanup(void) {
  global.functions.maintenance = NULL;
  global.functions.wage = NULL;
  free_gamedata();
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
  terrain_type * t_plain, * t_ocean;
  region * island[2];
  race * rc_human;
  int i;
  building_type * btype;
  ship_type * stype;

  t_plain = calloc(1, sizeof(terrain_type));
  t_plain->_name = strdup("plain");
  t_plain->flags = LAND_REGION|FOREST_REGION|WALK_INTO;
  register_terrain(t_plain);

  t_ocean = calloc(1, sizeof(terrain_type));
  t_ocean->_name = strdup("ocean");
  t_ocean->flags = SEA_REGION|SAIL_INTO|SWIM_INTO;
  register_terrain(t_ocean);

  island[0] = test_create_region(0, 0, t_plain);
  island[1] = test_create_region(1, 0, t_plain);
  for (i=0;i!=2;++i) {
    direction_t j;
    region * r = island[i];
    for (j=0;j!=MAXDIRECTIONS;++j) {
      region * rn = r_connect(r, j);
      if (!rn) {
        rn = test_create_region(r->x+delta_x[j], r->y+delta_y[j], t_ocean);
      }
    }
  }

  rc_human = rc_add(rc_new("human"));
  rc_human->maintenance = 10;

  btype = calloc(sizeof(building_type), 1);
  btype->_name = strdup("castle");
  bt_register(btype);

  stype = calloc(sizeof(ship_type), 1);
  stype->name[0] = strdup("boat");
  stype->name[1] = strdup("boat_p");
  st_register(stype);

}
