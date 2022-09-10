#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "orderfile.h"

#include "direction.h"

#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/keyword.h>
#include <util/language.h>
#include <util/message.h>
#include <util/param.h>
#include <util/password.h>

#include <CuTest.h>
#include <tests.h>
#include <strings.h>

static void test_unit_orders(CuTest *tc) {
    unit *u;
    faction *f;
    sbstring sbs;
    char orders[256];
    parser_state state = { NULL };
    OP_Parser parser = parser_create(&state);

    test_setup();
    sbs_init(&sbs, orders, sizeof(orders));
    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    f->locale = test_create_locale();
    u->orders = create_order(K_ENTERTAIN, f->locale, NULL);
    faction_setpassword(f, password_hash("password", PASSWORD_DEFAULT));
    sbs_printf(&sbs, "%s %s %s\n%s %s\n%s\n",
        LOC(f->locale, parameters[P_FACTION]), itoa36(f->no), "password",
        LOC(f->locale, parameters[P_UNIT]), itoa36(u->no),
        keyword_name(K_MOVE, f->locale));
    CuAssertIntEquals(tc, 0, parser_parse(parser, orders, sbs_length(&sbs), true));
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(u->orders));
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(u->defaults));
    test_teardown();
}

static void test_no_foreign_unit_orders(CuTest *tc) {
    unit *u;
    faction *f, *f2;
    sbstring sbs;
    char orders[256];
    parser_state state = { NULL };
    OP_Parser parser = parser_create(&state);

    test_setup();
    sbs_init(&sbs, orders, sizeof(orders));
    f2 = test_create_faction();
    f2->locale = test_create_locale();
    f = test_create_faction();
    f->locale = f2->locale;

    u = test_create_unit(f2, test_create_plain(0, 0));
    u->orders = create_order(K_ENTERTAIN, f->locale, NULL);
    faction_setpassword(f, password_hash("password", PASSWORD_DEFAULT));
    sbs_printf(&sbs, "%s %s %s\n%s %s\n%s\n",
        LOC(f->locale, parameters[P_FACTION]), itoa36(f->no), "password",
        LOC(f->locale, parameters[P_UNIT]), itoa36(u->no),
        keyword_name(K_WORK, f->locale));
    CuAssertIntEquals(tc, 0, parser_parse(parser, orders, sbs_length(&sbs), true));
    CuAssertPtrEquals(tc, NULL, u->defaults);
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(u->orders));

    test_teardown();
}

static void test_faction_password_okay(CuTest *tc) {
    faction *f;
    FILE *F;

    test_setup();
    f = test_create_faction();
    renumber_faction(f, 1);
    CuAssertIntEquals(tc, 1, f->no);
    faction_setpassword(f, "password");
    f->lastorders = turn - 1;
    F = tmpfile();
    fprintf(F, "ERESSEA 1 password\n");
    rewind(F);
    CuAssertIntEquals(tc, 0, parseorders(F));
    CuAssertIntEquals(tc, turn, f->lastorders);
    fclose(F);
    test_teardown();
}

static void test_faction_password_bad(CuTest *tc) {
    faction *f;
    FILE *F;

    test_setup();
    mt_create_va(mt_new("wrongpasswd", NULL), "password:string", MT_NEW_END);

    f = test_create_faction();
    renumber_faction(f, 1);
    CuAssertIntEquals(tc, 1, f->no);
    faction_setpassword(f, "patzword");
    f->lastorders = turn - 1;
    F = tmpfile();
    fprintf(F, "ERESSEA 1 password\n");
    rewind(F);
    CuAssertIntEquals(tc, 0, parseorders(F));
    CuAssertIntEquals(tc, turn - 1, f->lastorders);
    fclose(F);
    test_teardown();
}

CuSuite *get_orderfile_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_unit_orders);
    SUITE_ADD_TEST(suite, test_no_foreign_unit_orders);
    SUITE_ADD_TEST(suite, test_faction_password_okay);
    SUITE_ADD_TEST(suite, test_faction_password_bad);

    return suite;
}
