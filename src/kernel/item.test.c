#include <platform.h>

#include <kernel/item.h>
#include <kernel/pool.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>
#include <util/language.h>
#include <util/lists.h>
#include <util/log.h>
#include <util/functions.h>

#include <CuTest.h>
#include <tests.h>

#include <assert.h>

static void test_resourcename_no_appearance(CuTest *tc) {
    const resource_type *rtype;

    test_setup();
    init_resources(); /* creates R_SILVER */
    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    assert(rtype->itype->_appearance[0] == 0);
    assert(rtype->itype->_appearance[1] == 0);
    CuAssertStrEquals(tc, "money", resourcename(rtype, 0));
    CuAssertStrEquals(tc, "money_p", resourcename(rtype, NMF_PLURAL));
    CuAssertStrEquals(tc, "money", resourcename(rtype, NMF_APPEARANCE));
    CuAssertStrEquals(tc, "money_p", resourcename(rtype, NMF_APPEARANCE | NMF_PLURAL));
    test_teardown();
}

static void test_resourcename_with_appearance(CuTest *tc) {
    item_type *itype;

    test_setup();
    itype = it_get_or_create(rt_get_or_create("foo"));
    assert(itype && itype->rtype);
    it_set_appearance(itype, "bar");
    CuAssertStrEquals(tc, "foo", resourcename(itype->rtype, 0));
    CuAssertStrEquals(tc, "foo_p", resourcename(itype->rtype, NMF_PLURAL));
    CuAssertStrEquals(tc, "bar", resourcename(itype->rtype, NMF_APPEARANCE));
    CuAssertStrEquals(tc, "bar_p", resourcename(itype->rtype, NMF_APPEARANCE | NMF_PLURAL));
    test_teardown();
}

static void test_uchange(CuTest * tc, unit * u, const resource_type * rtype) {
    int n;
    struct log_t *log;
    strlist *sl = 0;

    assert(rtype);
    log = test_log_start(LOG_CPERROR, &sl);
    change_resource(u, rtype, 4);
    n = get_resource(u, rtype);
    CuAssertPtrNotNull(tc, rtype->uchange);
    CuAssertIntEquals(tc, n, rtype->uchange(u, rtype, 0));
    CuAssertIntEquals(tc, n - 3, rtype->uchange(u, rtype, -3));
    CuAssertIntEquals(tc, n - 3, get_resource(u, rtype));
    CuAssertIntEquals(tc, 0, rtype->uchange(u, rtype, -n));
    CuAssertPtrNotNull(tc, sl);
    CuAssertStrEquals(tc, "serious accounting error. number of items is %d.", sl->s);
    CuAssertPtrEquals(tc, NULL, sl->next);
    test_log_stop(log, sl);
}

void test_change_item(CuTest * tc)
{
    unit * u;

    test_setup();
    test_log_stderr(0);
    test_create_itemtype("iron");
    init_resources();

    u = test_create_unit(test_create_faction(NULL), test_create_region(0, 0, NULL));
    test_uchange(tc, u, get_resourcetype(R_IRON));
    test_log_stderr(1);
    test_teardown();
}

void test_resource_type(CuTest * tc)
{
    struct item_type *itype;

    test_setup();

    CuAssertPtrEquals(tc, NULL, rt_find("herpderp"));

    test_create_itemtype("herpderp");
    test_create_itemtype("herpes");
    itype = test_create_itemtype("herp");

    CuAssertPtrEquals(tc, itype->rtype, rt_find("herp"));
    test_teardown();
}

void test_finditemtype(CuTest * tc)
{
    const item_type *itype;
    struct locale * lang;

    test_setup();

    lang = get_or_create_locale("de");
    locale_setstring(lang, "horse", "Pferd");
    itype = test_create_itemtype("horse");
    CuAssertPtrEquals(tc, (void *)itype, (void *)finditemtype("Pferd", lang));
    test_teardown();
}

