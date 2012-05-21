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
  struct item_type *itype;
  const char *names[4] = { "herp" , "herp_p", "derp", "derp_p" };
  struct locale * lang = make_locale("de");

  test_cleanup();

  itype = test_create_itemtype(names);
  CuAssertPtrNotNull(tc, itype);
  locale_setstring(lang, names[0], "Foo");
  locale_setstring(lang, names[1], "Foos");

  CuAssertPtrEquals(tc, (void*)itype, (void*)finditemtype("Foo", lang));
  CuAssertPtrEquals(tc, (void*)itype, (void*)finditemtype("Foos", lang));

  itype = test_create_itemtype(names+2);
  CuAssertPtrNotNull(tc, itype);
  locale_setstring(lang, names[2], "Bar");
  locale_setstring(lang, names[3], "Bars");

  CuAssertPtrEquals(tc, (void*)itype, (void*)finditemtype("Bar", lang));
  CuAssertPtrEquals(tc, (void*)itype, (void*)finditemtype("Bars", lang));
}

CuSuite *get_item_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resource_type);
  SUITE_ADD_TEST(suite, test_finditemtype);
  return suite;
}
