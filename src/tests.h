#ifdef __cplusplus
extern "C" {
#endif

#ifndef DISABLE_TESTS
void test_cleanup(void);

struct race * test_create_race(const char * name);
struct region * test_create_region(int x, int y, const struct terrain_type * terrain);
struct faction * test_create_faction(const struct race * rc);
struct unit * test_create_unit(struct faction * f, struct region * r);
void test_create_world(void);

int RunAllTests(void);
#else
#define RunAllTests() 0
#endif

#ifdef __cplusplus
}
#endif
