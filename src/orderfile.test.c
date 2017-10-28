#include <platform.h>
#include <kernel/config.h>

#include "orderfile.h"

#include <kernel/faction.h>

#include <CuTest.h>
#include <tests.h>

static const char *getbuf_null(void *data)
{
    return NULL;
}

static void test_read_orders(CuTest *tc) {
    input in;
    test_setup();
    in.getbuf = getbuf_null;
    in.data = NULL;
    CuAssertIntEquals(tc, 0, read_orders(&in));
    test_cleanup();
}

typedef struct order_list {
    const char **orders;
    int next;
} order_list;

static const char *getbuf_list(void *data)
{
    order_list * olist = (order_list *)data;
    return olist->orders[olist->next++];
}

static void test_faction_password_okay(CuTest *tc) {
    input in;
    faction *f;
    order_list olist;
    const char *orders[] = { "ERESSEA 1 password", NULL };

    test_setup();
    f = test_create_faction(NULL);
    renumber_faction(f, 1);
    CuAssertIntEquals(tc, 1, f->no);
    faction_setpassword(f, "password");
    f->lastorders = turn - 1;
    olist.orders = orders;
    olist.next = 0;
    in.getbuf = getbuf_list;
    in.data = &olist;
    CuAssertIntEquals(tc, 0, read_orders(&in));
    CuAssertIntEquals(tc, 2, olist.next);
    CuAssertIntEquals(tc, turn, f->lastorders);
    test_cleanup();
}

static void test_faction_password_bad(CuTest *tc) {
    input in;
    faction *f;
    order_list olist;
    const char *orders[] = { "ERESSEA 1 password", NULL };

    test_setup();
    f = test_create_faction(NULL);
    renumber_faction(f, 1);
    CuAssertIntEquals(tc, 1, f->no);
    faction_setpassword(f, "patzword");
    f->lastorders = turn - 1;
    olist.orders = orders;
    olist.next = 0;
    in.getbuf = getbuf_list;
    in.data = &olist;
    CuAssertIntEquals(tc, 0, read_orders(&in));
    CuAssertIntEquals(tc, 2, olist.next);
    CuAssertIntEquals(tc, turn - 1, f->lastorders);
    test_cleanup();
}

CuSuite *get_orderfile_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_read_orders);
    SUITE_ADD_TEST(suite, test_faction_password_okay);
    SUITE_ADD_TEST(suite, test_faction_password_bad);

    return suite;
}
