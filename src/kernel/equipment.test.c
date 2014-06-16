#include <platform.h>

#include <kernel/types.h>
#include <kernel/equipment.h>
#include <kernel/item.h>
#include <kernel/unit.h>
#include <kernel/magic.h>
#include <kernel/skill.h>
#include <kernel/spell.h>

#include <quicklist.h>

#include <CuTest.h>
#include <tests.h>

void test_equipment(CuTest * tc)
{
  equipment * eq;
  unit * u;
  const item_type * it_horses;
  const char * names[] = {"horse", "horse_p"};
  spell *sp;
  sc_mage * mage;
  
  test_cleanup();
  test_create_race("human");
  skill_enabled[SK_MAGIC] = 1;
  it_horses = test_create_itemtype(names);
  CuAssertPtrNotNull(tc, it_horses);
  sp = create_spell("testspell", 0);
  CuAssertPtrNotNull(tc, sp);

  CuAssertPtrEquals(tc, 0, get_equipment("herpderp"));
  eq = create_equipment("herpderp");
  CuAssertPtrEquals(tc, eq, get_equipment("herpderp"));

  equipment_setitem(eq, it_horses, "1");
  equipment_setskill(eq, SK_MAGIC, "5");
  equipment_addspell(eq, sp, 1);

  u = test_create_unit(0, 0);
  equip_unit_mask(u, eq, EQUIP_ALL);
  CuAssertIntEquals(tc, 1, i_get(u->items, it_horses));
  CuAssertIntEquals(tc, 5, get_level(u, SK_MAGIC));

  mage = get_mage(u);
  CuAssertPtrNotNull(tc, mage);
  CuAssertPtrNotNull(tc, mage->spellbook);
  CuAssertTrue(tc, u_hasspell(u, sp));
}

CuSuite *get_equipment_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_equipment);
  return suite;
}
