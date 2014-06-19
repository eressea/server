#include <platform.h>
#include "types.h"
#include "curse.h"

#include <util/attrib.h>

#include <CuTest.h>

static void test_curse(CuTest * tc)
{
  attrib *attrs = NULL;
  curse *c, *result;
  int cid;

  curse_type ct_dummy = { "dummy", CURSETYP_NORM, 0, M_SUMEFFECT, NULL };
  c = create_curse(NULL, &attrs, &ct_dummy, 1.0, 1, 1, 1);
  cid = c->no;
  result = findcurse(cid);
  CuAssertPtrEquals(tc, c, result);
  destroy_curse(c);
  result = findcurse(cid);
  CuAssertPtrEquals(tc, NULL, result);
}

CuSuite *get_curse_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_curse);
  return suite;
}
