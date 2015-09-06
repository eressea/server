#include <platform.h>
#include <kernel/config.h>
#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/unit.h>
#include <util/language.h>
#include <util/attrib.h>
#include <spells/regioncurse.h>
#include "spells.h"

#include <CuTest.h>
#include <tests.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


static void test_create_castorder(castorder *order, unit *u, int level, float force, int range) {
    struct locale * lang;
    lang = get_or_create_locale("en");
    create_castorder(order, u, NULL, NULL, u->region, level, force, range, create_order(K_CAST, lang, ""), NULL);
}

static void test_good_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    int level;
    castorder order;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    test_create_castorder(&order, u1, 10, 10., 0);

    level = sp_gooddreams(&order);
    CuAssertIntEquals(tc, 10, level);
    curse *curse = get_curse(r->attribs, ct_find("gbdream"));
    CuAssertTrue(tc, curse && curse->duration > 1);
    CuAssertTrue(tc, curse->effect == 1);

    a_age(&r->attribs);
    CuAssertIntEquals_Msg(tc, "good dreams give +1 to allies", 1, get_modifier(u1, SK_MELEE, 11, r, false));
    CuAssertIntEquals_Msg(tc, "good dreams have no effect on non-allies", 0, get_modifier(u2, SK_MELEE, 11, r, false));
}

static void test_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    castorder order;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    test_create_castorder(&order, u1, 10, 10., 0);

    sp_gooddreams(&order);
    sp_baddreams(&order);
    a_age(&r->attribs);
    CuAssertIntEquals_Msg(tc, "good dreams in same region as bad dreams", 1, get_modifier(u1, SK_MELEE, 11, r, false));
    CuAssertIntEquals_Msg(tc, "bad dreams in same region as good dreams", -1, get_modifier(u2, SK_MELEE, 11, r, false));

    free_castorder(&order);
    test_cleanup();
}

static void test_bad_dreams(CuTest *tc) {
    struct region *r;
    struct faction *f1, *f2;
    unit *u1, *u2;
    int level;
    castorder order;

    test_cleanup();
    test_create_world();
    r = findregion(0, 0);
    f1 = test_create_faction(0);
    f2 = test_create_faction(0);
    u1 = test_create_unit(f1, r);
    u2 = test_create_unit(f2, r);

    test_create_castorder(&order, u1, 10, 10., 0);

    level = sp_baddreams(&order);
    CuAssertIntEquals(tc, 10, level);
    curse *curse = get_curse(r->attribs, ct_find("gbdream"));
    CuAssertTrue(tc, curse && curse->duration > 1);
    CuAssertTrue(tc, curse->effect == -1);

    a_age(&r->attribs);
    CuAssertIntEquals_Msg(tc, "bad dreams have no effect on allies", 0, get_modifier(u1, SK_MELEE, 11, r, false));
    CuAssertIntEquals_Msg(tc, "bad dreams give -1 to non-allies", -1, get_modifier(u2, SK_MELEE, 11, r, false));

    free_castorder(&order);
    test_cleanup();
}

CuSuite *get_spells_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_good_dreams);
    SUITE_ADD_TEST(suite, test_bad_dreams);
    SUITE_ADD_TEST(suite, test_dreams);
    return suite;
}
