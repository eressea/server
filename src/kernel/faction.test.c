#include <platform.h>
#include <kernel/types.h>
#include <kernel/config.h>
#include "faction.h"
#include <CuTest.h>
#include <stdio.h>

void test_get_monsters(CuTest *tc) {
    faction *f;
    CuAssertPtrEquals(tc, NULL, get_monsters());
    f = get_or_create_monsters();
    CuAssertPtrEquals(tc, f, get_monsters());
    CuAssertIntEquals(tc, 666, f->no);
    CuAssertStrEquals(tc, "Monster", f->name);
    free_gamedata();
    CuAssertPtrEquals(tc, NULL, get_monsters());
    f = get_or_create_monsters();
    CuAssertPtrEquals(tc, f, get_monsters());
    CuAssertIntEquals(tc, 666, f->no);
}

CuSuite *get_faction_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_monsters);
    return suite;
}
