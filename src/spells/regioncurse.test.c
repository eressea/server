#include "regioncurse.h"

#include <kernel/curse.h>
#include <kernel/region.h>
#include <kernel/skill.h>
#include <kernel/unit.h>

#include <CuTest.h>
#include <tests.h>

#include <stdbool.h>        // for true
#include <stddef.h>         // for NULL

static void test_get_skill_modifier_cursed(CuTest* tc) {
    region* r;
    unit* u, * u2, * u3;
    curse* c;

    test_setup();
    u = test_create_unit(test_create_faction(), r = test_create_plain(0, 0));
    u2 = test_create_unit(u->faction, r); /* allied */
    u3 = test_create_unit(test_create_faction(), r); /* not allied */
    set_level(u2, SK_TAXING, 1);
    set_level(u3, SK_TAXING, 1);

    /* default: no effects: */
    CuAssertIntEquals(tc, 0, get_modifier(u2, SK_TAXING, 1, r, true));
    CuAssertIntEquals(tc, 0, get_modifier(u3, SK_TAXING, 1, r, true));

    /* cursed with good dreams */
    c = create_curse(u, &r->attribs, &ct_gbdream, 1, 1, 1, 0);
    CuAssertIntEquals(tc, 1, get_modifier(u2, SK_TAXING, 1, r, true));
    CuAssertIntEquals(tc, 0, get_modifier(u3, SK_TAXING, 1, r, true));

    /* cursed with good dreams, but magician just died */
    u->number = 0;
    CuAssertIntEquals(tc, 0, get_modifier(u2, SK_TAXING, 1, r, true));
    CuAssertIntEquals(tc, 0, get_modifier(u3, SK_TAXING, 1, r, true));

    /* cursed with good dreams, but magician is dead */
    c->magician = NULL;
    CuAssertIntEquals(tc, 0, get_modifier(u2, SK_TAXING, 1, r, true));
    CuAssertIntEquals(tc, 0, get_modifier(u3, SK_TAXING, 1, r, true));

    test_teardown();
}

CuSuite* get_regioncurse_suite(void)
{
    CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_skill_modifier_cursed);
    return suite;
}
