#include <platform.h>
#include "command.h"

#include "unit.h"
#include "order.h"

#include <CuTest.h>
#include <tests.h>

static void parser_two(const void *nodes, struct unit * u, struct order *ord) {
    scale_number(u, 2);
}

static void parser_six(const void *nodes, struct unit * u, struct order *ord) {
    scale_number(u, 6);
}

static void test_command(CuTest * tc) {
    struct syntaxtree *st;
    struct locale * loc;
    unit *u;

    test_setup();
    loc = test_create_locale();
    st = stree_create();
    CuAssertPtrNotNull(tc, st);
    CuAssertPtrEquals(tc, loc, (struct locale *)st->lang);
    CuAssertPtrEquals(tc, NULL, st->root);
    CuAssertPtrEquals(tc, NULL, st->next);
    stree_add(st, "two", parser_two);
    stree_add(st, "six", parser_six);
    CuAssertPtrNotNull(tc, st->root);
    CuAssertPtrEquals(tc, st->root, stree_find(st, loc));
    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    u->thisorder = create_order(K_ALLIANCE, loc, "two");
    do_command(st->root, u, u->thisorder);
    CuAssertIntEquals(tc, u->number, 2);
    stree_free(st);
    test_teardown();
}

CuSuite *get_command_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_command);
    return suite;
}