void test_findresourcetype(CuTest * tc)
{
    const item_type *itype;
    struct locale * lang;

    test_setup();

    lang = get_or_create_locale("de");
    locale_setstring(lang, "log", "Holz");
    locale_setstring(lang, "peasant", "Bauer");
    init_resources();
    CuAssertPtrNotNull(tc, rt_find("peasant"));
    CuAssertPtrEquals(tc, NULL, rt_find("log"));
    itype = test_create_itemtype("log");

    CuAssertPtrEquals(tc, (void*)itype->rtype, (void*)findresourcetype("Holz", lang));
    CuAssertPtrEquals(tc, (void *)rt_find("peasant"), (void *)findresourcetype("Bauer", lang));
    test_teardown();
}

#include <modules/autoseed.h>
static void test_fix_demand(CuTest *tc) {
    region *r;
    terrain_type *tplain;
    item_type *ltype;

    test_setup();
    ltype = test_create_itemtype("balm");
    ltype->rtype->flags |= (RTF_ITEM | RTF_POOLED);
    new_luxurytype(ltype, 0);
    ltype = test_create_itemtype("oint");
    ltype->rtype->flags |= (RTF_ITEM | RTF_POOLED);
    new_luxurytype(ltype, 0);
    tplain = test_create_terrain("plain", LAND_REGION);
    r = new_region(0, 0, NULL, 0);
    CuAssertPtrNotNull(tc, r);
    terraform_region(r, tplain);
    CuAssertPtrNotNull(tc, r->land);
    CuAssertIntEquals(tc, 0, fix_demand(r));
    CuAssertPtrNotNull(tc, r->land->demands);
    CuAssertPtrNotNull(tc, r->land->demands->next);
    CuAssertPtrNotNull(tc, r_luxury(r));
    test_teardown();
}

static void test_core_resources(CuTest *tc) {
    resource_type * rtype;

    test_setup();
    init_resources();
    CuAssertPtrNotNull(tc, rtype = rt_find("money"));
    CuAssertPtrNotNull(tc, rtype->itype);
    CuAssertPtrNotNull(tc, rtype->uchange);
    CuAssertPtrNotNull(tc, rtype = rt_find("peasant"));
    CuAssertPtrEquals(tc, NULL, rtype->itype);
    CuAssertPtrNotNull(tc, rtype = rt_find("permaura"));
    CuAssertPtrEquals(tc, NULL, rtype->itype);
    CuAssertPtrNotNull(tc, rtype = rt_find("hp"));
    CuAssertPtrEquals(tc, NULL, rtype->itype);
    CuAssertPtrNotNull(tc, rtype = rt_find("aura"));
    CuAssertPtrEquals(tc, NULL, rtype->itype);
    test_teardown();
}

static void test_get_resource(CuTest *tc) {
    resource_type *rtype;

    test_setup();

    CuAssertPtrEquals(tc, NULL, rt_find("catapultammo123"));
    rtype = rt_get_or_create("catapultammo123");
    CuAssertPtrNotNull(tc, rtype);
    CuAssertPtrEquals(tc, rtype, rt_find("catapultammo123"));
    CuAssertStrEquals(tc, "catapultammo123", rtype->_name);

    CuAssertPtrEquals(tc, NULL, rt_find("catapult"));
    rtype = rt_get_or_create("catapult");
    CuAssertPtrEquals(tc, rtype, rt_find("catapult"));
    CuAssertStrEquals(tc, "catapult", rtype->_name);
    test_teardown();
}

CuSuite *get_item_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_resourcename_no_appearance);
    SUITE_ADD_TEST(suite, test_resourcename_with_appearance);
    SUITE_ADD_TEST(suite, test_change_item);
    SUITE_ADD_TEST(suite, test_get_resource);
    SUITE_ADD_TEST(suite, test_resource_type);
    SUITE_ADD_TEST(suite, test_finditemtype);
    SUITE_ADD_TEST(suite, test_findresourcetype);
    SUITE_ADD_TEST(suite, test_fix_demand);
    SUITE_ADD_TEST(suite, test_core_resources);
    return suite;
}
