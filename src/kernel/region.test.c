#include <platform.h>

#include "region.h"
#include "resources.h"
#include "building.h"
#include "unit.h"
#include "terrain.h"
#include "item.h"

#include <CuTest.h>
#include <tests.h>

void test_terraform(CuTest *tc) {
    region *r;
    terrain_type *t_plain, *t_ocean;
    item_type *itype;

    test_setup();
    itype = test_create_itemtype("oil");
    itype->rtype->flags |= (RTF_ITEM | RTF_POOLED);
    new_luxurytype(itype, 0);

    t_plain = test_create_terrain("plain", LAND_REGION);
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    r = test_create_region(0, 0, t_ocean);
    CuAssertPtrEquals(tc, NULL, r->land);
    terraform_region(r, t_plain);
    CuAssertPtrNotNull(tc, r->land);
    CuAssertPtrNotNull(tc, r->land->demands);
    CuAssertPtrEquals(tc, itype, (void *)r->land->demands->type->itype);
    CuAssertIntEquals(tc, 0, r->land->demands->type->price);
    terraform_region(r, t_ocean);
    CuAssertPtrEquals(tc, NULL, r->land);
    test_teardown();
}

static void test_region_get_owner(CuTest *tc) {
    region *r;
    building *b1, *b2;
    unit *u1, *u2;

    test_setup();
    r = test_create_region(0, 0, NULL);
    b1 = test_create_building(r, NULL);
    b2 = test_create_building(r, NULL);
    b1->size = 5;
    b2->size = 10;
    u1 = test_create_unit(test_create_faction(NULL), r);
    u_set_building(u1, b1);
    u2 = test_create_unit(test_create_faction(NULL), r);
    u_set_building(u2, b2);
    CuAssertPtrEquals(tc, u2->faction, region_get_owner(r));
    test_teardown();
}

static void test_region_getset_resource(CuTest *tc) {
    region *r;
    item_type *itype;

    test_setup();
    init_resources();
    itype = test_create_itemtype("iron");
    itype->construction = calloc(1, sizeof(construction));
    rmt_create(itype->rtype);
    r = test_create_region(0, 0, NULL);

    region_setresource(r, itype->rtype, 50);
    CuAssertIntEquals(tc, 50, region_getresource(r, itype->rtype));

    region_setresource(r, get_resourcetype(R_HORSE), 10);
    CuAssertIntEquals(tc, 10, region_getresource(r, get_resourcetype(R_HORSE)));
    CuAssertIntEquals(tc, 10, rhorses(r));

    region_setresource(r, get_resourcetype(R_PEASANT), 10);
    CuAssertIntEquals(tc, 10, region_getresource(r, get_resourcetype(R_PEASANT)));
    CuAssertIntEquals(tc, 10, rpeasants(r));

    test_teardown();
}

static void test_trees(CuTest *tc) {
    region *r;

    test_setup();
    r = test_create_region(0, 0, NULL);
    rsettrees(r, 0, 1000);
    rsettrees(r, 1, 2000);
    rsettrees(r, 2, 3000);
    CuAssertIntEquals(tc, 1000, rtrees(r, 0));
    CuAssertIntEquals(tc, 2000, rtrees(r, 1));
    CuAssertIntEquals(tc, 3000, rtrees(r, 2));
    rsettrees(r, 0, MAXTREES);
    CuAssertIntEquals(tc, MAXTREES, rtrees(r, 0));
    rsettrees(r, 0, MAXTREES+100);
    CuAssertIntEquals(tc, MAXTREES, rtrees(r, 0));
    test_teardown();
}

CuSuite *get_region_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_terraform);
    SUITE_ADD_TEST(suite, test_trees);
    SUITE_ADD_TEST(suite, test_region_getset_resource);
    SUITE_ADD_TEST(suite, test_region_get_owner);
    return suite;
}
