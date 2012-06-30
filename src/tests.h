#ifndef ERESSEA_TESTS_H
#define ERESSEA_TESTS_H
#ifdef __cplusplus
extern "C" {
#endif
  #include <CuTest.h>
  
  CuSuite *get_tests_suite(void);
  CuSuite *get_economy_suite(void);
  CuSuite *get_laws_suite(void);
  CuSuite *get_market_suite(void);
  CuSuite *get_battle_suite(void);
  CuSuite *get_building_suite(void);
  CuSuite *get_curse_suite(void);
  CuSuite *get_equipment_suite(void);
  CuSuite *get_item_suite(void);
  CuSuite *get_magic_suite(void);
  CuSuite *get_move_suite(void);
  CuSuite *get_pool_suite(void);
  CuSuite *get_reports_suite(void);
  CuSuite *get_ship_suite(void);
  CuSuite *get_spellbook_suite(void);
  CuSuite *get_spell_suite(void);
  CuSuite *get_base36_suite(void);
  CuSuite *get_bsdstring_suite(void);
  CuSuite *get_functions_suite(void);
  CuSuite *get_umlaut_suite(void);
  CuSuite *get_ally_suite(void);

  void test_cleanup(void);

  struct terrain_type * test_create_terrain(const char * name, unsigned int flags);
  struct race *test_create_race(const char *name);
  struct region *test_create_region(int x, int y,
    const struct terrain_type *terrain);
  struct faction *test_create_faction(const struct race *rc);
  struct unit *test_create_unit(struct faction *f, struct region *r);
  void test_create_world(void);
  struct building * test_create_building(struct region * r, const struct building_type * btype);
  struct ship * test_create_ship(struct region * r, const struct ship_type * stype);
  struct item_type * test_create_itemtype(const char ** names);
  int RunAllTests(void);

#ifdef __cplusplus
}
#endif
#endif
