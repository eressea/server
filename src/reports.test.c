#include <platform.h>
#include <config.h>
#include "reports.h"
#include "report.h"

#include <kernel/building.h>
#include <kernel/faction.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/ship.h>
#include <kernel/unit.h>

#include <quicklist.h>
#include <stream.h>
#include <memstream.h>

#include <CuTest.h>
#include <tests.h>

#include <string.h>

static void test_reorder_units(CuTest * tc)
{
    region *r;
    building *b;
    ship * s;
    unit *u0, *u1, *u2, *u3, *u4;
    struct faction * f;
    const building_type *btype;
    const ship_type *stype;

    test_cleanup();
    test_create_world();

    btype = bt_find("castle");
    stype = st_find("boat");

    r = findregion(-1, 0);
    b = test_create_building(r, btype);
    s = test_create_ship(r, stype);
    f = test_create_faction(0);

    u0 = test_create_unit(f, r);
    u_set_ship(u0, s);
    u1 = test_create_unit(f, r);
    u_set_ship(u1, s);
    ship_set_owner(u1);
    u2 = test_create_unit(f, r);
    u3 = test_create_unit(f, r);
    u_set_building(u3, b);
    u4 = test_create_unit(f, r);
    u_set_building(u4, b);
    building_set_owner(u4);

    reorder_units(r);

    CuAssertPtrEquals(tc, u4, r->units);
    CuAssertPtrEquals(tc, u3, u4->next);
    CuAssertPtrEquals(tc, u2, u3->next);
    CuAssertPtrEquals(tc, u1, u2->next);
    CuAssertPtrEquals(tc, u0, u1->next);
    CuAssertPtrEquals(tc, 0, u0->next);
}

static void test_regionid(CuTest * tc) {
    size_t len;
    const struct terrain_type * plain;
    struct region * r;
    char buffer[64];

    test_cleanup();
    plain = test_create_terrain("plain", 0);
    r = test_create_region(0, 0, plain);

    memset(buffer, 0x7d, sizeof(buffer));
    len = f_regionid(r, 0, buffer, sizeof(buffer));
    CuAssertIntEquals(tc, 11, (int)len);
    CuAssertStrEquals(tc, "plain (0,0)", buffer);

    memset(buffer, 0x7d, sizeof(buffer));
    len = f_regionid(r, 0, buffer, 11);
    CuAssertIntEquals(tc, 10, (int)len);
    CuAssertStrEquals(tc, "plain (0,0", buffer);
    CuAssertIntEquals(tc, 0x7d, buffer[11]);
}

static void test_seen_faction(CuTest *tc) {
    faction *f1, *f2;
    race *rc = test_create_race("human");
    f1 = test_create_faction(rc);
    f2 = test_create_faction(rc);
    add_seen_faction(f1, f2);
    CuAssertPtrEquals(tc, f2, ql_get(f1->seen_factions, 0));
    CuAssertIntEquals(tc, 1, ql_length(f1->seen_factions));
    add_seen_faction(f1, f2);
    CuAssertIntEquals(tc, 1, ql_length(f1->seen_factions));
    add_seen_faction(f1, f1);
    CuAssertIntEquals(tc, 2, ql_length(f1->seen_factions));
    f2 = (faction *)ql_get(f1->seen_factions, 1);
    f1 = (faction *)ql_get(f1->seen_factions, 0);
    CuAssertTrue(tc, f1->no < f2->no);
}

static void test_write_spaces(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    size_t len;

    mstream_init(&out);
    write_spaces(&out, 4);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = '\0';
    CuAssertStrEquals(tc, "    ", buf);
    CuAssertIntEquals(tc, ' ', buf[3]);
    mstream_done(&out);
}

static void test_write_many_spaces(CuTest *tc) {
    stream out = { 0 };
    char buf[1024];
    size_t len;

    mstream_init(&out);
    write_spaces(&out, 100);
    out.api->rewind(out.handle);
    len = out.api->read(out.handle, buf, sizeof(buf));
    buf[len] = '\0';
    CuAssertIntEquals(tc, 100, (int)len);
    CuAssertIntEquals(tc, ' ', buf[99]);
    mstream_done(&out);
}

CuSuite *get_reports_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_reorder_units);
    SUITE_ADD_TEST(suite, test_seen_faction);
    SUITE_ADD_TEST(suite, test_regionid);
    SUITE_ADD_TEST(suite, test_write_spaces);
    SUITE_ADD_TEST(suite, test_write_many_spaces);
    return suite;
}
