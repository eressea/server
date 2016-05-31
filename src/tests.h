#ifndef ERESSEA_TESTS_H
#define ERESSEA_TESTS_H

#include <kernel/types.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

    #define ASSERT_DBL_DELTA 0.001

    struct region;
    struct unit;
    struct faction;
    struct building;
    struct ship;
    struct message;
    struct message_list;
    struct mlist;
    struct item_type;
    struct building_type;
    struct ship_type;
    struct terrain_type;
    struct castorder;
    struct spellparameter;
    struct spell;
    struct locale;

    struct CuTest;

    void test_cleanup(void);

    struct locale * test_create_locale(void);
    struct terrain_type * test_create_terrain(const char * name, unsigned int flags);
    struct race *test_create_race(const char *name);
    struct region *test_create_region(int x, int y,
        const struct terrain_type *terrain);
    struct faction *test_create_faction(const struct race *rc);
    struct unit *test_create_unit(struct faction *f, struct region *r);
    void test_create_world(void);
    struct item_type * test_create_horse(void);
    struct building * test_create_building(struct region * r, const struct building_type * btype);
    struct ship * test_create_ship(struct region * r, const struct ship_type * stype);
    struct item_type * test_create_itemtype(const char * name);
    struct ship_type *test_create_shiptype(const char * name);
    struct building_type *test_create_buildingtype(const char *name);
    void test_create_castorder(struct castorder *co, struct unit *u, int level, float force, int range, struct spellparameter *par);
    struct spell * test_create_spell(void);

    int RunAllTests(void);
    void test_translate_param(const struct locale *lang, param_t param, const char *text);
    const char * test_get_messagetype(const struct message *msg);
    struct message * test_find_messagetype(struct message_list *msgs, const char *name);
    struct message * test_get_last_message(struct message_list *mlist);
    void test_clear_messages(struct faction *f);
    void assert_message(struct CuTest * tc, struct message *msg, char *name, int numpar, ...);

    void disabled_test(void *suite, void (*)(struct CuTest *), const char *name);

#define DISABLE_TEST(SUITE, TEST) disabled_test(SUITE, TEST, #TEST)

#ifdef __cplusplus
}
#endif
#endif
