#include <platform.h>

#include "lighthouse.h"

#include <util/attrib.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/region.h>
#include <kernel/building.h>
#include <kernel/terrain.h>


#include <CuTest.h>
#include "tests.h"

static void test_lighthouse_range(CuTest * tc)
{
    faction *f;
    unit *u;
    region *r1;
    building *b;

    test_setup();
    r1 = test_create_region(0, 0, 0);
    f = test_create_faction(0);
    u = test_create_unit(f, r1);
    b = test_create_building(r1, test_create_buildingtype("lighthouse"));
    CuAssertIntEquals(tc, 0, lighthouse_range(b, NULL));
    CuAssertIntEquals(tc, 0, lighthouse_range(b, f));
    b->size = 10;
    CuAssertIntEquals(tc, 0, lighthouse_range(b, NULL));
    u->building = b;
    set_level(u, SK_PERCEPTION, 3);
    CuAssertIntEquals(tc, 0, lighthouse_range(b, NULL));
    b->flags |= BLD_MAINTAINED;
    CuAssertIntEquals(tc, 1, lighthouse_range(b, NULL));
    set_level(u, SK_PERCEPTION, 6);
    CuAssertIntEquals(tc, 2, lighthouse_range(b, NULL));
    b->size = 100;
    CuAssertIntEquals(tc, 2, lighthouse_range(b, NULL));
    set_level(u, SK_PERCEPTION, 9);
    CuAssertIntEquals(tc, 3, lighthouse_range(b, NULL));
    CuAssertIntEquals(tc, 3, lighthouse_range(b, f));
    CuAssertIntEquals(tc, 0, lighthouse_range(b, test_create_faction(0)));
    test_cleanup();
}

static void test_lighthouse_update(CuTest * tc)
{
    region *r1, *r2, *r3;
    building *b;
    const struct terrain_type *t_ocean, *t_plain;

    test_setup();
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    t_plain = test_create_terrain("plain", LAND_REGION);
    r1 = test_create_region(0, 0, t_plain);
    r2 = test_create_region(1, 0, t_ocean);
    r3 = test_create_region(2, 0, t_ocean);
    b = test_create_building(r1, test_create_buildingtype("lighthouse"));
    CuAssertIntEquals(tc, 0, r1->flags&RF_LIGHTHOUSE);
    CuAssertPtrEquals(tc, NULL, r1->attribs);
    CuAssertPtrEquals(tc, NULL, r2->attribs);
    CuAssertPtrEquals(tc, NULL, r3->attribs);

    r1->flags = 0;
    b->size = 1;
    update_lighthouse(b);
    CuAssertIntEquals(tc, RF_LIGHTHOUSE, r1->flags&RF_LIGHTHOUSE);
    CuAssertPtrNotNull(tc, r2->attribs);
    CuAssertPtrEquals(tc, (void *)&at_lighthouse, (void *)r2->attribs->type);
    CuAssertPtrEquals(tc, NULL, r1->attribs);
    CuAssertPtrEquals(tc, NULL, r3->attribs);

    a_removeall(&r2->attribs, NULL);
    r1->flags = 0;
    b->size = 10;
    update_lighthouse(b);
    CuAssertIntEquals(tc, RF_LIGHTHOUSE, r1->flags&RF_LIGHTHOUSE);
    CuAssertPtrNotNull(tc, r2->attribs);
    CuAssertPtrEquals(tc, (void *)&at_lighthouse, (void *)r2->attribs->type);
    CuAssertPtrNotNull(tc, r3->attribs);
    CuAssertPtrEquals(tc, (void *)&at_lighthouse, (void *)r3->attribs->type);
    test_cleanup();
}

CuSuite *get_lighthouse_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_lighthouse_range);
    SUITE_ADD_TEST(suite, test_lighthouse_update);
    return suite;
}
