#include <platform.h>
#include <kernel/config.h>
#include "economy.h"

#include <util/message.h>
#include <kernel/building.h>
#include <kernel/item.h>
#include <kernel/faction.h>
#include <kernel/messages.h>
#include <kernel/order.h>
#include <kernel/pool.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

static void test_give_control_building(CuTest * tc)
{
    unit *u1, *u2;
    building *b;
    struct faction *f;
    region *r;

    test_cleanup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    b = test_create_building(r, 0);
    u1 = test_create_unit(f, r);
    u_set_building(u1, b);
    u2 = test_create_unit(f, r);
    u_set_building(u2, b);
    CuAssertPtrEquals(tc, u1, building_owner(b));
    give_control(u1, u2);
    CuAssertPtrEquals(tc, u2, building_owner(b));
    test_cleanup();
}

static void test_give_control_ship(CuTest * tc)
{
    unit *u1, *u2;
    ship *sh;
    struct faction *f;
    region *r;

    test_cleanup();
    f = test_create_faction(0);
    r = test_create_region(0, 0, 0);
    sh = test_create_ship(r, 0);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, sh);
    u2 = test_create_unit(f, r);
    u_set_ship(u2, sh);
    CuAssertPtrEquals(tc, u1, ship_owner(sh));
    give_control(u1, u2);
    CuAssertPtrEquals(tc, u2, ship_owner(sh));
    test_cleanup();
}

struct steal {
    struct unit *u;
    struct region *r;
    struct faction *f;
};

static void setup_steal(struct steal *env, struct terrain_type *ter, struct race *rc) {
    env->r = test_create_region(0, 0, ter);
    env->f = test_create_faction(rc);
    env->u = test_create_unit(env->f, env->r);
}

static void test_steal_okay(CuTest * tc) {
    struct steal env;
    race *rc;
    struct terrain_type *ter;

    test_cleanup();
    ter = test_create_terrain("plain", LAND_REGION);
    rc = test_create_race("human");
    rc->flags = 0;
    setup_steal(&env, ter, rc);
    CuAssertPtrEquals(tc, 0, check_steal(env.u, 0));
    test_cleanup();
}

static void test_steal_nosteal(CuTest * tc) {
    struct steal env;
    race *rc;
    terrain_type *ter;
    message *msg;

    test_cleanup();
    ter = test_create_terrain("plain", LAND_REGION);
    rc = test_create_race("human");
    rc->flags = RCF_NOSTEAL;
    setup_steal(&env, ter, rc);
    CuAssertPtrNotNull(tc, msg = check_steal(env.u, 0));
    msg_release(msg);
    test_cleanup();
}

static void test_steal_ocean(CuTest * tc) {
    struct steal env;
    race *rc;
    terrain_type *ter;
    message *msg;

    test_cleanup();
    ter = test_create_terrain("ocean", SEA_REGION);
    rc = test_create_race("human");
    setup_steal(&env, ter, rc);
    CuAssertPtrNotNull(tc, msg = check_steal(env.u, 0));
    msg_release(msg);
    test_cleanup();
}

static struct unit *create_recruiter(void) {
    region *r;
    faction *f;
    unit *u;
    const resource_type* rtype;

    test_cleanup();
    test_create_world();

    r=findregion(0, 0);
    rsetpeasants(r, 999);
    f = test_create_faction(rc_find("human"));
    u = test_create_unit(f, r);
    rtype = get_resourcetype(R_SILVER);
    change_resource(u, rtype, 1000);
    return u;
}

static void test_heroes_dont_recruit(CuTest * tc) {
    unit *u;

    test_cleanup();

    u = create_recruiter();
    fset(u, UFL_HERO);
    unit_addorder(u, create_order(K_RECRUIT, default_locale, "1"));

    economics(u->region);

    CuAssertIntEquals(tc, 1, u->number);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error_herorecruit"));

    test_cleanup();
}

static void test_normals_recruit(CuTest * tc) {
    unit *u;

    test_cleanup();

    u = create_recruiter();
    unit_addorder(u, create_order(K_RECRUIT, default_locale, "1"));

    economics(u->region);

    CuAssertIntEquals(tc, 2, u->number);

    test_cleanup();
}

CuSuite *get_economy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_give_control_building);
    SUITE_ADD_TEST(suite, test_give_control_ship);
    SUITE_ADD_TEST(suite, test_steal_okay);
    SUITE_ADD_TEST(suite, test_steal_ocean);
    SUITE_ADD_TEST(suite, test_steal_nosteal);
    SUITE_ADD_TEST(suite, test_normals_recruit);
    SUITE_ADD_TEST(suite, test_heroes_dont_recruit);
    return suite;
}
