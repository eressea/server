#include "wormhole.h"
#include "tests.h"

#include <kernel/building.h>
#include <kernel/region.h>
#include <kernel/unit.h>

#include <kernel/attrib.h>
#include <util/message.h>

#include <selist.h>

#include <CuTest.h>

void sort_wormhole_regions(selist *rlist, region **match, int count);
void make_wormholes(region **match, int count, const building_type *bt_wormhole);

static void setup_wormholes(void) {
    mt_create_va(mt_new("wormhole_appear", NULL),
        "region:region", MT_NEW_END);
    mt_create_va(mt_new("wormhole_dissolve", NULL),
        "region:region", MT_NEW_END);
    mt_create_va(mt_new("wormhole_exit", NULL),
        "unit:unit", "region:region", MT_NEW_END);
    mt_create_va(mt_new("wormhole_requirements", NULL),
        "unit:unit", "region:region", MT_NEW_END);
}

static void test_make_wormholes(CuTest *tc) {
    region *r1, *r2, *match[2];
    building_type *btype;

    test_setup();
    setup_wormholes();
    btype = test_create_buildingtype("wormhole");
    btype->maxsize = 4;
    btype->maxcapacity = 4;
    match[0] = r1 = test_create_plain(0, 0);
    match[1] = r2 = test_create_plain(1, 0);
    make_wormholes(match, 2, btype);
    CuAssertPtrNotNull(tc, r1->buildings);
    CuAssertPtrNotNull(tc, r1->buildings->attribs);
    CuAssertPtrEquals(tc, (void *)r1->buildings->type, (void *)btype);
    CuAssertPtrNotNull(tc, r2->buildings);
    CuAssertPtrNotNull(tc, r2->buildings->attribs);
    CuAssertPtrEquals(tc, (void *)r2->buildings->type, (void *)btype);
    CuAssertPtrEquals(tc, (void *)r1->buildings->attribs->type, (void *)r2->buildings->attribs->type);
    CuAssertStrEquals(tc, r1->buildings->attribs->type->name, "wormhole");
    test_teardown();
}

static void test_sort_wormhole_regions(CuTest *tc) {
    region *r1, *r2, *match[2];
    selist *rlist = 0;

    test_setup();
    setup_wormholes();
    r1 = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    r1->age = 4;
    r2->age = 2;
    selist_push(&rlist, r1);
    selist_push(&rlist, r2);
    sort_wormhole_regions(rlist, match, 2);
    CuAssertPtrEquals(tc, r2, match[0]);
    CuAssertPtrEquals(tc, r1, match[1]);
    selist_free(rlist);
    test_teardown();
}

static void test_wormhole_transfer(CuTest *tc) {
    region *r1, *r2;
    building *b;
    unit *u1, *u2;
    struct faction *f;

    test_setup();
    setup_wormholes();
    r1 = test_create_plain(0, 0);
    r2 = test_create_plain(1, 0);
    b = test_create_building(r1, NULL);
    b->size = 4;
    f = test_create_faction();
    u1 = test_create_unit(f, r1);
    u1->number = 2;
    u_set_building(u1, b);
    u2 = test_create_unit(f, r1);
    u2->number = 3;
    u_set_building(u2, b);
    u1 = test_create_unit(f, r1);
    u1->number = 2;
    u_set_building(u1, b);
    wormhole_transfer(b, r2);
    CuAssertPtrEquals(tc, u2, r1->units);
    CuAssertPtrEquals(tc, NULL, u2->building);
    CuAssertPtrEquals(tc, NULL, u2->next);
    CuAssertPtrEquals(tc, r2, u1->region);
    CuAssertPtrEquals(tc, u1, r2->units->next);
    test_teardown();
}

CuSuite *get_wormhole_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_sort_wormhole_regions);
    SUITE_ADD_TEST(suite, test_make_wormholes);
    SUITE_ADD_TEST(suite, test_wormhole_transfer);
    return suite;
}
