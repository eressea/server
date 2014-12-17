#include <platform.h>
#include "tests.h"
#include "keyword.h"

#include <kernel/config.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/building.h>
#include <kernel/ship.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/terrain.h>
#include <util/functions.h>
#include <util/language.h>
#include <util/log.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct race *test_create_race(const char *name)
{
  race *rc = rc_get_or_create(name);
  rc->maintenance = 10;
  rc->ec_flags |= GETITEM | GIVEITEM;
  return rc;
}

struct region *test_create_region(int x, int y, const terrain_type *terrain)
{
  region *r = new_region(x, y, NULL, 0);
  terraform_region(r, terrain ? terrain : get_or_create_terrain("plain"));
  rsettrees(r, 0, 0);
  rsettrees(r, 1, 0);
  rsettrees(r, 2, 0);
  rsethorses(r, 0);
  rsetpeasants(r, r->terrain->size);
  return r;
}

struct faction *test_create_faction(const struct race *rc)
{
  faction *f = addfaction("nobody@eressea.de", NULL, rc?rc:rc_get_or_create("human"), default_locale, 0);
  return f;
}

struct unit *test_create_unit(struct faction *f, struct region *r)
{
    const struct race * rc = f ? f->race : 0;
    assert(f || !r);
    return create_unit(r, f, 1, rc ? rc : rc_get_or_create("human"), 0, 0, 0);
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
    free_buildingtypes();
    free_shiptypes();
    free_races();
    free_spellbooks();
    free_gamedata();
}

terrain_type *
test_create_terrain(const char * name, unsigned int flags)
{
  terrain_type * t = get_or_create_terrain(name);
  t->_name = _strdup(name);
  t->flags = flags;
  return t;
}

building * test_create_building(region * r, const building_type * btype)
{
  building * b = new_building(btype?btype:bt_find("castle"), r, default_locale);
  b->size = b->type->maxsize>0?b->type->maxsize:1;
  return b;
}

ship * test_create_ship(region * r, const ship_type * stype)
{
  ship * s = new_ship(stype?stype:st_find("boat"), r, default_locale);
  s->size = s->type->construction?s->type->construction->maxsize:1;
  return s;
}

ship_type * test_create_shiptype(const char * name)
{
  ship_type * stype = st_get_or_create(name);
  locale_setstring(default_locale, name, name);
  return stype;
}

building_type * test_create_buildingtype(const char * name)
{
    building_type *btype = (building_type *)calloc(sizeof(building_type), 1);
    btype->flags = BTF_NAMECHANGE;
    btype->_name = _strdup(name);
    btype->construction = (construction *)calloc(sizeof(construction), 1);
    btype->construction->skill = SK_BUILDING;
    btype->construction->maxsize = -1;
    btype->construction->minskill = 1;
    btype->construction->reqsize = 1;
    btype->construction->materials = (requirement *)calloc(sizeof(requirement), 2);
    btype->construction->materials[1].number = 0;
    btype->construction->materials[0].number = 1;
    btype->construction->materials[0].rtype = get_resourcetype(R_STONE);
    if (default_locale) {
        locale_setstring(default_locale, name, name);
    }
    bt_register(btype);
    return btype;
}

item_type * test_create_itemtype(const char * name) {
  resource_type * rtype;
  item_type * itype;

  rtype = rt_get_or_create(name);
  itype = it_get_or_create(rtype);

  return itype;
}

void test_translate_param(const struct locale *lang, param_t param, const char *text) {
    struct critbit_tree **cb;
    
    assert(lang && text);
    cb = (struct critbit_tree **)get_translations(lang, UT_PARAMS);
    add_translation(cb, text, param);
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
  item_type * itype;
  struct locale * loc;

  loc = get_or_create_locale("de");
  locale_setstring(loc, keyword(K_RESERVE), "RESERVIEREN");
  locale_setstring(loc, "money", "SILBER");
  init_resources();

  itype = test_create_itemtype("horse");
  itype->flags |= ITF_BIG | ITF_ANIMAL;
  itype->weight = 5000;
  itype->capacity = 7000;

  test_create_itemtype("iron");
  test_create_itemtype("stone");

  t_plain = test_create_terrain("plain", LAND_REGION | FOREST_REGION | WALK_INTO | CAVALRY_REGION);
  t_plain->size = 1000;
  t_plain->max_road = 100;
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

  test_create_buildingtype("castle");
  test_create_shiptype("boat");
}

