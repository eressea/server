#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISABLE_TESTS
  struct building_type;
  struct building;
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
  int RunAllTests(void);
#else
#define RunAllTests() 0
#endif

#ifdef __cplusplus
}
#endif
