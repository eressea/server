#include <platform.h>
#include <kernel/config.h>

#include "orderfile.h"

#include "direction.h"

#include <kernel/calendar.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/keyword.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/message.h>
#include <util/param.h>
#include <util/password.h>

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
    test_teardown();
}

static const char *getbuf_strings(void *data)
{
    strlist **listp = (strlist **)data;
    if (listp && *listp) {
        strlist *list = *listp;
        *listp = list->next;
        return list->s;
    }
    return NULL;
}

static void test_unit_orders(CuTest *tc) {
    input in;
    unit *u;
    faction *f;
    strlist *orders = NULL, *backup;
    char buf[64];

    test_setup();
    u = test_create_unit(f = test_create_faction(NULL), test_create_plain(0, 0));
    f->locale = test_create_locale();
    u->orders = create_order(K_ENTERTAIN, f->locale, NULL);
    faction_setpassword(f, password_hash("password", PASSWORD_DEFAULT));
    snprintf(buf, sizeof(buf), "%s %s %s",
        LOC(f->locale, parameters[P_FACTION]), itoa36(f->no), "password");
    addstrlist(&orders, buf);
    snprintf(buf, sizeof(buf), "%s %s",
        LOC(f->locale, parameters[P_UNIT]), itoa36(u->no));
    addstrlist(&orders, buf);
    snprintf(buf, sizeof(buf), "%s %s", keyword_name(K_MOVE, f->locale),
        LOC(f->locale, shortdirections[D_WEST]));
    backup = orders;
    addstrlist(&orders, buf);
    in.data = &orders;
    in.getbuf = getbuf_strings;
    CuAssertIntEquals(tc, 0, read_orders(&in));
    CuAssertPtrNotNull(tc, u->old_orders);
    CuAssertPtrNotNull(tc, u->orders);
    CuAssertPtrEquals(tc, NULL, orders);
    CuAssertIntEquals(tc, K_MOVE, getkeyword(u->orders));
    CuAssertIntEquals(tc, K_ENTERTAIN, getkeyword(u->old_orders));
    CuAssertIntEquals(tc, UFL_ORDERS, u->flags & UFL_ORDERS);
    freestrlist(backup);
    test_teardown();
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
    test_teardown();
}

static void test_faction_password_bad(CuTest *tc) {
    input in;
    faction *f;
    order_list olist;
    const char *orders[] = { "ERESSEA 1 password", NULL };

    test_setup();
    mt_create_va(mt_new("wrongpasswd", NULL), "password:string", MT_NEW_END);

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
    test_teardown();
}

CuSuite *get_orderfile_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_read_orders);
    SUITE_ADD_TEST(suite, test_unit_orders);
    SUITE_ADD_TEST(suite, test_faction_password_okay);
    SUITE_ADD_TEST(suite, test_faction_password_bad);

    return suite;
}
