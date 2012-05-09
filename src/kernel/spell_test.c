#include <util/quicklist.h>
#include <kernel/spell.h>
#include <kernel/magic.h>

#include <cutest/CuTest.h>
#include <tests.h>

#include <stdlib.h>

static void test_register_spell(CuTest * tc)
{
  spell * sp;

  CuAssertPtrEquals(tc, 0, find_spell("testspell"));

  CuAssertPtrEquals(tc, spells, 0);
  sp = create_spell("testspell");
  sp->magietyp = 5;
  register_spell(sp);
  CuAssertIntEquals(tc, 1, ql_length(spells));
  CuAssertPtrEquals(tc, sp, find_spell("testspell"));
}

CuSuite *get_spell_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_register_spell);
  return suite;
}
