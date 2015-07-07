#include <platform.h>

#include <magic.h>
#include <kernel/types.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/order.h>
#include <kernel/item.h>
#include <kernel/messages.h>
#include <util/attrib.h>
#include <util/language.h>
#include <util/message.h>
#include <util/crmessage.h>
#include <tests.h>

#include <attributes/otherfaction.h>

#include "spy.h"

#include <assert.h>
#include <stdio.h>

#include <CuTest.h>

typedef enum {
    M_BASE,
    M_MAGE,
    M_SKILLS,
    M_FACTION,
    M_ITEMS,
    NUM_TYPES
} m_type;

typedef struct {
    region *r;
    unit *spy;
    unit *victim;
    const message_type *msg_types[NUM_TYPES];
} spy_fixture;

static void setup_spy(spy_fixture *fix) {
    test_cleanup();
    fix->r = test_create_region(0, 0, NULL);
    fix->spy = test_create_unit(test_create_faction(NULL), fix->r);
    fix->victim = test_create_unit(test_create_faction(NULL), fix->r);
    fix->msg_types[M_BASE] = register_msg("spyreport", 3, "spy:unit", "target:unit", "status:string");
    fix->msg_types[M_MAGE] = register_msg("spyreport_mage", 3, "spy:unit", "target:unit", "type:string");
    fix->msg_types[M_SKILLS] = register_msg("spyreport_skills", 3, "spy:unit", "target:unit", "skills:string");
    fix->msg_types[M_FACTION] = register_msg("spyreport_faction", 3, "spy:unit", "target:unit", "faction:faction");
    fix->msg_types[M_ITEMS] = register_msg("spyreport_items", 3, "spy:unit", "target:unit", "items:items");

}

static void test_simple_spy_message(CuTest *tc) {
    spy_fixture fix;

    setup_spy(&fix);

    spy_message(0, fix.spy, fix.victim);

    assert_messages(tc, fix.spy->faction->msgs->begin, fix.msg_types, 1, true, M_BASE);


    test_cleanup();
}

static void set_factionstealth(unit *u, faction *f) {
    attrib *a = a_find(u->attribs, &at_otherfaction);
    if (!a)
	a = a_add(&u->attribs, make_otherfaction(f));
    else
	a->data.v = f;
}

static void test_all_spy_message(CuTest *tc) {
    spy_fixture fix;

    setup_spy(&fix);

    enable_skill(SK_MAGIC, true);
    set_level(fix.victim, SK_MINING, 2);
    set_level(fix.victim, SK_MAGIC, 2);
    create_mage(fix.victim, M_DRAIG);
    set_factionstealth(fix.victim, fix.spy->faction);

    item_type *itype;
    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, 0.0, NULL, 0, 0, 0, SK_MELEE, 2);
    i_change(&fix.victim->items, itype, 1);

    spy_message(99, fix.spy, fix.victim);

    assert_messages(tc, fix.spy->faction->msgs->begin, fix.msg_types, 5, true,
		    M_BASE,
		    M_MAGE,
		    M_FACTION,
		    M_SKILLS,
		    M_ITEMS);

    test_cleanup();
}

static void test_sabotage_self(CuTest *tc) {
    unit *u;
    region *r;
    order *ord;
    struct locale *lang;

    test_cleanup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, parameters[P_SHIP], "SCHIFF");
    test_create_world();
    init_locales();

    r = test_create_region(0, 0, NULL);
    assert(r);
    u = test_create_unit(test_create_faction(NULL), r);
    assert(u && u->faction && u->region==r);
    u->ship = test_create_ship(r, test_create_shiptype("boat"));
    assert(u->ship);
    ord = create_order(K_SABOTAGE, lang, "SCHIFF");
    assert(ord);
    CuAssertIntEquals(tc, 0, sabotage_cmd(u, ord));
    CuAssertPtrEquals(tc, 0, r->ships);
    test_cleanup();
}

CuSuite *get_spy_suite(void)
{
  CuSuite *suite = CuSuiteNew();
  SUITE_ADD_TEST(suite, test_simple_spy_message);
  SUITE_ADD_TEST(suite, test_all_spy_message);
  SUITE_ADD_TEST(suite, test_sabotage_self);
  return suite;
}
