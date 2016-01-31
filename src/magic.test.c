#include <platform.h>

#include "magic.h"
#include "teleport.h"

#include <kernel/config.h>
#include <kernel/race.h>
#include <kernel/faction.h>
#include <kernel/order.h>
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
    free_spellbook(book);
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
    CuAssertPtrNotNull(tc, herp);
    CuAssertPtrEquals(tc, herp, get_spellbook("herp"));
    derp = get_spellbook("derp");
    CuAssertPtrNotNull(tc, derp);
    CuAssertTrue(tc, derp != herp);
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

    sp = test_create_spell();
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
    test_cleanup();
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

    sp = test_create_spell();
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
    test_cleanup();
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
    test_cleanup();
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

static quicklist * casts;

static int cast_fireball(struct castorder * co) {
    ql_push(&casts, co);
    return 0;
}

void test_multi_cast(CuTest *tc) {
    unit *u;
    spell *sp;
    struct locale * lang;

    test_cleanup();
    sp = create_spell("fireball", 0);
    sp->cast = cast_fireball;
    CuAssertPtrEquals(tc, sp, find_spell("fireball"));

    u = test_create_unit(test_create_faction(0), test_create_region(0, 0, 0));
    u->faction->locale = lang = get_or_create_locale("de");
    locale_setstring(lang, parameters[P_ANY], "ALLE");
    init_parameters(lang);
    locale_setstring(lang, mkname("spell", sp->sname), "Feuerball");
    CuAssertStrEquals(tc, "Feuerball", spell_name(sp, lang));
    set_level(u, SK_MAGIC, 10);
    unit_add_spell(u, 0, sp, 1);
    CuAssertPtrEquals(tc, sp, unit_getspell(u, "Feuerball", lang));

    unit_addorder(u, create_order(K_CAST, u->faction->locale, "Feuerball"));
    unit_addorder(u, create_order(K_CAST, u->faction->locale, "Feuerball"));
    CuAssertPtrEquals(tc, casts, 0);
    magic();
    CuAssertPtrNotNull(tc, casts);
    CuAssertIntEquals(tc, 2, ql_length(casts));
    ql_free(casts);
    test_cleanup();
}

static void test_magic_resistance(CuTest *tc) {
    unit *u;
    race *rc;

    test_cleanup();
    rc = test_create_race("human");
    u = test_create_unit(test_create_faction(rc), test_create_region(0, 0, 0));
    CuAssertDblEquals(tc, rc->magres, magic_resistance(u), 0.01);
    rc->magres = 1.0;
    CuAssertDblEquals_Msg(tc, "magic resistance is capped at 0.9", 0.9, magic_resistance(u), 0.01);
    rc = test_create_race("braineater");
    rc->magres = 1.0;
    u_setrace(u, rc);
    CuAssertDblEquals_Msg(tc, "brain eaters outside astral space have 50% magres", 0.5, magic_resistance(u), 0.01);
    u->region->_plane = get_astralplane();
    CuAssertDblEquals_Msg(tc, "brain eaters in astral space have full magres", 0.9, magic_resistance(u), 0.01);
    test_cleanup();
}

CuSuite *get_magic_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_multi_cast);
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
    SUITE_ADD_TEST(suite, test_magic_resistance);
    return suite;
}
