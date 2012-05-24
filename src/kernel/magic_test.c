#include <platform.h>

#include <kernel/types.h>
#include <kernel/faction.h>
#include <kernel/magic.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <util/quicklist.h>
#include <util/language.h>

#include <cutest/CuTest.h>
#include <tests.h>

void test_updatespells(CuTest * tc)
{
  faction * f;
  spell * sp;
  spellbook *book = 0;

  test_cleanup();
  
  f = test_create_faction(0);
  sp = create_spell("testspell", 0);
  CuAssertPtrNotNull(tc, sp);

  book = create_spellbook("spells");
  CuAssertPtrNotNull(tc, book);
  spellbook_add(book, sp, 1);

  CuAssertIntEquals(tc, 0, ql_length(f->spellbook->spells));
  pick_random_spells(f, 1, book, 1);
  CuAssertPtrNotNull(tc, f->spellbook);
  CuAssertIntEquals(tc, 1, ql_length(f->spellbook->spells));
  CuAssertPtrNotNull(tc, spellbook_get(f->spellbook, sp));
}

void test_spellbooks(CuTest * tc)
{
  spell *sp;
  spellbook *herp, *derp;
  spellbook_entry *entry;
  const char * sname = "herpderp";
  test_cleanup();

  herp = get_spellbook("herp");
  derp = get_spellbook("derp");
  CuAssertPtrNotNull(tc, herp);
  CuAssertPtrNotNull(tc, derp);
  CuAssertTrue(tc, derp!=herp);
  CuAssertStrEquals(tc, "herp", herp->name);
  CuAssertStrEquals(tc, "derp", derp->name);

  sp = create_spell(sname, 0);
  spellbook_add(herp, sp, 1);
  CuAssertPtrNotNull(tc, sp);
  entry = spellbook_get(herp, sp);
  CuAssertPtrNotNull(tc, entry);
  CuAssertPtrEquals(tc, sp, entry->sp);
  /* CuAssertPtrEquals(tc, 0, spellbook_get(derp, sname)); */

  test_cleanup();
  herp = get_spellbook("herp");
  CuAssertPtrNotNull(tc, herp);
  /* CuAssertPtrEquals(tc, 0, spellbook_get(herp, sname)); */
}

CuSuite *get_magic_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_updatespells);
  SUITE_ADD_TEST(suite, test_spellbooks);
  return suite;
}
