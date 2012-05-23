#include <platform.h>

#include <kernel/types.h>
#include <kernel/item.h>
#include <util/language.h>

#include <cutest/CuTest.h>
#include <tests.h>

void test_resource_type(CuTest * tc)
{
  struct item_type *itype;
  const char *names[2] = { 0 , 0 };

  test_cleanup();

  CuAssertPtrEquals(tc, 0, rt_find("herpderp"));

  names[0] = names[1] = "herpderp";
  test_create_itemtype(names);

  names[0] = names[1] = "herp";
  itype = test_create_itemtype(names);

  names[0] = names[1] = "herpes";
  test_create_itemtype(names);

  CuAssertPtrEquals(tc, itype, it_find("herp"));
  CuAssertPtrEquals(tc, itype->rtype, rt_find("herp"));
}

void test_finditemtype(CuTest * tc)
{
  const item_type *itype, *iresult;
  struct locale * lang;

  test_cleanup();
  test_create_world();

  lang = find_locale("de");
  locale_setstring(lang, "horse", "Pferd");
  itype = it_find("horse");
  iresult = finditemtype("Pferd", lang);
  CuAssertPtrNotNull(tc, iresult);
  CuAssertPtrEquals(tc, (void*)itype, (void*)iresult);
}

void test_findresourcetype(CuTest * tc)
{
  const resource_type *rtype, *rresult;
  struct locale * lang;

  test_cleanup();
  test_create_world();

  lang = find_locale("de");
  locale_setstring(lang, "horse", "Pferd");
  locale_setstring(lang, "peasant", "Bauer");

  rtype = rt_find("horse");
  rresult = findresourcetype("Pferd", lang);
  CuAssertPtrNotNull(tc, rresult);
  CuAssertPtrEquals(tc, (void*)rtype, (void*)rresult);

  CuAssertPtrNotNull(tc, findresourcetype("Bauer", lang));
}

CuSuite *get_item_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resource_type);
  SUITE_ADD_TEST(suite, test_finditemtype);
  SUITE_ADD_TEST(suite, test_findresourcetype);
  return suite;
}
