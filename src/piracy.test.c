#include <platform.h>
#include "piracy.h"

#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/order.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_piracy_cmd(CuTest * tc) {
    unit *u;
    order *ord;

    test_cleanup();
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, u->faction->locale, "");
    assert(u);
    piracy_cmd(u, ord);
    CuAssertPtrNotNull(tc, test_find_messagetype(u->faction->msgs, "error144"));
    test_cleanup();
}


CuSuite *get_piracy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_piracy_cmd);
    return suite;
}
