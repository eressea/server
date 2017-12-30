#include <platform.h>

#include "academy.h"
#include "skill.h"

#include <kernel/config.h>
#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/item.h>
#include <kernel/region.h>

#include <CuTest.h>
#include "tests.h"

static void test_academy(CuTest * tc)
{
    faction *f;
    unit *u, *u2;
    region *r;
    building *b;
    const item_type *it_silver;

    test_setup();
    config_set_int("skills.cost.alchemy", 100);
    r = test_create_region(0, 0, NULL);
    f = test_create_faction(NULL);
    u = test_create_unit(f, r);
    b = test_create_building(r, test_create_buildingtype("academy"));
    u2 = test_create_unit(f, r);
    it_silver = test_create_silver();

    CuAssert(tc, "teacher must be in academy", !academy_can_teach(u, u2, SK_CROSSBOW));
    u_set_building(u, b);
    CuAssert(tc, "student must be in academy", !academy_can_teach(u, u2, SK_CROSSBOW));
    u_set_building(u2, b);
    CuAssert(tc, "student must have 50 silver", !academy_can_teach(u, u2, SK_CROSSBOW));
    i_change(&u2->items, it_silver, 50);
    CuAssert(tc, "building must be maintained", !academy_can_teach(u, u2, SK_CROSSBOW));
    b->flags |= BLD_MAINTAINED;
    CuAssert(tc, "building must have capacity", !academy_can_teach(u, u2, SK_CROSSBOW));
    b->size = 2;
    CuAssertTrue(tc, academy_can_teach(u, u2, SK_CROSSBOW));

    CuAssert(tc, "student must pay skillcost", !academy_can_teach(u, u2, SK_ALCHEMY));
    i_change(&u2->items, it_silver, 150);
    CuAssertTrue(tc, academy_can_teach(u, u2, SK_ALCHEMY));
    test_teardown();
}

CuSuite *get_academy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_academy);
    return suite;
}
