#include "items.h"
#include "alchemy.h"

#include <triggers/changerace.h>
#include <triggers/timeout.h>

#include "util/base36.h"
#include "util/keyword.h"
#include "util/language.h"

#include <kernel/attrib.h>
#include <kernel/event.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/skill.h>
#include <kernel/skills.h>
#include <kernel/unit.h>

#include <CuTest.h>
#include "tests.h"

static void test_antimagic_crystal(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    test_setup();
    itype = test_create_itemtype("antimagic");
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    CuAssertIntEquals(tc, 0, use_antimagiccrystal(u, itype, 1, NULL));
    test_teardown();
}

static void test_use_bloodpotion(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    struct race *rc_demon, *rc_toad;

    test_setup();
    itype = test_create_potiontype("bloodpotion", 1);
    rc_demon = test_create_race("demon");
    rc_toad = test_create_race("toad");
    CuAssertPtrEquals(tc, rc_demon, (race *)get_race(RC_DAEMON));
    CuAssertPtrEquals(tc, rc_toad, (race *)get_race(RC_TOAD));
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u_setrace(u, rc_demon);
    use_bloodpotion(u, itype, 5, NULL);
    CuAssertPtrEquals(tc, NULL, get_timeout(u->attribs, "timer", &tt_changerace));
    CuAssertIntEquals(tc, 500, get_effect(u, itype));
    CuAssertPtrEquals(tc, rc_demon, (race *)u_race(u));

    test_teardown();
}

static void test_bloodpotion_fail(CuTest *tc) {
    unit *u;
    struct item_type *itype;
    struct race *rc_demon, *rc_smurf;
    const struct race *rc;
    trigger *t;
    changerace_data *crd;

    test_setup();
    itype = test_create_potiontype("bloodpotion", 1);
    rc_demon = test_create_race("demon");
    rc_smurf = test_create_race("smurf");
    CuAssertPtrEquals(tc, rc_demon, (race *)get_race(RC_DAEMON));
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    rc = u_race(u);
    CuAssertTrue(tc, rc_demon != u_race(u));
    use_bloodpotion(u, itype, 1, NULL);
    CuAssertIntEquals(tc, 0, get_effect(u, itype));
    CuAssertPtrEquals(tc, rc_smurf, (race *)u_race(u));
    CuAssertPtrNotNull(tc, t = get_timeout(u->attribs, "timer", &tt_changerace));
    CuAssertPtrNotNull(tc, crd = (changerace_data *)t->data.v);
    CuAssertPtrEquals(tc, (race *)rc, (race *)crd->race);
    CuAssertPtrEquals(tc, NULL, (race *)crd->irace);
    test_teardown();
}

static void test_foolpotion_effect(CuTest *tc) {
    unit *u;
    const struct item_type *itype;

    test_setup();
    itype = oldpotiontype[P_FOOL] = test_create_potiontype("balm", 1);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    test_set_skill(u, SK_MAGIC, 3, 1);
    test_set_skill(u, SK_CROSSBOW, 2, 1);
    change_effect(u, itype, 2);
    CuAssertIntEquals(tc, 2, get_effect(u, itype));
    potion_effects(u);
    CuAssertIntEquals(tc, 1, get_effect(u, itype));
    CuAssertIntEquals(tc, 2 * SKILL_DAYS_PER_WEEK, unit_skill(u, SK_MAGIC)->days);
    CuAssertIntEquals(tc, SKILL_DAYS_PER_WEEK, unit_skill(u, SK_CROSSBOW)->days);
    test_teardown();
}

static void test_use_foolpotion(CuTest *tc) {
    unit *u, *u2;
    const struct item_type *itype;

    test_setup();
    itype = oldpotiontype[P_FOOL] = test_create_potiontype("balm", 1);
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    u2 = test_create_unit(test_create_faction(), u->region);
    u->thisorder = create_order(K_USE, u->faction->locale, itoa36(u2->no), NULL);

    init_order(u->thisorder, u->faction->locale);
    CuAssertIntEquals(tc, ECUSTOM, use_foolpotion(u, itype, 2, u->thisorder));

    /* Maximal 10 Wirkungen pro Person: */
    test_set_skill(u, SK_STEALTH, 1, 1);
    init_order(u->thisorder, u->faction->locale);
    CuAssertIntEquals(tc, 1, use_foolpotion(u, itype, 2, u->thisorder));
    CuAssertIntEquals(tc, 0, get_effect(u, itype));
    CuAssertIntEquals(tc, 10, get_effect(u2, itype));

    a_removeall(&u2->attribs, &at_effect);
    scale_number(u2, 2);
    init_order(u->thisorder, u->faction->locale);
    CuAssertIntEquals(tc, 2, use_foolpotion(u, itype, 3, u->thisorder));
    CuAssertIntEquals(tc, 20, get_effect(u2, itype));

    a_removeall(&u2->attribs, &at_effect);
    scale_number(u2, 10);
    init_order(u->thisorder, u->faction->locale);
    CuAssertIntEquals(tc, 10, use_foolpotion(u, itype, 20, u->thisorder));
    CuAssertIntEquals(tc, 100, get_effect(u2, itype));

    /* limited use: */
    a_removeall(&u2->attribs, &at_effect);
    scale_number(u2, 2);
    init_order(u->thisorder, u->faction->locale);
    CuAssertIntEquals(tc, 1, use_foolpotion(u, itype, 1, u->thisorder));
    CuAssertIntEquals(tc, 10, get_effect(u2, itype));

    /* stacking only up to 10 effect/person: */
    init_order(u->thisorder, u->faction->locale);
    CuAssertIntEquals(tc, 1, use_foolpotion(u, itype, 2, u->thisorder));
    CuAssertIntEquals(tc, 20, get_effect(u2, itype));

    test_teardown();
}

CuSuite *get_items_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_use_bloodpotion);
    SUITE_ADD_TEST(suite, test_bloodpotion_fail);
    SUITE_ADD_TEST(suite, test_use_foolpotion);
    SUITE_ADD_TEST(suite, test_foolpotion_effect);
    SUITE_ADD_TEST(suite, test_antimagic_crystal);
    return suite;
}
