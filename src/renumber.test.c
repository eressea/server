#include "renumber.h"

#include <tests.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <util/base36.h>
#include <util/language.h>

#include <stddef.h>
#include <CuTest.h>

static void test_renumber_faction(CuTest *tc) {
    unit *u;
    int uno, no;
    const struct locale *lang;

    test_setup_ex(tc);
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    no = u->faction->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", LOC(lang, parameters[P_FACTION]), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    renumber_factions();
    CuAssertIntEquals(tc, uno, u->faction->no);
    test_cleanup();
}

static void test_renumber_unit(CuTest *tc) {
    unit *u;
    int uno, no;
    const struct locale *lang;

    test_setup_ex(tc);
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    no = u->no;
    uno = (no > 1) ? no - 1 : no + 1;
    lang = u->faction->locale;
    u->thisorder = create_order(K_NUMBER, lang, "%s %s", LOC(lang, parameters[P_UNIT]), itoa36(uno));
    renumber_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, uno, u->no);
    CuAssertIntEquals(tc, -no, ualias(u)); // TODO: why is ualias negative?
    test_cleanup();
}

CuSuite *get_renumber_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_renumber_unit);
    SUITE_ADD_TEST(suite, test_renumber_faction);
    return suite;
}
