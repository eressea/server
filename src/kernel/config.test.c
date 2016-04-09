#include <platform.h>
#include <stdlib.h>

#include <kernel/config.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <util/language.h>
#include <util/base36.h>
#include <util/attrib.h>

#include <CuTest.h>
#include <tests.h>

struct critbit_tree;

static void test_read_unitid(CuTest *tc) {
    unit *u;
    order *ord;
    attrib *a;
    struct locale *lang;
    struct terrain_type *t_plain;

    test_cleanup();
    lang = test_create_locale();
    /* note that the english order is FIGHT, not COMBAT, so this is a poor example */
    t_plain = test_create_terrain("plain", LAND_REGION);
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, t_plain));
    a = a_add(&u->attribs, a_new(&at_alias));
    a->data.i = atoi36("42"); /* this unit is also TEMP 42 */

    ord = create_order(K_GIVE, lang, "TEMP 42");
    init_order(ord);
    CuAssertIntEquals(tc, u->no, read_unitid(u->faction, u->region));
    free_order(ord);

    ord = create_order(K_GIVE, lang, "8");
    init_order(ord);
    CuAssertIntEquals(tc, 8, read_unitid(u->faction, u->region));
    free_order(ord);

    ord = create_order(K_GIVE, lang, "");
    init_order(ord);
    CuAssertIntEquals(tc, -1, read_unitid(u->faction, u->region));
    free_order(ord);

    ord = create_order(K_GIVE, lang, "TEMP");
    init_order(ord);
    CuAssertIntEquals(tc, -1, read_unitid(u->faction, u->region));
    free_order(ord);

    // bug https://bugs.eressea.de/view.php?id=1685
    ord = create_order(K_GIVE, lang, "##");
    init_order(ord);
    CuAssertIntEquals(tc, -1, read_unitid(u->faction, u->region));
    free_order(ord);

    test_cleanup();
}

static void test_getunit(CuTest *tc) {
    unit *u, *u2;
    order *ord;
    attrib *a;
    struct region *r;
    struct locale *lang;
    struct terrain_type *t_plain;

    test_cleanup();
    lang = test_create_locale();
    /* note that the english order is FIGHT, not COMBAT, so this is a poor example */
    t_plain = test_create_terrain("plain", LAND_REGION);
    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, t_plain));
    a = a_add(&u->attribs, a_new(&at_alias));
    a->data.i = atoi36("42"); /* this unit is also TEMP 42 */
    r = test_create_region(1, 0, t_plain);

    ord = create_order(K_GIVE, lang, itoa36(u->no));
    init_order(ord);
    CuAssertIntEquals(tc, GET_UNIT, getunit(u->region, u->faction, &u2));
    CuAssertPtrEquals(tc, u, u2);
    init_order(ord);
    CuAssertIntEquals(tc, GET_NOTFOUND, getunit(r, u->faction, &u2));
    CuAssertPtrEquals(tc, NULL, u2);
    free_order(ord);

    ord = create_order(K_GIVE, lang, itoa36(u->no + 1));
    init_order(ord);
    CuAssertIntEquals(tc, GET_NOTFOUND, getunit(u->region, u->faction, &u2));
    CuAssertPtrEquals(tc, NULL, u2);
    free_order(ord);

    ord = create_order(K_GIVE, lang, "0");
    init_order(ord);
    CuAssertIntEquals(tc, GET_PEASANTS, getunit(u->region, u->faction, &u2));
    CuAssertPtrEquals(tc, NULL, u2);
    free_order(ord);

    // bug https://bugs.eressea.de/view.php?id=1685
    ord = create_order(K_GIVE, lang, "TEMP ##");
    init_order(ord);
    CuAssertIntEquals(tc, GET_NOTFOUND, getunit(u->region, u->faction, &u2));
    CuAssertPtrEquals(tc, NULL, u2);
    free_order(ord);

    // bug https://bugs.eressea.de/view.php?id=1685
    ord = create_order(K_GIVE, lang, "##");
    init_order(ord);
    CuAssertIntEquals(tc, GET_NOTFOUND, getunit(u->region, u->faction, &u2));
    CuAssertPtrEquals(tc, NULL, u2);
    free_order(ord);

    ord = create_order(K_GIVE, lang, "TEMP 42");
    init_order(ord);
    CuAssertIntEquals(tc, GET_UNIT, getunit(u->region, u->faction, &u2));
    CuAssertPtrEquals(tc, u, u2);
    free_order(ord);

    test_cleanup();
}

static void test_get_set_param(CuTest * tc)
{
    struct param *par = 0;
    test_cleanup();
    CuAssertStrEquals(tc, 0, get_param(par, "foo"));
    set_param(&par, "foo", "bar");
    set_param(&par, "bar", "foo");
    CuAssertStrEquals(tc, "bar", get_param(par, "foo"));
    CuAssertStrEquals(tc, "foo", get_param(par, "bar"));
    set_param(&par, "bar", "bar");
    CuAssertStrEquals(tc, "bar", get_param(par, "bar"));
    set_param(&par, "bar", NULL);
    CuAssertPtrEquals(tc, NULL, (void *)get_param(par, "bar"));
    free_params(&par);
    test_cleanup();
}

static void test_param_int(CuTest * tc)
{
    struct param *par = 0;
    test_cleanup();
    CuAssertIntEquals(tc, 13, get_param_int(par, "foo", 13));
    set_param(&par, "foo", "23");
    set_param(&par, "bar", "42");
    CuAssertIntEquals(tc, 23, get_param_int(par, "foo", 0));
    CuAssertIntEquals(tc, 42, get_param_int(par, "bar", 0));
    free_params(&par);
    test_cleanup();
}

static void test_param_flt(CuTest * tc)
{
    struct param *par = 0;
    test_cleanup();
    CuAssertDblEquals(tc, 13, get_param_flt(par, "foo", 13), 0.01);
    set_param(&par, "foo", "23.0");
    set_param(&par, "bar", "42.0");
    CuAssertDblEquals(tc, 23.0, get_param_flt(par, "foo", 0.0), 0.01);
    CuAssertDblEquals(tc, 42.0, get_param_flt(par, "bar", 0.0), 0.01);
    free_params(&par);
    test_cleanup();
}

static void test_forbiddenid(CuTest *tc) {
    CuAssertIntEquals(tc, 0, forbiddenid(1));
    CuAssertIntEquals(tc, 1, forbiddenid(0));
    CuAssertIntEquals(tc, 1, forbiddenid(-1));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("temp")));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("tem")));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("te")));
    CuAssertIntEquals(tc, 1, forbiddenid(atoi36("t")));
}

static void test_default_order(CuTest *tc) {
    order *ord;
    struct locale * loc;

    test_cleanup();
    loc = test_create_locale();
    ord = default_order(loc);
    CuAssertPtrEquals(tc, 0, ord);
    locale_setstring(loc, "defaultorder", "work");
    ord = default_order(loc);
    CuAssertPtrNotNull(tc, ord);
    CuAssertIntEquals(tc, K_WORK, getkeyword(ord));
    CuAssertPtrEquals(tc, ord->data, default_order(loc)->data);
    test_cleanup();
}

CuSuite *get_config_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_get_set_param);
    SUITE_ADD_TEST(suite, test_param_int);
    SUITE_ADD_TEST(suite, test_param_flt);
    SUITE_ADD_TEST(suite, test_forbiddenid);
    SUITE_ADD_TEST(suite, test_getunit);
    SUITE_ADD_TEST(suite, test_read_unitid);
    SUITE_ADD_TEST(suite, test_default_order);
    return suite;
}
