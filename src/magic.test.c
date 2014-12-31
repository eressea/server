#include <platform.h>

#include "magic.h"

#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/spell.h>
#include <kernel/spellbook.h>
#include <kernel/unit.h>
#include <kernel/pool.h>
#include <quicklist.h>
#include <util/language.h>

#include <CuTest.h>
#include <tests.h>

#include <stdlib.h>

void test_updatespells(CuTest * tc)
{
  faction * f;
  spell * sp;
  spellbook *book = 0;

  test_cleanup();
  test_create_race("human");

  f = test_create_faction(0);
  sp = create_spell("testspell", 0);
  CuAssertPtrNotNull(tc, sp);

  book = create_spellbook("spells");
  CuAssertPtrNotNull(tc, book);
  spellbook_add(book, sp, 1);

  CuAssertPtrEquals(tc, 0, f->spellbook);
  pick_random_spells(f, 1, book, 1);
  CuAssertPtrNotNull(tc, f->spellbook);
  CuAssertIntEquals(tc, 1, ql_length(f->spellbook->spells));
  CuAssertPtrNotNull(tc, spellbook_get(f->spellbook, sp));
  test_cleanup();
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

  test_cleanup();
  herp = get_spellbook("herp");
  CuAssertPtrNotNull(tc, herp);
  test_cleanup();
}

static spell * test_magic_create_spell(void)
{
  spell *sp;
  sp = create_spell("testspell", 0);

  sp->components = (spell_component *) calloc(4, sizeof(spell_component));
  sp->components[0].amount = 1;
  sp->components[0].type = get_resourcetype(R_SILVER);
  sp->components[0].cost = SPC_FIX;
  sp->components[1].amount = 1;
  sp->components[1].type = get_resourcetype(R_AURA);
  sp->components[1].cost = SPC_LEVEL;
  sp->components[2].amount = 1;
  sp->components[2].type = get_resourcetype(R_HORSE);
  sp->components[2].cost = SPC_LINEAR;
  return sp;
}

void test_pay_spell(CuTest * tc)
{
  spell *sp;
  unit * u;
  faction * f;
  region * r;
  int level;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  u = test_create_unit(f, r);
  CuAssertPtrNotNull(tc, u);

  sp = test_magic_create_spell();
  CuAssertPtrNotNull(tc, sp);

  set_level(u, SK_MAGIC, 5);
  unit_add_spell(u, 0, sp, 1);

  change_resource(u, get_resourcetype(R_SILVER), 1);
  change_resource(u, get_resourcetype(R_AURA), 3);
  change_resource(u, get_resourcetype(R_HORSE), 3);

  level = eff_spelllevel(u, sp, 3, 1);
  CuAssertIntEquals(tc, 3, level);
  pay_spell(u, sp, level, 1);
  CuAssertIntEquals(tc, 0, get_resource(u, get_resourcetype(R_SILVER)));
  CuAssertIntEquals(tc, 0, get_resource(u, get_resourcetype(R_AURA)));
  CuAssertIntEquals(tc, 0, get_resource(u, get_resourcetype(R_HORSE)));
}

void test_pay_spell_failure(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  int level;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  u = test_create_unit(f, r);
  CuAssertPtrNotNull(tc, u);

  sp = test_magic_create_spell();
  CuAssertPtrNotNull(tc, sp);

  set_level(u, SK_MAGIC, 5);
  unit_add_spell(u, 0, sp, 1);

  CuAssertIntEquals(tc, 1, change_resource(u, get_resourcetype(R_SILVER), 1));
  CuAssertIntEquals(tc, 2, change_resource(u, get_resourcetype(R_AURA), 2));
  CuAssertIntEquals(tc, 3, change_resource(u, get_resourcetype(R_HORSE), 3));

  level = eff_spelllevel(u, sp, 3, 1);
  CuAssertIntEquals(tc, 2, level);
  pay_spell(u, sp, level, 1);
  CuAssertIntEquals(tc, 1, change_resource(u, get_resourcetype(R_SILVER), 1));
  CuAssertIntEquals(tc, 3, change_resource(u, get_resourcetype(R_AURA), 3));
  CuAssertIntEquals(tc, 2, change_resource(u, get_resourcetype(R_HORSE), 1));

  CuAssertIntEquals(tc, 0, eff_spelllevel(u, sp, 3, 1));
  CuAssertIntEquals(tc, 0, change_resource(u, get_resourcetype(R_SILVER), -1));
  CuAssertIntEquals(tc, 0, eff_spelllevel(u, sp, 2, 1));
}

void test_getspell_unit(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  struct locale * lang;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  u = test_create_unit(f, r);
  create_mage(u, M_GRAY);
  enable_skill(SK_MAGIC, true);

  set_level(u, SK_MAGIC, 1);

  lang = get_locale("de");
  sp = create_spell("testspell", 0);
  locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

  CuAssertPtrEquals(tc, 0, unit_getspell(u, "Herp-a-derp", lang));

  unit_add_spell(u, 0, sp, 1);
  CuAssertPtrNotNull(tc, unit_getspell(u, "Herp-a-derp", lang));
}

