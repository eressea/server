#include <cutest/CuTest.h>

#include <util/language.h>
#include <kernel/terrain.h>

static void market_curse(CuTest * tc)
{
  region *r;
  building *b;
  unit *u;
  faction *f;
  int x, y;
  const char *names[4] = { "herb", "herbs", "balm", "balms" };
  terrain_type *terrain;
  resource_type *hres = new_resourcetype(names, 0, RTF_ITEM | RTF_POOLED);
  item_type *htype = new_itemtype(hres, ITF_HERB, 0, 0);
  resource_type *lres = new_resourcetype(names + 2, 0, RTF_ITEM | RTF_POOLED);
  item_type *ltype = new_itemtype(lres, ITF_NONE, 0, 0);
  luxury_type *lux = new_luxurytype(ltype, 0);
  building_type *btype;
  race *rc = rc_add(rc_new("human"));
  struct locale *lang = make_locale("en");

  free_gamedata();

  set_param(&global.parameters, "rules.region_owners", "1");

  btype = calloc(sizeof(building_type), 1);
  btype->_name = "market";
  bt_register(btype);

  terrain = test_create_terrain("plain", LAND_REGION | WALK_INTO);

  for (x = 0; x != 3; ++x) {
    for (y = 0; y != 3; ++y) {
      r = new_region(x, y, NULL, 0);
      terraform_region(r, terrain);
      rsetpeasants(r, 5000);
      r_setdemand(r, lux, 0);
      rsetherbtype(r, htype);
    }
  }
  r = findregion(1, 1);
  b = new_building(btype, r, lang);
  b->flags |= BLD_WORKING;
  b->size = b->type->maxsize;

  f = addfaction("nobody@eressea.de", NULL, rc, default_locale, 0);
  u = create_unit(r, f, 1, f->race, 0, 0, 0);
  u->building = b;
  u->flags |= UFL_OWNER;

  do_markets();

  CuAssertIntEquals(tc, 70, i_get(u->items, htype));
  CuAssertIntEquals(tc, 35, i_get(u->items, ltype));
}

CuSuite *get_market_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, market_curse);
  return suite;
}
