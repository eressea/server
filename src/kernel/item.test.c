#include <platform.h>

#include <kernel/item.h>
#include <kernel/pool.h>
#include <kernel/unit.h>
#include <util/language.h>
#include <util/functions.h>

#include <CuTest.h>
#include <tests.h>

#include <assert.h>

static void test_resourcename_no_appearance(CuTest *tc) {
    const resource_type *rtype;

    test_cleanup();
    init_resources(); // creates R_SILVER
    rtype = get_resourcetype(R_SILVER);
    assert(rtype && rtype->itype);
    assert(rtype->itype->_appearance[0] == 0);
    assert(rtype->itype->_appearance[1] == 0);
    CuAssertStrEquals(tc, "money", resourcename(rtype, 0));
    CuAssertStrEquals(tc, "money_p", resourcename(rtype, NMF_PLURAL));
    CuAssertStrEquals(tc, "money", resourcename(rtype, NMF_APPEARANCE));
    CuAssertStrEquals(tc, "money_p", resourcename(rtype, NMF_APPEARANCE|NMF_PLURAL));
    test_cleanup();
}

static void test_resourcename_with_appearance(CuTest *tc) {
    item_type *itype;

    test_cleanup();
    itype = it_get_or_create(rt_get_or_create("foo"));
    assert(itype && itype->rtype);
    it_set_appearance(itype, "bar");
    CuAssertStrEquals(tc, "foo", resourcename(itype->rtype, 0));
    CuAssertStrEquals(tc, "foo_p", resourcename(itype->rtype, NMF_PLURAL));
    CuAssertStrEquals(tc, "bar", resourcename(itype->rtype, NMF_APPEARANCE));
    CuAssertStrEquals(tc, "bar_p", resourcename(itype->rtype, NMF_APPEARANCE | NMF_PLURAL));
    test_cleanup();
}

static void test_uchange(CuTest * tc, unit * u, const resource_type * rtype) {
  int n;
  change_resource(u, rtype, 4);
  n = get_resource(u, rtype);
  CuAssertPtrNotNull(tc, rtype->uchange);
  CuAssertIntEquals(tc, n, rtype->uchange(u, rtype, 0));
  CuAssertIntEquals(tc, n-3, rtype->uchange(u, rtype, -3));
  CuAssertIntEquals(tc, n-3, get_resource(u, rtype));
  CuAssertIntEquals(tc, 0, rtype->uchange(u, rtype, -n));
}

void test_change_item(CuTest * tc)
{
  unit * u;

  test_cleanup();
  init_resources();
  test_create_world();

  u = test_create_unit(0, 0);
  test_uchange(tc, u, get_resourcetype(R_IRON));
}

void test_resource_type(CuTest * tc)
{
  struct item_type *itype;

  test_cleanup();

  CuAssertPtrEquals(tc, 0, rt_find("herpderp"));

  test_create_itemtype("herpderp");
  test_create_itemtype("herpes");
  itype = test_create_itemtype("herp");

  CuAssertPtrEquals(tc, itype->rtype, rt_find("herp"));
}

void test_finditemtype(CuTest * tc)
{
  const item_type *itype;
  const resource_type *rtype;
  struct locale * lang;

  test_cleanup();
  test_create_world();

  lang = get_locale("de");
  locale_setstring(lang, "horse", "Pferd");
  rtype = get_resourcetype(R_HORSE);
  itype = finditemtype("Pferd", lang);
  CuAssertPtrNotNull(tc, itype);
  CuAssertPtrEquals(tc, (void*)rtype->itype, (void*)itype);
}

void test_findresourcetype(CuTest * tc)
{
  const resource_type *rtype, *rresult;
  struct locale * lang;

  test_cleanup();
  test_create_world();

  lang = get_locale("de");
  locale_setstring(lang, "horse", "Pferd");
  locale_setstring(lang, "peasant", "Bauer");

  rtype = get_resourcetype(R_HORSE);
  rresult = findresourcetype("Pferd", lang);
  CuAssertPtrNotNull(tc, rresult);
  CuAssertPtrEquals(tc, (void*)rtype, (void*)rresult);

  CuAssertPtrNotNull(tc, findresourcetype("Bauer", lang));
}

CuSuite *get_item_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resourcename_no_appearance);
  SUITE_ADD_TEST(suite, test_resourcename_with_appearance);
  SUITE_ADD_TEST(suite, test_change_item);
  SUITE_ADD_TEST(suite, test_resource_type);
  SUITE_ADD_TEST(suite, test_finditemtype);
  SUITE_ADD_TEST(suite, test_findresourcetype);
  return suite;
}
