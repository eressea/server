#include <platform.h>

#include <kernel/types.h>
#include <kernel/magic.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <util/quicklist.h>
#include <util/language.h>

#include <cutest/CuTest.h>
#include <tests.h>


int count_spell_cb(spellbook_entry * sbe, void * ptr)
{
  int * counter = (int *)ptr;
  ++*counter;
  return 0;
}

void test_spellbook(CuTest * tc)
{
  spell * sp;
  spellbook * sb = 0;
  int counter = 0;
  
  sp = create_spell("testspell", 0);
  spellbook_add(&sb, sp, 1);
  CuAssertPtrNotNull(tc, sb);
  spellbook_foreach(sb, count_spell_cb, &counter);
  CuAssertIntEquals(tc, 1, counter);
  spellbook_free(sb);
}

CuSuite *get_spellbook_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_spellbook);
  return suite;
}
