#include <platform.h>

#include <kernel/types.h>
#include <kernel/magic.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <util/quicklist.h>
#include <util/language.h>

#include <cutest/CuTest.h>
#include <tests.h>

void test_updatespells(CuTest * tc)
{
  struct faction * f;
  spell * sp;
  spellbook * book = 0;

  test_cleanup();
  
  f = test_create_faction(0);
  sp = create_spell("testspell", 0);
  CuAssertPtrNotNull(tc, sp);
  spellbook_add(&book, sp, 1);

  update_spellbook(f, 1);
}

CuSuite *get_magic_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_updatespells);
  return suite;
}
