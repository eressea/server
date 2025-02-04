#include "charming.h"

#include <kernel/curse.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/unit.h>

#include <util/keyword.h>
#include <util/message.h>
#include <util/rand.h>

#include <laws.h>
#include <magic.h>
#include <tests.h>

#include <stb_ds.h>
#include <CuTest.h>

#include <stddef.h>          // for NULL

static void test_charm_unit(CuTest * tc)
{
    unit *u, *mage;
    faction *f;

    test_setup();

    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    mage = test_create_unit(test_create_faction(), u->region);
    u->thisorder = create_order(K_WORK, u->faction->locale, NULL);
    unit_addorder(u,copy_order(u->thisorder));
    CuAssertTrue(tc, !unit_is_slaved(u));
    charm_unit(u, mage, 3.0, 2);
    CuAssertPtrEquals(tc, NULL, u->orders);
    CuAssertPtrEquals(tc, NULL, u->thisorder);
    CuAssertPtrNotNull(tc, u->attribs);
    CuAssertTrue(tc, unit_is_slaved(u));
    CuAssertPtrEquals(tc, mage->faction, u->faction);
    age_unit(&u);
    CuAssertTrue(tc, unit_is_slaved(u));
    age_unit(&u);
    CuAssertTrue(tc, !unit_is_slaved(u));
    CuAssertPtrEquals(tc, f, u->faction);

    test_teardown();
}

static void test_charmed_unit_original_faction_dies(CuTest *tc)
{
    unit *u, *mage;
    faction *f;

    test_setup();

    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    mage = test_create_unit(test_create_faction(), u->region);

    CuAssertTrue(tc, !unit_is_slaved(u));
    charm_unit(u, mage, 3.0, 2);
    CuAssertPtrNotNull(tc, u->attribs);
    CuAssertTrue(tc, unit_is_slaved(u));
    CuAssertPtrEquals(tc, factions, f);
    destroyfaction(&factions);
    age_unit(&u);
    CuAssertIntEquals(tc, 0, u->number);

    test_teardown();
}

static void test_charmed_unit_new_faction_dies(CuTest *tc)
{
    unit *u, *mage;
    faction *f;

    test_setup();

    u = test_create_unit(f = test_create_faction(), test_create_plain(0, 0));
    mage = test_create_unit(test_create_faction(), u->region);

    CuAssertTrue(tc, !unit_is_slaved(u));
    charm_unit(u, mage, 3.0, 2);
    CuAssertPtrNotNull(tc, u->attribs);
    CuAssertTrue(tc, unit_is_slaved(u));
    CuAssertPtrEquals(tc, factions->next, u->faction);
    destroyfaction(&factions->next);
    CuAssertIntEquals(tc, 0, u->number);

    test_teardown();
}

static void test_charmingsong(CuTest *tc) {
    struct region *r;
    struct faction *f, *f2;
    unit *u, *u2;
    castorder co;
    spellparameter param, *args = NULL;

    test_setup();

    r = test_create_plain(0, 0);
    u = test_create_unit(f = test_create_faction(), r);
    param.typ = SPP_UNIT;
    param.data.u = u2 = test_create_unit(f2 = test_create_faction(), r);
    param.flag = TARGET_NOTFOUND;
    arrput(args, param);
    test_create_castorder(&co, u, 3, 4., 0, args);

    /* fail spell if any additional resistance checks are made: */
    random_source_inject_constants(.0, 0);
    /* maximum unit size is force, beyond that, increased magic resistance: */
    scale_number(u2, (int)co.force);
    /* target's best skills is not greater than magician's magic skill, beyond that, increased magic resistance: */
    set_level(u, SK_MAGIC, 8);
    set_level(u2, SK_ENTERTAINMENT, 8);
    /* if target's pais skills exceed force / 2, they get a magic resistance bonus */
    set_level(u2, SK_TACTICS, (int)(co.force / 2));

    /* spell fails, unit not found: */
    CuAssertIntEquals(tc, 0, sp_charmingsong(&co));
    CuAssertPtrEquals(tc, f2, u2->faction);
    CuAssertPtrEquals(tc, NULL, f->msgs);

    co.a_params[0].flag = TARGET_RESISTS;
    CuAssertIntEquals(tc, co.level, sp_charmingsong(&co));
    CuAssertPtrEquals(tc, f2, u2->faction);
    CuAssertPtrEquals(tc, NULL, f->msgs);
    co.a_params[0].flag = TARGET_OK;

    // fail because target has a limited skill:
    set_level(u2, SK_ALCHEMY, 1);
    CuAssertIntEquals(tc, co.level, sp_charmingsong(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spellfail_noexpensives"));
    CuAssertPtrEquals(tc, NULL, f2->msgs);
    test_clear_messages(f);
    remove_skill(u2, SK_ALCHEMY);

    // fail because target has expensive skills > force / 2:
    set_level(u2, SK_TACTICS, 1 + (int)(co.force / 2));
    CuAssertIntEquals(tc, co.level, sp_charmingsong(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spellfail_noexpensives"));
    CuAssertPtrEquals(tc, NULL, f2->msgs);
    test_clear_messages(f);
    set_level(u2, SK_TACTICS, (int)(co.force / 2));

    // unit has a skill that's higher than magician's magic skill, gets a resistance bonus:
    set_level(u2, SK_ENTERTAINMENT, 9);
    CuAssertIntEquals(tc, co.level, sp_charmingsong(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spellunitresists"));
    test_clear_messages(f);
    set_level(u2, SK_ENTERTAINMENT, 1);

    // big unit, gets a resistance bonus and spell fails:
    scale_number(u2, 1 + (int)co.force);
    CuAssertIntEquals(tc, co.level, sp_charmingsong(&co));
    CuAssertPtrNotNull(tc, test_find_faction_message(f, "spellunitresists"));
    scale_number(u2, (int)co.force);
    test_clear_messages(f);

    // success:
    CuAssertIntEquals(tc, co.level, sp_charmingsong(&co));
    CuAssertPtrEquals(tc, u->faction, u2->faction);
    CuAssertTrue(tc, unit_is_slaved(u2));

    CuAssertPtrEquals(tc, NULL, f2->msgs);

    test_teardown();
}

CuSuite *get_charming_suite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, test_charm_unit);
    SUITE_ADD_TEST(suite, test_charmingsong);
    SUITE_ADD_TEST(suite, test_charmed_unit_original_faction_dies);
    SUITE_ADD_TEST(suite, test_charmed_unit_new_faction_dies);

    return suite;
}
