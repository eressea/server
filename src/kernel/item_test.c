#include <platform.h>

#include <kernel/types.h>
#include <kernel/item.h>

#include <cutest/CuTest.h>
#include <tests.h>

void test_resource_type(CuTest * tc)
{
  struct item_type *itype;
  const char *names[2] = { 0 , 0 };

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

CuSuite *get_item_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resource_type);
  return suite;
}
