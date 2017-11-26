/*
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2003   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed
 without prior permission by the authors of Eressea.

 */
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
    CuAssertPtrEquals(tc, 0, st->root);
    CuAssertPtrEquals(tc, 0, st->next);
    stree_add(st, "two", parser_two);
    stree_add(st, "six", parser_six);
    CuAssertPtrNotNull(tc, st->root);
    CuAssertPtrEquals(tc, st->root, stree_find(st, loc));
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->thisorder = create_order(K_ALLIANCE, loc, "two");
    do_command(st->root, u, u->thisorder);
    CuAssertIntEquals(tc, u->number, 2);
    stree_free(st);
    test_cleanup();
}

CuSuite *get_command_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_command);
    return suite;
}
