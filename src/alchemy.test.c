#include "alchemy.h"

#include "guard.h"
#include "laws.h"
#include "study.h"

#include <util/base36.h>
#include <util/keyword.h>
#include "util/rand.h"

#include <kernel/attrib.h>
#include <kernel/faction.h>
#include <kernel/unit.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/item.h>
#include <kernel/region.h>
#include <kernel/skill.h>
#include <kernel/skills.h>

#include <CuTest.h>
#include "tests.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

static void test_herbsearch(CuTest * tc)
{
    faction *f;
    race *rc;
    unit *u, *u2;
    region *r;
    const item_type *itype;

    test_setup();
    r = test_create_plain(0, 0);
    rc = rc_get_or_create("dragon");
    rc->flags |= RCF_UNARMEDGUARD;
    u2 = test_create_unit(test_create_faction_ex(rc, NULL), r);
    setguard(u2, true);

    f = test_create_faction();
    u = test_create_unit(f, r);
    itype = test_create_itemtype("rosemary");

    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    set_level(u, SK_HERBALISM, 1);
    CuAssertPtrEquals(tc, u2, is_guarded(r, u));
    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    setguard(u2, false);
    CuAssertPtrEquals(tc, NULL, is_guarded(r, u));
    CuAssertPtrEquals(tc, NULL, (void *)rherbtype(r));
    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    rsetherbtype(r, itype);
    CuAssertPtrEquals(tc, (void *)itype, (void *)rherbtype(r));
    CuAssertIntEquals(tc, 0, rherbs(r));
    herbsearch(u, INT_MAX);
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "researchherb_none"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    rsetherbs(r, 100);
    CuAssertIntEquals(tc, 100, rherbs(r));
    herbsearch(u, INT_MAX);
    CuAssertIntEquals(tc, 99, rherbs(r));
    CuAssertIntEquals(tc, 1, i_get(u->items, itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(f->msgs, "herbfound"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "researchherb_none"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error108"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error70"));
    CuAssertPtrEquals(tc, NULL, test_find_messagetype(f->msgs, "error59"));
    test_clear_messages(f);

    test_teardown();
}

static void test_foolpotion_effect_high(CuTest *tc) {
    unit *u;
    const struct item_type *itype;

    test_setup();
	itype = oldpotiontype[P_FOOL] = it_get_or_create(rt_get_or_create("hodor"));
	u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
	test_set_skill(u, SK_MELEE, 3, 1);
	test_set_skill(u, SK_CROSSBOW, 2, 1);
    change_effect(u, itype, 2);
	demographics();
	CuAssertIntEquals(tc, 1, get_effect(u, itype));
    CuAssertIntEquals(tc, 2, skill_weeks(u, SK_MELEE));
    CuAssertIntEquals(tc, 1, skill_weeks(u, SK_CROSSBOW));
	test_teardown();
}

static void test_foolpotion_effect_low(CuTest *tc) {
    unit *u;
    const struct item_type *itype;

    test_setup();
	itype = oldpotiontype[P_FOOL] = it_get_or_create(rt_get_or_create("hodor"));
	u = test_create_unit(test_create_faction(), test_create_plain(0, 0));
    scale_number(u, 2);
	test_set_skill(u, SK_MELEE, 3, 1);
	test_set_skill(u, SK_CROSSBOW, 2, 1);
    change_effect(u, itype, 1);
    random_source_inject_constants(0.f, 0);
    demographics();
	CuAssertIntEquals(tc, 0, get_effect(u, itype));
	CuAssertIntEquals(tc, 1, skill_weeks(u, SK_MELEE));
	CuAssertIntEquals(tc, 1, skill_weeks(u, SK_CROSSBOW));
	test_teardown();
}

static void test_use_foolpotion(CuTest *tc) {
    unit *u, *u2;
    const struct item_type *itype;

    test_setup();
    itype = oldpotiontype[P_FOOL] = it_get_or_create(rt_get_or_create("hodor"));
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

static void test_scale_effects(CuTest *tc) {
    unit* u;
    const struct item_type* ptype;

    test_setup();
    ptype = it_get_or_create(rt_get_or_create("hodor"));
    u = test_create_unit(test_create_faction(), test_create_plain(0, 0));

    change_effect(u, ptype, 2);
    scale_effects(u->attribs, 2, 4);
    CuAssertIntEquals(tc, 1, get_effect(u, ptype));

    u->hp = 35;
    CuAssertIntEquals(tc, 1, u->number);
    CuAssertIntEquals(tc, 35, u->hp);
    CuAssertIntEquals(tc, 1, get_effect(u, ptype));
    scale_number(u, 2);
    CuAssertIntEquals(tc, 2, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    CuAssertIntEquals(tc, u->number, get_effect(u, ptype));
    scale_number(u, 8237);
    CuAssertIntEquals(tc, 8237, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    CuAssertIntEquals(tc, u->number, get_effect(u, ptype));
    scale_number(u, 8100);
    CuAssertIntEquals(tc, 8100, u->number);
    CuAssertIntEquals(tc, 35 * u->number, u->hp);
    CuAssertIntEquals(tc, u->number, get_effect(u, ptype));
    set_level(u, SK_ALCHEMY, 1);
    scale_number(u, 0);
    CuAssertIntEquals(tc, 0, get_level(u, SK_ALCHEMY));
    test_teardown();
}

CuSuite *get_alchemy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_herbsearch);
    SUITE_ADD_TEST(suite, test_scale_effects);
    SUITE_ADD_TEST(suite, test_foolpotion_effect_high);
    SUITE_ADD_TEST(suite, test_foolpotion_effect_low);
    SUITE_ADD_TEST(suite, test_use_foolpotion);
    return suite;
}