void test_getspell_faction(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  struct locale * lang;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  f->magiegebiet = M_TYBIED;
  u = test_create_unit(f, r);
  create_mage(u, f->magiegebiet);
  enable_skill(SK_MAGIC, true);

  set_level(u, SK_MAGIC, 1);

  lang = get_locale("de");
  sp = create_spell("testspell", 0);
  locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

  CuAssertPtrEquals(tc, 0, unit_getspell(u, "Herp-a-derp", lang));

  f->spellbook = create_spellbook(0);
  spellbook_add(f->spellbook, sp, 1);
  CuAssertPtrEquals(tc, sp, unit_getspell(u, "Herp-a-derp", lang));
  test_cleanup();
}

void test_getspell_school(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  struct locale * lang;
  struct spellbook * book;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  f->magiegebiet = M_TYBIED;
  u = test_create_unit(f, r);
  create_mage(u, f->magiegebiet);
  enable_skill(SK_MAGIC, true);
  set_level(u, SK_MAGIC, 1);

  lang = get_locale("de");
  sp = create_spell("testspell", 0);
  locale_setstring(lang, mkname("spell", sp->sname), "Herp-a-derp");

  CuAssertPtrEquals(tc, 0, unit_getspell(u, "Herp-a-derp", lang));

  book = faction_get_spellbook(f);
  CuAssertPtrNotNull(tc, book);
  spellbook_add(book, sp, 1);
  CuAssertPtrEquals(tc, sp, unit_getspell(u, "Herp-a-derp", lang));
  test_cleanup();
}

void test_set_pre_combatspell(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  const int index = 0;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  f->magiegebiet = M_TYBIED;
  u = test_create_unit(f, r);
  enable_skill(SK_MAGIC, true);
  set_level(u, SK_MAGIC, 1);
  sp = create_spell("testspell", 0);
  sp->sptyp |= PRECOMBATSPELL;

  unit_add_spell(u, 0, sp, 1);

  set_combatspell(u, sp, 0, 2);
  CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
  set_level(u, SK_MAGIC, 2);
  CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
  set_level(u, SK_MAGIC, 1);
  CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
  unset_combatspell(u, sp);
  CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
  CuAssertPtrEquals(tc, 0, (spell *)get_combatspell(u, index));
  test_cleanup();
}

void test_set_main_combatspell(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  const int index = 1;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  f->magiegebiet = M_TYBIED;
  u = test_create_unit(f, r);
  enable_skill(SK_MAGIC, true);
  set_level(u, SK_MAGIC, 1);
  sp = create_spell("testspell", 0);
  sp->sptyp |= COMBATSPELL;

  unit_add_spell(u, 0, sp, 1);

  set_combatspell(u, sp, 0, 2);
  CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
  set_level(u, SK_MAGIC, 2);
  CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
  set_level(u, SK_MAGIC, 1);
  CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
  unset_combatspell(u, sp);
  CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
  CuAssertPtrEquals(tc, 0, (spell *)get_combatspell(u, index));
  test_cleanup();
}

void test_set_post_combatspell(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;
  const int index = 2;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  f->magiegebiet = M_TYBIED;
  u = test_create_unit(f, r);
  enable_skill(SK_MAGIC, true);
  set_level(u, SK_MAGIC, 1);
  sp = create_spell("testspell", 0);
  sp->sptyp |= POSTCOMBATSPELL;

  unit_add_spell(u, 0, sp, 1);

  set_combatspell(u, sp, 0, 2);
  CuAssertPtrEquals(tc, sp, (spell *)get_combatspell(u, index));
  set_level(u, SK_MAGIC, 2);
  CuAssertIntEquals(tc, 2, get_combatspelllevel(u, index));
  set_level(u, SK_MAGIC, 1);
  CuAssertIntEquals(tc, 1, get_combatspelllevel(u, index));
  unset_combatspell(u, sp);
  CuAssertIntEquals(tc, 0, get_combatspelllevel(u, index));
  CuAssertPtrEquals(tc, 0, (spell *)get_combatspell(u, index));
  test_cleanup();
}

void test_hasspell(CuTest * tc)
{
  spell *sp;
  struct unit * u;
  struct faction * f;
  struct region * r;

  test_cleanup();
  test_create_world();
  r = findregion(0, 0);
  f = test_create_faction(0);
  f->magiegebiet = M_TYBIED;
  u = test_create_unit(f, r);
  enable_skill(SK_MAGIC, true);
  sp = create_spell("testspell", 0);
  sp->sptyp |= POSTCOMBATSPELL;

  unit_add_spell(u, 0, sp, 2);

  set_level(u, SK_MAGIC, 1);
  CuAssertTrue(tc, !u_hasspell(u, sp));

  set_level(u, SK_MAGIC, 2);
  CuAssertTrue(tc, u_hasspell(u, sp));

  set_level(u, SK_MAGIC, 1);
  CuAssertTrue(tc, !u_hasspell(u, sp));
  test_cleanup();
}

CuSuite *get_magic_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_updatespells);
  SUITE_ADD_TEST(suite, test_spellbooks);
  SUITE_ADD_TEST(suite, test_pay_spell);
  SUITE_ADD_TEST(suite, test_pay_spell_failure);
  SUITE_ADD_TEST(suite, test_getspell_unit);
  SUITE_ADD_TEST(suite, test_getspell_faction);
  SUITE_ADD_TEST(suite, test_getspell_school);
  SUITE_ADD_TEST(suite, test_set_pre_combatspell);
  SUITE_ADD_TEST(suite, test_set_main_combatspell);
  SUITE_ADD_TEST(suite, test_set_post_combatspell);
  SUITE_ADD_TEST(suite, test_hasspell);
  return suite;
}
