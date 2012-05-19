#include <platform.h>

#include <kernel/item.h>

#include <cutest/CuTest.h>
#include <tests.h>

void test_resource_type(CuTest * tc)
{
  resource_type *rtype, *rherp;
  const char *names[2] = { 0 , 0 };

  CuAssertPtrEquals(tc, 0, rt_find("herpderp"));

  names[0] = "herpderp";
  rtype = new_resourcetype(names, NULL, RTF_NONE);
  names[0] = "herp";
  rherp = new_resourcetype(names, NULL, RTF_NONE);
  names[0] = "herpes";
  rtype = new_resourcetype(names, NULL, RTF_NONE);

  CuAssertPtrEquals(tc, rherp, rt_find("herp"));
}

CuSuite *get_item_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_resource_type);
  return suite;
}
