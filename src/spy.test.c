#include <platform.h>

#include <magic.h>
#include <kernel/config.h>
#include <kernel/types.h>
#include <kernel/region.h>
#include <kernel/unit.h>
#include <kernel/faction.h>
#include <kernel/ship.h>
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

typedef struct {
    region *r;
    unit *spy;
    unit *victim;
} spy_fixture;

static void setup_spy(spy_fixture *fix) {
    test_setup();
    fix->r = test_create_region(0, 0, NULL);
    fix->spy = test_create_unit(test_create_faction(NULL), fix->r);
    fix->victim = test_create_unit(test_create_faction(NULL), fix->r);
}

static void test_simple_spy_message(CuTest *tc) {
    spy_fixture fix;

    setup_spy(&fix);

    spy_message(0, fix.spy, fix.victim);

    CuAssertPtrNotNull(tc, test_find_messagetype(fix.spy->faction->msgs, "spyreport"));

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
    item_type *itype;

    setup_spy(&fix);

    enable_skill(SK_MAGIC, true);
    set_level(fix.victim, SK_MINING, 2);
    set_level(fix.victim, SK_MAGIC, 2);
    create_mage(fix.victim, M_DRAIG);
    set_factionstealth(fix.victim, fix.spy->faction);

    itype = it_get_or_create(rt_get_or_create("sword"));
    new_weapontype(itype, 0, frac_zero, NULL, 0, 0, 0, SK_MELEE, 2);
    i_change(&fix.victim->items, itype, 1);

    spy_message(99, fix.spy, fix.victim);

    CuAssertPtrNotNull(tc, test_find_messagetype(fix.spy->faction->msgs, "spyreport"));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.spy->faction->msgs, "spyreport_mage"));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.spy->faction->msgs, "spyreport_skills"));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.spy->faction->msgs, "spyreport_faction"));
    CuAssertPtrNotNull(tc, test_find_messagetype(fix.spy->faction->msgs, "spyreport_items"));

    test_cleanup();
}

static void setup_sabotage(void) {
    struct locale *lang;

    test_setup();
    lang = get_or_create_locale("de");
    locale_setstring(lang, parameters[P_SHIP], "SCHIFF");
    locale_setstring(lang, parameters[P_ANY], "ALLE");
    init_parameters(lang);
    init_locales();
}

static void test_sabotage_self(CuTest *tc) {
    unit *u;
    region *r;
    order *ord;

    setup_sabotage();
    r = test_create_region(0, 0, 0);
    assert(r);
    u = test_create_unit(test_create_faction(NULL), r);
    assert(u && u->faction && u->region == r);
    u->ship = test_create_ship(r, test_create_shiptype("boat"));
    assert(u->ship);
    ord = create_order(K_SABOTAGE, u->faction->locale, "SCHIFF");
    assert(ord);
    CuAssertIntEquals(tc, 0, sabotage_cmd(u, ord));
    CuAssertPtrEquals(tc, 0, r->ships);
    free_order(ord);
    test_cleanup();
}


static void test_sabotage_other_fail(CuTest *tc) {
    unit *u, *u2;
    region *r;
    order *ord;
    message *msg;

    setup_sabotage();
    r = test_create_region(0, 0, 0);
    assert(r);
    u = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    assert(u && u2);
    u2->ship = test_create_ship(r, test_create_shiptype("boat"));
    assert(u2->ship);
    u->ship = u2->ship;
    ship_update_owner(u->ship);
    assert(ship_owner(u->ship) == u);
    ord = create_order(K_SABOTAGE, u->faction->locale, "SCHIFF");
    assert(ord);
    CuAssertIntEquals(tc, 0, sabotage_cmd(u2, ord));
    msg = test_get_last_message(u2->faction->msgs);
    CuAssertStrEquals(tc, "destroy_ship_1", test_get_messagetype(msg));
    msg = test_get_last_message(u->faction->msgs);
    CuAssertStrEquals(tc, "destroy_ship_3", test_get_messagetype(msg));
    CuAssertPtrNotNull(tc, r->ships);
    free_order(ord);
    test_cleanup();
}

static void test_setstealth_cmd(CuTest *tc) {
    unit *u;
    const struct locale *lang;
    
    test_setup();
    u = test_create_unit(test_create_faction(0), test_create_region(0,0,0));
    lang = u->faction->locale;
    u->flags = UFL_ANON_FACTION|UFL_SIEGE;
    u->thisorder = create_order(K_SETSTEALTH, lang, "%s %s",
            LOC(lang, parameters[P_FACTION]),
            LOC(lang, parameters[P_NOT]));
    setstealth_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, UFL_SIEGE, u->flags);
    free_order(u->thisorder);
    u->thisorder = create_order(K_SETSTEALTH, lang, "%s",
            LOC(lang, parameters[P_FACTION]));
    setstealth_cmd(u, u->thisorder);
    CuAssertIntEquals(tc, UFL_SIEGE|UFL_ANON_FACTION, u->flags);
    test_cleanup();
}

static void test_sabotage_other_success(CuTest *tc) {
    unit *u, *u2;
    region *r;
    order *ord;

    setup_sabotage();
    r = test_create_region(0, 0, 0);
    assert(r);
    u = test_create_unit(test_create_faction(NULL), r);
    u2 = test_create_unit(test_create_faction(NULL), r);
    assert(u && u2);
    u2->ship = test_create_ship(r, test_create_shiptype("boat"));
    assert(u2->ship);
    u->ship = u2->ship;
    ship_update_owner(u->ship);
    assert(ship_owner(u->ship) == u);
    ord = create_order(K_SABOTAGE, u->faction->locale, "SCHIFF");
    assert(ord);
    set_level(u2, SK_SPY, 1);
    CuAssertIntEquals(tc, 0, sabotage_cmd(u2, ord));
    CuAssertPtrEquals(tc, 0, r->ships);
    free_order(ord);
    test_cleanup();
}

CuSuite *get_spy_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_simple_spy_message);
    SUITE_ADD_TEST(suite, test_all_spy_message);
    SUITE_ADD_TEST(suite, test_sabotage_self);
    SUITE_ADD_TEST(suite, test_setstealth_cmd);
    SUITE_ADD_TEST(suite, test_sabotage_other_fail);
    SUITE_ADD_TEST(suite, test_sabotage_other_success);
    return suite;
}
