#include <platform.h>

#include "region.h"
#include "terrain.h"
#include "item.h"

#include <CuTest.h>
#include <tests.h>

void test_terraform(CuTest *tc) {
    region *r;
    terrain_type *t_plain, *t_ocean;
    item_type *itype;

    test_setup();
    itype = test_create_itemtype("ointment");
    itype->rtype->flags |= (RTF_ITEM | RTF_POOLED);
    new_luxurytype(itype, 0);

    t_plain = test_create_terrain("plain", LAND_REGION);
    t_ocean = test_create_terrain("ocean", SEA_REGION);
    r = test_create_region(0, 0, t_ocean);
    CuAssertPtrEquals(tc, 0, r->land);
    terraform_region(r, t_plain);
    CuAssertPtrNotNull(tc, r->land);
    CuAssertPtrNotNull(tc, r->land->demands);
    CuAssertPtrEquals(tc, itype, (void *)r->land->demands->type->itype);
    CuAssertIntEquals(tc, 0, r->land->demands->type->price);
    terraform_region(r, t_ocean);
    CuAssertPtrEquals(tc, 0, r->land);
    test_cleanup();
}

CuSuite *get_region_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_terraform);
    return suite;
}
