#include <platform.h>
#include "piracy.h"

#include <kernel/unit.h>
#include <kernel/ship.h>
#include <kernel/faction.h>
#include <kernel/order.h>

#include <util/language.h>

#include <CuTest.h>
#include <tests.h>
#include <assert.h>

static void test_piracy_cmd_errors(CuTest * tc) {
    faction *f;
    unit *u, *u2;
    order *ord;

    test_cleanup();
    u = test_create_unit(f = test_create_faction(0), test_create_region(0, 0, 0));
    f->locale = get_or_create_locale("de");
    ord = create_order(K_PIRACY, f->locale, "");
    assert(u);
    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must be on a ship for PIRACY", test_find_messagetype(f->msgs, "error144"));

    u_set_ship(u, test_create_ship(u->region, 0));

    u2 = test_create_unit(u->faction, u->region);
    u_set_ship(u2, u->ship);

    test_clear_messages(f);
    piracy_cmd(u2, ord);
    CuAssertPtrNotNullMsg(tc, "must be owner for PIRACY", test_find_messagetype(f->msgs, "error146"));

    test_clear_messages(f);
    piracy_cmd(u, ord);
    CuAssertPtrNotNullMsg(tc, "must specify target for PIRACY", test_find_messagetype(f->msgs, "piratenovictim"));

    test_cleanup();
}


CuSuite *get_piracy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_piracy_cmd_errors);
    return suite;
}
